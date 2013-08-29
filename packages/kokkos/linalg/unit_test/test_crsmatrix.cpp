#include <cstdio>

#include <ctime>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <limits.h>
#include <cmath>

#ifdef _OPENMP
#include <Kokkos_OpenMP.hpp>
#else
#include <Kokkos_Threads.hpp>
#endif
#include <Kokkos_Cuda.hpp>
#include <Kokkos_MultiVector.hpp>
#include <Kokkos_CrsMatrix.hpp>
#ifndef DEVICE
#define DEVICE 1
#endif
#if DEVICE==1
#ifdef _OPENMP
typedef Kokkos::OpenMP device_type;
#else
typedef Kokkos::Threads device_type;
#endif
#define KokkosHost(a) a
#define KokkosCUDA(a)
#else
typedef Kokkos::Cuda device_type;
#define KokkosHost(a)
#define KokkosCUDA(a) a
#endif

#define EPSILON 1e-5;

void cuda_check_error(char* comment)
{
#if DEVICE==1
  printf("ERROR %s in %s:%i\n", comment, __FILE__, __LINE__);
#endif
#if DEVICE==2
  printf("ERROR-CUDA %s %s in %s:%i\n", comment, cudaGetErrorString(cudaGetLastError()), __FILE__, __LINE__);
#endif

}

struct test_data{
        int num_tests;
        int num_errors;
        std::string error_string;
        bool print_report;
};
template< typename ScalarType , typename OrdinalType>
int SparseMatrix_generate(OrdinalType nrows, OrdinalType ncols, OrdinalType &nnz, OrdinalType varianz_nel_row, OrdinalType width_row, ScalarType* &values, OrdinalType* &rowPtr, OrdinalType* &colInd)
{
  rowPtr = new OrdinalType[nrows+1];

  OrdinalType elements_per_row = nnz/nrows;
  srand(13721);
  rowPtr[0] = 0;
  for(int row=0;row<nrows;row++)
  {
    int varianz = (1.0*rand()/INT_MAX-0.5)*varianz_nel_row;
    rowPtr[row+1] = rowPtr[row] + elements_per_row+varianz;
  }
  nnz = rowPtr[nrows];
  values = new ScalarType[nnz];
  colInd = new OrdinalType[nnz];
  for(int row=0;row<nrows;row++)
  {
         for(int k=rowPtr[row];k<rowPtr[row+1];k++)
         {
                int pos = (1.0*rand()/INT_MAX-0.5)*width_row+row;
                if(pos<0) pos+=ncols;
                if(pos>=ncols) pos-=ncols;
                colInd[k]= pos;
                values[k] = 100.0*rand()/INT_MAX-50.0;
         }
  }
  return nnz;
}

template<typename Scalar, class Matrix, class RangeVector, class DomainVector>
int test_crs_matrix(test_data &test_sum, Matrix A, RangeVector y, DomainVector x, typename RangeVector::HostMirror h_y,
                typename RangeVector::HostMirror h_x, typename RangeVector::HostMirror h_y_compare,
                int alpha, int beta, bool vector_scalar, const char* type_string)
{
        typedef Matrix matrix_type ;
        typedef DomainVector mv_type;
        typedef typename Kokkos::MultiVectorDynamic<Scalar,device_type>::random_read_type mv_random_read_type;
        typedef typename mv_type::HostMirror h_mv_type;
    typename matrix_type::CrsArrayType::HostMirror h_graph = Kokkos::create_mirror(A.graph);
    typename matrix_type::values_type::HostMirror h_values = Kokkos::create_mirror_view(A.values);
    typedef Kokkos::View<Scalar*,device_type> vector;
    typedef typename vector::HostMirror h_vector;

    int numVecs = x.dimension_1();
    int numCols = A.numCols();
    int numRows = A.numRows();

    bool print_report_always = test_sum.print_report;
    vector a("a",numVecs);
    h_vector h_a = Kokkos::create_mirror_view(a);
    vector b("b",numVecs);
    h_vector h_b = Kokkos::create_mirror_view(b);

    for(int k=0;k<numVecs;k++){
          h_a(k) = (Scalar) (1.0*(rand()%40)-20.);
          h_b(k) = (Scalar) (1.0*(rand()%40)-20.);
          for(int i=0; i<numCols;i++) {
                  h_x(i,k) = (Scalar) (1.0*(rand()%40)-20.);
                  h_y(i,k) = (Scalar) (1.0*(rand()%40)-20.);
          }
    }

        Scalar s_a = alpha;
        Scalar s_b = beta;
        if(!vector_scalar) {
            for(int k=0;k<numVecs;k++){
                  h_a(k) = s_a;
                  h_b(k) = s_b;
            }
        }

    for(int i=0;i<numRows;i++) {
                int start = h_graph.row_map(i);
                int end = h_graph.row_map(i+1);
                for(int j=start;j<end;j++) {
                   h_graph.entries(j) = (i+j-start)%numCols;
                   h_values(j) = h_graph.entries(j) + i;
                }
                for(int k = 0; k<numVecs; k++)
                  h_y_compare(i,k) = h_b(k)*h_y(i,k);
                for(int j=start;j<end;j++) {
                   Scalar val = h_graph.entries(j) + i;
                   int idx = h_graph.entries(j);
                   for(int k = 0; k<numVecs; k++)
                           h_y_compare(i,k) += h_a(k)*val*h_x(idx,k);
                }
        }


        Kokkos::deep_copy(x,h_x);
        Kokkos::deep_copy(y,h_y);
        Kokkos::deep_copy(A.graph.entries,h_graph.entries);
        Kokkos::deep_copy(A.values,h_values);
        Kokkos::deep_copy(a,h_a);
        Kokkos::deep_copy(b,h_b);

        if(vector_scalar)
          Kokkos::MV_Multiply(b,y,a,A,x);
        else
          Kokkos::MV_Multiply(s_b,y,s_a,A,x);
        Kokkos::deep_copy(h_y,y);
        Scalar* error = new Scalar[numVecs];
        Scalar* sum = new Scalar[numVecs];
        for(int k = 0; k<numVecs; k++)
                {error[k] = 0; sum[k] = 0;}
        for(int i=0;i<numRows;i++)
                for(int k = 0; k<numVecs; k++) {
          error[k]+=(h_y_compare(i,k)-h_y(i,k))*(h_y_compare(i,k)-h_y(i,k));
          sum[k] += h_y_compare(i,k)*h_y_compare(i,k);
                }

    int num_errors = 0;
    double total_error = 0;
    double total_sum = 0;
        for(int k = 0; k<numVecs; k++) {
                num_errors += (error[k]/(sum[k]==0?1:sum[k]))>1e-5?1:0;
                total_error += error[k];
                total_sum += sum[k];
        }

        if((num_errors>0?true:false)||print_report_always) {
          if(num_errors>0)
                test_sum.num_errors++;
          int testnumber = 0;
          testnumber+=alpha+1;
          testnumber+=(beta+1)*10;
          if(vector_scalar) testnumber+=100;
          testnumber += numVecs*1000;

          char str[512];
          sprintf(str,"%s %s y = b*y + a*A*x with A: %ix%i numVecs: %i a/b: %s a: %i b: %i Result: %e Error: %e Testnumber: %i\n",
                          type_string,num_errors>0?"FAILED":"PASSED",numRows,numCols,numVecs,vector_scalar?"Vector":"Scalar",
                          alpha,beta,total_sum, total_error,testnumber);
          printf("%s",str);
          if(num_errors>0)
            test_sum.error_string.append(str);
        }
        test_sum.num_tests++;
    delete [] error;
    delete [] sum;
        return num_errors;
}

template<typename Scalar>
int test_crs_matrix_test(test_data &test_sum, int numRows, int numCols, int nnz, int numVecs, int test, const char* typestring) {
        typedef Kokkos::CrsMatrix<Scalar,int,device_type> matrix_type ;
        typedef typename Kokkos::MultiVectorDynamic<Scalar,device_type>::type mv_type;
        typedef typename Kokkos::MultiVectorDynamic<Scalar,device_type>::random_read_type mv_random_read_type;
        typedef typename mv_type::HostMirror h_mv_type;

        Scalar* val = NULL;
        int* row = NULL;
        int* col = NULL;

        srand(17312837);
        nnz = SparseMatrix_generate<Scalar,int>(numRows,numCols,nnz,nnz/numRows*0.2,numRows*0.01,val,row,col);

        matrix_type A("CRS::A",numRows,numCols,nnz,val,row,col,false);
        mv_type x("X",numCols,numVecs);
        mv_random_read_type t_x(x);
        mv_type y("Y",numRows,numVecs);
        h_mv_type h_x = Kokkos::create_mirror_view(x);
        h_mv_type h_y = Kokkos::create_mirror_view(y);
        h_mv_type h_y_compare = Kokkos::create_mirror(y);

        int num_errors = 0;
        if(test==-1) {
    for(int alpha=-1;alpha<3;alpha++)
       for(int beta=-1;beta<3;beta++)
          num_errors += test_crs_matrix<Scalar>(test_sum,A,y,x,h_y,h_x,h_y_compare,alpha,beta,false,typestring);
    for(int alpha=-1;alpha<3;alpha++)
       for(int beta=-1;beta<3;beta++)
          num_errors += test_crs_matrix<Scalar>(test_sum,A,y,x,h_y,h_x,h_y_compare,alpha,beta,true,typestring);
        } else {
          int alpha = ((test%10)%4)-1;
          int beta = (((test/10)%10)%4)-1;
          bool do_vector = (test/100)%2;
          num_errors += test_crs_matrix<Scalar>(test_sum,A,y,x,h_y,h_x,h_y_compare,alpha,beta,do_vector,typestring);
        }
    return num_errors;
}


int test_crs_matrix_type(test_data &test_sum, int numrows, int numcols, int nnz, int numVecs, int type, int test) {
  int maxtype = type<1?4:type;
  int mintype = type<1?1:type;
  int total_errors = 0;
  type = mintype;
  while(type<=maxtype) {
    if(type == 1) total_errors += test_crs_matrix_test<int>(test_sum,numrows,numcols,nnz,numVecs,test,"int          ");
    if(type == 2) total_errors += test_crs_matrix_test<long long int>(test_sum,numrows,numcols,nnz,numVecs,test,"long long int");
    if(type == 3) total_errors += test_crs_matrix_test<float>(test_sum,numrows,numcols,nnz,numVecs,test,"float        ");
    if(type == 4) total_errors += test_crs_matrix_test<double>(test_sum,numrows,numcols,nnz,numVecs,test,"double       ");
    type ++;
  }
  return total_errors;
}

int main(int argc, char **argv)
{
 long long int size = 110503; // a prime number
 int numVecs = 4;
 int threads=1;
 int device = 0;
 int numa=1;
 int test=-1;
 int type=-1;
 bool print_results=false;

 for(int i=0;i<argc;i++)
 {
  if((strcmp(argv[i],"-d")==0)) {device=atoi(argv[++i]); continue;}
  if((strcmp(argv[i],"-s")==0)) {size=atoi(argv[++i]); continue;}
  if((strcmp(argv[i],"-v")==0)) {numVecs=atoi(argv[++i]); continue;}
  if((strcmp(argv[i],"--threads")==0)) {threads=atoi(argv[++i]); continue;}
  if((strcmp(argv[i],"--numa")==0)) {numa=atoi(argv[++i]); continue;}
  if((strcmp(argv[i],"--test")==0)) {test=atoi(argv[++i]); continue;}
  if((strcmp(argv[i],"--type")==0)) {type=atoi(argv[++i]); continue;}
  if((strcmp(argv[i],"-p")==0)) {print_results=atoi(argv[++i])==1; continue;}
 }


 KokkosCUDA(
   Kokkos::Cuda::SelectDevice select_device(device);
   Kokkos::Cuda::initialize( select_device );
 )

#ifdef _OPENMP
   omp_set_num_threads(numa*threads);
   Kokkos::OpenMP::initialize( numa);
#pragma message "Compile OpenMP"
#else
   Kokkos::Threads::initialize( std::pair<unsigned,unsigned>( numa , threads ) );
#pragma message "Compile PThreads"
#endif

 int numVecsList[10] = {1, 2, 3, 4, 5, 8, 11, 15, 16, 17};
 int maxNumVecs = numVecs==-1?17:numVecs;
 int numVecIdx = 0;
 if(numVecs == -1) numVecs = numVecsList[numVecIdx++];
 if(test>=1000) maxNumVecs = numVecs = test/1000;

 int total_errors = 0;
 test_data test_sum;
 test_sum.num_errors = 0;
 test_sum.num_tests = 0;
 test_sum.print_report = print_results;
 while(numVecs<=maxNumVecs) {
   total_errors += test_crs_matrix_type(test_sum,size,size/2,size*10,numVecs,type,test);
   if(numVecs<maxNumVecs) numVecs = numVecsList[numVecIdx++];
   else numVecs++;
 }

 if(total_errors == 0)
   printf("Kokkos::MatVec Test: Passed %i tests\n",test_sum.num_tests);
 else
   printf("Kokkos::MatVec Test: Failed %i of %i tests\n",test_sum.num_errors,test_sum.num_tests);


 KokkosCUDA(
#ifdef _OPENMP
 Kokkos::OpenMP::finalize();
#else
 Kokkos::Threads::finalize();
#endif
 )
 device_type::finalize(  );
}