/*!
 \file AndroidOpenCL.h
 \brief Contains supported OpenCL version and init/destroy method prototypes
 \details
 This headerfile contains the supported OpenCL version and two method definitions.
 Every file that uses the OpenCL.c wrapper must include this headerfile **before**
 any native OpenCL header file is included. Otherwise the OpenCL versions for
 the wrapper and the program might not match.
 \remark
 Include this header file **before** you include any other OpenCL header file (otherwise
 the versions of the library and your program will not match.
 \copyright Copyright Robert Fritze 2020
 \version 1.0
 \author Robert Fritze
 \date 4.5.2020
 */



#ifndef OPENCLAPP_ANDROIDOPENCL_H
#define OPENCLAPP_ANDROIDOPENCL_H




#define OCLANDROID_SUCCESS 0
#define OCLANDROID_ERROR -1



/*!
 \def CL_TARGET_OPENCL_VERSION
 This version should match the version of the OpenCL-library on the Android device.
 If this version number is lower than the OpenCL version supported by the
 OpenCL library on the device, some methods will not be accessible.
 If this version number is higher than the OpenCL version supported by the
 OpenCL library on the device, the wrapper will still work. But if you try
 to call an unsupported function, the wrapper will produce a runtime error.
 */

/*!
 \fn int loadOpenCL( const char* )
 \brief Loads the OpenCL library of the Android device dynamically.
 \details
 Loads the OpenCL library dynamically. This function **MUST** be called exactly once
 before any other call to an OpenCL function. The function stores the
 path of the library. If the library is unloaded during the execution of
 the Android App (e.g., after an 'onStop' event), the library will be automatically
 reloaded once the execution resumes. There is no need to explicitly call
 this method again. A second and subsequent calls will have no effect. The path of
 the OpenCL library on the device can be set only once at the very beginning of the
 program execution. Any call to an OpenCL function without prior call to this method
 will result in an error.
 \return OK: 0, failure: -1, This method has already been called: 1
 \multithreading fully threadsafe
 */
int loadOpenCL( const char* );

/*!
\fn void unloadOpenCL( void )
\brief Unloads the OpenCL library.
\details This function unloads the library. The library cannot be unloaded while it
 is being loaded or any other OpenCL function is executed. Once the library has been unloaded, it
 will be reloaded automatically before the next OpenCL function call.
\multithreading fully threadsafe
\remark This method may be called by the 'onStop' and 'onDestroy' events of Android.
 It will help to save some memory while the execution of the app is stopped.
*/
void unloadOpenCL( void );

#endif //OPENCLAPP_ANDROIDOPENCL_H
