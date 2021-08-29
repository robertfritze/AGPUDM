package com.example.dmocl;

import android.os.Build;

import java.io.FileWriter;
import java.text.SimpleDateFormat;
import java.util.Date;

public final class LinkToFile {

  static synchronized void DMwriteToFileheader(String fn) {
    DMwriteToFileheader(fn, false);
  }


  static synchronized void DMwriteToFileheader(String fn, boolean append) {

    try {

      FileWriter myWriter = new FileWriter(fn, append);
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
  }

  static synchronized void DMwriteToFile(String fn, String s) {

    try {

      FileWriter myWriter = new FileWriter(fn, true);
      myWriter.write(s + "\n");
      myWriter.flush();
      myWriter.close();

    } catch (Exception e) {
      String se = e.toString();
    }
  }

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
  }

  static synchronized void DMwriteToFile(String fn, int cores, double[] wct,
                                         int size, int cluno,
                                         int features, int zotti) {

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
          zotti + "\n"
      );
      myWriter.flush();
      myWriter.close();
    } catch (Exception e) {
      String se = e.toString();
    }
  }

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
  }

}

