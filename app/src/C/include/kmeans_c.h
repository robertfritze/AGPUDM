/*!
 \file kmeans_c.h
 \brief Header file for the C/C+GPU implementations of the Kmeans algorithm
 \details
 This header file contains three method prototypes, that allow to perform single- or multithreaded CPU or
 GPU based Kmeans cluster searches.
 \copyright Copyright Robert Fritze 2021
 \license MIT
 \version 1.0
 \author Robert Fritze
 \warning This file is machine generated
 \date 11.9.2021
 */

/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class androiddm_kmeans */

#ifndef _Included_androiddm_kmeans
#define _Included_androiddm_kmeans
#ifdef __cplusplus
extern "C" {
#endif


/*!
 \details
 Performs a Kmeans cluster search on the CPU (one thread).
 \param env JNI environment variable
 \param jc JNI class variable
 \param b (out) Array of cluster numbers (0=noise point)
 \param rf (in) Array of data points
 \param eps (in) search radius
 \param kk (in) number of clusters to search for
 \param features (in) number of features per data item contained in the data array
 \returns 0 = no error, <0 = error number
 \mt fully threadsafe
 */
JNIEXPORT jshort JNICALL Java_com_example_dmocl_kmeans_kmeans_1c
  (JNIEnv *env, jclass jc, jshortArray b, jfloatArray rf, jfloat eps, jint kk, jint features);

/*!
 \details
 Performs a Kmeans cluster search on the GPU.
 \param env JNI environment variable
 \param jc JNI class variable
 \param b (out) Array of cluster numbers (0=noise point)
 \param rf (in) Array of data points
 \param eps (in) search radius
 \param cluno (in) number of clusters to search for
 \param features (in) number of features per data item contained in the data array
 \param e (out) Array of exactly one long value, contains the exclusive time needed (in ns)
 \returns 0 = no error, <0 = error number
 \mt fully threadsafe
 */
JNIEXPORT jshort JNICALL Java_com_example_dmocl_kmeans_kmeans_1c_1gpu
  (JNIEnv *env, jclass jc, jshortArray b, jfloatArray rf, jfloat eps, jint cluno, jint features,
   jlongArray e);

/*!
 \details
 Performs a Kmeans cluster search on the CPU (multiple threads).
 \param env JNI environment variable
 \param jc JNI class variable
 \param b (out) Array of cluster numbers
 \param rf (in) Array of data points
 \param eps (in) search radius
 \param cluno (in) numbers of clusters that should be found
 \param features (in) number of features per data item contained in the data array
 \param cores (in) number of cores that should be used
 \param e (out) Array of exactly one long value, contains the exclusive time needed (in ns)
 \returns 0 = no error, <0 = error number
 \mt fully threadsafe
 */
JNIEXPORT jshort JNICALL Java_com_example_dmocl_kmeans_kmeans_1c_1phtreads
  (JNIEnv *env, jclass jc, jshortArray b, jfloatArray rf, jfloat eps, jint cluno, jint features,
   jint cores, jlongArray e );




/*!
 \details
 Signals all running Kmeans algorithms to abort immediately.
 Any new Kmeans cluster search will be aborted imediately.
 \warning This function acts on a 'global' scale: All callers that use this library will
 not be any more able to make calls to the library functions of this library.
 \param env JNI environment variable
 \param clazz JNI class variable
 \mt fully threadsafe
 */
JNIEXPORT void JNICALL
Java_com_example_dmocl_kmeans_kmabort_1c(JNIEnv *env, jclass clazz);


/*!
 \details
 Allows to make new Kmeans cluster searches.
 \warning This function acts on a 'global' scale. It reverts the effect of
 Java_com_example_dmocl_kmeans_kmabort_1c.
 \param env JNI environment variable
 \param clazz JNI class variable
 \mt fully threadsafe
 */
JNIEXPORT void JNICALL
Java_com_example_dmocl_kmeans_kmresume_1c(JNIEnv *env, jclass clazz);


#ifdef __cplusplus
}
#endif
#endif

