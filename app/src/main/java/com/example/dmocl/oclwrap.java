/*
 * Author & Copyright: Robert Fritze
 * Version: 1.0
 * Created: 2020
 * Last modified: 12.7.2020
 */


package com.example.dmocl;


/**
 * <p>A singleton class for the OpenCL wrapper.</p>
 * <p>Each class (or activity) that needs the use OpenCL can get a
 * reference to the OpenCL runtime invoking the method <i>getOclWrapper</i></p>
 * <p>The OpenCL runtime implemented has two abstraction layers:
 * The first abstraction layer is
 * the link between Java (Android) and a C-Wrapper. The communication between Java and C
 * is done using JNI. This wrapper has to be written by the user. It can expose the entire set of
 * OpenCL calls or implement an own
 * API that exposes the functionality of more complex tasks that are carried out in C/OpenCL.</p>
 * <p>The second abstraction layer is the link between C and OpenCL. It is a thin wrapper that loads
 * dynamically at runtime the local OpenCL library (libOpenCL.so) and exposes its functionality
 * to the user. This layer must not be modified by the user. The user has to call once (before the
 * very first call to an OpenCL function the function <i>loadOpenCL</i> that loads the libOpenCL.so
 * library on the device. This library does not have to be present at compile time. After the last
 * call to an OpenCL function, the function <i>unloadOpenCL</i> should be called. If the user wishes,
 * on Android onStop-Event the OpenCL library can be unloaded (saving some
 * memory). If one tries to call an OpenCL function WITHOUT having called 'loadOpenCL' a runtime
 * error will occur. The best place to call 'loadOpenCL' would be Androids 'onCreate' method.</p>
 * <p>All methods of this class are fully thread-safe if the underlying JNI methods are thread-safe.
 * The OpenCL library cannot be unloaded while there is still some calculation in progress. If one
 * tries to call an OpenCL function AFTER the OpenCL library has been unloaded, the library will
 * be reloaded automatically. The Java- and JNI part do not have to matter about synchronization
 * issues. Synchronization is provided by the OpenCL wrapper library.</p>
*/
public final class oclwrap {

    public class oclinforet{
      int result;
      String s;
      float oclversion;
      int devtype;

      String getDevType() {

        String sr = "";
        String sep = "";

        if ((devtype & 1) != 0){
          sr = "CPU";
          sep = ", ";
        }

        if ((devtype & 2) != 0){
          sr += sep + "GPU";
          sep = ", ";
        }

        if ((devtype & 4) != 0){
          sr += sep + "ACCELERATOR";
          sep = ", ";
        }

        if ((devtype & 8) != 0){
          sr += sep + "DEFAULT";
          sep = ", ";
        }

        return( sr );
      }
    }

    /*
     * Load the oclwrapper
     */
    static
    {
        System.loadLibrary("oclwrapper");
    }


    /**
     * A static variable that holds the instance.
     */
    private static oclwrap oclwrap_instance = null;


    /**
     * A static variable that serves as synchronize object.
     */
    private static final Object syncobj = new Object();

  /**
   * Returns the type of architecture.
   * @return The the architecture used (0=arm-v7, 1=arm-v8, 2=x86, 3=x86_64, -1=unknown)
   * @multithreading fully
   */
    static public native int getArchitecture();


    /**
     * Loads the OpenCL library on the device. The library does not have to be present at compile time.
     * Must be called once before any other call to an OpenCL function. Subsequent calls to this
     * method have no effect (even if the library is currently not loaded).
     * The path to the library can be set only at the call to
     * this function and is immutable afterwards (you have to restart the app to change the path).
     * @param ocllibpath The path and name of the OpenCL-library on the device (e.g. "/system/vendor/lib/libOpenCL.so"
     *                   for Mali graphics cards.
     * @return -1 = if the library could not be loaded, 1 = this method has already been called,
     *         0 else; if the return value is greater or equal to zero, the library can be used.
     *         If the return value is negative, most probably the path to the OpenCL
     *         library on the device was wrong or there is no OpenCL shared library on the device.
     * @multithreading fully thread safe as long as underlying JNI function is
     * thread-safe (synchronization is provided in library function).
     */
    public native int loadOpenCL ( String ocllibpath );


    /**
     * Unloads the OpenCL library. This method can be called also if Android's 'onStop' event occurs.
     * @multithreading Fully thread-safe as long as the underlying JNI implementation is
     * thread save. The underlying library function provides synchronization methods.
     */
    public native void unloadOpenCL ();


    /**
     * Returns the number of platforms available.
     * @remark If only number of platforms is relevant, this method is much faster than
     * *AndrCLGetPlatformIDs*
     * @return OCLANDROID_ERROR or number of platforms found
     * @multithreading fully threadsafe
     */
    public native int AndrCLGetPlatformCnt();
    public native int AndrCLGetDeviceCnt( int platf );
    public native oclinforet AndrCLgetDeviceName( int platf, int dev );


    /**
     * Returns a reference to the singleton.
     * @return The (single) instance of the OpenCL wrapper.
     * @multithreading Fully threadsafe.
     */
    public static oclwrap getOclWrapper(){

                         // synchronize access to avoid concurrent call
        synchronized (syncobj) {

                             // already initialized
            if (oclwrap_instance == null) {
                             // initialize, oclwrap_instance is from now on readonly
                oclwrap_instance = new oclwrap();
            }

        }

        return( oclwrap_instance );   // return instance
    }


    /**
     * A private singleton constructor.
     */
    private oclwrap(){
    }

} // oclwrap



