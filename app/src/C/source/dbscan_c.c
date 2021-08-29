//
// Created by robert on 11.08.21.
//

#include <jni.h>        // JNI header provided by JDK
#include "dbscan_c.h"   // Generated
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <rwlock_wp.h>

#include "oclwrapper.h"
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#include <CL/opencl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

#define MAXVALUE ((short) 0xFFFF)
#define GPUTIMING

volatile struct rwlockwp abortcalc = RWLOCK_STATIC_INITIALIZER;
volatile int doabort = 0;

const char *clsource = \
"                                                                         \n" \
"                                                                         \n" \
"  __kernel void testdistance1(                                           \n" \
"                                                                         \n" \
"    global const float* data,                                            \n" \
"    global unsigned short* b,                                            \n" \
"    const int features,                                                  \n" \
"    const int cmpto,                                                     \n" \
"    const float epseps                                                   \n" \
"                                                                         \n" \
"  ) {                                                                    \n" \
"       size_t gid = get_global_id( 0 );                                  \n" \
"                                                                         \n" \
"       b[gid] &= 65535-6;             // Bits 2 und 3 löschen            \n" \
"       float s = 0;                                                      \n" \
"                                                                         \n" \
"       for( int i1 = 0; i1 < features; i1++ ){                           \n" \
"         s += pown( data[gid*features+i1] - data[cmpto*features+i1], 2 );\n" \
"       }                                                                 \n" \
"                                                                         \n" \
"       if (s <= epseps) {                                                \n" \
"                                                                         \n" \
"         b[gid] |= 2;                  // Distanzbit (2) setzen          \n" \
"       }                                                                 \n" \
"                                                                         \n" \
"  }                                                                      \n" \
"                                                                         \n" \
"                                                                         \n" \
"                                                                         \n" \
"                                                                         \n" \
"  __kernel void testdistance2(                                           \n" \
"                                                                         \n" \
"    global const float* data,                                            \n" \
"    global unsigned short* b,                                            \n" \
"    const int features,                                                  \n" \
"    const int cmpto,                                                     \n" \
"    const float epseps                                                   \n" \
"                                                                         \n" \
"  ) {                                                                    \n" \
"                                                                         \n" \
"       size_t gid = get_global_id( 0 );                                  \n" \
"                                                                         \n" \
"       b[gid] &= 65535-4;             // Bit 3 löschen                   \n" \
"       float s = 0;                                                      \n" \
"                                                                         \n" \
"       for( int i1 = 0; i1 < features; i1++ ){                           \n" \
"         s += pown( data[gid*features+i1] - data[cmpto*features+i1], 2 );\n" \
"       }                                                                 \n" \
"                                                                         \n" \
"       if (s <= epseps) {                                                \n" \
"                                                                         \n" \
"         b[gid] |= 4;                  // Distanzbit (3) setzen          \n" \
"       }                                                                 \n" \
"                                                                         \n" \
"  }                                                                      \n" \
"                                                                         \n" \
"                                                                         \n" \
" ";


int expandCluster(int key, const short clusternumber,
                  unsigned short *b, const float *data, const float epseps, const int kk,
                  const int datalen, const int features) {


  b[key] &= 7;            // Bits 3-15 löschen
  b[key] |= (clusternumber << 3);
  // Clusternummer abspeichern


  int weiter = 0;
  int ret = 0;

  while (weiter == 0) {

    weiter = 1;

    rwlockwp_reader_acquire(&abortcalc);
    if (doabort > 0) {
      weiter = 2;
      ret = -1;
    }
    rwlockwp_reader_release(&abortcalc);

    if (weiter == 1) {

      for (int i1 = 0; i1 < datalen; i1++) {

        if ((b[i1] & 3) == 2) {

          b[i1] |= 1;

          int itemcounter2 = 0;


          for (int i2 = 0; i2 < datalen; i2++) {

            b[i2] &= MAXVALUE - 4;     // Bit 3 löschen

            float s = 0;

            for (int i3 = 0; i3 < features; i3++) {
              s += powf(data[i2 * features + i3] - data[i1 * features + i3], 2);
            }

            if (s <= epseps) {

              b[i2] |= 4;   // Distanzbit (3) setzen
              itemcounter2++;
            }
          }

          if (itemcounter2 >= kk) {

            for (int i2 = 0; i2 < datalen; i2++) {

              if ((b[i2] & 6) == 4) {

                b[i2] |= 2;         // Bit 2 setzen
                weiter = 0;
              }
            }
          }

          if ((b[i1] >> 3) == 0) {
            b[i1] |= (clusternumber << 3);
          }

        }
      }

      if (ret < 0) {
        break;
      }
    }
  }

  return (ret);
}


short dbscan(unsigned short *b, const float *data, const int blen, const float eps, const int kk,
             const int features) {

  short clusternumber = 0;

  const float epseps = eps * eps;

  for (int i1 = 0; i1 < blen; i1++) {
    b[i1] = 0;
  }


  for (int i1 = 0; i1 < blen; i1++) {

    if ((b[i1] & 1) == 0) {   // visited (1) ?

      rwlockwp_reader_acquire(&abortcalc);
      if (doabort > 0) {
        clusternumber = -1;
      }
      rwlockwp_reader_release(&abortcalc);

      if (clusternumber < 0) {
        break;
      }

      b[i1] |= 1;       // visited (1) setzen

      int itemcounter = 0;

      for (int i2 = 0; i2 < blen; i2++) {

        b[i2] &= MAXVALUE - 6;      // Bits 2 und 3 löschen
        float s = 0;

        for (int i3 = 0; i3 < features; i3++) {
          s += powf(data[i2 * features + i3] - data[i1 * features + i3], 2);
        }

        if (s <= epseps) {

          b[i2] |= 2;   // Distanzbit (3) setzen
          itemcounter++;
        }
      }

      if (itemcounter < kk) {

        b[i1] &= 7;  // Cluster Nr. 0 = Noise

      } else {

        if (clusternumber == 4095) {
          clusternumber = -256;
          break;
        }

        clusternumber += 1;

        int ret2 = expandCluster(i1, clusternumber, b, data, epseps, kk, blen, features);

        if (ret2 < 0) {
          clusternumber = -1;
          break;
        }
      }
    }
  }

  return (clusternumber);
}


JNIEXPORT jshort JNICALL
Java_com_example_dmocl_dbscan_dbscan_1c
  (JNIEnv *env, jclass jc, jshortArray b, jfloatArray rf, jfloat eps, jint kk, jint features) {

  short ret = -1;

  if ((sizeof(jshort) == sizeof(unsigned short)) && (sizeof(jfloat) == sizeof(float))) {

    jsize datalen = (*env)->GetArrayLength(env, rf);
    jsize blen = (*env)->GetArrayLength(env, b);

    if (features * blen == datalen) {

      jfloat *condata = (*env)->GetFloatArrayElements(env, rf, NULL);

      if (condata != NULL) {

        unsigned short *conb = (unsigned short *) malloc(sizeof(unsigned short) * blen);

        if (conb != NULL) {

          ret = dbscan(conb, (float *) condata, blen, eps, kk, features);

          if (ret >= 0) {
            for (int i1 = 0; i1 < blen; i1++) {
              conb[i1] = (short) (((unsigned short) conb[i1]) >> 3);
            }

            (*env)->SetShortArrayRegion(env, b, 0, blen, (jshort *) conb);
          }

          free(conb);
        } else {
          ret = -105;
        }

        (*env)->ReleaseFloatArrayElements(env, rf, condata, JNI_ABORT);

      } else {
        ret = -104;
      }
    } else {
      ret = -103;
    }
  } else {
    ret = -102;
  }

  return (ret);
}


short expandCluster_gpu(int key, const short clusternumber,
                        unsigned short *b, const float *data, const float epseps, const int kk,
                        const int datalen, const int features,
                        cl_command_queue commands, cl_kernel kernel_testdistance2, cl_mem b_g,
                        const size_t *global_size) {

  short ret = 0;

  b[key] &= 7;            // Bits 3-15 löschen
  b[key] |= (clusternumber << 3);
  // Clusternummer abspeichern

  int weiter = 0;

  while (weiter == 0) {

    weiter = 1;

    rwlockwp_reader_acquire(&abortcalc);
    if (doabort > 0) {
      weiter = 2;
      ret = -1;
    }
    rwlockwp_reader_release(&abortcalc);

    if (weiter == 1) {

      for (int i1 = 0; i1 < datalen; i1++) {

        if ((b[i1] & 3) == 2) {

          b[i1] |= 1;

          cl_int err = clEnqueueWriteBuffer(commands, b_g, CL_TRUE, 0, sizeof(cl_ushort) * datalen,
                                            b, 0, NULL, NULL);

          if (err != CL_SUCCESS) {
            ret = -5;
            break;
          }

          cl_int cmpto_g = i1;
          err |= clSetKernelArg(kernel_testdistance2, 3, sizeof(cl_int), &cmpto_g);

          if (err != CL_SUCCESS) {
            ret = -6;
            break;
          }

          err = clEnqueueNDRangeKernel(commands, kernel_testdistance2, 1, NULL, global_size, NULL,
                                       0, NULL, NULL);

          if (err != CL_SUCCESS) {
            ret = -7;
            break;
          }

          clFinish(commands);

          err = clEnqueueReadBuffer(commands, b_g, CL_TRUE, 0, sizeof(cl_ushort) * datalen, b, 0,
                                    NULL, NULL);

          if (err != CL_SUCCESS) {
            ret = -8;
            break;
          }


          int itemcounter2 = 0;

          for (int i2 = 0; i2 < datalen; i2++) {

            itemcounter2 += (b[i2] >> 2) & 1;

          }


          if (itemcounter2 >= kk) {

            for (int i2 = 0; i2 < datalen; i2++) {

              if ((b[i2] & 6) == 4) {

                b[i2] |= 2;         // Bit 2 setzen
                weiter = 0;
              }
            }
          }


          if ((b[i1] >> 3) == 0) {
            b[i1] |= (clusternumber << 3);
          }

        }
      }

      if (ret < 0) {
        break;
      }
    }
  }

  return (ret);
}


short
dbscan_gpu( cl_ushort* b, const cl_float* data, const int blen, const float eps, const int kk,
           const int features,
           cl_command_queue commands, cl_program program, cl_device_id device,
           cl_kernel kernel_testdistance1, cl_kernel kernel_testdistance2,
           cl_mem data_g, cl_mem b_g, struct timespec *start2, struct timespec *finish2) {

  short clusternumber = 0;

  const cl_float epseps = eps * eps;
  const cl_int features_g = features;
  size_t global_size = blen;


  cl_int err = clSetKernelArg(kernel_testdistance1, 0, sizeof(cl_mem), &data_g);
  err |= clSetKernelArg(kernel_testdistance1, 1, sizeof(cl_mem), &b_g);
  err |= clSetKernelArg(kernel_testdistance1, 2, sizeof(cl_int), &features_g);
  err |= clSetKernelArg(kernel_testdistance1, 4, sizeof(cl_float), &epseps);

  if (err != CL_SUCCESS) {
    return (-20);
  }

  err = clSetKernelArg(kernel_testdistance2, 0, sizeof(cl_mem), &data_g);
  err |= clSetKernelArg(kernel_testdistance2, 1, sizeof(cl_mem), &b_g);
  err |= clSetKernelArg(kernel_testdistance2, 2, sizeof(cl_int), &features_g);
  err |= clSetKernelArg(kernel_testdistance2, 4, sizeof(cl_float), &epseps);

  if (err != CL_SUCCESS) {
    return (-21);
  }

#ifdef GPUTIMING
  clock_gettime(CLOCK_REALTIME, start2);
#endif

  for (int i1 = 0; i1 < blen; i1++) {
    b[i1] = 0;
  }

  for (int i1 = 0; i1 < blen; i1++) {

    if ((b[i1] & 1) == 0) {   // visited (1) ?

      rwlockwp_reader_acquire(&abortcalc);
      if (doabort > 0) {
        clusternumber = -1;
      }
      rwlockwp_reader_release(&abortcalc);

      if (clusternumber < 0) {
        break;
      }

      b[i1] |= 1;       // visited (1) setzen

      err = clEnqueueWriteBuffer(commands, b_g, CL_TRUE, 0, sizeof(cl_ushort) * blen, b, 0, NULL,
                                 NULL);

      if (err != CL_SUCCESS) {
        clusternumber = -21;
        break;
      }

      cl_int cmpto_g = i1;
      err |= clSetKernelArg(kernel_testdistance1, 3, sizeof(cl_int), &cmpto_g);

      if (err != CL_SUCCESS) {
        clusternumber = -22;
        break;
      }

      err = clEnqueueNDRangeKernel(commands, kernel_testdistance1, 1, NULL, &global_size, NULL, 0,
                                   NULL, NULL);

      if (err != CL_SUCCESS) {
        clusternumber = -23;
        break;
      }

      clFinish(commands);

      err = clEnqueueReadBuffer(commands, b_g, CL_TRUE, 0, sizeof(cl_ushort) * blen, b, 0, NULL,
                                NULL);

      if (err != CL_SUCCESS) {
        clusternumber = -24;
        break;
      }

      int itemcounter = 0;

      for (int i2 = 0; i2 < blen; i2++) {

        itemcounter += (b[i2] >> 1) & 1;

      }

      if (itemcounter < kk) {

        b[i1] &= 7;  // Cluster Nr. 0 = Noise

      } else {

        if (clusternumber == 4095) {
          clusternumber = -256;
          break;
        }

        clusternumber += 1;

        short rret = expandCluster_gpu(i1, clusternumber, b, data, epseps, kk, blen, features,
                                       commands, kernel_testdistance2, b_g, &global_size);

        if (rret < 0) {
          clusternumber = rret;
          break;
        }

      }
    }
  }

#ifdef GPUTIMING
  clock_gettime(CLOCK_REALTIME, finish2);
#endif


  return (clusternumber);
}

JNIEXPORT jshort JNICALL
Java_com_example_dmocl_dbscan_dbscan_1c_1gpu
  (JNIEnv *env, jclass jc, jshortArray b, jfloatArray rf, jfloat eps, jint kk, jint features,
   jlongArray e) {

  struct timespec start2, finish2;

  short ret = -1;

  if ((sizeof(jshort) == sizeof(cl_ushort)) && (sizeof(jfloat) == sizeof(cl_float))) {

    jsize datalen = (*env)->GetArrayLength(env, rf);
    jsize blen = (*env)->GetArrayLength(env, b);

    if (features * blen == datalen) {

      jfloat *condata = (*env)->GetFloatArrayElements(env, rf, NULL);

      if (condata != NULL) {

        cl_ushort* conb = (cl_ushort*) malloc(sizeof(cl_ushort) * blen);

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

                              cl_kernel kernel_testdistance1;

                              kernel_testdistance1 = clCreateKernel(program, "testdistance1",
                                                                    &err);

                              if (err == CL_SUCCESS) {

                                cl_kernel kernel_testdistance2;

                                kernel_testdistance2 = clCreateKernel(program, "testdistance2",
                                                                      &err);

                                if (err == CL_SUCCESS) {


                                  // create first buffer
                                  cl_mem data_g = clCreateBuffer(context,
                                                                 CL_MEM_READ_ONLY |
                                                                 CL_MEM_USE_HOST_PTR,
                                                                 sizeof(cl_float) * datalen,
                                                                 condata, &err);

                                  if (data_g != NULL) {

                                    // create second buffer
                                    cl_mem b_g = clCreateBuffer(context,
                                                                CL_MEM_READ_WRITE |
                                                                CL_MEM_USE_HOST_PTR,
                                                                sizeof(cl_ushort) * blen, conb,
                                                                &err);

                                    if (b_g != NULL) {

                                      ret = dbscan_gpu(conb, (cl_float *) condata, blen, eps, kk,
                                                       features,
                                                       commands, program, dev,
                                                       kernel_testdistance1, kernel_testdistance2,
                                                       data_g, b_g, &start2, &finish2);

                                      if (ret >= 0) {
                                        for (int i1 = 0; i1 < blen; i1++) {
                                          conb[i1] = conb[i1] >> 3;
                                        }

                                        (*env)->SetShortArrayRegion(env, b, 0, blen,
                                                                    (jshort *) conb);
                                      }

                                      clReleaseMemObject(b_g);

                                    } else {
                                      ret = -121;
                                    }

                                    clReleaseMemObject(data_g);

                                  } else {
                                    ret = -120;
                                  }

                                  clReleaseKernel(kernel_testdistance2);

                                } else {
                                  ret = -119;
                                }

                                clReleaseKernel(kernel_testdistance1);

                              } else {
                                ret = -118;
                              }

                            } else {
                              ret = -117;
                            }

                            clReleaseProgram(program);

                          } else {
                            ret = -116;

                          }

                          clReleaseCommandQueue(commands);

                        } else {
                          ret = -115;
                        }

                        clReleaseContext(context);

                      } else {
                        ret = -114;

                      }
                    } else {
                      ret = -111;
                    }
                  } else {

                    ret = -112;
                  }
                } else {
                  ret = -110;
                }
              } else {
                ret = -109;
              }
            } else {
              ret = -108;
            }
          } else {
            ret = -107;
          }

          free(conb);

        } else {
          ret = -105;
        }

        (*env)->ReleaseFloatArrayElements( env, rf, condata, JNI_ABORT );

      } else {
        ret = -104;
      }
    } else {
      ret = -103;
    }
  } else {
    ret = -102;
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


struct dbscan_pt {

  unsigned short int status;

  int num;
  unsigned short *b;
  float *data;
  int blen;
  float eps;
  int kk;
  int features;
  int start;
  int len;

  pthread_t thread1;
  pthread_t thread2;
  sem_t sem1;
  sem_t semret1;
  sem_t sem2;
  sem_t semret2;


  pthread_mutex_t MUTEX_var1;
  volatile int cmpto1;
  volatile int itemcounter1;

  pthread_mutex_t MUTEX_var2;
  volatile int cmpto2;
  volatile int itemcounter2;

  pthread_mutex_t MUTEX_fertig1;
  volatile unsigned char fertig1;

  pthread_mutex_t MUTEX_fertig2;
  volatile unsigned char fertig2;

};


void *dbscanthread1(void *arg) {

  struct dbscan_pt *f = (struct dbscan_pt *) arg;

  int weiter = 0;
  float epseps = f->eps * f->eps;

  while (weiter == 0) {

    sem_wait(&f->sem1);

    unsigned char temp;
    pthread_mutex_lock(&f->MUTEX_fertig1);
    temp = f->fertig1;
    pthread_mutex_unlock(&f->MUTEX_fertig1);

    if (temp == 0) {

      pthread_mutex_lock(&f->MUTEX_var1);

      int itemcounter = 0;

      for (int i2 = f->start; i2 < f->start + f->len; i2++) {

        f->b[i2] &= MAXVALUE - 6;     // Bit 3 löschen

        float s = 0;

        for (int i3 = 0; i3 < f->features; i3++) {
          s += powf(f->data[i2 * f->features + i3] - f->data[f->cmpto1 * f->features + i3], 2);
        }

        if (s <= epseps) {

          f->b[i2] |= 2;   // Distanzbit (3) setzen
          itemcounter++;
        }
      }

      f->itemcounter1 = itemcounter;

      pthread_mutex_unlock(&f->MUTEX_var1);

      sem_post(&f->semret1);
    } else {
      weiter = 1;
    }
  }

  return (NULL);

}


void *dbscanthread2(void *arg) {

  struct dbscan_pt *f = (struct dbscan_pt *) arg;

  int weiter = 0;
  float epseps = f->eps * f->eps;

  while (weiter == 0) {

    sem_wait(&f->sem2);

    unsigned char temp;
    pthread_mutex_lock(&f->MUTEX_fertig2);
    temp = f->fertig2;
    pthread_mutex_unlock(&f->MUTEX_fertig2);

    if (temp == 0) {

      pthread_mutex_lock(&f->MUTEX_var2);

      int itemcounter = 0;

      for (int i2 = f->start; i2 < f->start + f->len; i2++) {

        f->b[i2] &= MAXVALUE - 4;     // Bit 3 löschen

        float s = 0;

        for (int i3 = 0; i3 < f->features; i3++) {
          s += powf(f->data[i2 * f->features + i3] - f->data[f->cmpto2 * f->features + i3], 2);
        }

        if (s <= epseps) {

          f->b[i2] |= 4;   // Distanzbit (3) setzen
          itemcounter++;
        }
      }

      f->itemcounter2 = itemcounter;

      pthread_mutex_unlock(&f->MUTEX_var2);


      sem_post(&f->semret2);
    } else {
      weiter = 1;
    }
  }

  return (NULL);

}


short expandCluster_pthreads(int key, const short clusternumber,
                             unsigned short *b, const float *data, const float epseps, const int kk,
                             const int datalen, const int features,
                             const int cores, struct dbscan_pt *dbthreads) {

  short ret = 0;

  b[key] &= 7;            // Bits 3-15 löschen
  b[key] |= (clusternumber << 3);
  // Clusternummer abspeichern


  int weiter = 0;

  while (weiter == 0) {

    weiter = 1;

    rwlockwp_reader_acquire(&abortcalc);
    if (doabort > 0) {
      weiter = 2;
      ret = -1;
    }
    rwlockwp_reader_release(&abortcalc);

    if (weiter == 1) {
      for (int i1 = 0; i1 < datalen; i1++) {


        if ((b[i1] & 3) == 2) {

          b[i1] |= 1;


          for (int i2 = 0; i2 < cores; i2++) {
            pthread_mutex_lock(&dbthreads[i2].MUTEX_var2);
            dbthreads[i2].cmpto2 = i1;
            pthread_mutex_unlock(&dbthreads[i2].MUTEX_var2);
            sem_post(&dbthreads[i2].sem2);
          }

          int itemcounter2 = 0;

          for (int i2 = 0; i2 < cores; i2++) {
            sem_wait(&dbthreads[i2].semret2);
            pthread_mutex_lock(&dbthreads[i2].MUTEX_var2);
            itemcounter2 += dbthreads[i2].itemcounter2;
            pthread_mutex_unlock(&dbthreads[i2].MUTEX_var2);
          }


          if (itemcounter2 >= kk) {

            for (int i2 = 0; i2 < datalen; i2++) {

              if ((b[i2] & 6) == 4) {

                b[i2] |= 2;         // Bit 2 setzen
                weiter = 0;
              }
            }
          }


          if ((b[i1] >> 3) == 0) {
            b[i1] |= (clusternumber << 3);
          }

        }
      }

      if (ret < 0) {
        break;
      }
    }
  }

  return (ret);
}


short
dbscan_pthreads(unsigned short *b, const float *data, const int blen, const float eps, const int kk,
                const int features,
                const int cores, struct dbscan_pt *dbthreads) {

  short clusternumber = 0;

  const float epseps = eps * eps;

  for (int i1 = 0; i1 < blen; i1++) {
    b[i1] = 0;
  }


  for (int i1 = 0; i1 < blen; i1++) {

    if ((b[i1] & 1) == 0) {   // visited (1) ?

      rwlockwp_reader_acquire(&abortcalc);
      if (doabort > 0) {
        clusternumber = -1;
      }
      rwlockwp_reader_release(&abortcalc);

      if (clusternumber < 0) {
        break;
      }

      b[i1] |= 1;       // visited (1) setzen

      for (int i2 = 0; i2 < cores; i2++) {
        pthread_mutex_lock(&dbthreads[i2].MUTEX_var1);
        dbthreads[i2].cmpto1 = i1;
        pthread_mutex_unlock(&dbthreads[i2].MUTEX_var1);
        sem_post(&dbthreads[i2].sem1);
      }

      int itemcounter = 0;

      for (int i2 = 0; i2 < cores; i2++) {
        sem_wait(&dbthreads[i2].semret1);
        pthread_mutex_lock(&dbthreads[i2].MUTEX_var1);
        itemcounter += dbthreads[i2].itemcounter1;
        pthread_mutex_unlock(&dbthreads[i2].MUTEX_var1);
      }


      if (itemcounter < kk) {

        b[i1] &= 7;  // Cluster Nr. 0 = Noise

      } else {

        if (clusternumber == 4095) {
          clusternumber = -256;
          break;
        }

        clusternumber += 1;

        short ret2 = expandCluster_pthreads(i1, clusternumber, b, data, epseps, kk, blen, features,
                                            cores, dbthreads);

        if (ret2 < 0) {
          clusternumber = ret2;
          break;
        }
      }
    }
  }

  return (clusternumber);
}


JNIEXPORT jshort JNICALL
Java_com_example_dmocl_dbscan_dbscan_1c_1phtreads
  (JNIEnv *env, jclass jc, jshortArray b, jfloatArray rf, jfloat eps, jint kk, jint features,
   jint cores, jlongArray e) {

  short ret = -1;

  struct timespec start2, finish2;


  if ((sizeof(jshort) == sizeof(unsigned short)) && (sizeof(jfloat) == sizeof(float))) {

    jsize datalen = (*env)->GetArrayLength(env, rf);
    jsize blen = (*env)->GetArrayLength(env, b);

    if (features * blen == datalen) {

      jfloat *condata = (*env)->GetFloatArrayElements(env, rf, NULL);

      if (condata != NULL) {

        unsigned short *conb = (unsigned short *) malloc(sizeof(unsigned short) * blen);

        if (conb != NULL) {

          struct dbscan_pt *dbthreads = (struct dbscan_pt *) malloc(
            sizeof(struct dbscan_pt) * cores);

          if (dbthreads != NULL) {

            int initsucc = 0;

            int stepper = (blen / cores) + 1;
            int starter = 0;
            int reminder = blen;

            for (int i1 = 0; i1 < cores; i1++) {

              dbthreads[i1].status = 0;

              if (reminder < stepper) {
                stepper = reminder;
              }

              if (initsucc == 0) {

                dbthreads[i1].num = i1;
                dbthreads[i1].b = (unsigned short *) conb;
                dbthreads[i1].data = condata;
                dbthreads[i1].blen = blen;
                dbthreads[i1].eps = eps;
                dbthreads[i1].kk = kk;
                dbthreads[i1].features = features;
                dbthreads[i1].start = starter;
                dbthreads[i1].len = stepper;

                dbthreads[i1].fertig1 = 0;
                dbthreads[i1].fertig2 = 0;
                dbthreads[i1].itemcounter1 = 0;
                dbthreads[i1].itemcounter2 = 0;
                dbthreads[i1].cmpto1 = -1;
                dbthreads[i1].cmpto2 = -1;

                if (pthread_mutex_init(&dbthreads[i1].MUTEX_var1, NULL) == 0) {
                  dbthreads[i1].status |= 1;
                } else {
                  initsucc = 1;
                }

                if (pthread_mutex_init(&dbthreads[i1].MUTEX_var2, NULL) == 0) {
                  dbthreads[i1].status |= 2;
                } else {
                  initsucc = 1;
                }

                if (pthread_mutex_init(&dbthreads[i1].MUTEX_fertig1, NULL) == 0) {
                  dbthreads[i1].status |= 4;
                } else {
                  initsucc = 1;
                }

                if (pthread_mutex_init(&dbthreads[i1].MUTEX_fertig2, NULL) == 0) {
                  dbthreads[i1].status |= 8;
                } else {
                  initsucc = 1;
                }

                if (sem_init(&dbthreads[i1].sem1, 0, 0) == 0) {
                  dbthreads[i1].status |= 16;
                } else {
                  initsucc = 1;
                }

                if (sem_init(&dbthreads[i1].semret1, 0, 0) == 0) {
                  dbthreads[i1].status |= 32;
                } else {
                  initsucc = 1;
                }

                if (sem_init(&dbthreads[i1].sem2, 0, 0) == 0) {
                  dbthreads[i1].status |= 64;
                } else {
                  initsucc = 1;
                }

                if (sem_init(&dbthreads[i1].semret2, 0, 0) == 0) {
                  dbthreads[i1].status |= 128;
                } else {
                  initsucc = 1;
                }


                if ((dbthreads[i1].status & 255) == 255) {

                  if (pthread_create(&(dbthreads[i1].thread1), NULL, &dbscanthread1,
                                     &dbthreads[i1]) == 0) {
                    dbthreads[i1].status |= 256;
                  } else {
                    initsucc = 1;
                  }
                }

                if ((dbthreads[i1].status & 511) == 511) {

                  if (pthread_create(&(dbthreads[i1].thread2), NULL, &dbscanthread2,
                                     &dbthreads[i1]) == 0) {
                    dbthreads[i1].status |= 512;
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

              ret = dbscan_pthreads(conb, (float *) condata, blen, eps, kk, features,
                                    cores, dbthreads);

              if (ret >= 0) {
                for (int i1 = 0; i1 < blen; i1++) {
                  conb[i1] = (conb[i1] >> 3);
                }

                (*env)->SetShortArrayRegion(env, b, 0, blen, (jshort *) conb);
              }

#ifdef GPUTIMING
              clock_gettime(CLOCK_REALTIME, &finish2);
#endif

            } else {
              ret = -108;
            }

            for (int i1 = 0; i1 < cores; i1++) {

              if ((dbthreads[i1].status & 256) > 0) {

                pthread_mutex_lock(&dbthreads[i1].MUTEX_fertig1);
                dbthreads[i1].fertig1 = 1;
                pthread_mutex_unlock(&dbthreads[i1].MUTEX_fertig1);
                sem_post(&dbthreads[i1].sem1);

                pthread_join(dbthreads[i1].thread1, NULL);
              }

              if ((dbthreads[i1].status & 512) > 0) {

                pthread_mutex_lock(&dbthreads[i1].MUTEX_fertig2);
                dbthreads[i1].fertig2 = 1;
                pthread_mutex_unlock(&dbthreads[i1].MUTEX_fertig2);
                sem_post(&dbthreads[i1].sem2);

                pthread_join(dbthreads[i1].thread2, NULL);
              }


              if ((dbthreads[i1].status & 16) > 0) {
                sem_destroy(&dbthreads[i1].sem1);
              }

              if ((dbthreads[i1].status & 32) > 0) {
                sem_destroy(&dbthreads[i1].semret1);
              }

              if ((dbthreads[i1].status & 64) > 0) {
                sem_destroy(&dbthreads[i1].sem2);
              }

              if ((dbthreads[i1].status & 128) > 0) {
                sem_destroy(&dbthreads[i1].semret2);
              }

              if ((dbthreads[i1].status & 1) > 0) {
                pthread_mutex_destroy(&dbthreads[i1].MUTEX_var1);
              }

              if ((dbthreads[i1].status & 2) > 0) {
                pthread_mutex_destroy(&dbthreads[i1].MUTEX_var2);
              }

              if ((dbthreads[i1].status & 4) > 0) {
                pthread_mutex_destroy(&dbthreads[i1].MUTEX_fertig1);
              }

              if ((dbthreads[i1].status & 8) > 0) {
                pthread_mutex_destroy(&dbthreads[i1].MUTEX_fertig2);
              }

            }

            free(dbthreads);

          } else {
            ret = -107;
          }

          free(conb);

        } else {
          ret = -105;
        }

        (*env)->ReleaseFloatArrayElements(env, rf, condata, JNI_ABORT);

      } else {
        ret = -104;
      }
    } else {
      ret = -103;
    }
  } else {
    ret = -102;
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
Java_com_example_dmocl_dbscan_dbscanabort_1c(JNIEnv *env, jclass clazz) {

  rwlockwp_writer_acquire(&abortcalc);
  doabort = 1;
  rwlockwp_writer_release(&abortcalc);

}

JNIEXPORT void JNICALL
Java_com_example_dmocl_dbscan_dbscanresume_1c(JNIEnv *env, jclass clazz) {
  rwlockwp_writer_acquire(&abortcalc);
  doabort = 0;
  rwlockwp_writer_release(&abortcalc);
}

