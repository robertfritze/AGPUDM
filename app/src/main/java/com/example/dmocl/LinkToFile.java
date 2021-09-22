/*!
 @file LinkToFile.java
 @brief Save results and log to disk
 @details
 Saves the results and log information to the hard disk (SD-Card) of the device.
 @copyright Copyright Robert Fritze 2021
 @license MIT
 @version 1.0
 @author Robert Fritze
 @date 11.9.2021
 */
package com.example.dmocl;

import android.os.Build;

import java.io.FileWriter;
import java.text.SimpleDateFormat;
import java.util.Date;

/*!
 @class LinkToFile
 @brief Save results and log to disk
 @details
 Saves the results and log information to the hard disk (SD-Card) of the device.
 */
public final class LinkToFile {

  /*!
  @brief Writes the header line to the file
  @details
  Write the header line to the result file. Existing results are overwritten.
  @param fn file name in which to write the header
  @mt fully (methods are synchronized)
  */
  static synchronized void DMwriteToFileheader(String fn) {
    DMwriteToFileheader(fn, false);
  }


  /*!
  @brief Writes the header line to the file
  @details
  Write the header line to the result file. The header line may be
  appended to existing results.
  @param fn file name in which to write the header
  @param append true=append, false=overwrite
  @mt fully (methods are synchronized)
  */
  static synchronized void DMwriteToFileheader(String fn, boolean append) {

    try {

      FileWriter myWriter = new FileWriter(fn, append);
                    // write header
      myWriter.write("cores;size;cluster;features;" +
        "km_wct_java;km_wct_c;km_wct_c_gpu;km_wct_java_mt;km_wct_c_mt;" +
        "dbscan_wct_java;dbscan_wct_c;dbscan_wct_c_gpu;dbscan_wct_java_mt;dbscan_wct_c_mt;" +
        "gpu_excl_kmeans;java_mt_excl_kmeans;c_mt_excl_kmeans;" +
        "gpu_excl_dbscan;java_mt_excl_dbscan;c_mt_excl_dbscan;dbscan_cmp" + "\n"
      );
      myWriter.flush();
      myWriter.close();

    } catch (Exception e) {
      String se = e.toString();
    }
  } // DMwriteToFileheader


  /*!
  @brief Writes a line to the result file
  @details
  Write a single line to the result file.
  @param fn file name in which to write the header
  @param s the string to be written
  @mt fully (methods are synchronized)
  */
  static synchronized void DMwriteToFile(String fn, String s) {

    try {

      FileWriter myWriter = new FileWriter(fn, true);
      myWriter.write(s + "\n");
      myWriter.flush();
      myWriter.close();

    } catch (Exception e) {
      String se = e.toString();
    }
  } // DMwriteToFile


  /*!
  @brief Writes a footer line to the file
  @details
  Write a footer line to the result file.
  @param fn file name in which to write the header
  @mt fully (methods are synchronized)
  */
  static synchronized void DMwriteToFilefooter(String fn) {

    try {

      FileWriter myWriter = new FileWriter(fn, true);
      myWriter.write("---------- Done ----------------------" + "\n"
      );
      myWriter.flush();
      myWriter.close();

    } catch (Exception e) {
      String se = e.toString();
    }
  }  // DMwriteToFilefooter


  /*!
  @brief Writes results result file
  @details
  Write a single line of results to the result file.
  @param fn file name in which to write the header
  @param cores number of cores used
  @param wct array with the wall clock times used
  @param size size of the cluster
  @param cluno number of clusters used
  @param features number of features used
  @param z 0=DBSCAN results equal, 1=DBSCAN results different
  @mt fully (methods are synchronized)
  */
  static synchronized void DMwriteToFile(String fn, int cores, double[] wct,
                                         int size, int cluno,
                                         int features, int z) {

    try {
      FileWriter myWriter = new FileWriter(fn, true);
      myWriter.write(
        cores + ";" +
          size + ";" +
          cluno + ";" +
          features + ";" +
          wct[0] + ";" +
          wct[1] + ";" +
          wct[2] + ";" +
          wct[3] + ";" +
          wct[4] + ";" +
          wct[5] + ";" +
          wct[6] + ";" +
          wct[7] + ";" +
          wct[8] + ";" +
          wct[9] + ";" +
          wct[10] + ";" +
          wct[11] + ";" +
          wct[12] + ";" +
          wct[13] + ";" +
          wct[14] + ";" +
          wct[15] + ";" +
          z + "\n"
      );
      myWriter.flush();
      myWriter.close();
    } catch (Exception e) {
      String se = e.toString();
    }
  }  // DMwriteToFile


  /*!
  @brief Writes a line to the log file
  @details
  Write a single line to the log file.
  @param fn file name in which to write the header
  @param s string to write
  @param append true=append to log file, false=overwrite log file
  @mt fully (methods are synchronized)
  */
  static synchronized void logToFile(String fn, String s, boolean append) {

    try {
      FileWriter myWriter = new FileWriter(fn, append);

      SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS");
      Date now = new Date();
      String strDate = sdf.format(now);

      myWriter.write(android.os.Build.MANUFACTURER + Build.MODEL + ";" +
        strDate + ":  " + s + "\n");

      myWriter.close();
    } catch (Exception e) {
      String se = e.toString();
    }
  } // logToFile

} // LinkToFile

