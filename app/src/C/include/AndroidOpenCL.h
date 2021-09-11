/*!
 \file AndroidOpenCL.h
 \brief Load/Unload method prototypes
 \details
 This headerfile contains two method definitions for loading and unloading the OpenCL shared library..
 \copyright Copyright Robert Fritze 2021
 \license MIT
 \version 1.0
 \author Robert Fritze
 \date 11.9.2021
 */

#ifndef OPENCLAPP_ANDROIDOPENCL_H
#define OPENCLAPP_ANDROIDOPENCL_H



/*!
 \fn int loadOpenCL( const char* )
 \brief Loads the OpenCL library of the Android device dynamically.
 \details
 Loads the OpenCL library dynamically. This function **MUST** be called exactly once
 before any other call to an OpenCL function. The function stores the
 path of the library. If the library has already been loaded, a call to this method will have
 no effect. Any call to an OpenCL function without prior call to this method
 will result in an error.
 \param c The path and name of the OpenCL-library on the device (is copied, maximum length 1024 characters)
 \return OK: 0, library has already been loaded: -1, unable to load library: -2
 \mt fully threadsafe
 */
int loadOpenCL( const char* c );

/*!
\fn void unloadOpenCL( void )
\brief Unloads the OpenCL library.
\details This function unloads the library.
\mt fully threadsafe
*/
void unloadOpenCL( void );

#endif //OPENCLAPP_ANDROIDOPENCL_H
