/*!
 \file dbscan_c.h
 \brief Header file for the C/C+GPU implementations of the DBSCAN algorithm
 \details
 This header file contains three method prototypes, that allow to perform single- or multithreaded CPU or
 GPU based DBSCAN cluster searches.
 \copyright Copyright Robert Fritze 2021
 \license MIT
 \version 1.0
 \author Robert Fritze
 \warning This file is machine generated
 \date 11.9.2021
 */


/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class androiddm_dbscan */

#ifndef _Included_androiddm_dbscan
#define _Included_androiddm_dbscan
#ifdef __cplusplus
extern "C" {
#endif

/*!
 \details
 Performs a DBSCAN cluster search on the input data with one thread.
 \param env JNI environment variable
 \param jc JNI class variable
 \param b (out) Array of cluster numbers (0=noise point)
 \param rf (in) Array of data points
 \param eps search radius
 \param kk number of neighbours
 \param features number of features per data item contained in the data array
 \returns number of clusters found (can be zero if only noise points have been detected) or - if negative - an error code
 \mt fully threadsafe
 */
JNIEXPORT jshort JNICALL Java_com_example_dmocl_dbscan_dbscan_1c
  (JNIEnv *env, jclass jc, jshortArray b, jfloatArray rf, jfloat eps, jint kk, jint features);

/*!
 \details
 Performs a DBSCAN cluster search on the GPU.
 \param env JNI environment variable
 \param jc JNI class variable
 \param b (out) Array of cluster numbers (0=noise point)
 \param rf (in) Array of data points
 \param eps search radius
 \param kk number of neighbours
 \param features number of features per data item contained in the data array
 \param e (out) Array of exactly one long value, contains the exclusive time needed (in ns)
 \returns number of clusters found (can be zero if only noise points have been detected) or - if negative - an error code
 \mt fully threadsafe
 */
JNIEXPORT jshort JNICALL
Java_com_example_dmocl_dbscan_dbscan_1c_1gpu
  (JNIEnv *env, jclass jc, jshortArray b, jfloatArray rf, jfloat eps, jint kk, jint features,
   jlongArray e);


/*!
 \details
 Performs a DBSCAN cluster search on the input data with multiple threads.
 \param env JNI environment variable
 \param jc JNI class variable
 \param b (out) Array of cluster numbers (0=noise point)
 \param rf (in) Array of data points
 \param eps search radius
 \param kk number of neighbours
 \param features number of features per data item contained in the data array
 \param cores number of cores that should be used
 \param e (out) Array of exactly one long value, contains the exclusive time needed (in ns)
 \returns number of clusters found (can be zero if only noise points have been detected) or - if negative - an error code
 \mt fully threadsafe
 */
JNIEXPORT jshort JNICALL
Java_com_example_dmocl_dbscan_dbscan_1c_1phtreads
  (JNIEnv *env, jclass jc, jshortArray b, jfloatArray rf, jfloat eps, jint kk, jint features,
   jint cores, jlongArray e);

#ifdef __cplusplus
}
#endif
#endif
