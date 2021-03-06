/*!
 \file kmeans_c.c
 \brief Source file for the C/C+GPU implementations of the Kmeans algorithm
 \details
 This header file contains three method prototypes, that allow to perform single- or multithreaded CPU or
 GPU based Kmeans cluster searches.
 \copyright Copyright Robert Fritze 2021
 \license MIT
 \version 1.0
 \author Robert Fritze
 \date 11.9.2021
 */

#include <jni.h>
#include "kmeans_c.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "oclwrapper.h"

                                //! use older OpenCL APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#include <CL/opencl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <rwlock_wp.h>

                               //! Define if detailed timing for the GPU should be made
#define GPUTIMING

                               //! A reader writer lock for premature abort
volatile struct rwlockwp abortcalckm = RWLOCK_STATIC_INITIALIZER;
                               //! 1=abort, access with *abortcalckm*
volatile int doabort = 0;

/*!
 \brief Kmeans OpenCL kernel
 \details
 <h3>__kernel void testdistance</h3>
 (<br>
 &emsp;  global const float* data, <br>
 &emsp;  global unsigned short* b, <br>
 &emsp;  constant const float* clucent, <br>
 &emsp;  const int features, <br>
 &emsp;  const int cluno  <br>
 ) <br><br>

 calculates for each data item
 the eucledean distance to all cluster centers
 and saves the number of the cluster center with the smallest distance. <br>

 <table>
  <tr>
    <th>parameter</th>
    <th>in/out</th>
    <th>description</th>
  </tr>
  <tr>
    <td><em>global const float* data</em></td>
    <td>in</td>
    <td>input data</td>
  </tr>
  <tr>
    <td><em>global unsigned short* b</em></td>
    <td>out</td>
    <td>cluster number for each data item</td>
  </tr>
  <tr>
    <td><em>constant const float* clucent</em></td>
    <td>in</td>
    <td>cluster centers</td>
  </tr>
  <tr>
    <td><em>const int features</em> </td>
    <td>in</td>
    <td>the number of features per data item</td>
  </tr>
  <tr>
    <td><em>const int cluno</em> </td>
    <td>in</td>
    <td>the number of clusters to search for</td>
  </tr>
</table>

 */
const char* clsource = \
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



                                   //! maximum numbers of cycles for kmeans (to avoid endless cycling)
#define MAXCYCLES 100000


/*!
 \brief Random number generator
 \details
 Generates uniformly distributed random numbers [0,limit]
 \param limit (in) the maximum random number desired
 \returns a random number form a uniform distribution over [0,limit]
 \mt fully threadsafe
 */
int rand_lim(int limit) {

  int divisor = RAND_MAX / (limit + 1);   // devisor
  int retval;                             // return value

  do {
    retval = rand() / divisor;            // create random number
  } while (retval > limit);               // wait until inside [0,limit]

  return retval;
} // randlim


/*!
 \brief Kmeans cluster search
 \details
 Performs a Kmeans cluster search on the CPU (one thread).
 \param b (out) Array of cluster numbers
 \param data (in) Array of data points
 \param blen (in) number of data items in data
 \param eps (in) maximum cluster center displacement
 \param cluno (in) number of clusters to search for
 \param features (in) number of features per data item
 \returns 0 = no error, <0 = error number
 \mt fully threadsafe
 */
short kmeans(unsigned short *b, const float *data, const int blen, const float eps, const int cluno,
             const int features) {

  short ret = 0;                         // return value

  srand((unsigned int) time(NULL));          // initialize random generator

                                 // alloc memory for cluster centers
  float *clucent = (float *) malloc(sizeof(float) * features * cluno);

  if (clucent != NULL) {                 // malloc error?

                                           // a second buffer for cluster centers
    float *newclucent = (float *) malloc(sizeof(float) * features * cluno);

    if (newclucent != NULL) {                   // malloc error

                                                // size of the clusters
      int *clusize = (int *) malloc(sizeof(int) * cluno);

      if (clusize != NULL) {                     // malloc error?

                                       // iterate over all cluster centers
        for (int i1 = 0; i1 < cluno; i1++) {

                                      // select a point randomly
          int cluxi = rand_lim(blen - 1);

                         // copy data point as new cluster center
          for (int i2 = 0; i2 < features; i2++) {
            clucent[features * i1 + i2] = data[cluxi * features + i2];
          }
        }


        int weiter = 0;                // loop abort condition
        int cycles = 0;                // counts the number of cycles

        while (weiter >= 0) {          // continue as long as max number of cycles has
                                      // not been reached or cluster center displacement
                                      // has become very small

                                       // iterate over all data items
          for (int i1 = 0; i1 < blen; i1 += 1) {

            float noxi = INFINITY;        // assign initial minimum distance

                                           // iterate over all cluster centers
            for (unsigned short i2 = 0; i2 < cluno; i2++) {

              float noxi2 = 0;                  // temporary value for cluster distance

                                        // iterate over all features and calculate
                                        // euclidean distance
              for (int i3 = 0; i3 < features; i3++) {
                noxi2 += powf(clucent[i2 * features + i3] - data[i1 * features + i3], 2);
              }

                                          // compare distances
              if (noxi2 < noxi) {
                noxi = noxi2;              // set new cluster center for data item if
                b[i1] = i2;                // distance is smaller
              }
            }
          }

                                          // now update the cluster centers
          for (int i1 = 0; i1 < cluno; i1++) {
                                       // set new cluster centers to zero
            for (int i2 = 0; i2 < features; i2++) {
              newclucent[i1 * features + i2] = 0;
            }
            clusize[i1] = 0;                // set cluster size to zero
          }

                                         // add all points belonging to each cluster center
          for (int i1 = 0; i1 < blen; i1++) {
            for (int i2 = 0; i2 < features; i2++) {
              newclucent[b[i1] * features + i2] += data[i1 * features + i2];
            }
            clusize[b[i1]]++;            // count cluster members
          }

                                         // find midpoint of cluster center
          for (int i1 = 0; i1 < cluno; i1++) {
            for (int i2 = 0; i2 < features; i2++) {
              newclucent[i1 * features + i2] /= (float) clusize[i1];
            }
          }

          float newdist = 0;                 // calculate cluster center displacement

                                             // iterate over all cluster centers
          for (int i1 = 0; i1 < cluno; i1++) {

            float newdist2 = 0;

                              // add cluster center displacement
            for (int i2 = 0; i2 < features; i2++) {
              newdist2 += powf(clucent[i1 * features + i2] - newclucent[i1 * features + i2], 2);
            }

            newdist += sqrt(newdist2);          // calculate euclidean distance
          }
                                         // check loop conditions
          if ((newdist <= eps) || (cycles > MAXCYCLES)) {
            weiter = -1;
          }

          cycles++;                        // cound cycles

                                   // copy cluster centers
          memcpy(clucent, newclucent, sizeof(float) * features * cluno);

        }

        free(clusize);            // free used memory
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

} // kmeans



// see header file
JNIEXPORT jshort JNICALL Java_com_example_dmocl_kmeans_kmeans_1c
  (JNIEnv *env, jclass jc, jshortArray b, jfloatArray rf, jfloat eps, jint kk, jint features) {

  short ret = 0;                  // return value

                  // check for architecture compartibility
  if ((sizeof(jshort) == sizeof(unsigned short)) && (sizeof(jfloat) == sizeof(float))) {

                    // get number of data items*features
    jsize datalen = (*env)->GetArrayLength(env, rf);
                     // get number of data items
    jsize blen = (*env)->GetArrayLength(env, b);

    if (features * blen == datalen) {   // must correspond

                                 // get array data
      jfloat* condata = (*env)->GetFloatArrayElements(env, rf, NULL);

      if (condata != NULL) {

                                  // allocate buffer for the cluster assignment
        unsigned short *conb = (unsigned short *) malloc(sizeof(unsigned short) * blen);

        if (conb != NULL) {              // malloc error

                                           // perform kmeans search
          kmeans(conb, condata, blen, eps, kk, features);

                             // copy result
          (*env)->SetShortArrayRegion(env, b, 0, blen, (jshort *) conb);

          free(conb);                   // clean

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
} // Java_com_example_dmocl_kmeans_kmeans_1c



/*!
 \brief kmeans cluster search on the GPU
 \details
 Performs a Kmeans cluster search on the GPU
 \param b (out) Array of cluster numbers
 \param data (in) Array of data points
 \param blen (in) number of data items in data
 \param eps (in) maximum cluster center displacement
 \param cluno (in) number of clusters to search for
 \param features (in) number of features per data item
 \param commands (in) the OpenCL command queue
 \param program (in) the OpenCL program
 \param device (in) the OpenCL device
 \param kernel_testdistance (in) the OpenCL kernel
 \param data_g (in) OpenCL data buffer
 \param b_g (in) OpenCL cluster number buffer
 \param clucent_g (in) OpenCL cluster center buffer
 \returns 0 = no error, <0 = error number
 \mt fully threadsafe
 */
short kmeans_gpu(cl_ushort *b, const cl_float *data, const int blen, const float eps, const int cluno,
           const int features,
           cl_command_queue commands, cl_program program, cl_device_id device,
           cl_kernel kernel_testdistance, cl_mem data_g, cl_mem b_g, cl_mem clucent_g) {

  short ret = 0;                                 // return value

                                        // copy values to OpenCL data types
  const cl_int features_g = features;               // copy features
  const size_t global_size = blen;                  // copy data array length
  const cl_int cluno_g = cluno;                     // copy cluster count

                                                    // set the kernal arguments
  cl_int err = clSetKernelArg(kernel_testdistance, 0, sizeof(cl_mem), &data_g);
  err |= clSetKernelArg(kernel_testdistance, 1, sizeof(cl_mem), &b_g);
  err |= clSetKernelArg(kernel_testdistance, 2, sizeof(cl_mem), &clucent_g);
  err |= clSetKernelArg(kernel_testdistance, 3, sizeof(cl_int), &features_g);
  err |= clSetKernelArg(kernel_testdistance, 4, sizeof(cl_int), &cluno_g);

  if (err != CL_SUCCESS) {                   // error?
    return (-4);
  }

  srand((unsigned int) time(NULL));             // initialize random generator

                                    // allocate memory for the cluster centers
  cl_float *clucent = (cl_float *) malloc(sizeof(cl_float) * features * cluno);

  if (clucent != NULL) {                           // malloc error?

                                  // allocate second buffer for cluster centers
    cl_float *newclucent = (cl_float *) malloc(sizeof(cl_float) * features * cluno);

    if (newclucent != NULL) {                       // malloc error?

                                     // allocate buffer for the cluster member cound
      int *clusize = malloc(sizeof(int) * cluno);

      if (clusize != NULL) {                          // malloc error?

                                      // iterate over cluster centers
        for (int i1 = 0; i1 < cluno; i1++) {

                                    // get random initial cluster center point
          int cluxi = rand_lim(blen - 1);

                                    // copy new random cluster center point
          for (int i2 = 0; i2 < features; i2++) {
            clucent[features * i1 + i2] = data[cluxi * features + i2];
          }
        }


        int weiter = 0;                    // loop abort condition
        int cycles = 0;                   // counts the cycles

        while (weiter >= 0) {           // loop as long as cluster center displacement is
                                       // sufficiently large or maximum cycle count has not been reached

                                        // copy data to GPU
          err = clEnqueueWriteBuffer(commands, clucent_g, CL_TRUE, 0,
                                     sizeof(cl_float) * features * cluno, clucent, 0, NULL, NULL);

          if (err != CL_SUCCESS) {            // success?
            ret = -6;
            break;
          }
                                           // enqueue kernel
          err = clEnqueueNDRangeKernel(commands, kernel_testdistance, 1, NULL, &global_size, NULL,
                                       0, NULL, NULL);

          if (err != CL_SUCCESS) {           // success?
            ret = -7;
            break;
          }

                              // initialize array with new cluster centers
          for (int i1 = 0; i1 < cluno; i1++) {

            for (int i2 = 0; i2 < features; i2++) {
              newclucent[i1 * features + i2] = 0;
            }

            clusize[i1] = 0;
          }

                        // wait until GPU has finished calculations
          clFinish(commands);

                             // read new cluster assignment
          err = clEnqueueReadBuffer(commands, b_g, CL_TRUE, 0, sizeof(cl_ushort) * blen, b, 0, NULL,
                                    NULL);

                                     // error?
          if (err != CL_SUCCESS) {
            ret = -8;
            break;
          }

                                 // calculate new midpoints (cluster centers)
          for (int i1 = 0; i1 < blen; i1++) {
            for (int i2 = 0; i2 < features; i2++) {
              newclucent[b[i1] * features + i2] += data[i1 * features + i2];
            }
            clusize[b[i1]]++;           // count cluster members
          }

                             // divide by number of cluster members
          for (int i1 = 0; i1 < cluno; i1++) {
            for (int i2 = 0; i2 < features; i2++) {
              newclucent[i1 * features + i2] /= (cl_float) clusize[i1];
            }
          }

          float newdist = 0;            // calculate cluster center displacement

                                   // iterate over cluster centers
          for (int i1 = 0; i1 < cluno; i1++) {

            float newdist2 = 0;

                           // calculate euclidean distance of cluster center displacement
            for (int i2 = 0; i2 < features; i2++) {
              newdist2 += powf(clucent[i1 * features + i2] - newclucent[i1 * features + i2], 2);
            }

            newdist += sqrt(newdist2);
          }

                           // check loop conditions
          if ((newdist <= eps) || (cycles>MAXCYCLES)) {
            weiter = -1;
          }

                          // copy new cluster centers
          memcpy(clucent, newclucent, sizeof(cl_float) * features * cluno);

          cycles++;                    // count cycles
        }

        free(clusize);                   // clean
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

} // kmeans_gpu



// see header file
JNIEXPORT jshort JNICALL Java_com_example_dmocl_kmeans_kmeans_1c_1gpu
  (JNIEnv *env, jclass jc, jshortArray b, jfloatArray rf, jfloat eps, jint cluno, jint features,
   jlongArray e) {


  struct timespec start2, finish2;     // for calculation of exclusive runtime

  short ret = 0;                          // return value

                  // check architecture
  if ((sizeof(jshort) == sizeof(cl_ushort)) && (sizeof(jfloat) == sizeof(cl_float))) {

                    // get data array and data item length
    jsize datalen = (*env)->GetArrayLength(env, rf);
    jsize blen = (*env)->GetArrayLength(env, b);

    if (features * blen == datalen) {             // must match

                            // copy data items
      jfloat* condata = (*env)->GetFloatArrayElements(env, rf, NULL);

      if (condata != NULL) {             // error?

                         // allocate array for cluster assignment
        cl_ushort *conb = (cl_ushort *) malloc(sizeof(cl_ushort) * blen);

        if (conb != NULL) {              // malloc error

          cl_uint numplatf;                 // holds platform number

                                            // get number of platforms
          cl_int err = clGetPlatformIDs(0, NULL, &numplatf);

          if (err == CL_SUCCESS) {             // error?

            if (numplatf >= 1) {           // there mus be at least one

                                             // yes -> select first
              cl_platform_id platf;          // platform data

                                           // get all platforms
              err = clGetPlatformIDs(1, &platf, NULL);

              if (err == CL_SUCCESS) {      // error?

                cl_uint numdevices;           // device info

                                      // get number of devices
                err = clGetDeviceIDs(platf, CL_DEVICE_TYPE_ALL, 0, NULL, &numdevices);

                if (err == CL_SUCCESS) {       // error?

                  if (numdevices >= 1) {       // there must be at least one device

                    cl_device_id dev;           // get device info

                    err = clGetDeviceIDs(platf, CL_DEVICE_TYPE_ALL, 1, &dev, NULL);

                    if (err == CL_SUCCESS) {       // error?
                                                   // select first device


                      // create a context with the specified
                      // platform and device
                      cl_context context = clCreateContext(NULL, 1, &dev, NULL, NULL, &err);

                      if (context != NULL) {           // context error?

                        cl_command_queue commands;        // compute command queue

                                                        // Create a command queue
                        commands = clCreateCommandQueue(context, dev, 0, &err);

                        if (commands != NULL) {             // error?

                                                      // create program
                          cl_program program = clCreateProgramWithSource(context, 1, &clsource,
                                                                         NULL, &err);

                          if (program != NULL) {             // error?

                                                       // build program
                            err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

                            if (err == CL_SUCCESS) {             // error?

                              cl_kernel kernel_testdistance;    // holds kernel

                                                       // extract kernel
                              kernel_testdistance = clCreateKernel(program, "testdistance", &err);

                              if (err == CL_SUCCESS) {           // error?


                                                       // create first buffer
                                cl_mem data_g = clCreateBuffer(context,
                                                               CL_MEM_READ_ONLY |
                                                               CL_MEM_USE_HOST_PTR,
                                                               sizeof(cl_float) * datalen, condata,
                                                               &err);

                                if (data_g != NULL) {              // error?

                                                          // create second buffer
                                  cl_mem b_g = clCreateBuffer(context,
                                                              CL_MEM_READ_WRITE |
                                                              CL_MEM_USE_HOST_PTR,
                                                              sizeof(cl_ushort) * blen, conb, &err);

                                  if (b_g != NULL) {             // error?

                                                             // create third buffer
                                    cl_mem clucent_g = clCreateBuffer(context,
                                                                      CL_MEM_READ_ONLY,
                                                                      sizeof(cl_float) * features *
                                                                      cluno, NULL, &err);

                                    if (clucent_g != NULL) {   // error

#ifdef GPUTIMING
                                             // measure start time
                                      clock_gettime(CLOCK_REALTIME, &start2);
#endif

                                                   // perform kmeans on the GPU
                                      ret = kmeans_gpu(conb, (cl_float*) condata, blen, eps, cluno, features,
                                                       commands, program, dev, kernel_testdistance,
                                                       data_g, b_g, clucent_g);


                                               // store results
                                      (*env)->SetShortArrayRegion(env, b, 0, blen, (jshort*) conb);

#ifdef GPUTIMING
                                                    // measure time
                                      clock_gettime(CLOCK_REALTIME, &finish2);
#endif

                                                     // release data and clean up
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
                       // caluclate time elapsed
  long long int elapsed2 = ((long long int) (finish2.tv_sec - start2.tv_sec)) * 1000000000L;
  elapsed2 += (finish2.tv_nsec - start2.tv_nsec);

                           // store as first (and unique) array element
                           // array has to be pinned
  jlong *edata = (*env)->GetLongArrayElements(env, e, NULL);
  edata[0] = elapsed2;
  (*env)->ReleaseLongArrayElements(env, e, edata, 0);    // "unpin"
#endif

  return (ret);

} // Java_com_example_dmocl_kmeans_kmeans_1c_1gpu



/*!
 \brief Parameters for the kmeans thread
 \details
This struct holds the parameters for each kmeans thread. (in) means parameters that are NOT changed
by the thread but may be changed by the method that submits the job. (const) means that the value is
never changed after setup. (out) attributes are changed by the thread. 
The submitting method must not write access any (in) or (out) field while the calculations are running.
The submitting method may read access all (in) while the calculations are running.
 */
struct kmeans_pt {

  unsigned short int status;          //!< (const) status (needed only for setup and destruction)
  int num;                            //!< (const) number of thread      
  unsigned short *b;                  //!< (out) number of closest cluster center
  float *data;                        //!< (const) input data
  volatile float *clucent;            //!< (in) cluster centers
  int blen;                           //!< (const) number of data items 
  int cluno;                          //!< (const) number of clusters  
  int features;                       //!< (const) number of features per data item
  int start;                          //!< (const) first data item
  int len;                            //!< (const) last data item
  volatile unsigned char fertig;      //!< (in) 1=quit loop

  pthread_t thread;                   //!< (const) reference to the thread
  sem_t sem;                          //!< (const) semaphore used for start
  sem_t semret;                       //!< (const) semaphore for notification that results are ready

};  // struct kmeans_pt


/*!
 \brief thread for calculating kmeans in parallel
 \details
 One or more threads perform a kmeans search in parallel. The thread calculates the distances
to the cluster centers and saves the number of the cluster center with the smallest distance.
Two semaphores are used. The first is acquired by this thread and released by the method that
submits the calculations. The second semaphore is acquired by the method that submits the job
and released by this thread 
 \param arg (in+out) A pointer to the struct with the parameters
 \returns NULL
 */
void* kmthread(void *arg) {

  struct kmeans_pt *f = (struct kmeans_pt *) arg;   // access parameters

  int weiter =  0;                                  // loop abort condition

  while (weiter == 0) {                             // continue?

    sem_wait(&f->sem);                               // wait on semaphore

    if (f->fertig == 0) {                            // exit loop?

      for (int i1 = f->start; i1 < f->start + f->len; i1++) { // iterate over lines assigned

        float noxi = INFINITY;                        // initial smallest distance

        for (short i2 = 0; i2 < f->cluno; i2++) {    // iterate over cluster centers

          float noxi2 = 0;                         // for distance
   
                     // iterate over features and calculate euclidean distance  
          for (int i3 = 0; i3 < f->features; i3++) {
            noxi2 += powf(f->clucent[i2 * f->features + i3] - f->data[i1 * f->features + i3], 2);
          }

          if (noxi2 < noxi) {          // new distance smaller?
            noxi = noxi2;            // yes save distance and cluster center number
            f->b[i1] = i2;
          }
        }
      }

      sem_post(&f->semret);                 // notify that results are ready

    } else {
      weiter = 1;                          // quit loop and thread
    }    
  }

  return (NULL);

}  // kmthread





/*!
 \brief Perform multithreaded Kmeans cluster search
 \details
   Performs a multithreaded Kmeans cluster search on the CPU
 \param b (out) Array of cluster numbers
 \param data (in) Array of data points
 \param clucent (out) Array of cluster centers
 \param blen (in) number of data items in data
 \param cluno (in) number of clusters to search for
 \param features (in) number of features per data item
 \param cores (in) number of threads to be used (CPU can be oversubscribed)
 \param kmthreads (in) pointer to the threads (array must contain 'cores' elements)
 \param eps (in) maximum cluster center displacement
 \returns 0=algorithm finished correctly, <0 error occurred
 */
short kmeans_pthreads(unsigned short* b, const float* data, float* clucent,
                      const int blen, const int cluno, const int features,
                      const int cores, struct kmeans_pt* kmthreads, const float eps) {

  short ret = 0;                      // return value

                                  // allocate memory for cluster centers
  float* newclucent = (float*) malloc(sizeof(float) * features * cluno);

  if (newclucent != NULL) {       // malloc error?

                            // allocate memory for the cluster member counts
    int* clusize = (int*) malloc(sizeof(int) * cluno);

    if (clusize != NULL) {            // malloc error?

                             // iterate over cluster centers
      for (int i1 = 0; i1 < cluno; i1++) {

                             // create random number
        int cluxi = rand_lim(blen - 1);

                              // copy cluster center
        for (int i2 = 0; i2 < features; i2++) {
          clucent[features * i1 + i2] = data[cluxi * features + i2];
        }
      }

      int weiter = 0;              // loop break condition
      int cycles = 0;              // cycle counter

      while (weiter >= 0) {        // loop until cluster centers do not move any more

                                  // iterate over cores
        for (int i1 = 0; i1 < cores; i1++) {
          sem_post(&kmthreads[i1].sem);    // wake up all threads
        }

                          // while threads are calculating -> reset cluster centers
        for (int i1 = 0; i1 < cluno; i1++) {

          for (int i2 = 0; i2 < features; i2++) {
            newclucent[i1 * features + i2] = 0;
          }

          clusize[i1] = 0;         // ... and cluster size
        }

                          // wait until threads have finished
        for (int i1 = 0; i1 < cores; i1++) {
          sem_wait(&kmthreads[i1].semret);
        }

                         // calculate new cluster centers
        for (int i1 = 0; i1 < blen; i1++) {
          for (int i2 = 0; i2 < features; i2++) {
            newclucent[b[i1] * features + i2] += data[i1 * features + i2];
          }
          clusize[b[i1]]++;    // count cluster members
        }

               // iterate of cluster centers and features and divide by number of cluster members
        for (int i1 = 0; i1 < cluno; i1++) {
          for (int i2 = 0; i2 < features; i2++) {
            newclucent[i1 * features + i2] /= (float) clusize[i1];
          }
        }

        float newdist = 0;              // distance

                     // iterate over cluster centers
        for (int i1 = 0; i1 < cluno; i1++) {

          float newdist2 = 0;          // for cluster center displacement

                           // iterate over features and build euclidean distance
          for (int i2 = 0; i2 < features; i2++) {
            newdist2 += powf(clucent[i1 * features + i2] - newclucent[i1 * features + i2], 2);
          }

          newdist += sqrt(newdist2);
        }

                        // check abort conditions
        if ((newdist <= eps) || (cycles > MAXCYCLES)){
          weiter = -1;
        }

                       // copy cluster centers
        memcpy(clucent, newclucent, sizeof(float) * features * cluno);

        cycles++;         // increment cycle counter

      }

      free(clusize);            // free and clean up
    } else {
      ret = -3;
    }

    free(newclucent);
  } else {
    ret = -2;
  }

  return (ret);

}  // kmeans_pthreads



// see header file
JNIEXPORT jshort JNICALL Java_com_example_dmocl_kmeans_kmeans_1c_1phtreads
  (JNIEnv *env, jclass jc, jshortArray b, jfloatArray rf, jfloat eps, jint cluno, jint features,
   jint cores, jlongArray e ) {

  short ret = 0;                              // return value

  struct timespec start2, finish2;       // two timepoints


                              // check architecture
  if ((sizeof(jshort) == sizeof(unsigned short)) && (sizeof(jfloat) == sizeof(float))) {

                               // get the number of data array entries
    jsize datalen = (*env)->GetArrayLength(env, rf);
    jsize blen = (*env)->GetArrayLength(env, b);  // get the number of data elements

    if (features * blen == datalen) {   // ...these must match

                                   // get float array elements and pin array
      jfloat* condata = (*env)->GetFloatArrayElements(env, rf, NULL);

      if (condata != NULL) {     // error?

                          // allocate memory for the cluster center numbers
        unsigned short *conb = (unsigned short *) malloc(sizeof(unsigned short) * blen);

        if (conb != NULL) {                // malloc error?

                                 // allocate memory for the cluster centers
          float *clucent = (float *) malloc(sizeof(float) * features * cluno);

          if (clucent != NULL) {               // malloc error?

                             // allocate memory for the threads
            struct kmeans_pt *kmthreads = (struct kmeans_pt *) malloc(
              sizeof(struct kmeans_pt) * cores);

            if (kmthreads != NULL) {            // malloc error?

              int initsucc = 0;           // 1 = thread initialization was not successfull

              int stepper = (blen / cores) + 1;        // caluclate data items per thread
              int starter = 0;                    // start with item #0
              int reminder = blen;               // data items left

                                      // iterate over cores
              for (int i1 = 0; i1 < cores; i1++) {

                kmthreads[i1].status = 0;          // init status

                if (reminder < stepper) {          // only few left?
                  stepper = reminder;          // assign the remaining ones to the thread
                }

                if (initsucc == 0) {                // successfully so far?

                                            // initialize the thread arguments
                  kmthreads[i1].num = i1;         // thread number
                  kmthreads[i1].b = conb;       // reference to cluster number array
                  kmthreads[i1].data = condata;   // reference to data items
                  kmthreads[i1].blen = blen;     // number of data items
                  kmthreads[i1].cluno = cluno;    // number of clusters to search for
                  kmthreads[i1].features = features;    // number of features
                  kmthreads[i1].start = starter;     // start data element
                  kmthreads[i1].len = stepper;      // number of elements for the thread
                  kmthreads[i1].clucent = clucent;    // reference to cluster center
                  kmthreads[i1].fertig = 0;        // stop condition


                                              // initialize first semaphore
                  if (sem_init(&kmthreads[i1].sem, 0, 0) == 0) {
                    kmthreads[i1].status |= 1;     // OK -> set status
                  } else {
                    initsucc = 1;                    // abort
                  }

                                             // initialize second semaphore
                  if ((kmthreads[i1].status & 1) == 1) {
                    if (sem_init(&kmthreads[i1].semret, 0, 0) == 0) {
                      kmthreads[i1].status |= 2;     // OK -> set status
                    } else {
                      initsucc = 1;                 // abort
                    }
                  }

                                // both semaphores initialized?
                  if ((kmthreads[i1].status & 3) == 3) {

                                  // yes -> create thread
                    if (
                      pthread_create(&(kmthreads[i1].thread), NULL, &kmthread, &kmthreads[i1]) ==
                      0) {
                      kmthreads[i1].status |= 4;   // OK -> set status
                    } else {
                      initsucc = 1;                 // error -> abort
                    }
                  }

                }

                starter += stepper;               // step data elements
                reminder -= stepper;
              }

                                      // threads created successfully?
              if (initsucc == 0) {

#ifdef GPUTIMING
                                       // get time
                clock_gettime(CLOCK_REALTIME, &start2);
#endif
                                 // perform calculations
                ret = kmeans_pthreads(conb, condata, clucent, blen, cluno, features, cores,
                                      kmthreads, eps);

                                // copy results
                (*env)->SetShortArrayRegion(env, b, 0, blen, (jshort *) conb);

#ifdef GPUTIMING
                                 // get time
                clock_gettime(CLOCK_REALTIME, &finish2);
#endif

              } else {
                ret = -8;
              }

                            // iterate over cores
              for (int i1 = 0; i1 < cores; i1++) {

                                  // first semaphore initialized?
                if ((kmthreads[i1].status & 4) > 0) {

                               // yes -> signal abort
                  kmthreads[i1].fertig = 1;
                  sem_post(&kmthreads[i1].sem);    // weak up

                                   // wait until finished
                  pthread_join(kmthreads[i1].thread, NULL);

                }

                             // second semaphore initialized
                if ((kmthreads[i1].status & 2) > 0) {
                  sem_destroy(&kmthreads[i1].semret);   // yes -> destroy
                }

                              // first semaphore initialized
                if ((kmthreads[i1].status & 1) > 0) {
                  sem_destroy(&kmthreads[i1].sem);    // yes -> destroy
                }
              }

              free(kmthreads);            // free and clean up

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

                            // unpin data elements
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
                  // calculate elapsed time
  long long int elapsed2 = ((long long int) (finish2.tv_sec - start2.tv_sec)) * 1000000000L;
  elapsed2 += (finish2.tv_nsec - start2.tv_nsec);

            // lock array
  jlong *edata = (*env)->GetLongArrayElements(env, e, NULL);
  edata[0] = elapsed2;            // set first (and single) array element
                     // unlock array
  (*env)->ReleaseLongArrayElements(env, e, edata, 0);
#endif

  return (ret);
}  // Java_com_example_dmocl_kmeans_kmeans_1c_1phtreads




// see header file
JNIEXPORT void JNICALL
Java_com_example_dmocl_kmeans_kmabort_1c(JNIEnv *env, jclass clazz) {
  rwlockwp_writer_acquire(&abortcalckm);   // acquire writer lock
  doabort = 1;                              // signal to abort
  rwlockwp_writer_release(&abortcalckm);   // release writer lock
}

// see header file
JNIEXPORT void JNICALL
Java_com_example_dmocl_kmeans_kmresume_1c(JNIEnv *env, jclass clazz) {
  rwlockwp_writer_acquire(&abortcalckm);  // acquire writer lock
  doabort = 0;                            // signal to resume
  rwlockwp_writer_release(&abortcalckm);  // relese writer lock
}

