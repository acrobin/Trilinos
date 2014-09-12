/*------------------------------------------------------------------------*/
/*                 Copyright 2014 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require  license from the                  */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/

#ifndef STK_MESH_BASE_FIELDBLAS_HPP
#define STK_MESH_BASE_FIELDBLAS_HPP

#include <stk_mesh/base/Entity.hpp>
#include <stk_mesh/base/Bucket.hpp>
#include <stk_mesh/base/Selector.hpp>
#include <stk_mesh/base/GetBuckets.hpp>
#include <stk_mesh/base/Field.hpp>
#include <stk_util/util/Fortran.hpp> // For SIERRA_FORTRAN
#include <stk_util/parallel/ParallelReduce.hpp>
#include <stk_mesh/base/MetaData.hpp>
#include <Slib_Exception.h>
#include <Slib_ExceptionReport.h>

#include <complex>
#include <string>
#include <iostream>
#include <algorithm>

#if defined(_OPENMP) && !defined(__INTEL_COMPILER)
#define OPEN_MP_ACTIVE_FIELDBLAS_HPP
// there seems to be an issue with OpenMP combined with GoogleTest macros with Intel compilers
// example of error:
//    openMP.C(206): internal error: assertion failed at: "shared/cfe/edgcpfe/checkdir.c", line 5531
#include <omp.h>

#endif

extern "C"
{
void SIERRA_FORTRAN(daxpy)(const int *n, const double *dscale, const double x[], const int *incx, double y[],const int *incy); // y=y+dscale*x
void SIERRA_FORTRAN(dscal)(const int *n, const double *dscale, double *vect, const int *inc); //vect = dscale * vect
double SIERRA_FORTRAN(ddot)(const int * n, const double* x, const int * incx, const double* y, const int * incy); // < x , y >
double SIERRA_FORTRAN(dnrm2)(const int * n, const double* x, const int * incx); // || x ||
void SIERRA_FORTRAN(sscal)(const int *n, const float *sscale, float *vect, const int *inc); //vect = sscale * vect
void SIERRA_FORTRAN(dcopy)(const int* n, const double* d, const int* inc, double* d1, const int* inc1); // d1 = d
void SIERRA_FORTRAN(dswap)(const int* n, double* d, const int* inc, double* d1, const int* inc1); // switch d1 , d // D.N.E.
//void SIERRA_FORTRAN(dfill)(const int *n, const double *fill, double *vect, const int *inc); // x = fill // D.N.E.
double SIERRA_FORTRAN(dasum)(const int * n,const double * x,const int * incx);
int SIERRA_FORTRAN(idamax)(const int *n, const double *vect, const int *inc);
//int SIERRA_FORTRAN(idamin)(const int *n, const double *vect, const int *inc);
void SIERRA_FORTRAN(saxpy)(const int *n, const float *xscale, const float x[], const int *incx, float y[],const int *incy); // y=y+sscale*x
void SIERRA_FORTRAN(scopy)(const int* n, const float* s, const int* inc, float* s1, const int* inc1); // s1 = s
#ifdef __INTEL_COMPILER
float SIERRA_FORTRAN(sdot)(const int * n, const float* x, const int * incx, const float* y, const int * incy); // < x , y >
float SIERRA_FORTRAN(snrm2)(const int * n, const float* x, const int * incx); // || x ||
float SIERRA_FORTRAN(sasum)(const int * n,const float * x,const int * incx);
#else
double SIERRA_FORTRAN(sdot)(const int * n, const float* x, const int * incx, const float* y, const int * incy); // < x , y >
double SIERRA_FORTRAN(snrm2)(const int * n, const float* x, const int * incx); // || x ||
double SIERRA_FORTRAN(sasum)(const int * n,const float * x,const int * incx);
#endif
void SIERRA_FORTRAN(sswap)(const int* n, float* s, const int* inc, float* s1, const int* inc1); // switch s1 , s // D.N.E.
//void SIERRA_FORTRAN(sfill)(const int *n, const float *fill, double *vect, const int *inc); // x = fill // D.N.E.
int SIERRA_FORTRAN(isamax)(const int *n, const float *vect, const int *inc);
//int SIERRA_FORTRAN(isamin)(const int *n, const float *vect, const int *inc);
//void SIERRA_FORTRAN(zaxpy)(const int *n, const complex<double> *xscale, const complex<double> x[], const int *incx, complex<double> y[],const int *incy); // y=y+sscale*x
//void SIERRA_FORTRAN(zdscal)(const int *n, const complex<double> *sscale, complex<double> *vect, const int *inc); //vect = sscale * vect
//void SIERRA_FORTRAN(zcopy)(const int* n, const complex<double>* s, const int* inc, complex<double>* s1, const int* inc1); // s1 = s
//complex<double> SIERRA_FORTRAN(zdotu)(const int * n, const complex<double>* x, const int * incx, const complex<double>* y, const int * incy); // < x , y >
//complex<double> SIERRA_FORTRAN(zdotc)(const int * n, const complex<double>* x, const int * incx, const complex<double>* y, const int * incy); // < x , conj y >
//complex<double> SIERRA_FORTRAN(dznrm2)(const int * n, const complex<double>* x, const int * incx); // || x ||
//void SIERRA_FORTRAN(zswap)(const int* n, complex<double>* s, const int* inc, complex<double>* s1, const int* inc1); // switch s1 , s // D.N.E.
////void SIERRA_FORTRAN(cfill)(const int *n, const complex<double> *fill, double *vect, const int *inc); // x = fill // D.N.E.
//double SIERRA_FORTRAN(dzasum)(const int * n,const complex<double> * x,const int * incx);
//int SIERRA_FORTRAN(izamax)(const int *n, const complex<double> *vect, const int *inc);
////int SIERRA_FORTRAN(izamin)(const int *n, const complex<double> *vect, const int *inc);
}

namespace stk {
namespace mesh {

template<class Scalar>
struct FortranBLAS
{
    inline
    static void axpy( const int & kmax, const Scalar & alpha, const Scalar x[], Scalar y[])
    {
        for(int k = 0 ; k < kmax ; ++k) {
            y[k] = alpha * x[k] + y[k];
        }
    }

    inline
    static void copy( const int & kmax, const Scalar x[], Scalar y[])
    {
        for(int k = 0 ; k < kmax ; ++k) {
            y[k] = x[k];
        }
    }

    inline
    static void product( const int & kmax, const Scalar x[], const Scalar y[], Scalar z[])
    {
        for (int k=0;k<kmax;k++)
        {
            z[k]=x[k]*y[k];
        }
    }

    inline
    static Scalar dot( const int & kmax, const Scalar x[], const Scalar y[])
    {
        Scalar result = Scalar(0.0);
        for(int k = 0 ; k < kmax ; ++k) {
            result+=y[k] * x[k];
        }
        return result;
    }

    inline
    static Scalar nrm2( const int & kmax, const Scalar x[])
    {
        Scalar result = Scalar(0.0);
        for(int k = 0 ; k < kmax ; ++k) {
            result+=pow(std::abs(x[k]),2);
        }
        return Scalar(sqrt(result));
    }

    inline
    static void scal( const int & kmax, const Scalar alpha, Scalar x[])
    {
        for(int k = 0 ; k < kmax ; ++k) {
            x[k] = alpha * x[k];
        }
    }

    inline
    static void fill(const int & kmax, const Scalar alpha, Scalar x[],const int inc=1)
    {
        for(int k = 0 ; k < kmax*inc ; k+=inc) {
            x[k] = alpha;
        }
    }

    //    inline
    //    static void fill(const int & kmax, const Scalar alpha, Scalar x[])
    //    {
    //        std::fill(x,x+kmax,alpha);
    //    }

    inline
    static void swap(const int & kmax, Scalar x[], Scalar y[])
    {
        Scalar temp;
        for(int k = 0 ; k < kmax ; ++k) {
            temp = y[k];
            y[k] = x[k];
            x[k] = temp;
        }
    }

    inline
    static Scalar asum(const int & kmax, const Scalar x[])
    {
        Scalar result = Scalar(0.0);
        for(int k = 0 ; k < kmax ; ++k) {
            result+=std::abs(x[k]);
        }
        return Scalar(result);
    }

    inline
    static int iamax( const int & kmax, const Scalar x[])
    {
        double amax = 0.0;
        int result = 0;
        for(int k = 0 ; k < kmax ; ++k) {
            if (amax<std::abs(x[k])) {
                result = k;
                amax = std::abs(x[k]);
            }
        }
        return result;
    }

    inline
    static int iamin( const int & kmax, const Scalar x[])
    {
        int result = 0;
        double amin = std::abs(x[0]);
        for(int k = 0 ; k < kmax ; ++k) {
            if (std::abs(x[k])<amin) {
                result = k;
                amin = std::abs(x[k]);
            }
        }
        return result;
    }
};

template<class Scalar>
struct FortranBLAS<std::complex<Scalar> >
{
    inline
    static void axpy( const int & kmax, const std::complex<Scalar>  & alpha, const std::complex<Scalar>  x[], std::complex<Scalar>  y[])
    {
        //        const int one = 1;
        //        SIERRA_FORTRAN(zaxpy)(&kmax,&alpha,x,&one,y,&one);
        for(int k = 0 ; k < kmax ; ++k) {
            y[k] = alpha * x[k] + y[k];
        }
    }

    inline
    static void product( const int & kmax, const std::complex<Scalar> x[], const std::complex<Scalar> y[], std::complex<Scalar> z[])
    {
        for (int k=0;k<kmax;k++)
        {
            z[k]=x[k]*y[k];
        }
    }

    inline
    static void copy( const int & kmax, const std::complex<Scalar>  x[], std::complex<Scalar>  y[])
    {
        //        const int one = 1;
        //        SIERRA_FORTRAN(zcopy)(&kmax,x,&one,y,&one);
        for(int k = 0 ; k < kmax ; ++k) {
            y[k] = x[k];
        }
    }

    inline
    static std::complex<Scalar> dot( const int & kmax, const std::complex<Scalar>  x[], const std::complex<Scalar>  y[]) {
        //        const int one = 1;
        //        return SIERRA_FORTRAN(zdotu)(&kmax,x,&one,y,&one);
        std::complex<Scalar> result = std::complex<Scalar>(0.0);
        for(int k = 0 ; k < kmax ; ++k) {
            result+=y[k] * x[k];
        }
        return result;
    }

    inline
    static std::complex<Scalar> nrm2( const int & kmax, const std::complex<Scalar>  x[]) {
        //        const int one = 1;
        //        return sqrt(SIERRA_FORTRAN(zdotc)(&kmax,x,&one,x,&one).real());
        Scalar result = Scalar(0.0);
        for(int k = 0 ; k < kmax ; ++k) {
            result+=pow(std::abs(x[k]),2);
        }
        return std::complex<Scalar>(sqrt(result));
    }

    inline
    static void scal( const int & kmax, const std::complex<Scalar>  alpha, std::complex<Scalar>  x[])
    {
        //        const int one = 1;
        //        SIERRA_FORTRAN(zdscal)(&kmax,&alpha,x,&one);
        for(int k = 0 ; k < kmax ; ++k) {
            x[k] = alpha * x[k];
        }
    }

    inline
    static void fill(const int & kmax, const std::complex<Scalar>  alpha, std::complex<Scalar>  x[],const int inc=1)
    {
        for(int k = 0 ; k < kmax*inc ; k+=inc) {
            x[k] = alpha;
        }
    }

    //    inline
    //    static void fill(const int & kmax, const std::complex<Scalar> alpha, std::complex<Scalar> x[])
    //    {
    //        std::fill(x,x+kmax,alpha);
    //    }

    inline
    static void swap(const int & kmax, std::complex<Scalar>  x[], std::complex<Scalar>  y[])
    {
        //        const int one = 1;
        //        SIERRA_FORTRAN(zswap)(&kmax,x,&one,y,&one);
        std::complex<Scalar> temp;
        for(int k = 0 ; k < kmax ; ++k) {
            temp = y[k];
            y[k] = x[k];
            x[k] = temp;
        }
    }

    inline
    static std::complex<Scalar> asum( const int & kmax, const std::complex<Scalar>  x[])
    {
        //        const int one = 1;
        //        return SIERRA_FORTRAN(dzasum)(&kmax,x,&one);
        Scalar result = Scalar(0.0);
        for(int k = 0 ; k < kmax ; ++k) {
            result+=std::abs(x[k]);
        }
        return std::complex<Scalar>(result,0.0);
    }

    inline
    static int iamax( const int & kmax, const std::complex<Scalar>  x[]) {
        //        const int one = 1;
        //        return (SIERRA_FORTRAN(izamax)(&kmax, x, &one) - 1);
        Scalar amax = Scalar(0.0);
        int result = 0;
        for(int k = 0 ; k < kmax ; ++k) {
            if (amax < std::norm(x[k])) {
                result = k;
                amax = std::norm(x[k]);
            }
        }
        return result;
    }

    inline
    static int iamin( const int & kmax, const std::complex<Scalar>  x[]) {
        //        const int one = 1;
        //        return (SIERRA_FORTRAN(izamin)(&kmax, x, &one) - 1); //dne
        int result = 0;
        Scalar amin=std::norm(x[0]);
        for(int k = 0 ; k < kmax ; ++k) {
            if (std::norm(x[k])<amin) {
                result = k;
                amin = std::norm(x[k]);
            }
        }
        return result;
    }

};

template<>
struct FortranBLAS<double>
{
    inline
    static void axpy( const int & kmax, const double & alpha, const double x[], double y[])
    {
        const int one = 1;
        SIERRA_FORTRAN(daxpy)(&kmax,&alpha,x,&one,y,&one);
    }

    inline
    static void product( const int & kmax, const double x[], const double y[], double z[])
    {
        for (int k=0;k<kmax;k++)
        {
            z[k]=x[k]*y[k];
        }
    }

    inline
    static void copy( const int & kmax, const double x[], double y[])
    {
        const int one = 1;
        SIERRA_FORTRAN(dcopy)(&kmax,x,&one,y,&one);
    }

    inline
    static double dot( const int & kmax, const double x[], const double y[])
    {
        const int one = 1;
        return SIERRA_FORTRAN(ddot)(&kmax,x,&one,y,&one);
    }

    inline
    static double nrm2( const int & kmax, const double x[])
    {
        const int one = 1;
        return sqrt(SIERRA_FORTRAN(ddot)(&kmax,x,&one,x,&one));
    }

    inline
    static void scal( const int & kmax, const double alpha, double x[])
    {
        const int one = 1;
        SIERRA_FORTRAN(dscal)(&kmax,&alpha,x,&one);
    }

    inline
    static void fill(const int & kmax, const double alpha, double x[],const int inc=1) //CR consider std::fill
    {
        for(int k = 0 ; k < kmax*inc ; k+=inc) {
            x[k] = alpha;
        }
    }

    //    inline
    //    static void fill(const int & kmax, const double alpha, double x[])
    //    {
    //        std::fill(x,x+kmax,alpha);
    //    }

    inline
    static void swap(const int & kmax, double x[], double y[])
    {
        const int one = 1;
        SIERRA_FORTRAN(dswap)(&kmax,x,&one,y,&one);
    }

    inline
    static double asum( const int & kmax, const double x[])
    {
        const int one = 1;
        return SIERRA_FORTRAN(dasum)(&kmax,x,&one);
    }

    inline
    static int iamax( const int & kmax, const double x[]) {
        const int one = 1;
        return (SIERRA_FORTRAN(idamax)(&kmax, x, &one) - 1);
    }

    inline
    static int iamin( const int & kmax, const double x[]) {
        int result = 0;
        double amin = std::abs(x[0]);
        for(int k = 0 ; k < kmax ; ++k) {
            if (std::abs(x[k])<amin) {
                result = k;
                amin = std::abs(x[k]);
            }
        }
        return result;
    }

};

template<>
struct FortranBLAS<float>
{
    inline
    static void axpy( const int & kmax, const float & alpha, const float x[], float y[])
    {
        const int one = 1;
        SIERRA_FORTRAN(saxpy)(&kmax,&alpha,x,&one,y,&one);
    }

    inline
    static void product( const int & kmax, const float x[], const float y[], float z[])
    {
        for (int k=0;k<kmax;k++)
        {
            z[k]=x[k]*y[k];
        }
    }

    inline
    static void copy( const int & kmax, const float x[], float y[])
    {
        const int one = 1;
        SIERRA_FORTRAN(scopy)(&kmax,x,&one,y,&one);
    }

    inline
    static float dot( const int & kmax, const float x[], const float y[])
    {
        const int one = 1;
        return float(SIERRA_FORTRAN(sdot)(&kmax,x,&one,y,&one));
    }

    inline
    static float nrm2( const int & kmax, const float x[])
    {
        const int one = 1;
        return sqrt(float(SIERRA_FORTRAN(sdot)(&kmax,x,&one,x,&one)));
    }

    inline
    static void scal( const int & kmax, const float alpha, float x[])
    {
        const int one = 1;
        SIERRA_FORTRAN(sscal)(&kmax,&alpha,x,&one);
    }

    inline
    static void fill(const int & kmax, const float alpha, float x[],const int inc=1)
    {
        for(int k = 0 ; k < kmax*inc ; k+=inc) {
            x[k] = alpha;
        }
    }

    //    inline
    //    static void fill(const int & kmax, const float alpha, float x[])
    //    {
    //        std::fill(x,x+kmax,alpha);
    //    }

    inline
    static void swap(const int & kmax, float x[], float y[])
    {
        const int one = 1;
        SIERRA_FORTRAN(sswap)(&kmax,x,&one,y,&one);
    }

    inline
    static float asum( const int & kmax, const float x[])
    {
        const int one = 1;
        return float(SIERRA_FORTRAN(sasum)(&kmax,x,&one));
    }

    inline
    static int iamax( const int & kmax, const float x[]) {
        const int one = 1;
        return (SIERRA_FORTRAN(isamax)(&kmax, x, &one) - 1);
    }

    inline
    static int iamin( const int & kmax, const float x[]) {
        int result = 0;
        float amin = std::abs(x[0]);
        for(int k = 0 ; k < kmax ; ++k) {
            if (std::abs(x[k])<amin) {
                result = k;
                amin = std::abs(x[k]);
            }
        }
        return result;
    }

};

template<class Scalar>
inline
void field_axpy(
        const Scalar alpha,
        const FieldBase & xFieldBase,
        const FieldBase & yFieldBase,
        const Selector& selector)
{
    ThrowAssert(&xFieldBase.get_mesh()==&yFieldBase.get_mesh());
    ThrowAssert(xFieldBase.entity_rank() == yFieldBase.entity_rank());
    ThrowAssert(xFieldBase.data_traits().type_info == typeid(Scalar));
    ThrowAssert(yFieldBase.data_traits().type_info == typeid(Scalar));

    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets( xFieldBase.entity_rank(), selector );

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++)
    {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
        ThrowAssert(fieldSize == field_scalars_per_entity(yFieldBase, b));
        const int kmax = length * fieldSize;
        const Scalar * x = static_cast<Scalar*>(field_data(xFieldBase, b));
        Scalar * y = (Scalar*) field_data(yFieldBase, b);

        FortranBLAS<Scalar>::axpy(kmax,alpha,x,y);
    }
}

template<class Scalar>
inline
void field_axpy(
        const Scalar alpha,
        const FieldBase & xFieldBase,
        const FieldBase & yFieldBase)
{
    const Selector selector = selectField(xFieldBase) & selectField(yFieldBase);
    field_axpy(alpha,xFieldBase,yFieldBase,selector);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
void field_axpy(
        const Scalar alpha,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & yField,
        const Selector& selector)
{
    ThrowAssert(&xField.get_mesh()==&yField.get_mesh());
    ThrowAssert(xField.entity_rank() == yField.entity_rank());

    BucketVector const& buckets = xField.get_mesh().get_buckets( xField.entity_rank(), selector );

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);} //CR mention in manual?
#pragma omp parallel for schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++)
    {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xField, b);
        ThrowAssert(fieldSize == field_scalars_per_entity(yField, b));
        const int kmax = length * fieldSize;
        const Scalar * x = static_cast<Scalar*>(field_data(xField, b));
        Scalar * y = (Scalar*) field_data(yField, b);

        FortranBLAS<Scalar>::axpy(kmax,alpha,x,y);
    }
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
void field_axpy(
        const Scalar alpha,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & yField)
{
    const Selector selector = selectField(xField) & selectField(yField);
    field_axpy(alpha,xField,yField,selector);
}

template<class Scalar>
inline
void INTERNAL_field_product(
        const FieldBase & xFieldBase,
        const FieldBase & yFieldBase,
        const FieldBase & zFieldBase,
        const Selector& selector)
{
    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets( xFieldBase.entity_rank(), selector );

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++)
    {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
        ThrowAssert(fieldSize == field_scalars_per_entity(yFieldBase, b));
        const int kmax = length * fieldSize;
        const Scalar * x = static_cast<Scalar*>(field_data(xFieldBase, b));
        const Scalar * y = static_cast<Scalar*>(field_data(yFieldBase, b));
        Scalar * z = static_cast<Scalar*>(field_data(zFieldBase, b));

        FortranBLAS<Scalar>::product(kmax,x,y,z);
    }
}

inline
void field_product(
        const FieldBase & xFieldBase,
        const FieldBase & yFieldBase,
        const FieldBase & zFieldBase,
        const Selector& selector)
{
    ThrowAssert(xFieldBase.entity_rank() == yFieldBase.entity_rank());
    ThrowAssert(yFieldBase.entity_rank() == zFieldBase.entity_rank());
    ThrowAssert(&xFieldBase.get_mesh() == &yFieldBase.get_mesh());
    ThrowAssert(&yFieldBase.get_mesh() == &zFieldBase.get_mesh());
    ThrowAssert(xFieldBase.data_traits().type_info == yFieldBase.data_traits().type_info);
    ThrowAssert(yFieldBase.data_traits().type_info == zFieldBase.data_traits().type_info);

    if (xFieldBase.data_traits().type_info == typeid(double)) {
        INTERNAL_field_product<double>(xFieldBase,yFieldBase,zFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(float)) {
        INTERNAL_field_product<float>(xFieldBase,yFieldBase,zFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(std::complex<double>)) {
        INTERNAL_field_product<std::complex<double> >(xFieldBase,yFieldBase,zFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(std::complex<float>)) {
        INTERNAL_field_product<std::complex<float> >(xFieldBase,yFieldBase,zFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(int)) {
        INTERNAL_field_product<int>(xFieldBase,yFieldBase,zFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(std::complex<int>)) {
        INTERNAL_field_product<std::complex<int> >(xFieldBase,yFieldBase,zFieldBase,selector);
    } else {
        ThrowAssertMsg(false,"Error in field_product; field is of type "<<xFieldBase.data_traits().type_info.name()<<" which is not supported");
    }
}

inline
void field_product(
        const FieldBase & xFieldBase,
        const FieldBase & yFieldBase,
        const FieldBase & zFieldBase)
{
    const Selector selector = selectField(xFieldBase) & selectField(yFieldBase);
    field_product(xFieldBase,yFieldBase,zFieldBase,selector);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
void field_product(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & yField,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & zField,
        const Selector& selector)
{
    ThrowAssert(xField.entity_rank() == yField.entity_rank());
    ThrowAssert(yField.entity_rank() == zField.entity_rank());
    ThrowAssert(&xField.get_mesh() == &yField.get_mesh());
    ThrowAssert(&yField.get_mesh() == &zField.get_mesh());

    BucketVector const& buckets = xField.get_mesh().get_buckets( xField.entity_rank(), selector );

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++)
    {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xField, b);
        ThrowAssert(fieldSize == field_scalars_per_entity(yField, b));
        ThrowAssert(fieldSize == field_scalars_per_entity(zField, b));
        const int kmax = length * fieldSize;
        const Scalar * x = static_cast<Scalar*>(field_data(xField, b));
        const Scalar * y = static_cast<Scalar*>(field_data(yField, b));
        Scalar * z = static_cast<Scalar*>(field_data(zField, b));

        FortranBLAS<Scalar>::product(kmax,x,y,z);
    }
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
void field_product(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & yField,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & zField)
{
    const Selector selector = selectField(xField) & selectField(yField);
    field_product(xField,yField,zField,selector);
}

template<class Scalar>
inline
void INTERNAL_field_copy(
        const FieldBase & xFieldBase,
        const FieldBase & yFieldBase,
        const Selector& selector)
{

    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets( xFieldBase.entity_rank(), selector );

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++)
    {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);

#ifndef NDEBUG
        if(fieldSize != field_scalars_per_entity(yFieldBase, b)) {
          sierra::RuntimeError x;
          x << "In INTERNAL_field_copy: found incomptaible field sizes.  "<<std::endl;
          x <<"  First field name: "<<xFieldBase.name()<<std::endl;
          x <<"  First field size: "<<field_scalars_per_entity(xFieldBase, b)<<std::endl;
          x <<"  Second field name: "<<yFieldBase.name()<<std::endl;
          x <<"  Second field size: "<<field_scalars_per_entity(yFieldBase, b)<<std::endl;
          throw x;
        }
#endif

        const int kmax = length * fieldSize;
        const Scalar * x = static_cast<Scalar*>(field_data(xFieldBase, b));
        Scalar * y = static_cast<Scalar*>(field_data(yFieldBase, b));

        FortranBLAS<Scalar>::copy(kmax,x,y);
    }
}

inline
void field_copy(
        const FieldBase & xFieldBase,
        const FieldBase & yFieldBase,
        const Selector& selector)
{
    ThrowAssert(xFieldBase.entity_rank() == yFieldBase.entity_rank());
    ThrowAssert(&xFieldBase.get_mesh() == &yFieldBase.get_mesh());
    ThrowAssert(xFieldBase.data_traits().type_info == yFieldBase.data_traits().type_info);

    if (xFieldBase.data_traits().type_info == typeid(double)) {
        INTERNAL_field_copy<double>(xFieldBase,yFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(float)) {
        INTERNAL_field_copy<float>(xFieldBase,yFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(std::complex<double>)) {
        INTERNAL_field_copy<std::complex<double> >(xFieldBase,yFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(std::complex<float>)) {
        INTERNAL_field_copy<std::complex<float> >(xFieldBase,yFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(int)) {
        INTERNAL_field_copy<int>(xFieldBase,yFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(std::complex<int>)) {
        INTERNAL_field_copy<std::complex<int> >(xFieldBase,yFieldBase,selector);
    } else {
        ThrowAssertMsg(false,"Error in field_copy; field is of type "<<xFieldBase.data_traits().type_info.name()<<" which is not supported");
    }
}

inline
void field_copy(
        const FieldBase & xFieldBase,
        const FieldBase & yFieldBase)
{
    const Selector selector = selectField(xFieldBase) & selectField(yFieldBase);
    field_copy(xFieldBase,yFieldBase,selector);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
void field_copy(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & yField,
        const Selector& selector)
{

    ThrowAssert(xField.entity_rank() == yField.entity_rank());
    ThrowAssert(&xField.get_mesh() == &yField.get_mesh());

    BucketVector const& buckets = xField.get_mesh().get_buckets( xField.entity_rank(), selector );

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++) {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xField, b);
        ThrowAssert(fieldSize == field_scalars_per_entity(yField, b));
        const int kmax = length * fieldSize;
        const Scalar * x = static_cast<Scalar*>(field_data(xField, b));
        Scalar * y = static_cast<Scalar*>(field_data(yField, b));

        FortranBLAS<Scalar>::copy(kmax,x,y);
    }
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
void field_copy(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & yField)
{
    const Selector selector = selectField(xField) & selectField(yField);
    field_copy(xField,yField,selector);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Scalar field_dot(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & yField,
        const Selector& selector,
        const MPI_Comm comm)
{

    ThrowAssert(xField.entity_rank() == yField.entity_rank());
    ThrowAssert(&xField.get_mesh() == &yField.get_mesh());

    BucketVector const& buckets = xField.get_mesh().get_buckets(xField.entity_rank(),
                                                                selector & xField.mesh_meta_data().locally_owned_part());

    Scalar local_result = Scalar(0.0);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static) reduction(+:local_result)
#endif
    for(size_t i=0; i < buckets.size(); i++) {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xField, b);
        ThrowAssert(fieldSize == field_scalars_per_entity(yField, b));
        const int kmax = length * fieldSize;
        const Scalar * x = static_cast<Scalar*>(field_data(xField, b));
        const Scalar * y = static_cast<Scalar*>(field_data(yField, b));
        local_result+=FortranBLAS<Scalar>::dot(kmax,x,y);
    }

    Scalar glob_result = local_result;
    stk::all_reduce_sum(comm,&local_result,&glob_result,1u);
    return glob_result;
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
std::complex<Scalar>  field_dot(
        const Field<std::complex<Scalar> ,T1,T2,T3,T4,T5,T6,T7>& xField,
        const Field<std::complex<Scalar> ,T1,T2,T3,T4,T5,T6,T7>& yField,
        const Selector& selector,
        const MPI_Comm comm) {

    ThrowAssert(xField.entity_rank() == yField.entity_rank());
    ThrowAssert(&xField.get_mesh() == &yField.get_mesh());

    BucketVector const& buckets = xField.get_mesh().get_buckets(xField.entity_rank(),
                                                                selector & xField.mesh_meta_data().locally_owned_part());

    Scalar local_result_r = Scalar(0.0);
    Scalar local_result_i = Scalar(0.0);
    std::complex<Scalar> priv_tmp;

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for reduction(+:local_result_r,local_result_i) schedule(static) private(priv_tmp)
#endif
    for(size_t i=0; i < buckets.size(); i++) {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xField, b);
        ThrowAssert(fieldSize == field_scalars_per_entity(yField,b));
        const int kmax = length * fieldSize;
        const std::complex<Scalar>* x = static_cast<std::complex<Scalar>*>(field_data(xField, b));
        const std::complex<Scalar>* y = static_cast<std::complex<Scalar>*>(field_data(yField, b));
        priv_tmp=FortranBLAS<std::complex<Scalar> >::dot(kmax,x,y);
        local_result_r+=priv_tmp.real();
        local_result_i+=priv_tmp.imag();
    }

    Scalar local_result_ri [2] = { local_result_r     , local_result_i     };
    Scalar  glob_result_ri [2] = { local_result_ri[0] , local_result_ri[1] };
    stk::all_reduce_sum(comm,local_result_ri,glob_result_ri,2u);
    return std::complex<Scalar> (glob_result_ri[0],glob_result_ri[1]);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Scalar field_dot(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & yField,
        const Selector& selector)
{
    const MPI_Comm comm = xField.get_mesh().parallel();
    return field_dot(xField,yField,selector,comm);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Scalar field_dot(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & yField)
{
    const Selector selector = selectField(xField) & selectField(yField);
    return field_dot(xField,yField,selector);
}

template<class Scalar>
inline
void field_dot(
        std::complex<Scalar> & global_result,
        const FieldBase & xFieldBase, //CR LAST COMMENT LOCATION
        const FieldBase & yFieldBase,
        const Selector& selector,
        const MPI_Comm comm)
{
    ThrowAssert(xFieldBase.entity_rank() == yFieldBase.entity_rank());
    ThrowAssert(&xFieldBase.get_mesh() == &yFieldBase.get_mesh());
    ThrowAssert(xFieldBase.data_traits().type_info == yFieldBase.data_traits().type_info);
    ThrowAssert(typeid(std::complex<Scalar>) == xFieldBase.data_traits().type_info);

    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets(xFieldBase.entity_rank(),
                                                                    selector & xFieldBase.mesh_meta_data().locally_owned_part());

    Scalar local_result_r = Scalar(0.0);
    Scalar local_result_i = Scalar(0.0);
    std::complex<Scalar> priv_tmp;

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for reduction(+:local_result_r,local_result_i) schedule(static) private(priv_tmp)
#endif
    for(size_t i=0; i < buckets.size(); i++) {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
        ThrowAssert(fieldSize == field_scalars_per_entity(yFieldBase,b));
        const int kmax = length * fieldSize;
        const std::complex<Scalar>* x = static_cast<std::complex<Scalar>*>(field_data(xFieldBase, b));
        const std::complex<Scalar>* y = static_cast<std::complex<Scalar>*>(field_data(yFieldBase, b));
        priv_tmp=FortranBLAS<std::complex<Scalar> >::dot(kmax,x,y);
        local_result_r+=priv_tmp.real();
        local_result_i+=priv_tmp.imag();
    }

    Scalar local_result_ri [2] = { local_result_r     , local_result_i     };
    Scalar  glob_result_ri [2] = { local_result_ri[0] , local_result_ri[1] };
    stk::all_reduce_sum(comm,local_result_ri,glob_result_ri,2u);
    global_result = std::complex<Scalar> (glob_result_ri[0],glob_result_ri[1]);
}

template<class Scalar>
inline
void field_dot(
        Scalar & glob_result,
        const FieldBase & xFieldBase,
        const FieldBase & yFieldBase,
        const Selector& selector,
        const MPI_Comm comm)
{
    ThrowAssert(xFieldBase.entity_rank() == yFieldBase.entity_rank());
    ThrowAssert(&xFieldBase.get_mesh() == &yFieldBase.get_mesh());
    ThrowAssert(xFieldBase.data_traits().type_info == yFieldBase.data_traits().type_info);
    ThrowAssert(typeid(Scalar) == xFieldBase.data_traits().type_info);

    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets(xFieldBase.entity_rank(),
                                                                    selector & xFieldBase.mesh_meta_data().locally_owned_part());

    Scalar local_result = Scalar(0.0);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for reduction(+:local_result) schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++) {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
        ThrowAssert(fieldSize == field_scalars_per_entity(yFieldBase,b));
        const int kmax = length * fieldSize;
        const Scalar* x = static_cast<Scalar*>(field_data(xFieldBase, b));
        const Scalar* y = static_cast<Scalar*>(field_data(yFieldBase, b));

        local_result+=FortranBLAS<Scalar>::dot(kmax,x,y);
    }

    glob_result = local_result;
    stk::all_reduce_sum(comm,&local_result,&glob_result,1u);
}

template<class Scalar>
inline
void field_dot(
        Scalar & result,
        const FieldBase & xFieldBase,
        const FieldBase & yFieldBase,
        const Selector& selector)
{
    const MPI_Comm comm = xFieldBase.get_mesh().parallel();
    field_dot(result,xFieldBase,yFieldBase,selector,comm);
}

template<class Scalar>
inline
void field_dot(
        Scalar & result,
        const FieldBase & xFieldBase,
        const FieldBase & yFieldBase)
{
    const Selector selector = selectField(xFieldBase) & selectField(yFieldBase);
    field_dot(result,xFieldBase,yFieldBase,selector);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
void field_scale(
        const Scalar alpha,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Selector& selector)
{
    BucketVector const& buckets = xField.get_mesh().get_buckets(xField.entity_rank(),selector);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++) {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xField, b);
        const int kmax = length * fieldSize;
        Scalar * x = static_cast<Scalar*>(field_data(xField, b));

        FortranBLAS<Scalar>::scal(kmax,alpha,x);
    }
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
void field_scale(
        const Scalar alpha,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField)
{
    const Selector selector = selectField(xField);
    field_scale(alpha,xField,selector);
}

template<class Scalar>
inline
void field_scale(
        const Scalar alpha,
        const FieldBase & xFieldBase,
        const Selector& selector)
{
    ThrowAssert(xFieldBase.data_traits().type_info == typeid(Scalar));

    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets(xFieldBase.entity_rank(),selector);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++) {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
        const int kmax = length * fieldSize;
        Scalar * x = static_cast<Scalar*>(field_data(xFieldBase, b));

        FortranBLAS<Scalar>::scal(kmax,alpha,x);
    }
}

template<class Scalar>
inline
void field_scale(
        const Scalar alpha,
        const FieldBase & xFieldBase)
{
    const Selector selector = selectField(xFieldBase);
    field_scale(alpha,xFieldBase,selector);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
void field_fill(
        const Scalar alpha,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Selector& selector)
{
    BucketVector const& buckets = xField.get_mesh().get_buckets(xField.entity_rank(),selector);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++) {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xField, b);
        const int kmax = length * fieldSize;
        Scalar * x = static_cast<Scalar*>(field_data(xField, b));

        FortranBLAS<Scalar>::fill(kmax,alpha,x);
    }
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
void field_fill(
        const Scalar alpha,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField)
{
    const Selector selector = selectField(xField);
    field_fill(alpha,xField,selector);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
void field_fill_component(
        const Scalar* alpha,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Selector& selector)
{
    BucketVector const& buckets = xField.get_mesh().get_buckets(xField.entity_rank(),selector);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++) {
        const Bucket & b = *buckets[i];
        const int length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xField, b);
        Scalar * x = static_cast<Scalar*>(field_data(xField, b));

        for (unsigned int j = 0; j < fieldSize; j++) {
            FortranBLAS<Scalar>::fill(length,alpha[j],x+j,fieldSize);
        }
    }
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
void field_fill_component(
        const Scalar* alpha,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField)
{
    const Selector selector = selectField(xField);
    field_fill_component(alpha,xField,selector);
}

template<class Scalar>
inline
void field_fill_component(
        const Scalar* alpha,
        const FieldBase & xFieldBase,
        const Selector& selector)
{
    ThrowAssert(xFieldBase.data_traits().type_info == typeid(Scalar));

    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets(xFieldBase.entity_rank(),selector);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static)
#endif
    for(size_t i = 0; i < buckets.size(); i++) {
        const Bucket & b = *buckets[i];
        const int length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
        Scalar * x = static_cast<Scalar*>(field_data(xFieldBase, b));

        for (unsigned int j = 0; j < fieldSize; j++) {
            FortranBLAS<Scalar>::fill(length,alpha[j],x+j,fieldSize);
        }
    }
}

template<class Scalar>
inline
void field_fill_component(
        const Scalar* alpha,
        const FieldBase & xFieldBase)
{
    const Selector selector = selectField(xFieldBase);
    field_fill_component(alpha,xFieldBase,selector);
}

template<class Scalar>
inline
void field_fill(
        const Scalar alpha,
        const FieldBase & xFieldBase,
        const Selector& selector)
{
    ThrowAssert(xFieldBase.data_traits().type_info == typeid(Scalar));

    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets(xFieldBase.entity_rank(),selector);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static)
#endif
    for(size_t i = 0; i < buckets.size(); i++) {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
        const int kmax = length * fieldSize;
        Scalar * x = static_cast<Scalar*>(field_data(xFieldBase, b));

        FortranBLAS<Scalar>::fill(kmax,alpha,x);
    }
}

template<class Scalar>
inline
void field_fill(
        const Scalar alpha,
        const FieldBase & xFieldBase)
{
    const Selector selector = selectField(xFieldBase);
    field_fill(alpha,xFieldBase,selector);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
void field_swap(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & yField,
        const Selector& selector)
{
    ThrowAssert(xField.entity_rank() == yField.entity_rank());
    ThrowAssert(&xField.get_mesh() == &yField.get_mesh());

    BucketVector const& buckets = xField.get_mesh().get_buckets( xField.entity_rank(), selector );

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++) {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xField, b);
        ThrowAssert(fieldSize == field_scalars_per_entity(yField, b));
        const int kmax = length * fieldSize;
        Scalar * x = static_cast<Scalar*>(field_data(xField, b));
        Scalar * y = static_cast<Scalar*>(field_data(yField, b));

        FortranBLAS<Scalar>::swap(kmax,x,y);
    }
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
void field_swap(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & yField)
{
    const Selector selector = selectField(xField) & selectField(yField);
    field_swap(xField,yField,selector);
}

template<class Scalar>
inline
void INTERNAL_field_swap(
        const FieldBase & xFieldBase,
        const FieldBase & yFieldBase,
        const Selector& selector)
{
    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets( xFieldBase.entity_rank(), selector );

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++) {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
        ThrowAssert(fieldSize == field_scalars_per_entity(yFieldBase, b));
        const int kmax = length * fieldSize;
        Scalar * x = static_cast<Scalar*>(field_data(xFieldBase, b));
        Scalar * y = static_cast<Scalar*>(field_data(yFieldBase, b));

        FortranBLAS<Scalar>::swap(kmax,x,y);
    }
}

inline
void field_swap(
        const FieldBase & xFieldBase,
        const FieldBase & yFieldBase,
        const Selector& selector)
{
    ThrowAssert(xFieldBase.entity_rank() == yFieldBase.entity_rank());
    ThrowAssert(&xFieldBase.get_mesh() == &yFieldBase.get_mesh());
    ThrowAssert(xFieldBase.data_traits().type_info == yFieldBase.data_traits().type_info);

    if (xFieldBase.data_traits().type_info == typeid(double)) {
        INTERNAL_field_swap<double>(xFieldBase,yFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(float)) {
        INTERNAL_field_swap<float>(xFieldBase,yFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(std::complex<double>)) {
        INTERNAL_field_swap<std::complex<double> >(xFieldBase,yFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(std::complex<float>)) {
        INTERNAL_field_swap<std::complex<float> >(xFieldBase,yFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(int)) {
        INTERNAL_field_swap<int>(xFieldBase,yFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(std::complex<int>)) {
        INTERNAL_field_swap<std::complex<int> >(xFieldBase,yFieldBase,selector);
    } else {
        ThrowAssertMsg(false,"Error in field_swap; field is of type "<<xFieldBase.data_traits().type_info.name()<<" which is not supported");
    }
}

inline
void field_swap(
        const FieldBase & xFieldBase,
        const FieldBase & yFieldBase)
{
    const Selector selector = selectField(xFieldBase) & selectField(yFieldBase);
    field_swap(xFieldBase,yFieldBase,selector);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Scalar field_nrm2(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Selector& selector,
        const MPI_Comm comm)
{
    BucketVector const& buckets = xField.get_mesh().get_buckets(xField.entity_rank(),
                                                                selector & xField.mesh_meta_data().locally_owned_part());

    Scalar local_result = Scalar(0.0);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for reduction(+:local_result) schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++) {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xField, b);
        const int kmax = length * fieldSize;
        Scalar * x = static_cast<Scalar*>(field_data(xField, b));

        local_result+=FortranBLAS<Scalar>::dot(kmax,x,x);
    }

    Scalar glob_result=local_result;
    stk::all_reduce_sum(comm,&local_result,&glob_result,1u);
    return sqrt(glob_result);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
std::complex<Scalar> field_nrm2(
        const Field< std::complex<Scalar>,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Selector& selector,
        const MPI_Comm comm) {

    BucketVector const& buckets = xField.get_mesh().get_buckets(xField.entity_rank(),
                                                                selector & xField.mesh_meta_data().locally_owned_part());

    Scalar local_result_r = Scalar(0.0);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for reduction(+:local_result_r) schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++) {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xField, b);
        const int kmax = length * fieldSize;
        std::complex<Scalar>* x = static_cast<std::complex<Scalar>*>(field_data(xField, b));

        local_result_r+=pow(FortranBLAS<std::complex<Scalar> >::nrm2(kmax,x).real(),2.0);
    }

    Scalar glob_result=local_result_r;
    stk::all_reduce_sum(comm,&local_result_r,&glob_result,1u);
    return std::complex<Scalar>(sqrt(glob_result),0.0);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Scalar field_nrm2(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Selector& selector)
{
    const MPI_Comm comm = xField.get_mesh().parallel();
    return field_nrm2(xField,selector,comm);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Scalar field_nrm2(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField)
{
    const Selector selector = selectField(xField);
    return field_nrm2(xField,selector);
}


template<class Scalar>
inline
void field_nrm2(
        Scalar & glob_result,
        const FieldBase & xFieldBase,
        const Selector& selector,
        const MPI_Comm comm)
{
    ThrowAssert(xFieldBase.data_traits().type_info == typeid(Scalar));

    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets(xFieldBase.entity_rank(),
                                                                    selector & xFieldBase.mesh_meta_data().locally_owned_part());

    Scalar local_result = Scalar(0.0);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for reduction(+:local_result) schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++) {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
        const int kmax = length * fieldSize;
        Scalar * x = static_cast<Scalar*>(field_data(xFieldBase, b));

        local_result+=FortranBLAS<Scalar>::dot(kmax,x,x);
    }

    glob_result=local_result;
    stk::all_reduce_sum(comm,&local_result,&glob_result,1u);
    glob_result=sqrt(glob_result);
}

template<class Scalar>
inline
void field_nrm2(
        std::complex<Scalar> & result,
        const FieldBase & xFieldBase,
        const Selector& selector,
        const MPI_Comm comm)
{
    ThrowAssert(xFieldBase.data_traits().type_info == typeid(std::complex<Scalar>));

    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets(xFieldBase.entity_rank(),
                                                                    selector & xFieldBase.mesh_meta_data().locally_owned_part());

    Scalar local_result_r = Scalar(0.0);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for reduction(+:local_result_r) schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++) {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
        const int kmax = length * fieldSize;
        std::complex<Scalar>* x = static_cast<std::complex<Scalar>*>(field_data(xFieldBase, b));

        local_result_r+=pow(FortranBLAS<std::complex<Scalar> >::nrm2(kmax,x).real(),2.0);
    }

    Scalar glob_result=local_result_r;
    stk::all_reduce_sum(comm,&local_result_r,&glob_result,1u);
    result=std::complex<Scalar>(sqrt(glob_result),0.0);
}

template<class Scalar>
inline
void field_nrm2(
        Scalar & result,
        const FieldBase & xFieldBase,
        const Selector& selector)
{
    const MPI_Comm comm = xFieldBase.get_mesh().parallel();
    field_nrm2(result,xFieldBase,selector,comm);
}

template<class Scalar>
inline
void field_nrm2(
        Scalar & result,
        const FieldBase & xFieldBase)
{
    const Selector selector = selectField(xFieldBase);
    field_nrm2(result,xFieldBase,selector);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Scalar field_asum(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Selector& selector,
        const MPI_Comm comm)
{
    BucketVector const& buckets = xField.get_mesh().get_buckets(xField.entity_rank(),
                                                                selector & xField.mesh_meta_data().locally_owned_part());

    Scalar local_result = Scalar(0.0);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for reduction(+:local_result) schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++) {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xField, b);
        const int kmax = length * fieldSize;
        const Scalar * x = static_cast<Scalar*>(field_data(xField, b));

        local_result+=FortranBLAS<Scalar>::asum(kmax,x);
    }

    Scalar glob_result=local_result;
    stk::all_reduce_sum(comm,&local_result,&glob_result,1u);
    return glob_result;
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
std::complex<Scalar> field_asum(
        const Field<std::complex<Scalar>,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Selector& selector,
        const MPI_Comm comm) {

    BucketVector const& buckets = xField.get_mesh().get_buckets(xField.entity_rank(),
                                                                selector & xField.mesh_meta_data().locally_owned_part());

    Scalar local_result = Scalar(0.0);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for reduction(+:local_result) schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++) {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xField, b);
        const int kmax = length * fieldSize;
        const std::complex<Scalar>* x = static_cast<std::complex<Scalar>*>(field_data(xField, b));

        local_result += FortranBLAS<std::complex<Scalar> >::asum(kmax,x).real();
    }

    Scalar glob_result = local_result;
    stk::all_reduce_sum(comm,&local_result,&glob_result,1u);
    return std::complex<Scalar>(glob_result,0.0);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Scalar field_asum(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Selector& selector)
{
    const MPI_Comm comm = xField.get_mesh().parallel();
    return field_asum(xField,selector,comm);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Scalar field_asum(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField)
{
    const Selector selector = selectField(xField);
    return field_asum(xField,selector);
}

template<class Scalar>
inline
void field_asum(
        Scalar & glob_result,
        const FieldBase & xFieldBase,
        const Selector selector,
        const MPI_Comm comm)
{
    ThrowAssert(typeid(Scalar) == xFieldBase.data_traits().type_info);

    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets(xFieldBase.entity_rank(),
                                                                    selector & xFieldBase.mesh_meta_data().locally_owned_part());

    Scalar local_result = Scalar(0.0);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for reduction(+:local_result) schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++) {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
        const int kmax = length * fieldSize;
        const Scalar * x = static_cast<Scalar*>(field_data(xFieldBase, b));

        local_result+=FortranBLAS<Scalar>::asum(kmax,x);
    }

    glob_result=local_result;
    stk::all_reduce_sum(comm,&local_result,&glob_result,1u);
}

template<class Scalar>
inline
void field_asum(
        std::complex<Scalar> & result,
        const FieldBase & xFieldBase,
        const Selector selector,
        const MPI_Comm comm)
{
    ThrowAssert(typeid(std::complex<Scalar>) == xFieldBase.data_traits().type_info);

    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets(xFieldBase.entity_rank(),
                                                                    selector & xFieldBase.mesh_meta_data().locally_owned_part());

    Scalar local_result = Scalar(0.0);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for reduction(+:local_result) schedule(static)
#endif
    for(size_t i=0; i < buckets.size(); i++) {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
        const int kmax = length * fieldSize;
        const std::complex<Scalar>* x = static_cast<std::complex<Scalar>*>(field_data(xFieldBase, b));

        local_result+=FortranBLAS<std::complex<Scalar> >::asum(kmax,x).real();
    }

    Scalar glob_result=local_result;
    stk::all_reduce_sum(comm,&local_result,&glob_result,1u);
    result=std::complex<Scalar>(glob_result,0.0);
}

template<class Scalar>
inline
void field_asum(
        Scalar & result,
        const FieldBase & xFieldBase,
        const Selector selector)
{
    const MPI_Comm comm = xFieldBase.get_mesh().parallel();
    field_asum(result,xFieldBase,selector,comm);
}

template<class Scalar>
inline
void field_asum(
        Scalar & result,
        const FieldBase & xFieldBase)
{
    const Selector selector = selectField(xFieldBase);
    field_asum(result,xFieldBase,selector);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Entity field_eamax(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Selector selector)
{
    BucketVector const& buckets = xField.get_mesh().get_buckets(xField.entity_rank(),
                                                                selector & xField.mesh_meta_data().locally_owned_part());

    int priv_iamax;
    Scalar priv_amax;
    Entity priv_result;
    Scalar local_amax=Scalar(-2.0);
    Entity local_result=Entity();

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel private(priv_iamax,priv_amax,priv_result)
    {
#endif
        priv_amax=Scalar(-1.0);
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
#pragma omp for schedule(static) reduction(max:local_amax)
#endif
        for(size_t i=0; i < buckets.size(); i++)
        {
            Bucket & b = *buckets[i];
            const Bucket::size_type length = b.size();
            const unsigned int fieldSize = field_scalars_per_entity(xField, b);
            const int kmax = length * fieldSize;
            Scalar * x = static_cast<Scalar*>(field_data(xField, b));

            priv_iamax = FortranBLAS<Scalar>::iamax(kmax,x);
            if (priv_amax<std::abs(x[priv_iamax]))
            {
                priv_result = b[priv_iamax];
                priv_amax = std::abs(x[priv_iamax]);
                local_amax = priv_amax;
            }
        }
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
        if (local_amax==priv_amax)
        {
#endif
            local_result=priv_result;
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
        }
    }
#endif

    EntityId local_EntityId = xField.get_mesh().identifier(local_result);
    EntityId  glob_EntityId = local_EntityId;
    Scalar glob_amax = local_amax;
    stk::all_reduce_maxloc(xField.get_mesh().parallel(),&local_amax,&local_EntityId,&glob_amax,&glob_EntityId,1u);
    if (glob_EntityId==local_EntityId)
    {
        local_result = xField.get_mesh().get_entity(xField.entity_rank(),glob_EntityId);
    } else {
        local_result = Entity();
    }
    return local_result;
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Entity field_eamax(
        const Field<std::complex<Scalar>,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Selector selector)
{
    BucketVector const& buckets = xField.get_mesh().get_buckets(xField.entity_rank(),
                                                                selector & xField.mesh_meta_data().locally_owned_part());

    int priv_iamax;
    Scalar priv_amax;
    Entity priv_result;
    Scalar local_amax=Scalar(-2.0);
    Entity local_result=Entity();

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel private(priv_iamax,priv_amax,priv_result)
    {
#endif
        priv_amax=Scalar(-1.0);
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
#pragma omp for schedule(static) reduction(max:local_amax)
#endif
        for(size_t i=0; i < buckets.size(); i++)
        {
            Bucket & b = *buckets[i];
            const Bucket::size_type length = b.size();
            const unsigned int fieldSize = field_scalars_per_entity(xField, b);
            const int kmax = length * fieldSize;
            std::complex<Scalar>* x = static_cast<std::complex<Scalar>*>(field_data(xField, b));

            priv_iamax = FortranBLAS<std::complex<Scalar> >::iamax(kmax,x);
            if (priv_amax<std::abs(x[priv_iamax]))
            {
                priv_result = b[priv_iamax];
                priv_amax = std::abs(x[priv_iamax]);
                local_amax = priv_amax;
            }
        }
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
        if (local_amax==priv_amax)
        {
#endif
            local_result=priv_result;
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
        }
    }
#endif

    EntityId     local_EntityId = xField.get_mesh().identifier(local_result);
    EntityId glob_EntityId = local_EntityId;
    Scalar global_amax = local_amax;
    stk::all_reduce_maxloc(xField.get_mesh().parallel(),&local_amax,&local_EntityId,&global_amax,&glob_EntityId,1u);
    if (glob_EntityId==local_EntityId)
    {
        local_result = xField.get_mesh().get_entity(xField.entity_rank(),glob_EntityId);
    } else {
        local_result = Entity();
    }
    return local_result;
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Entity field_eamax(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField)
{
    const Selector selector = selectField(xField);
    return field_eamax(xField,selector);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Scalar field_amax(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Selector selector,
        const MPI_Comm comm)
{
    BucketVector const& buckets = xField.get_mesh().get_buckets(xField.entity_rank(),
                                                                selector & xField.mesh_meta_data().locally_owned_part());

    Scalar priv_tmp;
    Scalar local_amax=Scalar(-1.0);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static) reduction(max:local_amax) private(priv_tmp)
#endif
    for(size_t i=0; i < buckets.size(); i++)
    {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xField, b);
        const int kmax = length * fieldSize;
        Scalar * x = static_cast<Scalar*>(field_data(xField, b));

        priv_tmp = std::abs(x[FortranBLAS<Scalar>::iamax(kmax,x)]);
        if (local_amax < priv_tmp) {
            local_amax = priv_tmp;
        }
    }

    Scalar global_amax = local_amax;
    stk::all_reduce_max(comm,&local_amax,&global_amax,1u);
    return global_amax;
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
std::complex<Scalar> field_amax(
        const Field<std::complex<Scalar>,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Selector selector,
        const MPI_Comm comm) {

    BucketVector const& buckets = xField.get_mesh().get_buckets(xField.entity_rank(),
                                                                selector & xField.mesh_meta_data().locally_owned_part());

    Scalar priv_tmp;
    Scalar local_amax=Scalar(0.0);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static) reduction(max:local_amax) private(priv_tmp)
#endif
    for(size_t i=0; i < buckets.size(); i++)
    {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xField, b);
        const int kmax = length * fieldSize;
        std::complex<Scalar>* x = static_cast<std::complex<Scalar>*>(field_data(xField, b));

        priv_tmp = std::abs(x[FortranBLAS<std::complex<Scalar> >::iamax(kmax,x)]);
        if (local_amax < priv_tmp) {
            local_amax = priv_tmp;
        }
    }

    Scalar glob_amax = local_amax;
    stk::all_reduce_max(comm,&local_amax,&glob_amax,1u);
    return std::complex<Scalar>(glob_amax,0.0);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Scalar field_amax(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Selector selector)
{
    const MPI_Comm comm = xField.get_mesh().parallel();
    return field_amax(xField,selector,comm);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Scalar field_amax(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField)
{
    const Selector selector = selectField(xField);
    return field_amax(xField,selector);
}

template<class Scalar>
inline
Entity INTERNAL_field_eamax_complex(
        const FieldBase & xFieldBase,
        const Selector selector)
{
    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets(xFieldBase.entity_rank(),
                                                                    selector & xFieldBase.mesh_meta_data().locally_owned_part());

    int priv_iamax;
    Scalar priv_amax;
    Entity priv_result;
    Scalar local_amax=Scalar(-2.0);
    Entity local_result=Entity();

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel private(priv_iamax,priv_amax,priv_result)
    {
#endif
        priv_amax=Scalar(-1.0);
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
#pragma omp for schedule(static) reduction(max:local_amax)
#endif
        for(size_t i=0; i < buckets.size(); i++)
        {
            Bucket & b = *buckets[i];
            const Bucket::size_type length = b.size();
            const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
            const int kmax = length * fieldSize;
            std::complex<Scalar>* x = static_cast<std::complex<Scalar>*>(field_data(xFieldBase, b));

            priv_iamax = FortranBLAS<std::complex<Scalar> >::iamax(kmax,x);
            if (priv_amax < std::norm(x[priv_iamax])) {
                priv_result = b[priv_iamax];
                priv_amax = std::norm(x[priv_iamax]);
                local_amax = priv_amax;
            }
        }
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
        if (local_amax == priv_amax)
        {
#endif
            local_result = priv_result;
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
        }
    }
#endif

    EntityId local_EntityId = xFieldBase.get_mesh().identifier(local_result);
    EntityId  glob_EntityId = local_EntityId;
    Scalar glob_amax = local_amax;
    stk::all_reduce_maxloc(xFieldBase.get_mesh().parallel(),&local_amax,&local_EntityId,&glob_amax,&glob_EntityId,1u);
    if (glob_EntityId == local_EntityId) {
        local_result = xFieldBase.get_mesh().get_entity(xFieldBase.entity_rank(),glob_EntityId);
    } else {
        local_result = Entity();
    }
    return local_result;
}

template<class Scalar>
inline
Entity INTERNAL_field_eamax(
        const FieldBase & xFieldBase,
        const Selector selector)
{
    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets(xFieldBase.entity_rank(),
                                                                    selector & xFieldBase.mesh_meta_data().locally_owned_part());

    int priv_iamax;
    Scalar priv_amax;
    Entity priv_result;
    Scalar local_amax = Scalar(-2.0);
    Entity local_result = Entity();

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel private(priv_iamax,priv_amax,priv_result)
    {
#endif
        priv_amax=Scalar(-1.0);
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
#pragma omp for schedule(static) reduction(max:local_amax)
#endif
        for(size_t i=0; i < buckets.size(); i++)
        {
            Bucket & b = *buckets[i];
            const Bucket::size_type length = b.size();
            const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
            const int kmax = length * fieldSize;
            Scalar* x = static_cast<Scalar*>(field_data(xFieldBase, b));

            priv_iamax = FortranBLAS<Scalar>::iamax(kmax,x);
            if (priv_amax<std::abs(x[priv_iamax]))
            {
                priv_result = b[priv_iamax];
                priv_amax = std::abs(x[priv_iamax]);
                local_amax = priv_amax;
            }
        }
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
        if (local_amax==priv_amax)
        {
#endif
            local_result=priv_result;
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
        }
    }
#endif

    EntityId local_EntityId = xFieldBase.get_mesh().identifier(local_result);
    EntityId glob_EntityId = local_EntityId;
    Scalar glob_amax = local_amax;
    stk::all_reduce_maxloc(xFieldBase.get_mesh().parallel(),&local_amax,&local_EntityId,&glob_amax,&glob_EntityId,1u);
    if (glob_EntityId==local_EntityId)
    {
        local_result = xFieldBase.get_mesh().get_entity(xFieldBase.entity_rank(),glob_EntityId);
    } else {
        local_result = Entity();
    }
    return local_result;
}

inline
Entity field_eamax(
        const FieldBase & xFieldBase,
        const Selector selector)
{
    if (xFieldBase.data_traits().type_info == typeid(double)) {
        return INTERNAL_field_eamax<double>(xFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(float)) {
        return INTERNAL_field_eamax<float>(xFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(std::complex<double>)) {
        return INTERNAL_field_eamax_complex<double>(xFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(std::complex<float>)) {
        return INTERNAL_field_eamax_complex<float>(xFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(int)) {
        return INTERNAL_field_eamax<int>(xFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(std::complex<int>)) {
        return INTERNAL_field_eamax_complex<int>(xFieldBase,selector);
    } else {
        ThrowAssertMsg(false,"Error in field_eamax; field is of type "<<xFieldBase.data_traits().type_info.name()<<" which is not supported");
    }
    return stk::mesh::Entity();
}

inline
Entity field_eamax(
        const FieldBase & xFieldBase)
{
    const Selector selector = selectField(xFieldBase);
    return field_eamax(xFieldBase,selector);
}

template<class Scalar>
inline
void field_amax(
        std::complex<Scalar> & result,
        const FieldBase & xFieldBase,
        const Selector selector,
        const MPI_Comm comm)
{
    ThrowAssert(xFieldBase.data_traits().type_info == typeid(std::complex<Scalar>));

    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets(xFieldBase.entity_rank(),
                                                                    selector & xFieldBase.mesh_meta_data().locally_owned_part());

    Scalar priv_tmp;
    Scalar local_amax=Scalar(-1.0);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static) reduction(max:local_amax) private(priv_tmp)
#endif
    for(size_t i=0; i < buckets.size(); i++)
    {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
        const int kmax = length * fieldSize;
        std::complex<Scalar>* x = static_cast<std::complex<Scalar>*>(field_data(xFieldBase, b));

        priv_tmp = std::abs(x[FortranBLAS<std::complex<Scalar> >::iamax(kmax,x)]);
        if (local_amax < priv_tmp) {
            local_amax = priv_tmp;
        }
    }

    Scalar glob_amax = local_amax;
    stk::all_reduce_max(comm,&local_amax,&glob_amax,1u);
    result = std::complex<Scalar>(glob_amax,0.0);
}

template<class Scalar>
inline
void field_amax(
        Scalar & result,
        const FieldBase & xFieldBase,
        const Selector selector,
        const MPI_Comm comm)
{
    ThrowAssert(xFieldBase.data_traits().type_info == typeid(Scalar));

    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets(xFieldBase.entity_rank(),
                                                                    selector & xFieldBase.mesh_meta_data().locally_owned_part());

    Scalar priv_tmp;
    Scalar local_amax=Scalar(-1.0);

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static) reduction(max:local_amax) private(priv_tmp)
#endif
    for(size_t i=0; i < buckets.size(); i++)
    {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
        const int kmax = length * fieldSize;
        Scalar * x = static_cast<Scalar*>(field_data(xFieldBase, b));

        priv_tmp = std::abs(x[FortranBLAS<Scalar>::iamax(kmax,x)]);
        if (local_amax < priv_tmp) {
            local_amax = priv_tmp;
        }
    }

    Scalar glob_amax = local_amax;
    stk::all_reduce_max(comm,&local_amax,&glob_amax,1u);
    result = glob_amax;
}

template<class Scalar>
inline
void field_amax(
        Scalar & result,
        const FieldBase & xFieldBase,
        const Selector selector)
{
    const MPI_Comm comm = xFieldBase.get_mesh().parallel();
    field_amax(result,xFieldBase,selector,comm);
}

template<class Scalar>
inline
void field_amax(
        Scalar & result,
        const FieldBase & xFieldBase)
{
    const Selector selector = selectField(xFieldBase);
    field_amax(result,xFieldBase,selector);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Entity field_eamin(
        const Field<std::complex<Scalar>,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Selector selector)
{
    BucketVector const& buckets = xField.get_mesh().get_buckets(xField.entity_rank(),
                                                                selector & xField.mesh_meta_data().locally_owned_part());

    int priv_iamin;
    Scalar priv_amin;
    Entity priv_result;
    Scalar local_amin=std::numeric_limits<Scalar>::max();
    Entity local_result;
    Entity glob_result;

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel private(priv_iamin,priv_amin,priv_result)
    {
#endif
        priv_amin=std::numeric_limits<Scalar>::max();
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
#pragma omp for schedule(static) reduction(min:local_amin)
#endif
        for(size_t i=0; i < buckets.size(); i++)
        {
            Bucket & b = *buckets[i];
            const Bucket::size_type length = b.size();
            const unsigned int fieldSize = field_scalars_per_entity(xField, b);
            const int kmax = length * fieldSize;
            std::complex<Scalar>* x = static_cast<std::complex<Scalar>*>(field_data(xField, b));

            priv_iamin = FortranBLAS<std::complex<Scalar> >::iamin(kmax,x);
            if (std::abs(x[priv_iamin])<priv_amin) {
                priv_result = b[priv_iamin];
                priv_amin = std::abs(x[priv_iamin]);
                local_amin = priv_amin;
            }
        }
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
        if (priv_amin==local_amin)
        {
#endif
            local_result = priv_result;
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
        }
    }
#endif

    EntityId local_EntityId = xField.get_mesh().identifier(local_result);
    EntityId glob_EntityId = local_EntityId;
    Scalar glob_amin = local_amin;
    stk::all_reduce_minloc(xField.get_mesh().parallel(),&local_amin,&local_EntityId,&glob_amin,&glob_EntityId,1u);
    if (glob_EntityId == local_EntityId) {
        glob_result = xField.get_mesh().get_entity(xField.entity_rank(),glob_EntityId);
    } else {
        glob_result = Entity();
    }
    return glob_result;
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Entity field_eamin(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Selector selector)
{
    BucketVector const& buckets = xField.get_mesh().get_buckets(xField.entity_rank(),
                                                                selector & xField.mesh_meta_data().locally_owned_part());

    int priv_iamin;
    Scalar priv_amin;
    Entity priv_result;
    Scalar local_amin=std::numeric_limits<Scalar>::max();
    Entity local_result;
    Entity glob_result;

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel private(priv_iamin,priv_amin,priv_result)
    {
#endif
        priv_amin=std::numeric_limits<Scalar>::max();
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
#pragma omp for schedule(static) reduction(min:local_amin)
#endif
        for(size_t i=0; i < buckets.size(); i++)
        {
            Bucket & b = *buckets[i];
            const Bucket::size_type length = b.size();
            const unsigned int fieldSize = field_scalars_per_entity(xField, b);
            const int kmax = length * fieldSize;
            Scalar * x = static_cast<Scalar*>(field_data(xField, b));

            priv_iamin = FortranBLAS<Scalar>::iamin(kmax,x);
            if (std::abs(x[priv_iamin])<priv_amin)
            {
                priv_result = b[priv_iamin];
                priv_amin = std::abs(x[priv_iamin]);
                local_amin = priv_amin;
            }
        }
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
        if (priv_amin == local_amin) {
#endif
            local_result = priv_result;
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
        }
    }
#endif

    EntityId local_EntityId = xField.get_mesh().identifier(local_result);
    EntityId glob_EntityId = local_EntityId;
    Scalar glob_amin = local_amin;
    stk::all_reduce_minloc(xField.get_mesh().parallel(),&local_amin,&local_EntityId,&glob_amin,&glob_EntityId,1u);
    if (glob_EntityId == local_EntityId) {
        glob_result = xField.get_mesh().get_entity(xField.entity_rank(),glob_EntityId);
    } else {
        glob_result = Entity();
    }
    return glob_result;
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Entity field_eamin(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField)
{
    const Selector selector = selectField(xField);
    return field_eamin(xField,selector);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
std::complex<Scalar> field_amin(
        const Field<std::complex<Scalar>,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Selector selector,
        const MPI_Comm comm) {
    BucketVector const& buckets = xField.get_mesh().get_buckets(xField.entity_rank(),
                                                                selector & xField.mesh_meta_data().locally_owned_part());

    Scalar priv_tmp;
    Scalar local_amin= std::numeric_limits<Scalar>::max();

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static) reduction(min:local_amin) private(priv_tmp)
#endif
    for(size_t i=0; i < buckets.size(); i++)
    {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xField, b);
        const int kmax = length * fieldSize;
        std::complex<Scalar> * x = static_cast<std::complex<Scalar>*>(field_data(xField, b));

        priv_tmp = std::norm(x[FortranBLAS<std::complex<Scalar> >::iamin(kmax,x)]);
        if (priv_tmp < local_amin)
        {
            local_amin = priv_tmp;
        }
    }

    Scalar glob_amin = local_amin;
    stk::all_reduce_min(comm,&local_amin,&glob_amin,1u);
    return sqrt(glob_amin);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Scalar field_amin(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Selector selector,
        const MPI_Comm comm)
{
    BucketVector const& buckets = xField.get_mesh().get_buckets(xField.entity_rank(),
                                                                selector & xField.mesh_meta_data().locally_owned_part());

    Scalar priv_tmp;
    Scalar local_amin= std::numeric_limits<Scalar>::max();

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static) reduction(min:local_amin) private(priv_tmp)
#endif
    for(size_t i=0; i < buckets.size(); i++)
    {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xField, b);
        const int kmax = length * fieldSize;
        Scalar * x = static_cast<Scalar*>(field_data(xField, b));

        priv_tmp = std::abs(x[FortranBLAS<Scalar>::iamin(kmax,x)]);
        if (priv_tmp < local_amin) {
            local_amin = priv_tmp;
        }
    }

    Scalar glob_amin = local_amin;
    stk::all_reduce_min(comm,&local_amin,&glob_amin,1u);
    return glob_amin;
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Scalar field_amin(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField,
        const Selector selector)
{
    const MPI_Comm comm = xField.get_mesh().parallel();
    return field_amin(xField,selector,comm);
}

template<class Scalar,class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline
Scalar field_amin(
        const Field<Scalar,T1,T2,T3,T4,T5,T6,T7> & xField)
{
    const Selector selector = selectField(xField);
    return field_amin(xField,selector);
}

template<class Scalar>
inline
Entity INTERNAL_field_eamin_complex(
        const FieldBase & xFieldBase,
        const Selector selector) {
    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets(xFieldBase.entity_rank(),
                                                                    selector & xFieldBase.mesh_meta_data().locally_owned_part());

    int priv_iamin;
    double priv_amin;
    Entity priv_result;
    double local_amin= std::numeric_limits<double>::max();
    Entity local_result;
    Entity glob_result;

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel private(priv_iamin,priv_amin,priv_result)
    {
#endif
        priv_amin=std::numeric_limits<double>::max();
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
#pragma omp for schedule(static) reduction(min:local_amin)
#endif
        for(size_t i=0; i < buckets.size(); i++)
        {
            Bucket & b = *buckets[i];
            const Bucket::size_type length = b.size();
            const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
            const int kmax = length * fieldSize;
            std::complex<Scalar>* x = static_cast<std::complex<Scalar>*>(field_data(xFieldBase, b));

            priv_iamin = FortranBLAS<std::complex<Scalar> >::iamin(kmax,x);
            if (std::norm(x[priv_iamin])<priv_amin)
            {
                priv_result = b[priv_iamin];
                priv_amin = std::norm(x[priv_iamin]);
                local_amin = priv_amin;
            }
        }
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
        if (priv_amin == local_amin) {
#endif
            local_result=priv_result;
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
        }
    }
#endif

    EntityId local_EntityId = xFieldBase.get_mesh().identifier(local_result);
    EntityId glob_EntityId = local_EntityId;
    double glob_amin = local_amin;
    stk::all_reduce_minloc(xFieldBase.get_mesh().parallel(),&local_amin,&local_EntityId,&glob_amin,&glob_EntityId,1u);
    if (glob_EntityId == local_EntityId) {
        glob_result = xFieldBase.get_mesh().get_entity(xFieldBase.entity_rank(),glob_EntityId);
    } else {
        glob_result = Entity();
    }
    return glob_result;
}

template<class Scalar>
inline
Entity INTERNAL_field_eamin(
        const FieldBase & xFieldBase,
        const Selector selector) {
    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets(xFieldBase.entity_rank(),
                                                                    selector & xFieldBase.mesh_meta_data().locally_owned_part());

    int priv_iamin;
    double priv_amin;
    Entity priv_result;
    double local_amin= std::numeric_limits<double>::max();
    Entity local_result;
    Entity glob_result;

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel private(priv_iamin,priv_amin,priv_result)
    {
#endif
        priv_amin=std::numeric_limits<double>::max();
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
#pragma omp for schedule(static) reduction(min:local_amin)
#endif
        for(size_t i=0; i < buckets.size(); i++)
        {
            Bucket & b = *buckets[i];
            const Bucket::size_type length = b.size();
            const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
            const int kmax = length * fieldSize;
            Scalar * x = static_cast<Scalar*>(field_data(xFieldBase, b));

            priv_iamin = FortranBLAS<Scalar>::iamin(kmax,x);
            if (std::abs(x[priv_iamin])<priv_amin)
            {
                priv_result = b[priv_iamin];
                priv_amin = std::abs(x[priv_iamin]);
                local_amin = priv_amin;
            }
        }
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
        if (priv_amin==local_amin)
        {
#endif
            local_result=priv_result;
#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
        }
    }
#endif

    EntityId local_EntityId = xFieldBase.get_mesh().identifier(local_result);
    EntityId glob_EntityId = local_EntityId;
    double glob_amin = local_amin;
    stk::all_reduce_minloc(xFieldBase.get_mesh().parallel(),&local_amin,&local_EntityId,&glob_amin,&glob_EntityId,1u);
   if (glob_EntityId == local_EntityId) {
        glob_result = xFieldBase.get_mesh().get_entity(xFieldBase.entity_rank(),glob_EntityId);
    } else {
        glob_result = Entity();
    }
    return glob_result;
}

inline
Entity field_eamin(
        const FieldBase & xFieldBase,
        const Selector selector)
{
    if (xFieldBase.data_traits().type_info == typeid(double)) {
        return INTERNAL_field_eamin<double>(xFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(float)) {
        return INTERNAL_field_eamin<float>(xFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(std::complex<double>)) {
        return INTERNAL_field_eamin_complex<double>(xFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(std::complex<float>)) {
        return INTERNAL_field_eamin_complex<float>(xFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(int)) {
        return INTERNAL_field_eamin<int>(xFieldBase,selector);
    } else if (xFieldBase.data_traits().type_info == typeid(std::complex<int>)) {
        return INTERNAL_field_eamin_complex<int>(xFieldBase,selector);
    } else {
        ThrowAssertMsg(false,"Error in field_eamin; field is of type "<<xFieldBase.data_traits().type_info.name()<<" which is not supported");
    }
    return stk::mesh::Entity();
}

inline
Entity field_eamin(
        const FieldBase & xFieldBase)
{
    const Selector selector = selectField(xFieldBase);
    return field_eamin(xFieldBase,selector);
}

template<class Scalar>
inline
void field_amin(
        std::complex<Scalar> & result,
        const FieldBase & xFieldBase,
        const Selector selector,
        const MPI_Comm comm)
{
    ThrowAssert(xFieldBase.data_traits().type_info == typeid(std::complex<Scalar>));

    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets(xFieldBase.entity_rank(),
                                                                    selector & xFieldBase.mesh_meta_data().locally_owned_part());

    Scalar priv_tmp;
    Scalar local_amin= std::numeric_limits<Scalar>::max();

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static) reduction(min:local_amin) private(priv_tmp)
#endif
    for(size_t i=0; i < buckets.size(); i++)
    {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
        const int kmax = length * fieldSize;
        std::complex<Scalar>* x = static_cast<std::complex<Scalar>*>(field_data(xFieldBase, b));

        priv_tmp = std::abs(x[FortranBLAS<std::complex<Scalar> >::iamin(kmax,x)]);
        if (priv_tmp<local_amin)
        {
            local_amin=priv_tmp;
        }
    }

    Scalar glob_amin = local_amin;
    stk::all_reduce_min(comm,&local_amin,&glob_amin,1u);
    result=std::complex<Scalar>(glob_amin,0.0);
}

template<class Scalar>
inline
void field_amin(
        Scalar & result,
        const FieldBase & xFieldBase,
        const Selector selector,
        const MPI_Comm comm)
{
    ThrowAssert(xFieldBase.data_traits().type_info == typeid(Scalar));

    BucketVector const& buckets = xFieldBase.get_mesh().get_buckets(xFieldBase.entity_rank(),
                                                                    selector & xFieldBase.mesh_meta_data().locally_owned_part());

    Scalar priv_tmp;
    Scalar local_amin= std::numeric_limits<Scalar>::max();

#ifdef OPEN_MP_ACTIVE_FIELDBLAS_HPP
    if (omp_get_max_threads() >= omp_get_num_procs()) {omp_set_num_threads(1);}
#pragma omp parallel for schedule(static) reduction(min:local_amin) private(priv_tmp)
#endif
    for(size_t i=0; i < buckets.size(); i++)
    {
        Bucket & b = *buckets[i];
        const Bucket::size_type length = b.size();
        const unsigned int fieldSize = field_scalars_per_entity(xFieldBase, b);
        const int kmax = length * fieldSize;
        Scalar * x = static_cast<Scalar*>(field_data(xFieldBase, b));

        priv_tmp = std::abs(x[FortranBLAS<Scalar>::iamin(kmax,x)]);
        if (priv_tmp<local_amin)
        {
            local_amin=priv_tmp;
        }
    }
    Scalar glob_amin = local_amin;
    stk::all_reduce_min(comm,&local_amin,&glob_amin,1u);
    result=glob_amin;
}

template<class Scalar>
inline
void field_amin(
        Scalar & result,
        const FieldBase & xFieldBase,
        const Selector selector)
{
    const MPI_Comm comm = xFieldBase.get_mesh().parallel();
    field_amin(result,xFieldBase,selector,comm);
}

template<class Scalar>
inline
void field_amin(
        Scalar & result,
        const FieldBase & xFieldBase)
{
    const Selector selector = selectField(xFieldBase);
    field_amin(result,xFieldBase,selector);
}

} // mesh
} // stk

#endif // STK_MESH_BASE_FIELDBLAS_HPP

