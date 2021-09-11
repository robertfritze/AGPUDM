//
// Created by robert on 30.04.20.
//
#ifdef __clang__
const char isclang = 1;
const int clmaj = __clang_major__;
const int clmin = __clang_minor__;
const int clpat = __clang_patchlevel__;
#else
const char isclang = 0;
#endif


#define CL_TARGET_OPENCL_VERSION 300    //! the default target OpenCL version

#include <jni.h>
#include "oclwrapper.h"
#include "AndroidOpenCL.h"
#include "CL/cl.h"
#include "CL/cl_platform.h"
#include <string.h>
#include <stdlib.h>
//#include <android/log.h>

#define UNKNOWN -1
#define ARM32 0
#define ARM64 1
#define INTEL32 2
#define INTEL64 3

#ifdef __arm__
#define TARGETARCH ARM32
#elif defined(__aarch64__)
#define TARGETARCH ARM64
#elif defined(__i386__)
#define TARGETARCH INTEL32
#elif defined(__x86_64__)
#define TARGETARCH INTEL64
#else
#define TARGETARCH UNKNOWN
#endif


JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_isCLang(JNIEnv *env, jclass clazz) {
  return( isclang );
}

JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_getCLmaj(JNIEnv *env, jclass clazz) {
  return( clmaj );
}

JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_getCLmin(JNIEnv *env, jclass clazz) {
  return( clmin );
}

JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_getCLpatch(JNIEnv *env, jclass clazz) {
  return( clpat );
}

JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_loadOpenCL(JNIEnv *env, jobject thiz, jstring s ) {

    const char* c = (*env)->GetStringUTFChars( env, s, NULL );
    return( loadOpenCL( c ) );
}

    // only this method my call "unloadOpenCL". Never call this method
    // from any other method of this program.

JNIEXPORT void JNICALL
Java_com_example_dmocl_oclwrap_unloadOpenCL(JNIEnv *env, jobject thiz) {

    unloadOpenCL();

}


JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_AndrCLGetPlatformCnt(JNIEnv *env, jobject thiz) {

    jint ret = -1;

    cl_uint numplatf;
    cl_int err = clGetPlatformIDs( 	0, NULL, &numplatf );

    if (err == CL_SUCCESS){
      ret = (jint) numplatf;
    }

    return( ret );

}


JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_AndrCLGetDeviceCnt(JNIEnv *env, jobject thiz, jint i) {

  jint ret = 0;

  cl_uint numplatf;
  cl_int err = clGetPlatformIDs(0, NULL, &numplatf);

  if (err == CL_SUCCESS) {

    if (i < numplatf) {

      cl_platform_id* clpi = (cl_platform_id *) malloc(sizeof(cl_platform_id) * numplatf);

      if (clpi != NULL) {

        err = clGetPlatformIDs(numplatf, clpi, NULL);

        if (err == CL_SUCCESS) {

          cl_uint numdev;

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

}




JNIEXPORT jobject JNICALL
Java_com_example_dmocl_oclwrap_AndrCLgetDeviceName(JNIEnv *env, jobject thiz, jint platf,
                                                   jint dev) {

  int ret = 0;

  char msg[256] = "";
  float zu = -1;
  int cldevt = 0;

  cl_uint numplatf;
  cl_int err = clGetPlatformIDs(0, NULL, &numplatf);

  if (err == CL_SUCCESS) {

      if (platf < numplatf) {

        cl_platform_id* clpi = (cl_platform_id *) malloc(sizeof(cl_platform_id) * numplatf);

        if (clpi != NULL) {

          err = clGetPlatformIDs(numplatf, clpi, NULL);

          if (err == CL_SUCCESS) {

            cl_uint numdev;

            err = clGetDeviceIDs( clpi[platf], CL_DEVICE_TYPE_ALL, 0, NULL, & numdev );

            if (err == CL_SUCCESS){

              if (dev < numdev) {

                cl_device_id* cldi = (cl_device_id *) malloc(sizeof(cl_device_id ) * numdev );

                if (cldi != NULL) {

                  err = clGetDeviceIDs( clpi[platf], CL_DEVICE_TYPE_ALL, numdev, cldi, NULL );

                  if (err == CL_SUCCESS) {

                    err =clGetDeviceInfo( cldi[dev], CL_DEVICE_VERSION, 256*sizeof(char),
                                          msg, NULL );

                    msg[255] = 0;

                    char* msg2 = msg+7;

                    int i1=0;

                    while ((msg2[i1] != 0) && (msg2[i1] != ' ')){
                      i1++;
                    }

                    msg[i1] = 0;

                    zu = atof(msg2);

                    if (err == CL_SUCCESS) {

                      err =clGetDeviceInfo( cldi[dev], CL_DEVICE_NAME, 256*sizeof(char),
                                            msg, NULL );

                      msg[255] = 0;

                      if (err == CL_SUCCESS) {

                        cl_device_type cdt;

                        err =clGetDeviceInfo( cldi[dev], CL_DEVICE_TYPE, sizeof(cl_device_type ),
                                              &cdt, NULL );

                        if (err == CL_SUCCESS) {

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

}


JNIEXPORT jint JNICALL
Java_com_example_dmocl_oclwrap_getArchitecture(JNIEnv *env, jclass clazz) {
  return( TARGETARCH );
}

