/*!
 \file dbscan_c.c
 \brief Source file for the C/C+GPU implementations of the DBSCAN algorithm
 \details
 This C source file contains three methods, that allow to perform single- or multithreaded CPU or
 GPU based DBSCAN cluster searches.
 \copyright Copyright Robert Fritze 2021
 \license MIT
 \version 1.0
 \author Robert Fritze
 \date 11.9.2021
 */

#include <jni.h>
#include "dbscan_c.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <rwlock_wp.h>

#include "oclwrapper.h"
                              //! User older APIs
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#include <CL/opencl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

#define MAXVALUE ((short) 0xFFFF)       //!< Maximum value for 16 Bit short
#define GPUTIMING                       //!< Define if exclusive GPU time should be measured

                          //! a lock for premature abort of algorithms
volatile struct rwlockwp abortcalc = RWLOCK_STATIC_INITIALIZER;
volatile int doabort = 0;           //!< flag for premature abort of algorithms




/*!
 \brief DBSCAN OpenCL kernel
 \details

 <h3>__kernel void testdistance1</h3>
 (<br>
 &emsp;  global const float* data, <br>
 &emsp;  global unsigned short* b, <br>
 &emsp;  const int features, <br>
 &emsp;  const int cmpto, <br>
 &emsp;  const float epseps <br>
 ) <br><br>

 Calculates the euclidean distance of each data item to a specific given data item.
 This method is called from the main loop.<br>

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
    <td>Bit 0: Point has been clustered (1) or not (0)<br>
        Bit 1: Point is distance reachable (main loop)<br>
        Bit 2: Point is distance reachable (cluster expansion loop)<br>
        Bit 3-15: Cluster number for each data item</td>
  </tr>
  <tr>
    <td><em>const int features</em> </td>
    <td>in</td>
    <td>the number of features per data item</td>
  </tr>
  <tr>
    <td><em>const int cmpto</em> </td>
    <td>in</td>
    <td>the number of the data point the other data item should be compared to</td>
  </tr>
  <tr>
    <td><em>const float epseps</em></td>
    <td>in</td>
    <td>square of the search radius</td>
  </tr>
</table>


 <h3>__kernel void testdistance2</h3>
 (<br>
 &emsp;  global const float* data, <br>
 &emsp;  global unsigned short* b, <br>
 &emsp;  const int features, <br>
 &emsp;  const int cmpto, <br>
 &emsp;  const float epseps <br>
 ) <br><br>

 Calculates the euclidean distance of each data item to a specific given data item.
 This method is used during the expansion of the clusters.<br>

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
    <td>Bit 0: Point has been clustered (1) or not (0)<br>
        Bit 1: Point is distance reachable (main loop)<br>
        Bit 2: Point is distance reachable (cluster expansion loop)<br>
        Bit 3-15: Cluster number for each data item</td>
  </tr>
  <tr>
    <td><em>const int features</em> </td>
    <td>in</td>
    <td>the number of features per data item</td>
  </tr>
  <tr>
    <td><em>const int cmpto</em> </td>
    <td>in</td>
    <td>the number of the data point the other data item should be compared to</td>
  </tr>
  <tr>
    <td><em>const float epseps</em></td>
    <td>in</td>
    <td>square of the search radius</td>
  </tr>
</table>

 */
const char* clsource = \
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




/*!
 \brief Expands a cluster found
 \details
 This method expands a cluster found to the largest size possible.
 \param key (in) data item number of the new cluster seed
 \param clusternumber (in) cluster number to assign to all members of the cluster
 \param b (out) Cluster number + status bits (Bit 0: data item classified, Bit 1: distance
 reachable from main loop, Bit 2: distance reachable from cluster expansion
 \param data (in) input data
 \param epseps (in) square of search radius
 \param kk (in) number of neighbours
 \param datalen (in) number of data items (datalen*features = number of floats in 'data')
 \param features (in) number of features
 \returns 0 = OK, <0 interrupt by flag
 */
int expandCluster(int key, const short clusternumber,
                  unsigned short *b, const float *data, const float epseps, const int kk,
                  const int datalen, const int features) {


  b[key] &= 7;            // clear bits 3-15
  b[key] |= (clusternumber << 3);     // save cluster number

  int weiter = 0;              // loop break condition
  int ret = 0;                // return value

  while (weiter == 0) {    // loop until no more new data items can be appended to the cluster

    weiter = 1;               // suppose to quit loop

                                // check if calculations should be aborted
    rwlockwp_reader_acquire(&abortcalc);
    if (doabort > 0) {
      weiter = 2;
      ret = -1;
    }
    rwlockwp_reader_release(&abortcalc);

    if (weiter == 1) {        // abort?

                       // iterate over all data items
      for (int i1 = 0; i1 < datalen; i1++) {

        if ((b[i1] & 3) == 2) {    // is distance reachable?

          b[i1] |= 1;             // set visited

          int itemcounter2 = 0;      // count the data items inside radius

                                      // iterate over data items
          for (int i2 = 0; i2 < datalen; i2++) {

            b[i2] &= MAXVALUE - 4;     // clear bit 3

            float s = 0;                   // holds distance

                                 // iterate over features
                                 // and calculate euclidean distance
            for (int i3 = 0; i3 < features; i3++) {
              s += powf(data[i2 * features + i3] - data[i1 * features + i3], 2);
            }

            if (s <= epseps) {           // inside radius?

              b[i2] |= 4;               // yes->set reachable bit
              itemcounter2++;           // increment counter of new reachable data items
            }
          }

                                      // enough data items in radius?
          if (itemcounter2 >= kk) {

                          // yes -> expand cluster
                           // iterate again over all data items
            for (int i2 = 0; i2 < datalen; i2++) {

                                 // inside radius?
              if ((b[i2] & 6) == 4) {

                b[i2] |= 2;         // set bit 2 (expand cluster)
                weiter = 0;        // restart loop because cluster has been expanded
              }
            }
          }

                             // data item has not yet been classified (or is noise)?
          if ((b[i1] >> 3) == 0) {    // yes -> store new custer number
            b[i1] |= (clusternumber << 3);
          }

        }
      }

                          // interrupt -> exit
      if (ret < 0) {
        break;
      }
    }
  }

  return (ret);
}  // expandCluster




/*!
 \brief Performs a DBSCAN search on the CPU (one thread)
 \details
 This method searches clusters with the DBSCAN method on the CPU with a single thread
 \param b (out) Cluster number + status bits (Bit 0: data item classified, Bit 1: distance
 reachable from main loop, Bit 2: distance reachable from cluster expansion
 \param data (in) input data
 \param blen (in) number of data items (blen*features = number of floats in 'data')
 \param eps (in) search radius
 \param kk (in) number of neighbours
 \param features (in) number of features
 \returns >0 number of clusters found (0=only noise points), -1=permature abort,
 -256=too many clusters (number can not be stored with 12 bits)
 \mt fully threadsafe
 */
short dbscan(unsigned short* b, const float* data, const int blen, const float eps, const int kk,
             const int features) {

  short clusternumber = 0;    // initial cluster number (0=noise)

  const float epseps = eps * eps;      // caluclate square radius

  for (int i1 = 0; i1 < blen; i1++) {        // reset cluster number array
    b[i1] = 0;
  }

                         // iterate over data points
  for (int i1 = 0; i1 < blen; i1++) {

    if ((b[i1] & 1) == 0) {                         // visited?

                                      // check if algorithm should abort?
      rwlockwp_reader_acquire(&abortcalc);
      if (doabort > 0) {
        clusternumber = -1;            // result
      }
      rwlockwp_reader_release(&abortcalc);

      if (clusternumber < 0) {              // abort?
        break;
      }

      b[i1] |= 1;                              // set visited bit

      int itemcounter = 0;                     // counts the number of items

      for (int i2 = 0; i2 < blen; i2++) {             // iterate over all data points

        b[i2] &= MAXVALUE - 6;                  // clear bits 2+3
        float s = 0;                             // holds the distance

                                          // calculate eucleadean distance
        for (int i3 = 0; i3 < features; i3++) {
          s += powf(data[i2 * features + i3] - data[i1 * features + i3], 2);
        }

        if (s <= epseps) {                   // inside radius

          b[i2] |= 2;                            // set distance bit
          itemcounter++;                        // increment counter
        }
      }

      if (itemcounter < kk) {                   // not enough neighbours?

        b[i1] &= 7;                      // set noise (clear bits 3-15)

      } else {
                                          // enough neighbours ->
        if (clusternumber == 4095) {          // free cluster number available?
          clusternumber = -256;                // no signal error and quit
          break;
        }

        clusternumber += 1;                // increment number

                                    // increase cluster to maximum size possible
        int ret2 = expandCluster(i1, clusternumber, b, data, epseps, kk, blen, features);

        if (ret2 < 0) {                  // error during cluster expansion?
          clusternumber = -1;              // signal error and exit
          break;
        }
      }
    }
  }

  return (clusternumber);
} // dbscan



//see header file
JNIEXPORT jshort JNICALL
Java_com_example_dmocl_dbscan_dbscan_1c
  (JNIEnv *env, jclass jc, jshortArray b, jfloatArray rf, jfloat eps, jint kk, jint features) {

  short ret = -1;                   // return value

                              // check architecture
  if ((sizeof(jshort) == sizeof(unsigned short)) && (sizeof(jfloat) == sizeof(float))) {

                                // get length of data array
    jsize datalen = (*env)->GetArrayLength(env, rf);
    jsize blen = (*env)->GetArrayLength(env, b);   // get number of data items

    if (features * blen == datalen) {   // these must match

                                      // get and pin data items
      jfloat *condata = (*env)->GetFloatArrayElements(env, rf, NULL);

      if (condata != NULL) {              // error?

                                   // allocate memory for cluster number array
        unsigned short *conb = (unsigned short *) malloc(sizeof(unsigned short) * blen);

        if (conb != NULL) {            // malloc error

                               // call dbscan
          ret = dbscan(conb, (float*) condata, blen, eps, kk, features);

          if (ret >= 0) {      // correct result?

                        // shift left 3 bits (delete status bits)
            for (int i1 = 0; i1 < blen; i1++) {
              conb[i1] = (short) (((unsigned short) conb[i1]) >> 3);
            }

                                // store cluster numbers
            (*env)->SetShortArrayRegion(env, b, 0, blen, (jshort *) conb);
          }

          free(conb);                  // free and clean up

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

}  // Java_com_example_dmocl_dbscan_dbscan_1c



/*!
 \brief Expands a cluster found on the GPU
 \details
 This method expands a cluster found to the largest size possible using the GPU.
 \param key (in) data item number of the new cluster seed
 \param clusternumber (in) cluster number to assign to all members of the cluster
 \param b (out) Cluster number + status bits (Bit 0: data item classified, Bit 1: distance
 reachable from main loop, Bit 2: distance reachable from cluster expansion
 \param data (in) input data
 \param epseps (in) square of search radius
 \param kk (in) number of neighbours
 \param datalen (in) number of data items (datalen*features = number of floats in 'data')
 \param features (in) number of features
 \param commands (in) the OpenCL command queue
 \param kernel_testdistance2 (in) the OpenCL kernel for the cluster expand part
 \param b_g (out) OpenCL cluster number buffer
 \param global_size (in) Global work size on the GPU
 \returns 0 = no error, <0 = error number
 \mt fully threadsafe
 */
short expandCluster_gpu(int key, const short clusternumber,
                        unsigned short *b, const float *data, const float epseps, const int kk,
                        const int datalen, const int features,
                        cl_command_queue commands, cl_kernel kernel_testdistance2, cl_mem b_g,
                        const size_t* global_size) {

  short ret = 0;                    // return value

  b[key] &= 7;                       // clear bits 3-15
  b[key] |= (clusternumber << 3);       // set current cluster number

  int weiter = 0;                        // loop condition

  while (weiter == 0) {           // loop until no new data items have been found

    weiter = 1;                   // suppose that there are no new data items...

                             // test if calculations should be aborted
    rwlockwp_reader_acquire(&abortcalc);
    if (doabort > 0) {
      weiter = 2;
      ret = -1;
    }
    rwlockwp_reader_release(&abortcalc);

    if (weiter == 1) {              // abort?

                          // iterate over all data items
      for (int i1 = 0; i1 < datalen; i1++) {

        if ((b[i1] & 3) == 2) {    // if reachable but not yet visited...

          b[i1] |= 1;                 // set visited

                              // enqueue cluster number buffer
          cl_int err = clEnqueueWriteBuffer(commands, b_g, CL_TRUE, 0, sizeof(cl_ushort) * datalen,
                                            b, 0, NULL, NULL);

          if (err != CL_SUCCESS) {             // error?
            ret = -5;
            break;
          }

          cl_int cmpto_g = i1;          // set data item to which to compare to
                                      // set as kernal argument
          err |= clSetKernelArg(kernel_testdistance2, 3, sizeof(cl_int), &cmpto_g);

          if (err != CL_SUCCESS) {                  // error?
            ret = -6;
            break;
          }

                                        // test distance
          err = clEnqueueNDRangeKernel(commands, kernel_testdistance2, 1, NULL, global_size, NULL,
                                       0, NULL, NULL);

          if (err != CL_SUCCESS) {           // error?
            ret = -7;
            break;
          }

          clFinish(commands);            // wait for kernel

                                  // fetch results
          err = clEnqueueReadBuffer(commands, b_g, CL_TRUE, 0, sizeof(cl_ushort) * datalen, b, 0,
                                    NULL, NULL);

          if (err != CL_SUCCESS) {            // error?
            ret = -8;
            break;
          }


          int itemcounter2 = 0;              // holds number of neighbours

                               // iterate over data points
          for (int i2 = 0; i2 < datalen; i2++) {

            itemcounter2 += (b[i2] >> 2) & 1;       // test if reachable and step counter

          }


          if (itemcounter2 >= kk) {             // enough neighbours?

                                       // yes -> mark all new data points as reachable
            for (int i2 = 0; i2 < datalen; i2++) {

              if ((b[i2] & 6) == 4) {            // if reachable...

                b[i2] |= 2;         // set cluster member
                weiter = 0;         // loop again
              }
            }
          }

                             // no assigned cluster number?
          if ((b[i1] >> 3) == 0) {
            b[i1] |= (clusternumber << 3);    // set cluster number
          }

        }
      }

      if (ret < 0) {       // error -> quit
        break;
      }
    }
  }

  return (ret);
} // expandCluster_gpu



/*!
 \brief kmeans cluster search on the GPU
 \details
 Performs a Kmeans cluster search on the GPU
 \param b (out) Array of cluster numbers
 \param data (in) Array of data points
 \param blen (in) number of data items in data
 \param eps (in) search radius
 \param kk (in) number of neighbours
 \param features (in) number of features per data item
 \param commands (in) the OpenCL command queue
 \param program (in) the OpenCL program
 \param device (in) the OpenCL device
 \param kernel_testdistance1 (in) the OpenCL kernel for the main loop
 \param kernel_testdistance2 (in) the OpenCL kernel for the cluster expand part
 \param data_g (in) OpenCL data buffer
 \param b_g (in) OpenCL cluster number buffer
 \param start2 (out) Start time point for exculsive GPU timing
 \param finish2 (out) End time point for exculsive GPU timing
 \returns 0 = no error, <0 = error number
 \mt fully threadsafe
 */
short dbscan_gpu( cl_ushort* b, const cl_float* data, const int blen, const float eps, const int kk,
           const int features,
           cl_command_queue commands, cl_program program, cl_device_id device,
           cl_kernel kernel_testdistance1, cl_kernel kernel_testdistance2,
           cl_mem data_g, cl_mem b_g, struct timespec *start2, struct timespec *finish2) {

  short clusternumber = 0;            // number of cluster found

                        // convert variables to cl-fromat
  const cl_float epseps = eps * eps;            // caclulate square radius
  const cl_int features_g = features;
  size_t global_size = blen;

                              // set kernel arguments
  cl_int err = clSetKernelArg(kernel_testdistance1, 0, sizeof(cl_mem), &data_g);
  err |= clSetKernelArg(kernel_testdistance1, 1, sizeof(cl_mem), &b_g);
  err |= clSetKernelArg(kernel_testdistance1, 2, sizeof(cl_int), &features_g);
  err |= clSetKernelArg(kernel_testdistance1, 4, sizeof(cl_float), &epseps);

  if (err != CL_SUCCESS) {               // error?
    return (-20);
  }

                                     // set kernel arguments
  err = clSetKernelArg(kernel_testdistance2, 0, sizeof(cl_mem), &data_g);
  err |= clSetKernelArg(kernel_testdistance2, 1, sizeof(cl_mem), &b_g);
  err |= clSetKernelArg(kernel_testdistance2, 2, sizeof(cl_int), &features_g);
  err |= clSetKernelArg(kernel_testdistance2, 4, sizeof(cl_float), &epseps);

  if (err != CL_SUCCESS) {                   // error?
    return (-21);
  }

#ifdef GPUTIMING
                                      // get start time
  clock_gettime(CLOCK_REALTIME, start2);
#endif

                         // initialize the cluster number array
  for (int i1 = 0; i1 < blen; i1++) {
    b[i1] = 0;
  }

                                    // iterate over all data points
  for (int i1 = 0; i1 < blen; i1++) {

    if ((b[i1] & 1) == 0) {                       // visited ?

                                // test if algorithm should abort
      rwlockwp_reader_acquire(&abortcalc);
      if (doabort > 0) {
        clusternumber = -1;
      }
      rwlockwp_reader_release(&abortcalc);

      if (clusternumber < 0) {                 // terminate?
        break;
      }

      b[i1] |= 1;                          // set visited bit

                             // copy cluster number buffer
      err = clEnqueueWriteBuffer(commands, b_g, CL_TRUE, 0, sizeof(cl_ushort) * blen, b, 0, NULL,
                                 NULL);

      if (err != CL_SUCCESS) {             // error?
        clusternumber = -21;
        break;
      }

      cl_int cmpto_g = i1;                // set data item number to which to compare
                                        // set kernel arg
      err |= clSetKernelArg(kernel_testdistance1, 3, sizeof(cl_int), &cmpto_g);

      if (err != CL_SUCCESS) {               // error?
        clusternumber = -22;
        break;
      }

                                     // perform distance test
      err = clEnqueueNDRangeKernel(commands, kernel_testdistance1, 1, NULL, &global_size, NULL, 0,
                                   NULL, NULL);

      if (err != CL_SUCCESS) {                // error?
        clusternumber = -23;
        break;
      }

      clFinish(commands);              // wait on results

                              // get cluster numbers
      err = clEnqueueReadBuffer(commands, b_g, CL_TRUE, 0, sizeof(cl_ushort) * blen, b, 0, NULL,
                                NULL);

      if (err != CL_SUCCESS) {                   // error?
        clusternumber = -24;
        break;
      }

      int itemcounter = 0;               // holds number of neighbours

                                      // count all neighbours
      for (int i2 = 0; i2 < blen; i2++) {

        itemcounter += (b[i2] >> 1) & 1;

      }

      if (itemcounter < kk) {           // enough?

        b[i1] &= 7;                  // no -> mark as noise

      } else {

                      // else create new cluster

        if (clusternumber == 4095) {          // still free number?
          clusternumber = -256;                // no -> error
          break;
        }

        clusternumber += 1;                   // increment counter

                      // expand cluster
        short rret = expandCluster_gpu(i1, clusternumber, b, data, epseps, kk, blen, features,
                                       commands, kernel_testdistance2, b_g, &global_size);

        if (rret < 0) {                   // error?
          clusternumber = rret;          // return
          break;
        }

      }
    }
  }

#ifdef GPUTIMING
                   // second timepoint
  clock_gettime(CLOCK_REALTIME, finish2);
#endif


  return (clusternumber);
} // dbscan_gpu




// see header file
JNIEXPORT jshort JNICALL
Java_com_example_dmocl_dbscan_dbscan_1c_1gpu
  (JNIEnv *env, jclass jc, jshortArray b, jfloatArray rf, jfloat eps, jint kk, jint features,
   jlongArray e) {

  struct timespec start2, finish2;                // hold two timepoints

  short ret = -1;                                 // return variable

                                       // architecture check
  if ((sizeof(jshort) == sizeof(cl_ushort)) && (sizeof(jfloat) == sizeof(cl_float))) {

    jsize datalen = (*env)->GetArrayLength(env, rf);        // get number of floats in data array
    jsize blen = (*env)->GetArrayLength(env, b);            // get number of data items

    if (features * blen == datalen) {                   // must match

                                  // pin data items and get reference
      jfloat *condata = (*env)->GetFloatArrayElements(env, rf, NULL);

      if (condata != NULL) {                         // error?

                                // allocate array for cluster numbers
        cl_ushort* conb = (cl_ushort*) malloc(sizeof(cl_ushort) * blen);

        if (conb != NULL) {                   // malloc error?

          cl_uint numplatf;                  // holds number of platforms
          cl_int err = clGetPlatformIDs(0, NULL, &numplatf);
                                       // get number of platforms

          if (err == CL_SUCCESS) {                // error?

            if (numplatf >= 1) {               // there must be at least one

              cl_platform_id platf;               // holds platform info

                                                    // get platform info
              err = clGetPlatformIDs(1, &platf, NULL);

              if (err == CL_SUCCESS) {                // error?

                cl_uint numdevices;             // holds number of devices for platform #1

                                      //  get device info
                err = clGetDeviceIDs(platf, CL_DEVICE_TYPE_ALL, 0, NULL, &numdevices);

                if (err == CL_SUCCESS) {            // error?

                  if (numdevices >= 1) {          // at least one device?

                    cl_device_id dev;           // stores device info

                                                         // get device info for device #1
                    err = clGetDeviceIDs(platf, CL_DEVICE_TYPE_ALL, 1, &dev, NULL);

                    if (err == CL_SUCCESS) {               // error?


                                    // create a context with the specified
                                       // platform and device
                      cl_context context = clCreateContext(NULL, 1, &dev, NULL, NULL, &err);

                      if (context != NULL) {               // error?

                        cl_command_queue commands;        // compute command queue

                                                            // Create a command queue
                        commands = clCreateCommandQueue(context, dev, 0, &err);

                        if (commands != NULL) {                    // error?

                                                                   // create program
                          cl_program program = clCreateProgramWithSource(context, 1, &clsource,
                                                                         NULL, &err);

                          if (program != NULL) {               // error?

                                                               // build program
                            err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

                            if (err == CL_SUCCESS) {                  // error?

                              cl_kernel kernel_testdistance1;        // holds first kernel

                                                                       // extract kernel #1
                              kernel_testdistance1 = clCreateKernel(program, "testdistance1",
                                                                    &err);

                              if (err == CL_SUCCESS) {               // error?

                                cl_kernel kernel_testdistance2;    // holds kernel #2

                                                                       // extract kernel #2
                                kernel_testdistance2 = clCreateKernel(program, "testdistance2",
                                                                      &err);

                                if (err == CL_SUCCESS) {                // error?


                                                    // create first buffer (for data items)
                                  cl_mem data_g = clCreateBuffer(context,
                                                                 CL_MEM_READ_ONLY |
                                                                 CL_MEM_USE_HOST_PTR,
                                                                 sizeof(cl_float) * datalen,
                                                                 condata, &err);

                                  if (data_g != NULL) {                 // error?

                                                  // create second buffer (for cluster numbers)
                                    cl_mem b_g = clCreateBuffer(context,
                                                                CL_MEM_READ_WRITE |
                                                                CL_MEM_USE_HOST_PTR,
                                                                sizeof(cl_ushort) * blen, conb,
                                                                &err);

                                    if (b_g != NULL) {              // error?

                                                  // call dbscan
                                      ret = dbscan_gpu(conb, (cl_float *) condata, blen, eps, kk,
                                                       features,
                                                       commands, program, dev,
                                                       kernel_testdistance1, kernel_testdistance2,
                                                       data_g, b_g, &start2, &finish2);

                                      if (ret >= 0) {          // error?
                                                       // no -> delete first three bits
                                        for (int i1 = 0; i1 < blen; i1++) {
                                          conb[i1] = conb[i1] >> 3;
                                        }

                                                         // copy results
                                        (*env)->SetShortArrayRegion(env, b, 0, blen,
                                                                    (jshort *) conb);
                                      }

                                      clReleaseMemObject(b_g);        // release buffer

                                    } else {
                                      ret = -121;
                                    }

                                    clReleaseMemObject(data_g);          // release buffer

                                  } else {
                                    ret = -120;
                                  }

                                  clReleaseKernel(kernel_testdistance2);    // release kernel

                                } else {
                                  ret = -119;
                                }

                                clReleaseKernel(kernel_testdistance1);    // release kernel

                              } else {
                                ret = -118;
                              }

                            } else {
                              ret = -117;
                            }

                            clReleaseProgram(program);            // release program

                          } else {
                            ret = -116;

                          }

                          clReleaseCommandQueue(commands);         // release command queue

                        } else {
                          ret = -115;
                        }

                        clReleaseContext(context);                // release context

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

          free(conb);              // free cluster number buffer

        } else {
          ret = -105;
        }

        (*env)->ReleaseFloatArrayElements( env, rf, condata, JNI_ABORT );
                         // unpin data item array

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
               // calculate time elapsed
  long long int elapsed2 = ((long long int) (finish2.tv_sec - start2.tv_sec)) * 1000000000L;
  elapsed2 += (finish2.tv_nsec - start2.tv_nsec);

            // pin array, store time and unpin array
  jlong *edata = (*env)->GetLongArrayElements(env, e, NULL);
  edata[0] = elapsed2;
  (*env)->ReleaseLongArrayElements(env, e, edata, 0);
#endif


  return (ret);
}  // Java_com_example_dmocl_dbscan_dbscan_1c_1gpu



/*!
 \brief Parameters for the DBSCAN thread
 \details
This struct holds the parameters for each DBSCAN thread. (in) means parameters that are NOT changed
by the thread but may be changed by the method that submits the job. (const) means that the value is
never changed after setup. (out) attributes are changed by the threads.
This struct is used by two threads (dbscanthread1 and dbscanthread2).
This struct may be used only by one of these two threads. The caller has to take care that this
struct is not used by both threads at the same time.
Nevertheless, all (const) attributes are readonly after initialization and may be read at any time.
 */
struct dbscan_pt {

  unsigned short int status;         //!< (const) status (needed only for setup and destruction)

  int num;                            //!< (const) number of thread
  unsigned short *b;               //!< (out) number of closest cluster center
  float *data;                      //!< (const) input data
  int blen;                         //!< (const) number of data items
  float eps;                        //!< (const) radius
  int kk;                           //!< (const) number of neighbours
  int features;                      //!< (const) number of features per data item
  int start;                        //!< (const) first data item
  int len;                             //!< (const) last data item

  pthread_t thread1;                 //!< (const) holds the thread reference for the main loop thread
  pthread_t thread2;             //!< (const) holds the thread reference for the cluster expand thread
  sem_t sem1;                    //!< (in) semaphore to wait on (main loop thread)
  sem_t semret1;                 //!< (in) results ready sempahore (main loop thread)
  sem_t sem2;                   //!< (in) semaphore to wait on (cluster expand thread)
  sem_t semret2;                    //!< (in) results ready sempahore (cluster expand thread)

                             // variables for the main loop thread
  pthread_mutex_t MUTEX_var1;             //!< (in) lock the access to cmpto1 and itemcounter1
  volatile int cmpto1;                    //!< (in) data item number to which to compare all others
  volatile int itemcounter1;              //!< (out) number of itmes within radius

  pthread_mutex_t MUTEX_var2;             //!< (in) lock the access to cmpto2 and itemcounter2
  volatile int cmpto2;                    //!< (in) data item number to which to compare all others
  volatile int itemcounter2;              //!< (out) number of itmes within radius

  pthread_mutex_t MUTEX_fertig1;          //!< (in) locks the access to fertig1
  volatile unsigned char fertig1;         //!< (in) 1=terminate thread

  pthread_mutex_t MUTEX_fertig2;          //!< (in) locks the access to fertig2
  volatile unsigned char fertig2;         //!< (in) 1=terminate thread

};   // dbscan_pt




/*!
 \brief Distance test for cluster expansion
 \details
 This method is designed to test in parallel the distance to a given point and mark the
 data items that are within a defined radius. This method is called from the main DBSCAN loop.
 The workload is parallelized over the data items. The first and
 the last data item to used are defined in the struct passed as  argument.
 \param arg (in+out) a pointer to the parameter struct
 \returns NULL
 */
void* dbscanthread1(void* arg) {

                             // cast arguments
  struct dbscan_pt *f = (struct dbscan_pt *) arg;

  int weiter = 0;                            // loop condition
  float epseps = f->eps * f->eps;              // square radius

  while (weiter == 0) {                      // loop until externally aborted

    sem_wait(&f->sem1);                        // wait on data

    unsigned char temp;                          // test if finished
    pthread_mutex_lock(&f->MUTEX_fertig1);
    temp = f->fertig1;
    pthread_mutex_unlock(&f->MUTEX_fertig1);

    if (temp == 0) {                                 // do abort?

      pthread_mutex_lock(&f->MUTEX_var1);          // lock access to struct

      int itemcounter = 0;                           // counts the number of data items inside radius

                                        // iterate over data items assigned
      for (int i2 = f->start; i2 < f->start + f->len; i2++) {


        f->b[i2] &= MAXVALUE - 6;              // clear bits 2 and 3

        float s = 0;                             // holds the euclidean distance

                                            // calculate distance
        for (int i3 = 0; i3 < f->features; i3++) {
          s += powf(f->data[i2 * f->features + i3] - f->data[f->cmpto1 * f->features + i3], 2);
        }

        if (s <= epseps) {                                // inside radius

          f->b[i2] |= 2;                          // set distance bit
          itemcounter++;                              // step counter
        }
      }

      f->itemcounter1 = itemcounter;                   // save number of items inside radius

      pthread_mutex_unlock(&f->MUTEX_var1);    // unlock access to struct

      sem_post(&f->semret1);                         // signal results ready
    } else {
      weiter = 1;                            // exit loop and terminate thread
    }
  }

  return (NULL);

}  // dbscanthread1




/*!
 \brief Distance test for cluster expansion
 \details
 This method is designed to test in parallel the distance to a given point and mark the
 data items that are within a defined radius. This method is called during the
 cluster expansion. The workload is parallelized over the data items. The first and
 the last data item to used are defined in the struct passed as  argument.
 \param arg (in+out) a pointer to the parameter struct
 \returns NULL
 */
void* dbscanthread2(void* arg) {

  struct dbscan_pt *f = (struct dbscan_pt *) arg;    // cast arguments

  int weiter = 0;                        // loop condition variable
  float epseps = f->eps * f->eps;            // calculate square radius

  while (weiter == 0) {               // continue until externally aborted

    sem_wait(&f->sem2);                      // wait on signal

    unsigned char temp;                        // check if thread should terminate
    pthread_mutex_lock(&f->MUTEX_fertig2);
    temp = f->fertig2;
    pthread_mutex_unlock(&f->MUTEX_fertig2);

    if (temp == 0) {                       // do abort?

      pthread_mutex_lock(&f->MUTEX_var2);             // lock the variables of the struct

      int itemcounter = 0;                     // items that are inside radius

                                    // iterate over all assigned items
      for (int i2 = f->start; i2 < f->start + f->len; i2++) {

        f->b[i2] &= MAXVALUE - 4;     // clear bit 4

        float s = 0;              // holds eulidean distance

        for (int i3 = 0; i3 < f->features; i3++) {
          s += powf(f->data[i2 * f->features + i3] - f->data[f->cmpto2 * f->features + i3], 2);
        }

        if (s <= epseps) {                // inside radius?

          f->b[i2] |= 4;                  // set distance bit
          itemcounter++;                 // step counter
        }
      }

      f->itemcounter2 = itemcounter;      // save result

      pthread_mutex_unlock(&f->MUTEX_var2);         // unlock struct


      sem_post(&f->semret2);             // signal that results are ready

    } else {
      weiter = 1;                          // abort
    }
  }

  return (NULL);

}  // dbscanthread2




/*!
 \brief Expands a cluster found by the main loop of the DBSCAN algorithm (multithreaded)
 \details
 Expands a cluster found by the main loop of the DBSCAN algorithm (multithreaded)
 \param key (in) number of data item that is the current seed
 \param clusternumber (in) number of current cluster
 \param b (out) Cluster number + status bits (Bit 0: data item classified, Bit 1: distance
    reachable from main loop, Bit 2: distance reachable from cluster expansion
 \param data (in) input data
 \param epseps (in) square search radius
 \param kk (in) number of neighbours
 \param datalen (in) number of data items (blen*features = number of floats in 'data')
 \param features (in) number of features
 \param cores (in) number of threads to be used (CPU may be oversubscribed)
 \param dbthreads (in) Array of structs (of length 'cores') that holds the thread infos
 \returns 0=OK, <0 error (premature abort)
 */
short expandCluster_pthreads(int key, const short clusternumber,
                             unsigned short *b, const float *data, const float epseps, const int kk,
                             const int datalen, const int features,
                             const int cores, struct dbscan_pt *dbthreads) {

  short ret = 0;              // return value

  b[key] &= 7;            // clear bits 3-15
  b[key] |= (clusternumber << 3);     // save current cluster number


  int weiter = 0;                       // loop break condition


  while (weiter == 0) {    // iterate until no more new data items for cluster are found

    weiter = 1;                    // assume that there are no new data items

                        // check if calculations should be aborted
    rwlockwp_reader_acquire(&abortcalc);
    if (doabort > 0) {
      weiter = 2;
      ret = -1;
    }
    rwlockwp_reader_release(&abortcalc);

    if (weiter == 1) {                    // abort?
                                    // iterate over data items
      for (int i1 = 0; i1 < datalen; i1++) {


        if ((b[i1] & 3) == 2) {             // has already been classified as cluster member?

          b[i1] |= 1;                     // set visited bit


                                    // search for new cluster members
          for (int i2 = 0; i2 < cores; i2++) {
            pthread_mutex_lock(&dbthreads[i2].MUTEX_var2);
            dbthreads[i2].cmpto2 = i1;
            pthread_mutex_unlock(&dbthreads[i2].MUTEX_var2);
            sem_post(&dbthreads[i2].sem2);
          }

          int itemcounter2 = 0;            // total item counter

          for (int i2 = 0; i2 < cores; i2++) {
            sem_wait(&dbthreads[i2].semret2);      // wait for threads to finish
            pthread_mutex_lock(&dbthreads[i2].MUTEX_var2);
            itemcounter2 += dbthreads[i2].itemcounter2;         // add up items found
            pthread_mutex_unlock(&dbthreads[i2].MUTEX_var2);
          }


          if (itemcounter2 >= kk) {               // enough items?

                        // mark all found items as cluster items
            for (int i2 = 0; i2 < datalen; i2++) {

              if ((b[i2] & 6) == 4) {          // not yet cluster member?

                b[i2] |= 2;         // make cluster member
                weiter = 0;            // new members found -> loop must continue
              }
            }
          }

                                 // set cluster number if not yet set
          if ((b[i1] >> 3) == 0) {
            b[i1] |= (clusternumber << 3);
          }

        }
      }

      if (ret < 0) {              // abort?
        break;
      }
    }
  }

  return (ret);
} // expandCluster_pthreads




/*!
 \brief Performs a DBSCAN search on the CPU (multithreaded)
 \details
 This method searches clusters with the DBSCAN method on the CPU with multiple threads
 \param b (out) Cluster number + status bits (Bit 0: data item classified, Bit 1: distance
    reachable from main loop, Bit 2: distance reachable from cluster expansion
 \param data (in) input data
 \param blen (in) number of data items (blen*features = number of floats in 'data')
 \param eps (in) search radius
 \param kk (in) number of neighbours
 \param features (in) number of features
 \param cores (in) number of threads to be used (CPU may be oversubscribed)
 \param dbthreads (in) Array of structs (of length 'cores') that holds the thread infos
 \returns >0 number of clusters found (0=only noise points), -1=permature abort,
 -256=too many clusters (number can not be stored with 12 bits)
 \mt fully threadsafe
 */
short dbscan_pthreads(unsigned short *b, const float *data, const int blen, const float eps, const int kk,
                const int features,
                const int cores, struct dbscan_pt *dbthreads) {

  short clusternumber = 0;                // initial cluster number

  const float epseps = eps * eps;         // get square of radius

  for (int i1 = 0; i1 < blen; i1++) {
    b[i1] = 0;                            // initialize cluster number array
  }

                             // iterate over data points
  for (int i1 = 0; i1 < blen; i1++) {

    if ((b[i1] & 1) == 0) {                           // visited?

                                  // check if calculations should be aborted
      rwlockwp_reader_acquire(&abortcalc);
      if (doabort > 0) {
        clusternumber = -1;
      }
      rwlockwp_reader_release(&abortcalc);

      if (clusternumber < 0) {     // abort?
        break;
      }

      b[i1] |= 1;                      // set visited-bit

                        // start main loop threads
      for (int i2 = 0; i2 < cores; i2++) {
        pthread_mutex_lock(&dbthreads[i2].MUTEX_var1);
        dbthreads[i2].cmpto1 = i1;
        pthread_mutex_unlock(&dbthreads[i2].MUTEX_var1);
        sem_post(&dbthreads[i2].sem1);
      }

      int itemcounter = 0;         // total item count

                    // iterate over threads
      for (int i2 = 0; i2 < cores; i2++) {
        sem_wait(&dbthreads[i2].semret1);     // wait until finished
        pthread_mutex_lock(&dbthreads[i2].MUTEX_var1);
        itemcounter += dbthreads[i2].itemcounter1;    // add items clustered
        pthread_mutex_unlock(&dbthreads[i2].MUTEX_var1);
      }


      if (itemcounter < kk) {             // not enough found?

        b[i1] &= 7;                       // Cluster Nr. 0 = Noise

      } else {

        if (clusternumber == 4095) {            // enough free numbers?
          clusternumber = -256;                  // no -> error
          break;                                  // and quit
        }

        clusternumber += 1;                   // step cluster number

                                     // expand cluster
        short ret2 = expandCluster_pthreads(i1, clusternumber, b, data, epseps, kk, blen, features,
                                            cores, dbthreads);

        if (ret2 < 0) {                        // error during expand cluster?
          clusternumber = ret2;               // yes -> quit
          break;
        }
      }
    }
  }

  return (clusternumber);
} // dbscan_pthreads


// see header file
JNIEXPORT jshort JNICALL
Java_com_example_dmocl_dbscan_dbscan_1c_1phtreads
  (JNIEnv *env, jclass jc, jshortArray b, jfloatArray rf, jfloat eps, jint kk, jint features,
   jint cores, jlongArray e) {

  short ret = -1;            // return value

  struct timespec start2, finish2;          // time points

                            // check architecture
  if ((sizeof(jshort) == sizeof(unsigned short)) && (sizeof(jfloat) == sizeof(float))) {

                          // get number of data array entries
    jsize datalen = (*env)->GetArrayLength(env, rf);
    jsize blen = (*env)->GetArrayLength(env, b);   // get number of data items

    if (features * blen == datalen) {           // must match

                                       // access and pin data items
      jfloat *condata = (*env)->GetFloatArrayElements(env, rf, NULL);

      if (condata != NULL) {             // error?

                               // allocate memory for cluster numbers
        unsigned short *conb = (unsigned short *) malloc(sizeof(unsigned short) * blen);

        if (conb != NULL) {              // malloc error?

                                //alloc memory for threads
          struct dbscan_pt *dbthreads = (struct dbscan_pt *) malloc(
            sizeof(struct dbscan_pt) * cores);

          if (dbthreads != NULL) {               // malloc error

            int initsucc = 0;             // initialization successfull?

            int stepper = (blen / cores) + 1;    // approx. data items per core
            int starter = 0;                    // start data item
            int reminder = blen;                 // remaining data items

            for (int i1 = 0; i1 < cores; i1++) {    // iterate over threads

              dbthreads[i1].status = 0;           // init status

              if (reminder < stepper) {      // only few data items remaining
                stepper = reminder;
              }

              if (initsucc == 0) {             // initialization so far successfull?

                dbthreads[i1].num = i1;                 // thread number
                dbthreads[i1].b = (unsigned short *) conb;   // reference to cluster number array
                dbthreads[i1].data = condata;             // reference to data array
                dbthreads[i1].blen = blen;               // number of data items
                dbthreads[i1].eps = eps;                  // search radius
                dbthreads[i1].kk = kk;                   // neighbours
                dbthreads[i1].features = features;        // feature count
                dbthreads[i1].start = starter;           // start data item
                dbthreads[i1].len = stepper;              // number of data items

                dbthreads[i1].fertig1 = 0;               // abort for first thread set
                dbthreads[i1].fertig2 = 0;               // abort for second thread set
                dbthreads[i1].itemcounter1 = 0;           // item counter for main loop
                dbthreads[i1].itemcounter2 = 0;           // item counter for expand cluster
                dbthreads[i1].cmpto1 = -1;               // item to compare to (main loop)
                dbthreads[i1].cmpto2 = -1;               // item to compare to (expand cluster)

                                          // try to initialize the main loop threads
                if (pthread_mutex_init(&dbthreads[i1].MUTEX_var1, NULL) == 0) {
                  dbthreads[i1].status |= 1;              // OK->set status
                } else {
                  initsucc = 1;                            // error
                }

                                // try to initialize the expand cluster threads
                if (pthread_mutex_init(&dbthreads[i1].MUTEX_var2, NULL) == 0) {
                  dbthreads[i1].status |= 2;            // OK -> set status
                } else {
                  initsucc = 1;                        // error
                }

                                   // initialize mutex for main loop
                if (pthread_mutex_init(&dbthreads[i1].MUTEX_fertig1, NULL) == 0) {
                  dbthreads[i1].status |= 4;
                } else {
                  initsucc = 1;
                }

                                  // initialize mutex for expand cluster
                if (pthread_mutex_init(&dbthreads[i1].MUTEX_fertig2, NULL) == 0) {
                  dbthreads[i1].status |= 8;
                } else {
                  initsucc = 1;
                }

                                  // initialize wake up semaphore for main loop
                if (sem_init(&dbthreads[i1].sem1, 0, 0) == 0) {
                  dbthreads[i1].status |= 16;
                } else {
                  initsucc = 1;
                }

                                 // initialize finished semaphore for main loop
                if (sem_init(&dbthreads[i1].semret1, 0, 0) == 0) {
                  dbthreads[i1].status |= 32;
                } else {
                  initsucc = 1;
                }

                                   // initialize wake up semaphore for expand cluster
                if (sem_init(&dbthreads[i1].sem2, 0, 0) == 0) {
                  dbthreads[i1].status |= 64;
                } else {
                  initsucc = 1;
                }

                                       // initialize finished semaphore for expand cluster
                if (sem_init(&dbthreads[i1].semret2, 0, 0) == 0) {
                  dbthreads[i1].status |= 128;
                } else {
                  initsucc = 1;
                }


                            // all inits so far OK?
                if ((dbthreads[i1].status & 255) == 255) {

                                  // create main loop threads
                  if (pthread_create(&(dbthreads[i1].thread1), NULL, &dbscanthread1,
                                     &dbthreads[i1]) == 0) {
                    dbthreads[i1].status |= 256;   // set status
                  } else {
                    initsucc = 1;              // error
                  }
                }

                         // all inits so far OK?
                if ((dbthreads[i1].status & 511) == 511) {

                               // create expand cluster threads
                  if (pthread_create(&(dbthreads[i1].thread2), NULL, &dbscanthread2,
                                     &dbthreads[i1]) == 0) {
                    dbthreads[i1].status |= 512;
                  } else {
                    initsucc = 1;
                  }
                }

              }

              starter += stepper;              // next data items
              reminder -= stepper;           // remaining data items
            }

            if (initsucc == 0) {               // init OK?

#ifdef GPUTIMING
                                              // get time
              clock_gettime(CLOCK_REALTIME, &start2);
#endif
                                      // call DBSCAN
              ret = dbscan_pthreads(conb, (float *) condata, blen, eps, kk, features,
                                    cores, dbthreads);

              if (ret >= 0) {               // error?
                for (int i1 = 0; i1 < blen; i1++) {        // delete first three bits
                  conb[i1] = (conb[i1] >> 3);
                }
                                        // copy results
                (*env)->SetShortArrayRegion(env, b, 0, blen, (jshort *) conb);
              }

#ifdef GPUTIMING
                                      // call DBSCAN
              clock_gettime(CLOCK_REALTIME, &finish2);
#endif

            } else {
              ret = -108;
            }
                          // iterate over threads
            for (int i1 = 0; i1 < cores; i1++) {

                           // destroy threads, mutexes, semaphores etc.
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

            free(dbthreads);            // clean up and free

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
                             // calculate time elapsed
  long long int elapsed2 = ((long long int) (finish2.tv_sec - start2.tv_sec)) * 1000000000L;
  elapsed2 += (finish2.tv_nsec - start2.tv_nsec);

              // pin array
  jlong *edata = (*env)->GetLongArrayElements(env, e, NULL);
  edata[0] = elapsed2;          // set first array element
  (*env)->ReleaseLongArrayElements(env, e, edata, 0);   // unpin array
#endif

  return (ret);
} // Java_com_example_dmocl_dbscan_dbscan_1c_1phtreads




// see header file
JNIEXPORT void JNICALL
Java_com_example_dmocl_dbscan_dbscanabort_1c(JNIEnv *env, jclass clazz) {

  rwlockwp_writer_acquire(&abortcalc);
  doabort = 1;                      // signal abort
  rwlockwp_writer_release(&abortcalc);

}

// see header file
JNIEXPORT void JNICALL
Java_com_example_dmocl_dbscan_dbscanresume_1c(JNIEnv *env, jclass clazz) {
  rwlockwp_writer_acquire(&abortcalc);
  doabort = 0;                      // allow calculations
  rwlockwp_writer_release(&abortcalc);
}

