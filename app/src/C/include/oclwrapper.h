/*!
 \file oclwrapper.h
 \brief Defines the default target OpenCL version
 \details
 Defines the default target OpenCL version
 \copyright Copyright Robert Fritze 2021
 \license MIT
 \version 1.0
 \warning This header file must be included **BEFORE** any OpenCL header file
 \author Robert Fritze
 \date 11.9.2021
 */


#ifndef OPENCLAPP_OCLWRAPPER_H
#define OPENCLAPP_OCLWRAPPER_H

                                        //! unknown architecture
#define UNKNOWN -1
                                        //! arm-v7
#define ARM32 0
                                        //! arm-v8
#define ARM64 1
                                        //! intel x86
#define INTEL32 2
                                        //! intel x86_64
#define INTEL64 3


                                      //! the default target OpenCL version
#define CL_TARGET_OPENCL_VERSION 120


/*!
 \brief Checks if CLANG has been used for compilation
 \param env pointer to JNI environment
 \param clazz reference to JNI class
 \return 1 compiled by CLANG, 0 compiled with other compiler
 \mt fully threadsafe
 */
JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_isCLang(JNIEnv *env, jclass clazz);


/*!
 \brief Returns the CLANG major version number
 \param env pointer to JNI environment
 \param clazz reference to JNI class
 \return version number or -1 if not compiled with CLANG
 \mt fully threadsafe
 */
JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_getCLmaj(JNIEnv *env, jclass clazz);


/*!
 \brief Returns the CLANG minor version number
 \param env pointer to JNI environment
 \param clazz reference to JNI class
 \return version number or -1 if not compiled with CLANG
 \mt fully threadsafe
 */
 JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_getCLmin(JNIEnv *env, jclass clazz);

/*!
 \brief Returns the CLANG patch version number
 \param env pointer to JNI environment
 \param clazz reference to JNI class
 \return version number or -1 if not compiled with CLANG
 \mt fully threadsafe
 */
JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_getCLpatch(JNIEnv *env, jclass clazz);




/*!
 \brief Java wrapper function to load the native OpenCL library
 \param env pointer to JNI environment
 \param thiz reference to JNI class
 \param s (in) path and name of the OpenCL library on the device
 \return OK: 0, library has already been loaded: -1, unable to load library: -2
 \mt fully threadsafe
 */
JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_loadOpenCL(JNIEnv *env, jobject thiz, jstring s );


/*!
 \brief Java wrapper function to unload the native OpenCL library
 \param env pointer to JNI environment
 \param thiz reference to JNI class
 \mt fully threadsafe
 */
 JNIEXPORT void JNICALL
Java_com_example_dmocl_oclwrap_unloadOpenCL(JNIEnv *env, jobject thiz);


/*!
\brief Returns the number of OpenCL platforms
\param env pointer to JNI environment
\param thiz reference to JNI class
\return >=0 number of platfroms, <0 error occurred
\mt fully threadsafe (if native OpenCL function is threadsafe)
*/
JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_AndrCLGetPlatformCnt(JNIEnv *env, jobject thiz);


/*!
\brief Returns the number of OpenCL devices for a given platform
\param env pointer to JNI environment
\param thiz reference to JNI class
\param i (in) platform number
\return >=0 number of devices, <0 error occurred
\mt fully threadsafe (if native OpenCL function is threadsafe)
*/
JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_AndrCLGetDeviceCnt(JNIEnv *env, jobject thiz, jint i);


/*!
\brief Returns some info for a given OpenCL device (and platform number)
\param env pointer to JNI environment
\param thiz reference to JNI class
\param platf (in) platform number
\param dev (in) device number
\return an instance of the class *oclinforet* with the information filled in
\mt fully threadsafe (if native OpenCL function is threadsafe)
*/
JNIEXPORT jobject JNICALL
Java_com_example_dmocl_oclwrap_AndrCLgetDeviceName(JNIEnv *env, jobject thiz, jint platf,
                                                   jint dev);


/*!
\brief Returns the CPU architecture
\param env pointer to JNI environment
\param clazz reference to JNI class
\return One of the constants ARM32, ARM64, INTEL32, INTEL64, UNKNOWN
\mt fully threadsafe
*/
JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_getArchitecture(JNIEnv *env, jclass clazz);


#endif OPENCLAPP_OCLWRAPPER_H
