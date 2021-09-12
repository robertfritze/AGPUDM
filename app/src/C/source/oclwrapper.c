/*!
 \file oclwrapper.c
 \brief Helper functions for OpenCL devices to be called directly form JAVA
 \details
 This source file contains some helper functions that allow to read out system information and
 some OpenCL device information directly without the need of C.
 \copyright Copyright Robert Fritze 2021
 \license MIT
 \version 1.0
 \author Robert Fritze
 \date 11.9.2021
 */

#ifdef __clang__
                                          //! 1=CLANG has been used for compilation
const char isclang = 1;
                                          //! copy clang major version number
const int clmaj = __clang_major__;
                                          //! copy clang minor version number
const int clmin = __clang_minor__;
                                          //! copy clang patch level
const int clpat = __clang_patchlevel__;
#else
                                          //! 0=CLANG has not been used
const char isclang = 0;
#endif


#include <jni.h>
#include "oclwrapper.h"
#include "AndroidOpenCL.h"
#include "CL/cl.h"
#include "CL/cl_platform.h"
#include <string.h>
#include <stdlib.h>


#ifdef __arm__
                                           //! armv-7
#define TARGETARCH ARM32
#elif defined(__aarch64__)
                                           //! armv-8
#define TARGETARCH ARM64
#elif defined(__i386__)
                                           //! intel x86
#define TARGETARCH INTEL32
#elif defined(__x86_64__)
                                           //! intel x86_64
#define TARGETARCH INTEL64
#else
                                           //! unknown target
#define TARGETARCH UNKNOWN
#endif


// see header file
JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_isCLang(JNIEnv *env, jclass clazz) {
  return( isclang );                     // return clang info
}

// see header file
JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_getCLmaj(JNIEnv *env, jclass clazz) {
  if (isclang==1) {                      // test if clang has been used
    return (clmaj);                      // return major version info
  }
  else {
    return( -1 );                        // return error
  }
}


// see header file
JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_getCLmin(JNIEnv *env, jclass clazz) {
  if (isclang==1) {                      // test if clang has been used
    return( clmin );                     // return minor CLANG version
  }
  else {
    return( -1 );                        // return error
  }
}


// see header file
JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_getCLpatch(JNIEnv *env, jclass clazz) {
  if (isclang==1) {                      // test if clang has been used
    return( clpat );                     // return CLANG patch level
  }
  else {
    return( -1 );                        // return error
  }
}


// see header file
JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_loadOpenCL(JNIEnv *env, jobject thiz, jstring s ) {

    const char* c = (*env)->GetStringUTFChars( env, s, NULL );    // convert JAVA-String to C-string
    return( loadOpenCL( c ) );                    // try to load library
}


// see header file
JNIEXPORT void JNICALL
Java_com_example_dmocl_oclwrap_unloadOpenCL(JNIEnv *env, jobject thiz) {

    unloadOpenCL();                 // unload library

}

// see header file
JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_AndrCLGetPlatformCnt(JNIEnv *env, jobject thiz) {

    jint ret = -1;

    cl_uint numplatf;
    cl_int err = clGetPlatformIDs( 	0, NULL, &numplatf );    // get platfrom count

    if (err == CL_SUCCESS){
      ret = (jint) numplatf;
    }

    return( ret );

}


// see header file
JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_AndrCLGetDeviceCnt(JNIEnv *env, jobject thiz, jint i) {

  jint ret = 0;

  cl_uint numplatf;                                  // get platform count
  cl_int err = clGetPlatformIDs(0, NULL, &numplatf);

  if (err == CL_SUCCESS) {

    if (i < numplatf) {                            // valid platform number?

                                           // allocate some space
      cl_platform_id* clpi = (cl_platform_id *) malloc(sizeof(cl_platform_id) * numplatf);

      if (clpi != NULL) {

        err = clGetPlatformIDs(numplatf, clpi, NULL);     // get platfrom info

        if (err == CL_SUCCESS) {

          cl_uint numdev;
                                               // get device count
          err = clGetDeviceIDs( clpi[i], CL_DEVICE_TYPE_ALL, 0, NULL, & numdev );

          if (err == CL_SUCCESS){
            ret = (int) numdev;
          } else {
            ret = -5;
          }

        } else {
          ret = -4;
        }

        free( clpi );

      } else {
        ret = -3;
      }
    } else {
      ret = -2;
    }
  }
  else {
    ret = -1;
  }

  return( ret );

} // Java_com_example_dmocl_oclwrap_AndrCLGetDeviceCnt



// see header file
JNIEXPORT jobject JNICALL
Java_com_example_dmocl_oclwrap_AndrCLgetDeviceName(JNIEnv *env, jobject thiz, jint platf,
                                                   jint dev) {

  int ret = 0;

  char msg[256] = "";             // intermediate buffer
  float zu = -1;                  // dummy variable
  int cldevt = 0;

  cl_uint numplatf;                // get number of platforms
  cl_int err = clGetPlatformIDs(0, NULL, &numplatf);

  if (err == CL_SUCCESS) {

      if (platf < numplatf) {                // valid request

                                           // allocate some space
        cl_platform_id* clpi = (cl_platform_id *) malloc(sizeof(cl_platform_id) * numplatf);

        if (clpi != NULL) {

          err = clGetPlatformIDs(numplatf, clpi, NULL);    // get detailed platform info

          if (err == CL_SUCCESS) {

            cl_uint numdev;

                                        // get number of devices
            err = clGetDeviceIDs( clpi[platf], CL_DEVICE_TYPE_ALL, 0, NULL, & numdev );

            if (err == CL_SUCCESS){

              if (dev < numdev) {              // valid request?

                                            // allocate space for device infos
                cl_device_id* cldi = (cl_device_id *) malloc(sizeof(cl_device_id ) * numdev );

                if (cldi != NULL) {

                                            // get detailed device info
                  err = clGetDeviceIDs( clpi[platf], CL_DEVICE_TYPE_ALL, numdev, cldi, NULL );

                  if (err == CL_SUCCESS) {

                                                 // get supported OpenCL version
                    err =clGetDeviceInfo( cldi[dev], CL_DEVICE_VERSION, 256*sizeof(char),
                                          msg, NULL );

                    msg[255] = 0;                        // truncate

                    if (err == CL_SUCCESS) {

                      char* msg2 = msg+7;          // set pointer to OpenCL version

                      int i1=0;
                                                    // search first space
                      while ((msg2[i1] != 0) && (msg2[i1] != ' ')){
                        i1++;
                      }

                      msg[i1] = 0;                        // truncate

                      zu = (float) atof(msg2);           // convert OpenCL version number to float

                                                          // search device name
                      err =clGetDeviceInfo( cldi[dev], CL_DEVICE_NAME, 256*sizeof(char),
                                            msg, NULL );

                      msg[255] = 0;                        // truncate

                      if (err == CL_SUCCESS) {

                        cl_device_type cdt;

                                                              // get device type
                        err =clGetDeviceInfo( cldi[dev], CL_DEVICE_TYPE, sizeof(cl_device_type ),
                                              &cdt, NULL );

                        if (err == CL_SUCCESS) {

                                                   // extract device type
                          if ((cdt & CL_DEVICE_TYPE_CPU) != 0) {
                            cldevt |= 1;
                          }

                          if ((cdt & CL_DEVICE_TYPE_GPU) != 0) {
                            cldevt |= 2;
                          }

                          if ((cdt & CL_DEVICE_TYPE_ACCELERATOR) != 0) {
                            cldevt |= 4;
                          }

                          if ((cdt & CL_DEVICE_TYPE_DEFAULT) != 0) {
                            cldevt |= 8;
                          }

                        } else {
                          ret = -10;
                        }

                      } else {
                        ret = -10;
                      }
                    } else {
                      ret = -9;
                    }
                  } else {
                    ret = -8;
                  }

                  free( cldi );

                } else {
                  ret = -7;
                }
              } else {
                ret = -6;
              }
            } else {
              ret = -5;
            }
          } else {
            ret = -4;
          }

          free( clpi );

        } else {
          ret = -3;
        }
      } else {
        ret = -2;
      }
  }
  else {
    ret = -1;
  }

                                   // create new Java-String
  jstring ress = (*env)->NewStringUTF(env,msg);

                      // Get the class we wish to return an instance of
  jclass clazz = (*env)->FindClass( env, "com/example/dmocl/oclwrap$oclinforet");

                    // Get the method id of an empty constructor in clazz
  jmethodID constructor = (*env)->GetMethodID(env, clazz, "<init>", "()V");

                         // Create an instance of clazz
  jobject obj = (*env)->NewObject(env, clazz, constructor);

                             // Get Field references
  jfieldID paramret = (*env)->GetFieldID(env, clazz, "result", "I");
  jfieldID paramstr = (*env)->GetFieldID(env, clazz, "s", "Ljava/lang/String;");
  jfieldID paramflt = (*env)->GetFieldID(env, clazz, "oclversion", "F");
  jfieldID paramdevt = (*env)->GetFieldID(env, clazz, "devtype", "I");

                                 // Set fields for object
  (*env)->SetIntField(env, obj, paramret, ret );
  (*env)->SetObjectField(env, obj, paramstr, ress );
  (*env)->SetFloatField(env, obj, paramflt, zu );
  (*env)->SetIntField(env, obj, paramdevt, cldevt );

                                      // return object
  return( obj );

} // Java_com_example_dmocl_oclwrap_AndrCLgetDeviceName


// see header file
JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_getArchitecture(JNIEnv *env, jclass clazz) {
  return( TARGETARCH );
}

