/*!
 \file OpenCL.c
 \brief A this OpenCL wrapper for the libOpenCL.so shared library on the Android device.
 \details
 This library acts as a small wrapper for the native OpenCL library on an Android device.
 The library on the device usually is not present at compile time and if it would, it would
 be added to the apk file at compile time. The app would run ONLY on this type of device
 (e.g., a Mali GPU) but could not be ported to other Android devices.
 Therefore this wrapper loads the OpenCL library and all necessary
 symbols at runtime.
 This library supports OpenCL 3.0. If your device does not support this version, either
 rebuild this library with the version appropriate for your device or simply do not call
 methods not supported on your device. If you do so, a runtime error will occur as the necessary
 symbol will not be found in the library.
 \copyright Copyright Robert Fritze 2020
 \version 1.0
 \author Robert Fritze
 \date 4.5.2020
 \license This program is released under the MIT License.
 */


#include "AndroidOpenCL.h"

#ifndef CL_TARGET_OPENCL_VERSION
#define CL_TARGET_OPENCL_VERSION 120
#endif


#include <CL/opencl.h>
#include <CL/cl_icd.h>
#include <dlfcn.h>
#include <string.h>
#include <pthread.h>



#define SAVECHECKER(a)                          \
      pthread_mutex_lock( &dllock );            \
                                                \
      if (dlcall == NULL){                      \
        weiter = 1;                             \
      }                                         \
                                                \
      if ((weiter == 0) && (cl_wrap_call.a==NULL)) {   \
        cl_wrap_call.a = (cl_api_##a) dlsym(dlcall, #a ); \
                                                \
        if (cl_wrap_call.a == NULL) {           \
          weiter = 1;                           \
        }                                       \
      }                                         \
                                                \
      pthread_mutex_unlock( &dllock );          \



#define WRAPPERCLFUNCT( a, b, c )               \
                                                \
    pthread_rwlock_rdlock( &lock );             \
                                                \
    int weiter = 0;                             \
                                                \
    SAVECHECKER( a )                            \
                                                \
    if (weiter == 0) {                          \
      ret = cl_wrap_call.a b;                   \
    }                                           \
    else {                                      \
      ret = c;                                  \
    }                                           \
                                                \
    pthread_rwlock_unlock( &lock );             \
                                                \
    return( ret );                              \



/*!
 * @brief Holds the reference to the loaded library *libOpenCL.so*
 * @access R+W
 * @locks Use *pthread_mutex_t* **dllock** to access
 */
void* dlcall = NULL;

/*!
 * @brief Holds the path to the OpenCL library on the device
 * @details If the path is longer than 1024 characters, the path is truncated (resulting in
 * an error most probably as the OpenCL library will not be found)
 * @access Write once and read only afterwards
 * @locks no locks required (can be written only during first call to *loadOpenCL* and
 * cannot be read before that call; after the call to *loadOpenCL* this variable is
 * read-only)
 */
char liboclpath[1024] = "";


/*!
 * @brief A R/W lock for the mutual exclusion of the library unload mechanism.
 * @details This lock provides the mechanism to exclude to OpenCL library unload mechanism
 * while the library is beeing loaded or some OpenCL function is executed. Arbitrary many
 * methods can acquire the reader lock (including loadOpenCL) but only *unloadOpenCL*
 * acquires the writer lock (resulting in an exclusive access to the entire library).
 * @warning If this lock AND **dllock** are acquired together, this lock has to be acquired first!
 */
pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;

/*!
 * @brief A mutex for the mechanism that loads the library and some attributes
 * @details This mutex guarantees exclusive access to the library load mechanism and
 * the attributes *loadChecker* and *dlcall*
 * @warning If this lock AND **lock** are acquired together, this lock has to be acquired first!
 */
pthread_mutex_t dllock = PTHREAD_MUTEX_INITIALIZER;



const cl_icd_dispatch CL_WRAP_CALL_ZERO = {

  /* OpenCL 1.0 */
  .clGetPlatformIDs = NULL,
  .clGetPlatformInfo = NULL,
  .clGetDeviceIDs = NULL,
  .clGetDeviceInfo = NULL,
  .clCreateContext = NULL,
  .clCreateContextFromType = NULL,
  .clRetainContext = NULL,
  .clReleaseContext = NULL,
  .clGetContextInfo = NULL,
  .clCreateCommandQueue = NULL,
  .clRetainCommandQueue = NULL,
  .clReleaseCommandQueue = NULL,
  .clGetCommandQueueInfo = NULL,
  .clSetCommandQueueProperty = NULL,
  .clCreateBuffer = NULL,
  .clCreateImage2D = NULL,
  .clCreateImage3D = NULL,
  .clRetainMemObject = NULL,
  .clReleaseMemObject  = NULL,
  .clGetSupportedImageFormats = NULL,
  .clGetMemObjectInfo = NULL,
  .clGetImageInfo = NULL,
  .clCreateSampler = NULL,
  .clRetainSampler = NULL,
  .clReleaseSampler = NULL,
  .clGetSamplerInfo = NULL,
  .clCreateProgramWithSource = NULL,
  .clCreateProgramWithBinary = NULL,
  .clRetainProgram = NULL,
  .clReleaseProgram = NULL,
  .clBuildProgram = NULL,
  .clUnloadCompiler = NULL,
  .clGetProgramInfo = NULL,
  .clGetProgramBuildInfo = NULL,
  .clCreateKernel = NULL,
  .clCreateKernelsInProgram = NULL,
  .clRetainKernel = NULL,
  .clReleaseKernel = NULL,
  .clSetKernelArg = NULL,
  .clGetKernelInfo = NULL,
  .clGetKernelWorkGroupInfo = NULL,
  .clWaitForEvents = NULL,
  .clGetEventInfo = NULL,
  .clRetainEvent = NULL,
  .clReleaseEvent = NULL,
  .clGetEventProfilingInfo = NULL,
  .clFlush = NULL,
  .clFinish = NULL,
  .clEnqueueReadBuffer = NULL,
  .clEnqueueWriteBuffer = NULL,
  .clEnqueueCopyBuffer = NULL,
  .clEnqueueReadImage = NULL,
  .clEnqueueWriteImage = NULL,
  .clEnqueueCopyImage = NULL,
  .clEnqueueCopyImageToBuffer = NULL,
  .clEnqueueCopyBufferToImage = NULL,
  .clEnqueueMapBuffer = NULL,
  .clEnqueueMapImage = NULL,
  .clEnqueueUnmapMemObject = NULL,
  .clEnqueueNDRangeKernel = NULL,
  .clEnqueueTask = NULL,
  .clEnqueueNativeKernel = NULL,
  .clEnqueueMarker = NULL,
  .clEnqueueWaitForEvents = NULL,
  .clEnqueueBarrier = NULL,
  .clGetExtensionFunctionAddress = NULL,
  .clCreateFromGLBuffer = NULL,
  .clCreateFromGLTexture2D = NULL,
  .clCreateFromGLTexture3D = NULL,
  .clCreateFromGLRenderbuffer = NULL,
  .clGetGLObjectInfo = NULL,
  .clGetGLTextureInfo = NULL,
  .clEnqueueAcquireGLObjects = NULL,
  .clEnqueueReleaseGLObjects = NULL,
  .clGetGLContextInfoKHR = NULL,

  /* cl_khr_d3d10_sharing */
  .clGetDeviceIDsFromD3D10KHR = NULL,
  .clCreateFromD3D10BufferKHR = NULL,
  .clCreateFromD3D10Texture2DKHR = NULL,
  .clCreateFromD3D10Texture3DKHR = NULL,
  .clEnqueueAcquireD3D10ObjectsKHR = NULL,
  .clEnqueueReleaseD3D10ObjectsKHR = NULL,

  /* OpenCL 1.1 */
  .clSetEventCallback = NULL,
  .clCreateSubBuffer = NULL,
  .clSetMemObjectDestructorCallback = NULL,
  .clCreateUserEvent = NULL,
  .clSetUserEventStatus = NULL,
  .clEnqueueReadBufferRect = NULL,
  .clEnqueueWriteBufferRect = NULL,
  .clEnqueueCopyBufferRect = NULL,

  /* cl_ext_device_fission */
  .clCreateSubDevicesEXT = NULL,
  .clRetainDeviceEXT = NULL,
  .clReleaseDeviceEXT = NULL,

  /* cl_khr_gl_event */
  .clCreateEventFromGLsyncKHR = NULL,

  /* OpenCL 1.2 */
  .clCreateSubDevices = NULL,
  .clRetainDevice = NULL,
  .clReleaseDevice = NULL,
  .clCreateImage = NULL,
  .clCreateProgramWithBuiltInKernels = NULL,
  .clCompileProgram = NULL,
  .clLinkProgram = NULL,
  .clUnloadPlatformCompiler = NULL,
  .clGetKernelArgInfo = NULL,
  .clEnqueueFillBuffer = NULL,
  .clEnqueueFillImage = NULL,
  .clEnqueueMigrateMemObjects = NULL,
  .clEnqueueMarkerWithWaitList = NULL,
  .clEnqueueBarrierWithWaitList = NULL,
  .clGetExtensionFunctionAddressForPlatform = NULL,
  .clCreateFromGLTexture = NULL,

  /* cl_khr_d3d11_sharing */
  .clGetDeviceIDsFromD3D11KHR = NULL,
  .clCreateFromD3D11BufferKHR = NULL,
  .clCreateFromD3D11Texture2DKHR = NULL,
  .clCreateFromD3D11Texture3DKHR = NULL,
  .clCreateFromDX9MediaSurfaceKHR = NULL,
  .clEnqueueAcquireD3D11ObjectsKHR = NULL,
  .clEnqueueReleaseD3D11ObjectsKHR = NULL,

  /* cl_khr_dx9_media_sharing */
  .clGetDeviceIDsFromDX9MediaAdapterKHR = NULL,
  .clEnqueueAcquireDX9MediaSurfacesKHR = NULL,
  .clEnqueueReleaseDX9MediaSurfacesKHR = NULL,

  /* cl_khr_egl_image */
  .clCreateFromEGLImageKHR = NULL,
  .clEnqueueAcquireEGLObjectsKHR = NULL,
  .clEnqueueReleaseEGLObjectsKHR = NULL,

  /* cl_khr_egl_event */
  .clCreateEventFromEGLSyncKHR = NULL,

  /* OpenCL 2.0 */
  .clCreateCommandQueueWithProperties = NULL,
  .clCreatePipe = NULL,
  .clGetPipeInfo = NULL,
  .clSVMAlloc = NULL,
  .clSVMFree = NULL,
  .clEnqueueSVMFree = NULL,
  .clEnqueueSVMMemcpy = NULL,
  .clEnqueueSVMMemFill = NULL,
  .clEnqueueSVMMap = NULL,
  .clEnqueueSVMUnmap = NULL,
  .clCreateSamplerWithProperties = NULL,
  .clSetKernelArgSVMPointer = NULL,
  .clSetKernelExecInfo = NULL,

  /* cl_khr_sub_groups */
  .clGetKernelSubGroupInfoKHR = NULL,

  /* OpenCL 2.1 */
  .clCloneKernel = NULL,
  .clCreateProgramWithIL = NULL,
  .clEnqueueSVMMigrateMem = NULL,
  .clGetDeviceAndHostTimer = NULL,
  .clGetHostTimer = NULL,
  .clGetKernelSubGroupInfo = NULL,
  .clSetDefaultDeviceCommandQueue = NULL,

  /* OpenCL 2.2 */
  .clSetProgramReleaseCallback = NULL,
  .clSetProgramSpecializationConstant = NULL,

  /* OpenCL 3.0 */
  .clCreateBufferWithProperties = NULL,
  .clCreateImageWithProperties = NULL

};


cl_icd_dispatch cl_wrap_call = {

/* OpenCL 1.0 */
.clGetPlatformIDs = NULL,
.clGetPlatformInfo = NULL,
.clGetDeviceIDs = NULL,
.clGetDeviceInfo = NULL,
.clCreateContext = NULL,
.clCreateContextFromType = NULL,
.clRetainContext = NULL,
.clReleaseContext = NULL,
.clGetContextInfo = NULL,
.clCreateCommandQueue = NULL,
.clRetainCommandQueue = NULL,
.clReleaseCommandQueue = NULL,
.clGetCommandQueueInfo = NULL,
.clSetCommandQueueProperty = NULL,
.clCreateBuffer = NULL,
.clCreateImage2D = NULL,
.clCreateImage3D = NULL,
.clRetainMemObject = NULL,
.clReleaseMemObject  = NULL,
.clGetSupportedImageFormats = NULL,
.clGetMemObjectInfo = NULL,
.clGetImageInfo = NULL,
.clCreateSampler = NULL,
.clRetainSampler = NULL,
.clReleaseSampler = NULL,
.clGetSamplerInfo = NULL,
.clCreateProgramWithSource = NULL,
.clCreateProgramWithBinary = NULL,
.clRetainProgram = NULL,
.clReleaseProgram = NULL,
.clBuildProgram = NULL,
.clUnloadCompiler = NULL,
.clGetProgramInfo = NULL,
.clGetProgramBuildInfo = NULL,
.clCreateKernel = NULL,
.clCreateKernelsInProgram = NULL,
.clRetainKernel = NULL,
.clReleaseKernel = NULL,
.clSetKernelArg = NULL,
.clGetKernelInfo = NULL,
.clGetKernelWorkGroupInfo = NULL,
.clWaitForEvents = NULL,
.clGetEventInfo = NULL,
.clRetainEvent = NULL,
.clReleaseEvent = NULL,
.clGetEventProfilingInfo = NULL,
.clFlush = NULL,
.clFinish = NULL,
.clEnqueueReadBuffer = NULL,
.clEnqueueWriteBuffer = NULL,
.clEnqueueCopyBuffer = NULL,
.clEnqueueReadImage = NULL,
.clEnqueueWriteImage = NULL,
.clEnqueueCopyImage = NULL,
.clEnqueueCopyImageToBuffer = NULL,
.clEnqueueCopyBufferToImage = NULL,
.clEnqueueMapBuffer = NULL,
.clEnqueueMapImage = NULL,
.clEnqueueUnmapMemObject = NULL,
.clEnqueueNDRangeKernel = NULL,
.clEnqueueTask = NULL,
.clEnqueueNativeKernel = NULL,
.clEnqueueMarker = NULL,
.clEnqueueWaitForEvents = NULL,
.clEnqueueBarrier = NULL,
.clGetExtensionFunctionAddress = NULL,
.clCreateFromGLBuffer = NULL,
.clCreateFromGLTexture2D = NULL,
.clCreateFromGLTexture3D = NULL,
.clCreateFromGLRenderbuffer = NULL,
.clGetGLObjectInfo = NULL,
.clGetGLTextureInfo = NULL,
.clEnqueueAcquireGLObjects = NULL,
.clEnqueueReleaseGLObjects = NULL,
.clGetGLContextInfoKHR = NULL,

/* cl_khr_d3d10_sharing */
.clGetDeviceIDsFromD3D10KHR = NULL,
.clCreateFromD3D10BufferKHR = NULL,
.clCreateFromD3D10Texture2DKHR = NULL,
.clCreateFromD3D10Texture3DKHR = NULL,
.clEnqueueAcquireD3D10ObjectsKHR = NULL,
.clEnqueueReleaseD3D10ObjectsKHR = NULL,

/* OpenCL 1.1 */
.clSetEventCallback = NULL,
.clCreateSubBuffer = NULL,
.clSetMemObjectDestructorCallback = NULL,
.clCreateUserEvent = NULL,
.clSetUserEventStatus = NULL,
.clEnqueueReadBufferRect = NULL,
.clEnqueueWriteBufferRect = NULL,
.clEnqueueCopyBufferRect = NULL,

/* cl_ext_device_fission */
.clCreateSubDevicesEXT = NULL,
.clRetainDeviceEXT = NULL,
.clReleaseDeviceEXT = NULL,

/* cl_khr_gl_event */
.clCreateEventFromGLsyncKHR = NULL,

/* OpenCL 1.2 */
.clCreateSubDevices = NULL,
.clRetainDevice = NULL,
.clReleaseDevice = NULL,
.clCreateImage = NULL,
.clCreateProgramWithBuiltInKernels = NULL,
.clCompileProgram = NULL,
.clLinkProgram = NULL,
.clUnloadPlatformCompiler = NULL,
.clGetKernelArgInfo = NULL,
.clEnqueueFillBuffer = NULL,
.clEnqueueFillImage = NULL,
.clEnqueueMigrateMemObjects = NULL,
.clEnqueueMarkerWithWaitList = NULL,
.clEnqueueBarrierWithWaitList = NULL,
.clGetExtensionFunctionAddressForPlatform = NULL,
.clCreateFromGLTexture = NULL,

/* cl_khr_d3d11_sharing */
.clGetDeviceIDsFromD3D11KHR = NULL,
.clCreateFromD3D11BufferKHR = NULL,
.clCreateFromD3D11Texture2DKHR = NULL,
.clCreateFromD3D11Texture3DKHR = NULL,
.clCreateFromDX9MediaSurfaceKHR = NULL,
.clEnqueueAcquireD3D11ObjectsKHR = NULL,
.clEnqueueReleaseD3D11ObjectsKHR = NULL,

/* cl_khr_dx9_media_sharing */
.clGetDeviceIDsFromDX9MediaAdapterKHR = NULL,
.clEnqueueAcquireDX9MediaSurfacesKHR = NULL,
.clEnqueueReleaseDX9MediaSurfacesKHR = NULL,

/* cl_khr_egl_image */
.clCreateFromEGLImageKHR = NULL,
.clEnqueueAcquireEGLObjectsKHR = NULL,
.clEnqueueReleaseEGLObjectsKHR = NULL,

/* cl_khr_egl_event */
.clCreateEventFromEGLSyncKHR = NULL,

/* OpenCL 2.0 */
.clCreateCommandQueueWithProperties = NULL,
.clCreatePipe = NULL,
.clGetPipeInfo = NULL,
.clSVMAlloc = NULL,
.clSVMFree = NULL,
.clEnqueueSVMFree = NULL,
.clEnqueueSVMMemcpy = NULL,
.clEnqueueSVMMemFill = NULL,
.clEnqueueSVMMap = NULL,
.clEnqueueSVMUnmap = NULL,
.clCreateSamplerWithProperties = NULL,
.clSetKernelArgSVMPointer = NULL,
.clSetKernelExecInfo = NULL,

/* cl_khr_sub_groups */
.clGetKernelSubGroupInfoKHR = NULL,

/* OpenCL 2.1 */
.clCloneKernel = NULL,
.clCreateProgramWithIL = NULL,
.clEnqueueSVMMigrateMem = NULL,
.clGetDeviceAndHostTimer = NULL,
.clGetHostTimer = NULL,
.clGetKernelSubGroupInfo = NULL,
.clSetDefaultDeviceCommandQueue = NULL,

/* OpenCL 2.2 */
.clSetProgramReleaseCallback = NULL,
.clSetProgramSpecializationConstant = NULL,

/* OpenCL 3.0 */
.clCreateBufferWithProperties = NULL,
.clCreateImageWithProperties = NULL

};








CL_API_ENTRY cl_int CL_API_CALL clGetPlatformIDs(
        cl_uint          num_entries,
        cl_platform_id*  platforms,
        cl_uint*         num_platforms
        ) CL_API_SUFFIX__VERSION_1_0{

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT( clGetPlatformIDs, (num_entries, platforms, num_platforms), CL_INVALID_VALUE )

}


CL_API_ENTRY cl_int CL_API_CALL clGetPlatformInfo(
        cl_platform_id   platform,
        cl_platform_info param_name,
        size_t           param_value_size,
        void*            param_value,
        size_t*          param_value_size_ret
        ) CL_API_SUFFIX__VERSION_1_0{

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT( clGetPlatformInfo, ( platform, param_name, param_value_size, \
                                     param_value, param_value_size_ret ), CL_INVALID_VALUE )

}


CL_API_ENTRY cl_int CL_API_CALL clGetDeviceIDs(
        cl_platform_id   platform,
        cl_device_type   device_type,
        cl_uint          num_entries,
        cl_device_id*    devices,
        cl_uint*         num_devices
        ) CL_API_SUFFIX__VERSION_1_0{

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT( clGetDeviceIDs, ( platform, device_type, num_entries, \
                                  devices, num_devices), CL_INVALID_VALUE )

}

CL_API_ENTRY cl_int CL_API_CALL clGetDeviceInfo(
        cl_device_id    device,
        cl_device_info  param_name,
        size_t          param_value_size,
        void*           param_value,
        size_t*         param_value_size_ret
        ) CL_API_SUFFIX__VERSION_1_0{


  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT( clGetDeviceInfo, ( device, param_name, param_value_size, \
                                   param_value, param_value_size_ret), CL_INVALID_VALUE )

}


#ifdef CL_VERSION_1_2

CL_API_ENTRY cl_int CL_API_CALL clCreateSubDevices(
        cl_device_id                         in_device,
        const cl_device_partition_property*  properties,
        cl_uint                              num_devices,
        cl_device_id*                        out_devices,
        cl_uint*                             num_devices_ret
        ) CL_API_SUFFIX__VERSION_1_2{

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT( clCreateSubDevices, ( in_device, properties, num_devices, \
                                      out_devices, num_devices_ret), CL_INVALID_VALUE );

 }



CL_API_ENTRY cl_int CL_API_CALL clRetainDevice(
        cl_device_id device
        ) CL_API_SUFFIX__VERSION_1_2{

    cl_int ret = CL_SUCCESS;
    WRAPPERCLFUNCT( clRetainDevice, ( device ), CL_INVALID_DEVICE )

}

CL_API_ENTRY cl_int CL_API_CALL clReleaseDevice(
        cl_device_id device
        ) CL_API_SUFFIX__VERSION_1_2{


    cl_int ret = CL_SUCCESS;
    WRAPPERCLFUNCT( clReleaseDevice, ( device ), CL_INVALID_DEVICE )

}

#endif


#ifdef CL_VERSION_2_1

CL_API_ENTRY cl_int CL_API_CALL clSetDefaultDeviceCommandQueue(
        cl_context           context,
        cl_device_id         device,
        cl_command_queue     command_queue
        ) CL_API_SUFFIX__VERSION_2_1{

    cl_int ret = CL_SUCCESS;
    WRAPPERCLFUNCT( clSetDefaultDeviceCommandQueue, ( context, device, command_queue ), \
         CL_OUT_OF_RESOURCES )

}

CL_API_ENTRY cl_int CL_API_CALL clGetDeviceAndHostTimer(
        cl_device_id    device,
        cl_ulong*       device_timestamp,
        cl_ulong*       host_timestamp
        ) CL_API_SUFFIX__VERSION_2_1{

    cl_int ret = CL_SUCCESS;
    WRAPPERCLFUNCT( clGetDeviceAndHostTimer, ( device, device_timestamp, host_timestamp ), \
       CL_INVALID_VALUE )

}



CL_API_ENTRY cl_int CL_API_CALL clGetHostTimer(
        cl_device_id device,
        cl_ulong*    host_timestamp
        ) CL_API_SUFFIX__VERSION_2_1{

    cl_int ret = CL_SUCCESS;
    WRAPPERCLFUNCT( clGetHostTimer, ( device, host_timestamp ), CL_INVALID_VALUE )

}

#endif





/* Context APIs */
CL_API_ENTRY cl_context CL_API_CALL clCreateContext(
        const cl_context_properties* properties,
        cl_uint                      num_devices,
        const cl_device_id*          devices,
        void (CL_CALLBACK * pfn_notify)(
                const char*  errinfo,
                const void*  private_info,
                size_t       cb,
                void*        user_data
                ),
        void*                user_data,
        cl_int*              errcode_ret
        ) CL_API_SUFFIX__VERSION_1_0 {

  cl_context ret = NULL;

  WRAPPERCLFUNCT( clCreateContext, ( properties, num_devices, devices, pfn_notify, \
                  user_data, errcode_ret ),  NULL )

}




CL_API_ENTRY cl_context CL_API_CALL clCreateContextFromType(
        const cl_context_properties*  properties,
        cl_device_type      device_type,
        void (CL_CALLBACK*  pfn_notify)(
                const char*  errinfo,
                const void*  private_info,
                size_t       cb,
                void*        user_data
                ),
        void*               user_data,
        cl_int*             errcode_ret
        ) CL_API_SUFFIX__VERSION_1_0 {


  cl_context ret = NULL;


  WRAPPERCLFUNCT( clCreateContextFromType, ( properties, device_type, pfn_notify, \
           user_data, errcode_ret ), NULL )

}



CL_API_ENTRY cl_int CL_API_CALL clRetainContext(
        cl_context context
        ) CL_API_SUFFIX__VERSION_1_0 {


  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT( clRetainContext, ( context ), CL_OUT_OF_RESOURCES )

}

CL_API_ENTRY cl_int CL_API_CALL clReleaseContext(
        cl_context context
        ) CL_API_SUFFIX__VERSION_1_0 {

    cl_int ret = CL_SUCCESS;
    WRAPPERCLFUNCT( clReleaseContext, ( context ), CL_INVALID_CONTEXT )

}

CL_API_ENTRY cl_int CL_API_CALL clGetContextInfo(
        cl_context         context,
        cl_context_info    param_name,
        size_t             param_value_size,
        void*              param_value,
        size_t*            param_value_size_ret
        ) CL_API_SUFFIX__VERSION_1_0 {

    cl_int ret = CL_SUCCESS;
    WRAPPERCLFUNCT( clGetContextInfo, ( context, param_name, param_value_size, \
             param_value, param_value_size_ret ), CL_INVALID_VALUE )

}



#ifdef CL_VERSION_2_0

CL_API_ENTRY cl_command_queue CL_API_CALL clCreateCommandQueueWithProperties(
  cl_context               context,
  cl_device_id             device,
  const cl_queue_properties *    properties,
  cl_int *                 errcode_ret
  ) CL_API_SUFFIX__VERSION_2_0 {

  cl_command_queue ret = NULL;
  WRAPPERCLFUNCT( clCreateCommandQueueWithProperties, ( context, device, \
    properties, errcode_ret ), NULL )

}

#endif


CL_API_ENTRY cl_int CL_API_CALL
clRetainCommandQueue(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0 {

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT( clRetainCommandQueue, ( command_queue ), CL_INVALID_COMMAND_QUEUE )

}


CL_API_ENTRY cl_int CL_API_CALL
clReleaseCommandQueue(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0 {

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT( clReleaseCommandQueue, ( command_queue ), CL_INVALID_COMMAND_QUEUE )

};


CL_API_ENTRY cl_int CL_API_CALL
clGetCommandQueueInfo(cl_command_queue      command_queue,
                      cl_command_queue_info param_name,
                      size_t                param_value_size,
                      void *                param_value,
                      size_t *              param_value_size_ret) CL_API_SUFFIX__VERSION_1_0 {

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT( clGetCommandQueueInfo, ( command_queue, \
  param_name, param_value_size, param_value, param_value_size_ret), CL_INVALID_COMMAND_QUEUE )

}


CL_API_ENTRY cl_mem CL_API_CALL
clCreateBuffer(cl_context   context,
               cl_mem_flags flags,
               size_t       size,
               void *       host_ptr,
               cl_int *     errcode_ret) CL_API_SUFFIX__VERSION_1_0 {

  cl_mem ret = NULL;
  WRAPPERCLFUNCT( clCreateBuffer, ( context, \
    flags, size, host_ptr, errcode_ret), NULL )


}

#ifdef CL_VERSION_1_1

CL_API_ENTRY cl_mem CL_API_CALL
clCreateSubBuffer(cl_mem                   buffer,
                  cl_mem_flags             flags,
                  cl_buffer_create_type    buffer_create_type,
                  const void *             buffer_create_info,
                  cl_int *                 errcode_ret) CL_API_SUFFIX__VERSION_1_1 {

  cl_mem ret = NULL;
  WRAPPERCLFUNCT( clCreateSubBuffer, (buffer, flags, \
    buffer_create_type, buffer_create_info, errcode_ret ), NULL )


}

#endif


#ifdef CL_VERSION_1_2

CL_API_ENTRY cl_mem CL_API_CALL
clCreateImage(cl_context              context,
              cl_mem_flags            flags,
              const cl_image_format * image_format,
              const cl_image_desc *   image_desc,
              void *                  host_ptr,
              cl_int *                errcode_ret) CL_API_SUFFIX__VERSION_1_2 {

  cl_mem ret = NULL;
  WRAPPERCLFUNCT( clCreateImage, ( context, flags, \
    image_format, image_desc, host_ptr, errcode_ret), NULL )



}

#endif


#ifdef CL_VERSION_2_0

CL_API_ENTRY cl_mem CL_API_CALL
clCreatePipe(cl_context                 context,
             cl_mem_flags               flags,
             cl_uint                    pipe_packet_size,
             cl_uint                    pipe_max_packets,
             const cl_pipe_properties * properties,
             cl_int *                   errcode_ret) CL_API_SUFFIX__VERSION_2_0  {


  cl_mem ret = NULL;
  WRAPPERCLFUNCT( clCreatePipe, ( context, flags, \
    pipe_packet_size, pipe_max_packets, properties, errcode_ret), NULL )

}



#endif


#ifdef CL_VERSION_3_0

CL_API_ENTRY cl_mem CL_API_CALL
clCreateBufferWithProperties(cl_context                context,
                             const cl_mem_properties * properties,
                             cl_mem_flags              flags,
                             size_t                    size,
                             void *                    host_ptr,
                             cl_int *                  errcode_ret) CL_API_SUFFIX__VERSION_3_0{

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT( clCreateBufferWithProperties, ( context, properties, \
    flags, size, host_ptr, errcode_ret), CL_INVALID_VALUE )

}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateImageWithProperties(cl_context                context,
                            const cl_mem_properties * properties,
                            cl_mem_flags              flags,
                            const cl_image_format *   image_format,
                            const cl_image_desc *     image_desc,
                            void *                    host_ptr,
                            cl_int *                  errcode_ret) CL_API_SUFFIX__VERSION_3_0 {

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT( clCreateImageWithProperties, ( context, properties,\
    flags, image_format, image_desc, host_ptr, errcode_ret), CL_INVALID_VALUE )

}

#endif


CL_API_ENTRY cl_int CL_API_CALL
clRetainMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0 {


  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT( clRetainMemObject, ( memobj ), CL_OUT_OF_RESOURCES )

}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0{

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT( clReleaseMemObject, ( memobj ), CL_OUT_OF_RESOURCES )

}

CL_API_ENTRY cl_int CL_API_CALL
clGetSupportedImageFormats(cl_context           context,
                           cl_mem_flags         flags,
                           cl_mem_object_type   image_type,
                           cl_uint              num_entries,
                           cl_image_format *    image_formats,
                           cl_uint *            num_image_formats) CL_API_SUFFIX__VERSION_1_0 {

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT( clGetSupportedImageFormats, ( context, flags, \
    image_type, num_entries, image_formats, num_image_formats ), CL_INVALID_VALUE )

}



CL_API_ENTRY cl_int CL_API_CALL
clGetMemObjectInfo(cl_mem           memobj,
                   cl_mem_info      param_name,
                   size_t           param_value_size,
                   void *           param_value,
                   size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0 {

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT( clGetMemObjectInfo, ( memobj, param_name, \
    param_value_size, param_value, param_value_size_ret ), CL_INVALID_VALUE )


}


CL_API_ENTRY cl_int CL_API_CALL
clGetImageInfo(cl_mem           image,
               cl_image_info    param_name,
               size_t           param_value_size,
               void *           param_value,
               size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0 {


  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT( clGetImageInfo, ( image, param_name, \
    param_value_size, param_value, param_value_size_ret ), CL_INVALID_VALUE )

}




#ifdef CL_VERSION_2_0

CL_API_ENTRY cl_int CL_API_CALL
clGetPipeInfo(cl_mem           pipe,
              cl_pipe_info     param_name,
              size_t           param_value_size,
              void *           param_value,
              size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_2_0 {

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT( clGetPipeInfo, ( pipe, param_name, \
    param_value_size, param_value, param_value_size_ret ), CL_INVALID_VALUE )

}

#endif


#ifdef CL_VERSION_1_1

CL_API_ENTRY cl_int CL_API_CALL
clSetMemObjectDestructorCallback(cl_mem memobj,
                                 void (CL_CALLBACK * pfn_notify)(cl_mem memobj,
                                                                 void * user_data),
                                 void * user_data) CL_API_SUFFIX__VERSION_1_1 {

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT(clSetMemObjectDestructorCallback, (memobj, pfn_notify, user_data), \
                 CL_INVALID_VALUE)
}

#endif



CL_API_ENTRY cl_int CL_API_CALL
clReleaseProgram(cl_program program) CL_API_SUFFIX__VERSION_1_0 {

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT(clReleaseProgram, ( program ), CL_OUT_OF_RESOURCES )

}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseKernel(cl_kernel   kernel) CL_API_SUFFIX__VERSION_1_0 {

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT(clReleaseKernel, ( kernel ), CL_OUT_OF_RESOURCES )

}



CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadBuffer(
  cl_command_queue    command_queue,
  cl_mem              buffer,
  cl_bool             blocking_read,
  size_t              offset,
  size_t              size,
  void *              ptr,
  cl_uint             num_events_in_wait_list,
  const cl_event *    event_wait_list,
  cl_event *          event) CL_API_SUFFIX__VERSION_1_0{

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT(clEnqueueReadBuffer, ( command_queue, buffer, \
    blocking_read, offset, size, ptr, num_events_in_wait_list, \
    event_wait_list, event ), CL_INVALID_VALUE )

}



CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNDRangeKernel(cl_command_queue command_queue,
                       cl_kernel        kernel,
                       cl_uint          work_dim,
                       const size_t *   global_work_offset,
                       const size_t *   global_work_size,
                       const size_t *   local_work_size,
                       cl_uint          num_events_in_wait_list,
                       const cl_event * event_wait_list,
                       cl_event *       event) CL_API_SUFFIX__VERSION_1_0 {

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT(clEnqueueNDRangeKernel, ( command_queue, kernel, \
    work_dim, global_work_offset, global_work_size, local_work_size, \
    num_events_in_wait_list, event_wait_list, event ), CL_OUT_OF_RESOURCES )

}

CL_API_ENTRY cl_int CL_API_CALL
clSetKernelArg(cl_kernel    kernel,
               cl_uint      arg_index,
               size_t       arg_size,
               const void * arg_value) CL_API_SUFFIX__VERSION_1_0{

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT(clSetKernelArg, ( kernel, arg_index, \
    arg_size, arg_value ), CL_OUT_OF_RESOURCES )

}


CL_API_ENTRY cl_kernel CL_API_CALL
clCreateKernel(cl_program      program,
               const char *    kernel_name,
               cl_int *        errcode_ret) CL_API_SUFFIX__VERSION_1_0 {

  cl_kernel ret = NULL;
  WRAPPERCLFUNCT(clCreateKernel, ( program, kernel_name, errcode_ret ), NULL )

}



CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithSource(cl_context        context,
                          cl_uint           count,
                          const char **     strings,
                          const size_t *    lengths,
                          cl_int *          errcode_ret) CL_API_SUFFIX__VERSION_1_0{

  cl_program ret = NULL;
  WRAPPERCLFUNCT(clCreateProgramWithSource, ( context, count, \
    strings, lengths, errcode_ret ), NULL )

}



CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_2_DEPRECATED cl_command_queue CL_API_CALL
clCreateCommandQueue(cl_context                     context,
                     cl_device_id                   device,
                     cl_command_queue_properties    properties,
                     cl_int *                       errcode_ret) CL_EXT_SUFFIX__VERSION_1_2_DEPRECATED {

  cl_command_queue ret = NULL;
  WRAPPERCLFUNCT(clCreateCommandQueue, ( context, device, properties, errcode_ret ), NULL )

}


CL_API_ENTRY cl_int CL_API_CALL
clGetProgramBuildInfo(cl_program            program,
                      cl_device_id          device,
                      cl_program_build_info param_name,
                      size_t                param_value_size,
                      void *                param_value,
                      size_t *              param_value_size_ret) CL_API_SUFFIX__VERSION_1_0{

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT(clGetProgramBuildInfo, ( program, device, \
    param_name, param_value_size, param_value, param_value_size_ret ), CL_OUT_OF_RESOURCES )

}


CL_API_ENTRY cl_int CL_API_CALL
clBuildProgram(cl_program           program,
               cl_uint              num_devices,
               const cl_device_id * device_list,
               const char *         options,
               void (CL_CALLBACK *  pfn_notify)(cl_program program,
                                                void * user_data),
               void *               user_data) CL_API_SUFFIX__VERSION_1_0{


  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT(clBuildProgram, ( program, num_devices, \
    device_list, options, pfn_notify, user_data ), CL_OUT_OF_RESOURCES )

}



CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteBuffer(cl_command_queue   command_queue,
                     cl_mem             buffer,
                     cl_bool            blocking_write,
                     size_t             offset,
                     size_t             size,
                     const void *       ptr,
                     cl_uint            num_events_in_wait_list,
                     const cl_event *   event_wait_list,
                     cl_event *         event) CL_API_SUFFIX__VERSION_1_0{

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT(clEnqueueWriteBuffer, ( command_queue, buffer, \
    blocking_write, offset, size, ptr, num_events_in_wait_list, event_wait_list, event ), CL_OUT_OF_RESOURCES )

}

CL_API_ENTRY cl_int CL_API_CALL
clFinish(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0{

  cl_int ret = CL_SUCCESS;
  WRAPPERCLFUNCT( clFinish, ( command_queue ), \
    CL_OUT_OF_RESOURCES )

}


/*



// SVM Allocation APIs

#ifdef CL_VERSION_2_0

extern CL_API_ENTRY void * CL_API_CALL
clSVMAlloc(cl_context       context,
           cl_svm_mem_flags flags,
           size_t           size,
           cl_uint          alignment) CL_API_SUFFIX__VERSION_2_0;

extern CL_API_ENTRY void CL_API_CALL
clSVMFree(cl_context        context,
          void *            svm_pointer) CL_API_SUFFIX__VERSION_2_0;

#endif

// Sampler APIs

#ifdef CL_VERSION_2_0

extern CL_API_ENTRY cl_sampler CL_API_CALL
clCreateSamplerWithProperties(cl_context                     context,
                              const cl_sampler_properties *  sampler_properties,
                              cl_int *                       errcode_ret) CL_API_SUFFIX__VERSION_2_0;

#endif

extern CL_API_ENTRY cl_int CL_API_CALL
clRetainSampler(cl_sampler sampler) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clReleaseSampler(cl_sampler sampler) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clGetSamplerInfo(cl_sampler         sampler,
                 cl_sampler_info    param_name,
                 size_t             param_value_size,
                 void *             param_value,
                 size_t *           param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;

// Program Object APIs

extern CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithBinary(cl_context                     context,
                          cl_uint                        num_devices,
                          const cl_device_id *           device_list,
                          const size_t *                 lengths,
                          const unsigned char **         binaries,
                          cl_int *                       binary_status,
                          cl_int *                       errcode_ret) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_2

extern CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithBuiltInKernels(cl_context            context,
                                  cl_uint               num_devices,
                                  const cl_device_id *  device_list,
                                  const char *          kernel_names,
                                  cl_int *              errcode_ret) CL_API_SUFFIX__VERSION_1_2;

#endif

#ifdef CL_VERSION_2_1

extern CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithIL(cl_context    context,
                      const void*    il,
                      size_t         length,
                      cl_int*        errcode_ret) CL_API_SUFFIX__VERSION_2_1;

#endif

extern CL_API_ENTRY cl_int CL_API_CALL
clRetainProgram(cl_program program) CL_API_SUFFIX__VERSION_1_0;



#ifdef CL_VERSION_1_2

extern CL_API_ENTRY cl_int CL_API_CALL
clCompileProgram(cl_program           program,
                 cl_uint              num_devices,
                 const cl_device_id * device_list,
                 const char *         options,
                 cl_uint              num_input_headers,
                 const cl_program *   input_headers,
                 const char **        header_include_names,
                 void (CL_CALLBACK *  pfn_notify)(cl_program program,
                                                  void * user_data),
                 void *               user_data) CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_program CL_API_CALL
clLinkProgram(cl_context           context,
              cl_uint              num_devices,
              const cl_device_id * device_list,
              const char *         options,
              cl_uint              num_input_programs,
              const cl_program *   input_programs,
              void (CL_CALLBACK *  pfn_notify)(cl_program program,
                                               void * user_data),
              void *               user_data,
              cl_int *             errcode_ret) CL_API_SUFFIX__VERSION_1_2;

#endif

#ifdef CL_VERSION_2_2

extern CL_API_ENTRY cl_int CL_API_CALL
clSetProgramReleaseCallback(cl_program          program,
                            void (CL_CALLBACK * pfn_notify)(cl_program program,
                                                            void * user_data),
                            void *              user_data) CL_API_SUFFIX__VERSION_2_2;

extern CL_API_ENTRY cl_int CL_API_CALL
clSetProgramSpecializationConstant(cl_program  program,
                                   cl_uint     spec_id,
                                   size_t      spec_size,
                                   const void* spec_value) CL_API_SUFFIX__VERSION_2_2;

#endif

#ifdef CL_VERSION_1_2

extern CL_API_ENTRY cl_int CL_API_CALL
clUnloadPlatformCompiler(cl_platform_id platform) CL_API_SUFFIX__VERSION_1_2;

#endif

extern CL_API_ENTRY cl_int CL_API_CALL
clGetProgramInfo(cl_program         program,
                 cl_program_info    param_name,
                 size_t             param_value_size,
                 void *             param_value,
                 size_t *           param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;


// Kernel Object APIs

extern CL_API_ENTRY cl_int CL_API_CALL
clCreateKernelsInProgram(cl_program     program,
                         cl_uint        num_kernels,
                         cl_kernel *    kernels,
                         cl_uint *      num_kernels_ret) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_2_1

extern CL_API_ENTRY cl_kernel CL_API_CALL
clCloneKernel(cl_kernel     source_kernel,
              cl_int*       errcode_ret) CL_API_SUFFIX__VERSION_2_1;

#endif

extern CL_API_ENTRY cl_int CL_API_CALL
clRetainKernel(cl_kernel    kernel) CL_API_SUFFIX__VERSION_1_0;



#ifdef CL_VERSION_2_0

extern CL_API_ENTRY cl_int CL_API_CALL
clSetKernelArgSVMPointer(cl_kernel    kernel,
                         cl_uint      arg_index,
                         const void * arg_value) CL_API_SUFFIX__VERSION_2_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clSetKernelExecInfo(cl_kernel            kernel,
                    cl_kernel_exec_info  param_name,
                    size_t               param_value_size,
                    const void *         param_value) CL_API_SUFFIX__VERSION_2_0;

#endif

extern CL_API_ENTRY cl_int CL_API_CALL
clGetKernelInfo(cl_kernel       kernel,
                cl_kernel_info  param_name,
                size_t          param_value_size,
                void *          param_value,
                size_t *        param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_2

extern CL_API_ENTRY cl_int CL_API_CALL
clGetKernelArgInfo(cl_kernel       kernel,
                   cl_uint         arg_indx,
                   cl_kernel_arg_info  param_name,
                   size_t          param_value_size,
                   void *          param_value,
                   size_t *        param_value_size_ret) CL_API_SUFFIX__VERSION_1_2;

#endif

extern CL_API_ENTRY cl_int CL_API_CALL
clGetKernelWorkGroupInfo(cl_kernel                  kernel,
                         cl_device_id               device,
                         cl_kernel_work_group_info  param_name,
                         size_t                     param_value_size,
                         void *                     param_value,
                         size_t *                   param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_2_1

extern CL_API_ENTRY cl_int CL_API_CALL
clGetKernelSubGroupInfo(cl_kernel                   kernel,
                        cl_device_id                device,
                        cl_kernel_sub_group_info    param_name,
                        size_t                      input_value_size,
                        const void*                 input_value,
                        size_t                      param_value_size,
                        void*                       param_value,
                        size_t*                     param_value_size_ret) CL_API_SUFFIX__VERSION_2_1;

#endif

// Event Object APIs
extern CL_API_ENTRY cl_int CL_API_CALL
clWaitForEvents(cl_uint             num_events,
                const cl_event *    event_list) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clGetEventInfo(cl_event         event,
               cl_event_info    param_name,
               size_t           param_value_size,
               void *           param_value,
               size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_1

extern CL_API_ENTRY cl_event CL_API_CALL
clCreateUserEvent(cl_context    context,
                  cl_int *      errcode_ret) CL_API_SUFFIX__VERSION_1_1;

#endif

extern CL_API_ENTRY cl_int CL_API_CALL
clRetainEvent(cl_event event) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clReleaseEvent(cl_event event) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_1

extern CL_API_ENTRY cl_int CL_API_CALL
clSetUserEventStatus(cl_event   event,
                     cl_int     execution_status) CL_API_SUFFIX__VERSION_1_1;

extern CL_API_ENTRY cl_int CL_API_CALL
clSetEventCallback(cl_event    event,
                   cl_int      command_exec_callback_type,
                   void (CL_CALLBACK * pfn_notify)(cl_event event,
                                                   cl_int   event_command_status,
                                                   void *   user_data),
                   void *      user_data) CL_API_SUFFIX__VERSION_1_1;

#endif

// Profiling APIs
extern CL_API_ENTRY cl_int CL_API_CALL
clGetEventProfilingInfo(cl_event            event,
                        cl_profiling_info   param_name,
                        size_t              param_value_size,
                        void *              param_value,
                        size_t *            param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;

// Flush and Finish APIs
extern CL_API_ENTRY cl_int CL_API_CALL
clFlush(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0;


// Enqueued Commands APIs
e
#ifdef CL_VERSION_1_1

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadBufferRect(cl_command_queue    command_queue,
                        cl_mem              buffer,
                        cl_bool             blocking_read,
                        const size_t *      buffer_offset,
                        const size_t *      host_offset,
                        const size_t *      region,
                        size_t              buffer_row_pitch,
                        size_t              buffer_slice_pitch,
                        size_t              host_row_pitch,
                        size_t              host_slice_pitch,
                        void *              ptr,
                        cl_uint             num_events_in_wait_list,
                        const cl_event *    event_wait_list,
                        cl_event *          event) CL_API_SUFFIX__VERSION_1_1;

#endif


#ifdef CL_VERSION_1_1

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteBufferRect(cl_command_queue    command_queue,
                         cl_mem              buffer,
                         cl_bool             blocking_write,
                         const size_t *      buffer_offset,
                         const size_t *      host_offset,
                         const size_t *      region,
                         size_t              buffer_row_pitch,
                         size_t              buffer_slice_pitch,
                         size_t              host_row_pitch,
                         size_t              host_slice_pitch,
                         const void *        ptr,
                         cl_uint             num_events_in_wait_list,
                         const cl_event *    event_wait_list,
                         cl_event *          event) CL_API_SUFFIX__VERSION_1_1;

#endif

#ifdef CL_VERSION_1_2

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueFillBuffer(cl_command_queue   command_queue,
                    cl_mem             buffer,
                    const void *       pattern,
                    size_t             pattern_size,
                    size_t             offset,
                    size_t             size,
                    cl_uint            num_events_in_wait_list,
                    const cl_event *   event_wait_list,
                    cl_event *         event) CL_API_SUFFIX__VERSION_1_2;

#endif

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBuffer(cl_command_queue    command_queue,
                    cl_mem              src_buffer,
                    cl_mem              dst_buffer,
                    size_t              src_offset,
                    size_t              dst_offset,
                    size_t              size,
                    cl_uint             num_events_in_wait_list,
                    const cl_event *    event_wait_list,
                    cl_event *          event) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_1

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBufferRect(cl_command_queue    command_queue,
                        cl_mem              src_buffer,
                        cl_mem              dst_buffer,
                        const size_t *      src_origin,
                        const size_t *      dst_origin,
                        const size_t *      region,
                        size_t              src_row_pitch,
                        size_t              src_slice_pitch,
                        size_t              dst_row_pitch,
                        size_t              dst_slice_pitch,
                        cl_uint             num_events_in_wait_list,
                        const cl_event *    event_wait_list,
                        cl_event *          event) CL_API_SUFFIX__VERSION_1_1;

#endif

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadImage(cl_command_queue     command_queue,
                   cl_mem               image,
                   cl_bool              blocking_read,
                   const size_t *       origin,
                   const size_t *       region,
                   size_t               row_pitch,
                   size_t               slice_pitch,
                   void *               ptr,
                   cl_uint              num_events_in_wait_list,
                   const cl_event *     event_wait_list,
                   cl_event *           event) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteImage(cl_command_queue    command_queue,
                    cl_mem              image,
                    cl_bool             blocking_write,
                    const size_t *      origin,
                    const size_t *      region,
                    size_t              input_row_pitch,
                    size_t              input_slice_pitch,
                    const void *        ptr,
                    cl_uint             num_events_in_wait_list,
                    const cl_event *    event_wait_list,
                    cl_event *          event) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_2

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueFillImage(cl_command_queue   command_queue,
                   cl_mem             image,
                   const void *       fill_color,
                   const size_t *     origin,
                   const size_t *     region,
                   cl_uint            num_events_in_wait_list,
                   const cl_event *   event_wait_list,
                   cl_event *         event) CL_API_SUFFIX__VERSION_1_2;

#endif

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyImage(cl_command_queue     command_queue,
                   cl_mem               src_image,
                   cl_mem               dst_image,
                   const size_t *       src_origin,
                   const size_t *       dst_origin,
                   const size_t *       region,
                   cl_uint              num_events_in_wait_list,
                   const cl_event *     event_wait_list,
                   cl_event *           event) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyImageToBuffer(cl_command_queue command_queue,
                           cl_mem           src_image,
                           cl_mem           dst_buffer,
                           const size_t *   src_origin,
                           const size_t *   region,
                           size_t           dst_offset,
                           cl_uint          num_events_in_wait_list,
                           const cl_event * event_wait_list,
                           cl_event *       event) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBufferToImage(cl_command_queue command_queue,
                           cl_mem           src_buffer,
                           cl_mem           dst_image,
                           size_t           src_offset,
                           const size_t *   dst_origin,
                           const size_t *   region,
                           cl_uint          num_events_in_wait_list,
                           const cl_event * event_wait_list,
                           cl_event *       event) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY void * CL_API_CALL
clEnqueueMapBuffer(cl_command_queue command_queue,
                   cl_mem           buffer,
                   cl_bool          blocking_map,
                   cl_map_flags     map_flags,
                   size_t           offset,
                   size_t           size,
                   cl_uint          num_events_in_wait_list,
                   const cl_event * event_wait_list,
                   cl_event *       event,
                   cl_int *         errcode_ret) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY void * CL_API_CALL
clEnqueueMapImage(cl_command_queue  command_queue,
                  cl_mem            image,
                  cl_bool           blocking_map,
                  cl_map_flags      map_flags,
                  const size_t *    origin,
                  const size_t *    region,
                  size_t *          image_row_pitch,
                  size_t *          image_slice_pitch,
                  cl_uint           num_events_in_wait_list,
                  const cl_event *  event_wait_list,
                  cl_event *        event,
                  cl_int *          errcode_ret) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueUnmapMemObject(cl_command_queue command_queue,
                        cl_mem           memobj,
                        void *           mapped_ptr,
                        cl_uint          num_events_in_wait_list,
                        const cl_event * event_wait_list,
                        cl_event *       event) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_2

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueMigrateMemObjects(cl_command_queue       command_queue,
                           cl_uint                num_mem_objects,
                           const cl_mem *         mem_objects,
                           cl_mem_migration_flags flags,
                           cl_uint                num_events_in_wait_list,
                           const cl_event *       event_wait_list,
                           cl_event *             event) CL_API_SUFFIX__VERSION_1_2;

#endif

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNDRangeKernel(cl_command_queue command_queue,
                       cl_kernel        kernel,
                       cl_uint          work_dim,
                       const size_t *   global_work_offset,
                       const size_t *   global_work_size,
                       const size_t *   local_work_size,
                       cl_uint          num_events_in_wait_list,
                       const cl_event * event_wait_list,
                       cl_event *       event) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNativeKernel(cl_command_queue  command_queue,
                      void (CL_CALLBACK * user_func)(void *),
                      void *            args,
                      size_t            cb_args,
                      cl_uint           num_mem_objects,
                      const cl_mem *    mem_list,
                      const void **     args_mem_loc,
                      cl_uint           num_events_in_wait_list,
                      const cl_event *  event_wait_list,
                      cl_event *        event) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_2

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueMarkerWithWaitList(cl_command_queue  command_queue,
                            cl_uint           num_events_in_wait_list,
                            const cl_event *  event_wait_list,
                            cl_event *        event) CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueBarrierWithWaitList(cl_command_queue  command_queue,
                             cl_uint           num_events_in_wait_list,
                             const cl_event *  event_wait_list,
                             cl_event *        event) CL_API_SUFFIX__VERSION_1_2;

#endif

#ifdef CL_VERSION_2_0

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueSVMFree(cl_command_queue  command_queue,
                 cl_uint           num_svm_pointers,
                 void *            svm_pointers[],
                 void (CL_CALLBACK * pfn_free_func)(cl_command_queue queue,
                                                    cl_uint          num_svm_pointers,
                                                    void *           svm_pointers[],
                                                    void *           user_data),
                 void *            user_data,
                 cl_uint           num_events_in_wait_list,
                 const cl_event *  event_wait_list,
                 cl_event *        event) CL_API_SUFFIX__VERSION_2_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueSVMMemcpy(cl_command_queue  command_queue,
                   cl_bool           blocking_copy,
                   void *            dst_ptr,
                   const void *      src_ptr,
                   size_t            size,
                   cl_uint           num_events_in_wait_list,
                   const cl_event *  event_wait_list,
                   cl_event *        event) CL_API_SUFFIX__VERSION_2_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueSVMMemFill(cl_command_queue  command_queue,
                    void *            svm_ptr,
                    const void *      pattern,
                    size_t            pattern_size,
                    size_t            size,
                    cl_uint           num_events_in_wait_list,
                    const cl_event *  event_wait_list,
                    cl_event *        event) CL_API_SUFFIX__VERSION_2_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueSVMMap(cl_command_queue  command_queue,
                cl_bool           blocking_map,
                cl_map_flags      flags,
                void *            svm_ptr,
                size_t            size,
                cl_uint           num_events_in_wait_list,
                const cl_event *  event_wait_list,
                cl_event *        event) CL_API_SUFFIX__VERSION_2_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueSVMUnmap(cl_command_queue  command_queue,
                  void *            svm_ptr,
                  cl_uint           num_events_in_wait_list,
                  const cl_event *  event_wait_list,
                  cl_event *        event) CL_API_SUFFIX__VERSION_2_0;

#endif

#ifdef CL_VERSION_2_1

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueSVMMigrateMem(cl_command_queue         command_queue,
                       cl_uint                  num_svm_pointers,
                       const void **            svm_pointers,
                       const size_t *           sizes,
                       cl_mem_migration_flags   flags,
                       cl_uint                  num_events_in_wait_list,
                       const cl_event *         event_wait_list,
                       cl_event *               event) CL_API_SUFFIX__VERSION_2_1;

#endif

#ifdef CL_VERSION_1_2

//* Extension function access
// *
// * Returns the extension function address for the given function name,
// * or NULL if a valid function can not be found.  The client must
// * check to make sure the address is not NULL, before using or
// * calling the returned function address.
// *
extern CL_API_ENTRY void * CL_API_CALL
clGetExtensionFunctionAddressForPlatform(cl_platform_id platform,
                                         const char *   func_name) CL_API_SUFFIX__VERSION_1_2;

#endif

#ifdef CL_USE_DEPRECATED_OPENCL_1_0_APIS
//
//     *  WARNING:
//     *     This API introduces mutable state into the OpenCL implementation. It has been REMOVED
//     *  to better facilitate thread safety.  The 1.0 API is not thread safe. It is not tested by the
//     *  OpenCL 1.1 conformance test, and consequently may not work or may not work dependably.
//     *  It is likely to be non-performant. Use of this API is not advised. Use at your own risk.
//     *
//     *  Software developers previously relying on this API are instructed to set the command queue
//     *  properties when creating the queue, instead.
//     *
    extern CL_API_ENTRY cl_int CL_API_CALL
    clSetCommandQueueProperty(cl_command_queue              command_queue,
                              cl_command_queue_properties   properties,
                              cl_bool                       enable,
                              cl_command_queue_properties * old_properties) CL_EXT_SUFFIX__VERSION_1_0_DEPRECATED;
#endif // CL_USE_DEPRECATED_OPENCL_1_0_APIS

// Deprecated OpenCL 1.1 APIs
extern CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_1_DEPRECATED cl_mem CL_API_CALL
clCreateImage2D(cl_context              context,
                cl_mem_flags            flags,
                const cl_image_format * image_format,
                size_t                  image_width,
                size_t                  image_height,
                size_t                  image_row_pitch,
                void *                  host_ptr,
                cl_int *                errcode_ret) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

extern CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_1_DEPRECATED cl_mem CL_API_CALL
clCreateImage3D(cl_context              context,
                cl_mem_flags            flags,
                const cl_image_format * image_format,
                size_t                  image_width,
                size_t                  image_height,
                size_t                  image_depth,
                size_t                  image_row_pitch,
                size_t                  image_slice_pitch,
                void *                  host_ptr,
                cl_int *                errcode_ret) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

extern CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_1_DEPRECATED cl_int CL_API_CALL
clEnqueueMarker(cl_command_queue    command_queue,
                cl_event *          event) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

extern CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_1_DEPRECATED cl_int CL_API_CALL
clEnqueueWaitForEvents(cl_command_queue  command_queue,
                       cl_uint          num_events,
                       const cl_event * event_list) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

extern CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_1_DEPRECATED cl_int CL_API_CALL
clEnqueueBarrier(cl_command_queue command_queue) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

extern CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_1_DEPRECATED cl_int CL_API_CALL
clUnloadCompiler(void) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

extern CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_1_DEPRECATED void * CL_API_CALL
clGetExtensionFunctionAddress(const char * func_name) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

// Deprecated OpenCL 2.0 APIs

extern CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_2_DEPRECATED cl_sampler CL_API_CALL
clCreateSampler(cl_context          context,
                cl_bool             normalized_coords,
                cl_addressing_mode  addressing_mode,
                cl_filter_mode      filter_mode,
                cl_int *            errcode_ret) CL_EXT_SUFFIX__VERSION_1_2_DEPRECATED;

extern CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_2_DEPRECATED cl_int CL_API_CALL
clEnqueueTask(cl_command_queue  command_queue,
              cl_kernel         kernel,
              cl_uint           num_events_in_wait_list,
              const cl_event *  event_wait_list,
              cl_event *        event) CL_EXT_SUFFIX__VERSION_1_2_DEPRECATED;




*/






int loadOpenCL( const char* p ){

    int ret = 0;                          // return value

    pthread_rwlock_rdlock( &lock );       // exclude unload mechanism
    pthread_mutex_lock( &dllock );        // get exclusive access to load mechanism

    if (dlcall == NULL) {

      strncpy(liboclpath, p, 1024);     // yes -> copy path

      // load library
      dlcall = dlopen( liboclpath, RTLD_NOW );

      if (dlcall == NULL) {
        ret = -2;    // report error
      }

    }
    else {
      ret = -1;       // library has already been loaded
    }
     // release locks
    pthread_mutex_unlock( &dllock );
    pthread_rwlock_unlock( &lock );

    return( ret );

}  // loadOpenCL



void unloadOpenCL(){

    pthread_rwlock_wrlock( &lock );        // get exclusive access to unload mechanism
    pthread_mutex_lock( &dllock );         // get exclusive access to load mechanism

    if (dlcall != NULL) {                 // is initialized?
        dlclose(dlcall);                  // yes -> unload library
    }

    dlcall = NULL;                        // set all relevant pointers to null

    cl_wrap_call = CL_WRAP_CALL_ZERO;     // clear all function pointers

    // release locks
    pthread_mutex_unlock( &dllock );
    pthread_rwlock_unlock( &lock );

}  // unloadOpenCL


