//
// Created by robert on 09.08.21.
//

#include "kmeans_c.h"

#include <jni.h>        // JNI header provided by JDK
#include "kmeans_c.h"   // Generated
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "oclwrapper.h"
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#include <CL/opencl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <rwlock_wp.h>

#define GPUTIMING

volatile struct rwlockwp abortcalckm = RWLOCK_STATIC_INITIALIZER;
volatile int doabort = 0;


const char *clsource = \
"                                                                         \n" \
"                                                                         \n" \
"  __kernel void testdistance(                                            \n" \
"                                                                         \n" \
"    global const float* data,                                            \n" \
"    global unsigned short* b,                                            \n" \
"    constant const float* clucent,                                       \n" \
"    const int features,                                                  \n" \
"    const int cluno                                                      \n" \
"                                                                         \n" \
"  ) {                                                                    \n" \
"      size_t gid = get_global_id( 0 );                                   \n" \
"                                                                         \n" \
"      float noxi = INFINITY;                                             \n" \
"                                                                         \n" \
"      for( unsigned short i2=0; i2<cluno; i2++ ){                        \n" \
"                                                                         \n" \
"        float noxi2 = 0;                                                 \n" \
"                                                                         \n" \
"        for( int i3=0; i3<features; i3++ ){                              \n" \
"          noxi2 += pown( clucent[i2*features+i3] - data[gid*features+i3], 2 ); \n" \
"        }                                                                \n" \
"                                                                         \n" \
"        if (noxi2<noxi){                                                 \n" \
"          noxi = noxi2;                                                  \n" \
"          b[gid] = i2;                                                   \n" \
"        }                                                                \n" \
"      }                                                                  \n" \
"    }                                                                    \n" \
"                                                                         \n" \
"                                                                         \n";


/* return a random number between 0 and limit inclusive.
 */

#define MAXCYCLES 100000

int rand_lim(int limit) {

  int divisor = RAND_MAX / (limit + 1);
  int retval;

  do {
    retval = rand() / divisor;
  } while (retval > limit);

  return retval;
}


short kmeans(unsigned short *b, const float *data, const int blen, const float eps, const int cluno,
             const int features) {
  short ret = 0;

  srand((unsigned int) time(NULL));

  float *clucent = (float *) malloc(sizeof(float) * features * cluno);

  if (clucent != NULL) {

    float *newclucent = (float *) malloc(sizeof(float) * features * cluno);

    if (newclucent != NULL) {

      int *clusize = (int *) malloc(sizeof(int) * cluno);

      if (clusize != NULL) {

        for (int i1 = 0; i1 < cluno; i1++) {

          int cluxi = rand_lim(blen - 1);

          for (int i2 = 0; i2 < features; i2++) {
            clucent[features * i1 + i2] = data[cluxi * features + i2];
          }
        }


        int weiter = 0;
        int cycles = 0;

        while (weiter >= 0) {

          for (int i1 = 0; i1 < blen; i1 += 1) {

            float noxi = INFINITY;

            for (unsigned short i2 = 0; i2 < cluno; i2++) {

              float noxi2 = 0;

              for (int i3 = 0; i3 < features; i3++) {
                noxi2 += powf(clucent[i2 * features + i3] - data[i1 * features + i3], 2);
              }

              if (noxi2 < noxi) {
                noxi = noxi2;
                b[i1] = i2;
              }
            }
          }

          for (int i1 = 0; i1 < cluno; i1++) {
            for (int i2 = 0; i2 < features; i2++) {
              newclucent[i1 * features + i2] = 0;
            }
            clusize[i1] = 0;
          }

          for (int i1 = 0; i1 < blen; i1++) {
            for (int i2 = 0; i2 < features; i2++) {
              newclucent[b[i1] * features + i2] += data[i1 * features + i2];
            }
            clusize[b[i1]]++;
          }

          for (int i1 = 0; i1 < cluno; i1++) {
            for (int i2 = 0; i2 < features; i2++) {
              newclucent[i1 * features + i2] /= (float) clusize[i1];
            }
          }

          float newdist = 0;

          for (int i1 = 0; i1 < cluno; i1++) {

            float newdist2 = 0;

            for (int i2 = 0; i2 < features; i2++) {
              newdist2 += powf(clucent[i1 * features + i2] - newclucent[i1 * features + i2], 2);
            }

            newdist += sqrt(newdist2);
          }

          if ((newdist <= eps) || (cycles > MAXCYCLES)) {
            weiter = -1;
          }

          cycles++;

          memcpy(clucent, newclucent, sizeof(float) * features * cluno);

        }

        free(clusize);
      } else {
        ret = -3;
      }

      free(newclucent);
    } else {
      ret = -2;
    }

    free(clucent);
  } else {
    ret = -1;
  }

  return (ret);

}


JNIEXPORT jshort JNICALL Java_com_example_dmocl_kmeans_kmeans_1c
  (JNIEnv *env, jclass jc, jshortArray b, jfloatArray rf, jfloat eps, jint kk, jint features) {

  short ret = 0;

  if ((sizeof(jshort) == sizeof(unsigned short)) && (sizeof(jfloat) == sizeof(float))) {

    jsize datalen = (*env)->GetArrayLength(env, rf);
    jsize blen = (*env)->GetArrayLength(env, b);

    if (features * blen == datalen) {

      jfloat* condata = (*env)->GetFloatArrayElements(env, rf, NULL);

      if (condata != NULL) {

        unsigned short *conb = (unsigned short *) malloc(sizeof(unsigned short) * blen);

        if (conb != NULL) {

          kmeans(conb, condata, blen, eps, kk, features);

          (*env)->SetShortArrayRegion(env, b, 0, blen, (jshort *) conb);

          free(conb);

        } else {
          ret = -5;
        }

        (*env)->ReleaseFloatArrayElements( env, rf, condata, JNI_ABORT );

      } else {
        ret = -4;
      }
    } else {
      ret = -3;
    }
  } else {
    ret = -2;
  }

  return (ret);
}


short kmeans_gpu(cl_ushort *b, const cl_float *data, const int blen, const float eps, const int cluno,
           const int features,
           cl_command_queue commands, cl_program program, cl_device_id device,
           cl_kernel kernel_testdistance, cl_mem data_g, cl_mem b_g, cl_mem clucent_g) {
  short ret = 0;

  const cl_int features_g = features;
  const size_t global_size = blen;
  const cl_int cluno_g = cluno;

  cl_int err = clSetKernelArg(kernel_testdistance, 0, sizeof(cl_mem), &data_g);
  err |= clSetKernelArg(kernel_testdistance, 1, sizeof(cl_mem), &b_g);
  err |= clSetKernelArg(kernel_testdistance, 2, sizeof(cl_mem), &clucent_g);
  err |= clSetKernelArg(kernel_testdistance, 3, sizeof(cl_int), &features_g);
  err |= clSetKernelArg(kernel_testdistance, 4, sizeof(cl_int), &cluno_g);

  if (err != CL_SUCCESS) {
    return (-4);
  }

  srand((unsigned int) time(NULL));

  cl_float *clucent = (cl_float *) malloc(sizeof(cl_float) * features * cluno);

  if (clucent != NULL) {

    cl_float *newclucent = (cl_float *) malloc(sizeof(cl_float) * features * cluno);

    if (newclucent != NULL) {

      int *clusize = malloc(sizeof(int) * cluno);

      if (clusize != NULL) {

        for (int i1 = 0; i1 < cluno; i1++) {

          int cluxi = rand_lim(blen - 1);

          for (int i2 = 0; i2 < features; i2++) {
            clucent[features * i1 + i2] = data[cluxi * features + i2];
          }
        }


        int weiter = 0;
        int cycles = 0;

        while (weiter >= 0) {

          err = clEnqueueWriteBuffer(commands, clucent_g, CL_TRUE, 0,
                                     sizeof(cl_float) * features * cluno, clucent, 0, NULL, NULL);

          if (err != CL_SUCCESS) {
            ret = -6;
            break;
          }

          err = clEnqueueNDRangeKernel(commands, kernel_testdistance, 1, NULL, &global_size, NULL,
                                       0, NULL, NULL);

          if (err != CL_SUCCESS) {
            ret = -7;
            break;
          }

          for (int i1 = 0; i1 < cluno; i1++) {

            for (int i2 = 0; i2 < features; i2++) {
              newclucent[i1 * features + i2] = 0;
            }

            clusize[i1] = 0;
          }

          clFinish(commands);


          err = clEnqueueReadBuffer(commands, b_g, CL_TRUE, 0, sizeof(cl_ushort) * blen, b, 0, NULL,
                                    NULL);

          if (err != CL_SUCCESS) {
            ret = -8;
            break;
          }


          for (int i1 = 0; i1 < blen; i1++) {
            for (int i2 = 0; i2 < features; i2++) {
              newclucent[b[i1] * features + i2] += data[i1 * features + i2];
            }
            clusize[b[i1]]++;
          }

          for (int i1 = 0; i1 < cluno; i1++) {
            for (int i2 = 0; i2 < features; i2++) {
              newclucent[i1 * features + i2] /= (cl_float) clusize[i1];
            }
          }

          float newdist = 0;

          for (int i1 = 0; i1 < cluno; i1++) {

            float newdist2 = 0;

            for (int i2 = 0; i2 < features; i2++) {
              newdist2 += powf(clucent[i1 * features + i2] - newclucent[i1 * features + i2], 2);
            }

            newdist += sqrt(newdist2);
          }

          if ((newdist <= eps) || (cycles>MAXCYCLES)) {
            weiter = -1;
          }


          memcpy(clucent, newclucent, sizeof(cl_float) * features * cluno);

          cycles++;
        }

        free(clusize);
      } else {
        ret = -3;
      }

      free(newclucent);
    } else {
      ret = -2;
    }

    free(clucent);
  } else {
    ret = -1;
  }

  return (ret);

}


JNIEXPORT jshort JNICALL Java_com_example_dmocl_kmeans_kmeans_1c_1gpu
  (JNIEnv *env, jclass jc, jshortArray b, jfloatArray rf, jfloat eps, jint cluno, jint features,
   jlongArray e) {


  struct timespec start2, finish2;

  short ret = 0;

  if ((sizeof(jshort) == sizeof(cl_ushort)) && (sizeof(jfloat) == sizeof(cl_float))) {

    jsize datalen = (*env)->GetArrayLength(env, rf);
    jsize blen = (*env)->GetArrayLength(env, b);

    if (features * blen == datalen) {

      jfloat* condata = (*env)->GetFloatArrayElements(env, rf, NULL);

      if (condata != NULL) {

        cl_ushort *conb = (cl_ushort *) malloc(sizeof(cl_ushort) * blen);

        if (conb != NULL) {

          cl_uint numplatf;

          cl_int err = clGetPlatformIDs(0, NULL, &numplatf);
          // get number of platforms

          if (err == CL_SUCCESS) {

            if (numplatf >= 1) {

              cl_platform_id platf;

              // get all platforms
              err = clGetPlatformIDs(1, &platf, NULL);

              if (err == CL_SUCCESS) {

                cl_uint numdevices;

                err = clGetDeviceIDs(platf, CL_DEVICE_TYPE_ALL, 0, NULL, &numdevices);

                if (err == CL_SUCCESS) {

                  if (numdevices >= 1) {

                    cl_device_id dev;

                    err = clGetDeviceIDs(platf, CL_DEVICE_TYPE_ALL, 1, &dev, NULL);

                    if (err == CL_SUCCESS) {



                      // create a context with the specified
                      // platform and device
                      cl_context context = clCreateContext(NULL, 1, &dev, NULL, NULL, &err);

                      if (context != NULL) {

                        cl_command_queue commands;        // compute command queue

                        // Create a command queue
                        commands = clCreateCommandQueue(context, dev, 0, &err);

                        if (commands != NULL) {
                          // create program
                          cl_program program = clCreateProgramWithSource(context, 1, &clsource,
                                                                         NULL, &err);

                          if (program != NULL) {

                            // build program
                            err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

                            if (err == CL_SUCCESS) {

                              cl_kernel kernel_testdistance;

                              kernel_testdistance = clCreateKernel(program, "testdistance", &err);

                              if (err == CL_SUCCESS) {


                                // create first buffer
                                cl_mem data_g = clCreateBuffer(context,
                                                               CL_MEM_READ_ONLY |
                                                               CL_MEM_USE_HOST_PTR,
                                                               sizeof(cl_float) * datalen, condata,
                                                               &err);

                                if (data_g != NULL) {

                                  // create second buffer
                                  cl_mem b_g = clCreateBuffer(context,
                                                              CL_MEM_READ_WRITE |
                                                              CL_MEM_USE_HOST_PTR,
                                                              sizeof(cl_ushort) * blen, conb, &err);

                                  if (b_g != NULL) {

                                    // create third buffer
                                    cl_mem clucent_g = clCreateBuffer(context,
                                                                      CL_MEM_READ_ONLY,
                                                                      sizeof(cl_float) * features *
                                                                      cluno, NULL, &err);

                                    if (clucent_g != NULL) {

#ifdef GPUTIMING
                                      clock_gettime(CLOCK_REALTIME, &start2);
#endif


                                      ret = kmeans_gpu(conb, (cl_float*) condata, blen, eps, cluno, features,
                                                       commands, program, dev, kernel_testdistance,
                                                       data_g, b_g, clucent_g);



                                      (*env)->SetShortArrayRegion(env, b, 0, blen, (jshort*) conb);

#ifdef GPUTIMING
                                      clock_gettime(CLOCK_REALTIME, &finish2);
#endif

                                      clReleaseMemObject(clucent_g);

                                    } else {

                                      ret = -20;
                                    }


                                    clReleaseMemObject(b_g);

                                  } else {

                                    ret = -19;
                                  }


                                  clReleaseMemObject(data_g);

                                } else {

                                  ret = -18;
                                }

                                clReleaseKernel(kernel_testdistance);

                              } else {

                                ret = -16;
                              }

                            } else {


                              ret = -16;

                            }

                            clReleaseProgram(program);

                          } else {

                            ret = -15;

                          }

                          clReleaseCommandQueue(commands);

                        } else {

                          ret = -15;
                        }

                        clReleaseContext(context);

                      } else {
                        ret = -14;
                      }
                    } else {
                      ret = -11;
                    }
                  } else {
                    ret = -12;
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
          } else {
            ret = -7;
          }

          free(conb);

        } else {
          ret = -6;
        }

        (*env)->ReleaseFloatArrayElements( env, rf, condata, JNI_ABORT );

      } else {
        ret = -5;
      }
    } else {
      ret = -4;
    }
  } else {
    ret = -3;
  }

#ifdef GPUTIMING
  long long int elapsed2 = ((long long int) (finish2.tv_sec - start2.tv_sec)) * 1000000000L;
  elapsed2 += (finish2.tv_nsec - start2.tv_nsec);

  jlong *edata = (*env)->GetLongArrayElements(env, e, NULL);
  edata[0] = elapsed2;
  (*env)->ReleaseLongArrayElements(env, e, edata, 0);
#endif

  return (ret);

}


struct kmeans_pt {

  unsigned short int status;
  int num;
  unsigned short *b;
  float *data;
  volatile float *clucent;
  int blen;
  int cluno;
  int features;
  int start;
  int len;
  volatile unsigned char fertig;

  pthread_t thread;
  sem_t sem;
  sem_t semret;

};


void *kmthread(void *arg) {

  struct kmeans_pt *f = (struct kmeans_pt *) arg;

  int weiter = 0;

  while (weiter == 0) {

    sem_wait(&f->sem);

    if (f->fertig == 0) {

      for (int i1 = f->start; i1 < f->start + f->len; i1++) {

        float noxi = INFINITY;

        for (short i2 = 0; i2 < f->cluno; i2++) {

          float noxi2 = 0;

          for (int i3 = 0; i3 < f->features; i3++) {
            noxi2 += powf(f->clucent[i2 * f->features + i3] - f->data[i1 * f->features + i3], 2);
          }

          if (noxi2 < noxi) {
            noxi = noxi2;
            f->b[i1] = i2;
          }
        }
      }

      sem_post(&f->semret);
    } else {
      weiter = 1;
    }
  }

  return (NULL);

}


short kmeans_pthreads(unsigned short *b, const float *data, float *clucent,
                      const int blen, const int cluno, const int features,
                      const int cores, struct kmeans_pt *kmthreads, const float eps) {
  short ret = 0;

  float *newclucent = (float *) malloc(sizeof(float) * features * cluno);

  if (newclucent != NULL) {

    int *clusize = (int *) malloc(sizeof(int) * cluno);

    if (clusize != NULL) {

      for (int i1 = 0; i1 < cluno; i1++) {

        int cluxi = rand_lim(blen - 1);

        for (int i2 = 0; i2 < features; i2++) {
          clucent[features * i1 + i2] = data[cluxi * features + i2];
        }
      }

      int weiter = 0;
      int cycles = 0;

      while (weiter >= 0) {

        for (int i1 = 0; i1 < cores; i1++) {
          sem_post(&kmthreads[i1].sem);
        }

        for (int i1 = 0; i1 < cluno; i1++) {

          for (int i2 = 0; i2 < features; i2++) {
            newclucent[i1 * features + i2] = 0;
          }

          clusize[i1] = 0;
        }

        for (int i1 = 0; i1 < cores; i1++) {
          sem_wait(&kmthreads[i1].semret);
        }


        for (int i1 = 0; i1 < blen; i1++) {
          for (int i2 = 0; i2 < features; i2++) {
            newclucent[b[i1] * features + i2] += data[i1 * features + i2];
          }
          clusize[b[i1]]++;
        }

        for (int i1 = 0; i1 < cluno; i1++) {
          for (int i2 = 0; i2 < features; i2++) {
            newclucent[i1 * features + i2] /= (float) clusize[i1];
          }
        }

        float newdist = 0;

        for (int i1 = 0; i1 < cluno; i1++) {

          float newdist2 = 0;

          for (int i2 = 0; i2 < features; i2++) {
            newdist2 += powf(clucent[i1 * features + i2] - newclucent[i1 * features + i2], 2);
          }

          newdist += sqrt(newdist2);
        }

        if ((newdist <= eps) || (cycles > MAXCYCLES)){
          weiter = -1;
        }


        memcpy(clucent, newclucent, sizeof(float) * features * cluno);

        cycles++;

      }

      free(clusize);
    } else {
      ret = -3;
    }

    free(newclucent);
  } else {
    ret = -2;
  }

  return (ret);

}


JNIEXPORT jshort JNICALL Java_com_example_dmocl_kmeans_kmeans_1c_1phtreads
  (JNIEnv *env, jclass jc, jshortArray b, jfloatArray rf, jfloat eps, jint cluno, jint features,
   jint cores, jlongArray e ) {

  short ret = 0;

  struct timespec start2, finish2;


  if ((sizeof(jshort) == sizeof(unsigned short)) && (sizeof(jfloat) == sizeof(float))) {

    jsize datalen = (*env)->GetArrayLength(env, rf);
    jsize blen = (*env)->GetArrayLength(env, b);

    if (features * blen == datalen) {

      jfloat* condata = (*env)->GetFloatArrayElements(env, rf, NULL);

      if (condata != NULL) {

        unsigned short *conb = (unsigned short *) malloc(sizeof(unsigned short) * blen);

        if (conb != NULL) {

          float *clucent = (float *) malloc(sizeof(float) * features * cluno);

          if (clucent != NULL) {

            struct kmeans_pt *kmthreads = (struct kmeans_pt *) malloc(
              sizeof(struct kmeans_pt) * cores);

            if (kmthreads != NULL) {

              int initsucc = 0;

              int stepper = (blen / cores) + 1;
              int starter = 0;
              int reminder = blen;

              for (int i1 = 0; i1 < cores; i1++) {

                kmthreads[i1].status = 0;

                if (reminder < stepper) {
                  stepper = reminder;
                }

                if (initsucc == 0) {

                  kmthreads[i1].num = i1;
                  kmthreads[i1].b = conb;
                  kmthreads[i1].data = condata;
                  kmthreads[i1].blen = blen;
                  kmthreads[i1].cluno = cluno;
                  kmthreads[i1].features = features;
                  kmthreads[i1].start = starter;
                  kmthreads[i1].len = stepper;
                  kmthreads[i1].clucent = clucent;
                  kmthreads[i1].fertig = 0;


                  if (sem_init(&kmthreads[i1].sem, 0, 0) == 0) {
                    kmthreads[i1].status |= 1;
                  } else {
                    initsucc = 1;
                  }

                  if ((kmthreads[i1].status & 1) == 1) {
                    if (sem_init(&kmthreads[i1].semret, 0, 0) == 0) {
                      kmthreads[i1].status |= 2;
                    } else {
                      initsucc = 1;
                    }
                  }


                  if ((kmthreads[i1].status & 3) == 3) {

                    if (
                      pthread_create(&(kmthreads[i1].thread), NULL, &kmthread, &kmthreads[i1]) ==
                      0) {
                      kmthreads[i1].status |= 4;
                    } else {
                      initsucc = 1;
                    }
                  }

                }

                starter += stepper;
                reminder -= stepper;
              }

              if (initsucc == 0) {

#ifdef GPUTIMING
                clock_gettime(CLOCK_REALTIME, &start2);
#endif

                ret = kmeans_pthreads(conb, condata, clucent, blen, cluno, features, cores,
                                      kmthreads, eps);

                (*env)->SetShortArrayRegion(env, b, 0, blen, (jshort *) conb);

#ifdef GPUTIMING
                clock_gettime(CLOCK_REALTIME, &finish2);
#endif

              } else {
                ret = -8;
              }

              for (int i1 = 0; i1 < cores; i1++) {

                if ((kmthreads[i1].status & 4) > 0) {

                  kmthreads[i1].fertig = 1;
                  sem_post(&kmthreads[i1].sem);

                  pthread_join(kmthreads[i1].thread, NULL);

                }

                if ((kmthreads[i1].status & 2) > 0) {
                  sem_destroy(&kmthreads[i1].semret);
                }

                if ((kmthreads[i1].status & 1) > 0) {
                  sem_destroy(&kmthreads[i1].sem);
                }
              }

              free(kmthreads);

            } else {
              ret = -7;
            }

            free(clucent);

          } else {
            ret = -9;
          }

          free(conb);

        } else {
          ret = -6;
        }

        (*env)->ReleaseFloatArrayElements( env, rf, condata, JNI_ABORT );

      } else {
        ret = -5;
      }

    } else {
      ret = -4;
    }
  } else {
    ret = -3;
  }

#ifdef GPUTIMING
  long long int elapsed2 = ((long long int) (finish2.tv_sec - start2.tv_sec)) * 1000000000L;
  elapsed2 += (finish2.tv_nsec - start2.tv_nsec);

  jlong *edata = (*env)->GetLongArrayElements(env, e, NULL);
  edata[0] = elapsed2;
  (*env)->ReleaseLongArrayElements(env, e, edata, 0);
#endif

  return (ret);
}


JNIEXPORT void JNICALL
Java_com_example_dmocl_kmeans_kmabort_1c(JNIEnv *env, jclass clazz) {
  rwlockwp_writer_acquire(&abortcalckm);
  doabort = 1;
  rwlockwp_writer_release(&abortcalckm);
}

JNIEXPORT void JNICALL
Java_com_example_dmocl_kmeans_kmresume_1c(JNIEnv *env, jclass clazz) {
  rwlockwp_writer_acquire(&abortcalckm);
  doabort = 0;
  rwlockwp_writer_release(&abortcalckm);
}

