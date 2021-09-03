package com.example.dmocl;

import android.annotation.SuppressLint;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.os.Build;
import android.os.PowerManager;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.core.app.NotificationCompat;
import androidx.work.Data;
import androidx.work.ForegroundInfo;
import androidx.work.WorkManager;
import androidx.work.Worker;
import androidx.work.WorkerParameters;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedList;
import java.util.Random;
import java.util.concurrent.atomic.AtomicBoolean;


import static android.content.Context.NOTIFICATION_SERVICE;
import static android.content.Context.POWER_SERVICE;
import static android.os.Trace.beginAsyncSection;
import static android.os.Trace.endAsyncSection;
import static com.example.dmocl.LinkToFile.DMwriteToFile;
import static com.example.dmocl.LinkToFile.logToFile;
import static com.example.dmocl.dbscan.dbscanabort;
import static com.example.dmocl.dbscan.dbscanresume;
import static com.example.dmocl.kmeans.kmabort;
import static com.example.dmocl.kmeans.kmresume;



public class dataminingtask extends Worker {

  private final int WAKELOCKTIMEOUT = 1000*60*15;

  private AtomicBoolean abortCalc = new AtomicBoolean(false);
  private Context context;

  private NotificationManager notificationManager;
  Notification notification;

  public static final String[] prependnames = {"Java", "C", "C+GPU", "Java+Threads", "C+Threads"};

  private final class dmitem {

    int clusterno;
    int features;
    int[] clusi;
    float kmeps;
    float dbscaneps;
    int kk;


    public dmitem(int clusterno, int[] clusi, int features, float kmeps,
                  float dbscaneps, int kk) {

      this.clusterno = clusterno;
      this.features = features;
      this.kmeps = kmeps;
      this.clusi = clusi.clone();
      this.dbscaneps = dbscaneps;
      this.kk = kk;
    }
  }


  public static final String compileprogressoutput(
    String fn,
    int z,
    int meth,
    int cores,
    int clusterno,
    short[] b,
    double wct
  ) {
    int[] clusi = null;

    if (clusterno > 0) {
      clusi = new int[clusterno];

      for (int i1 = 0; i1 < b.length; i1++) {
        if (b[i1]>=clusterno){

          String essi =  "clusterno:" + String.valueOf(clusterno) + "\n" +
          "i1:" + String.valueOf(i1) + "\n" +
          "b[i1]:" + String.valueOf(b[i1]) + "\n" +
          "cores:" + String.valueOf(cores) + "\n" +
          "z:" + String.valueOf(z) + "\n" +
          "meth:" + String.valueOf(meth) + "\n";

          DMwriteToFile( fn, essi );

        }

        clusi[b[i1]]++;
      }
    }

    String prepend = "KMEANS";

    if (z == 1) {
      prepend = "DBSCAN";
    }

    String s = "[" + prepend + " " + prependnames[meth] + "]: " + cores + " threads used.\n" +
      "[" + prepend + " " + prependnames[meth] + "] Clusters found: " + clusterno + "\n";

    if (clusterno >= 0) {

      for (int i1 = 0; i1 < clusterno; i1++) {
        s += "[" + prepend + " " + prependnames[meth] + "] Cluster #" + i1 + ": " + clusi[i1] + " points\n";
      }

      final int clusu = Arrays.stream(clusi).sum();

      s += "[" + prepend + " " + prependnames[meth] + "] Total points classified: " + clusu + "\n";
      s += "[" + prepend + " " + prependnames[meth] + "] Wall clock time: " + wct + " sec\n";

    }

    return (s);
  }

  private float[] createRandomClusters(int[] clustersize, int features) {

    final int clusterno = clustersize.length;

    LinkedList<float[]> floatList = new LinkedList<>();
    Random r = new Random();

    for (int i1 = 0; i1 < clusterno; i1++) {

      float[] fm = new float[features];
      float[] fd = new float[features];

      for (int i2 = 0; i2 < features; i2++) {

        fm[i2] = (float) r.nextGaussian() * 10;
        fd[i2] = (float) (r.nextGaussian() * 2.5);

      }

      for (int i2 = 0; i2 < clustersize[i1]; i2 += 1) {

        float[] rf2 = new float[features];

        for (int i3 = 0; i3 < features; i3 += 1) {
          rf2[i3] = fm[i3] + (float) r.nextGaussian() * fd[i3];
        }

        floatList.add(rf2);

      }
    }

    Collections.shuffle(floatList);

    float[] rf = new float[floatList.size() * features];
    int counter = 0;

    while (floatList.size() > 0) {

      float[] af = floatList.poll();

      for (int i1 = 0; i1 < features; i1++) {
        rf[counter] = af[i1];
        counter++;
      }

    }

    return (rf);

  }


  public dataminingtask(

    @NonNull Context context,
    @NonNull WorkerParameters params

  ) {
    super(context, params);
    this.context = context;
    notificationManager = (NotificationManager)
      context.getSystemService(NOTIFICATION_SERVICE);

    setProgressAsync(
      new Data.Builder()
        .putInt("PROGRESS", 0)
        .putInt("SIZE",-1)
        .putInt("DONE",0)
        .build()
    );

  }

  @Override
  public void onStopped() {

    super.onStopped();
    abortCalc.set(true);
    dbscanabort();
    kmabort();

    final boolean doexport = getInputData().getBoolean("doexport", false);
    final boolean dolog = getInputData().getBoolean("doexport", false);
    final String logfn = getInputData().getString("logfn");
    final String fn = getInputData().getString("fn");

    if (doexport) {
      DMwriteToFile(fn, "Job cancelled");
    }

    if (dolog) {
      logToFile(logfn, "Job cancelled.", true);
    }

  }



  @NonNull
  private ForegroundInfo createForegroundInfo(@NonNull String progress) {
    // Build a notification using bytesRead and contentLength

    Context context = getApplicationContext();
    String id = "AndroidOCLNotChannel";
    String title = context.getString(R.string.app_header);
    String cancel = context.getString(R.string.startbuttoncancel);
    // This PendingIntent can be used to cancel the worker
    PendingIntent intent = WorkManager.getInstance(context)
      .createCancelPendingIntent(getId());

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      createNotificationChannel();
    }

    Notification notification = new NotificationCompat.Builder(context, id)
      .setSmallIcon(R.drawable.notification_icon)
      .setContentTitle(title)
      .setTicker(title)
      .setOngoing(true)
      .setSilent(true)
      .setContentText( progress )
      .setPriority(NotificationCompat.PRIORITY_DEFAULT)
      .addAction(android.R.drawable.ic_delete, cancel, intent)
      .build();


    return new ForegroundInfo(0,notification);
  }

  @RequiresApi(Build.VERSION_CODES.O)
  private void createNotificationChannel() {
      // Create the NotificationChannel, but only on API 26+ because
      // the NotificationChannel class is new and not in the support library
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        CharSequence name = "Android Data Mining OCL";
        int importance = NotificationManager.IMPORTANCE_DEFAULT;
        NotificationChannel channel = new NotificationChannel("AndroidOCLNotChannel", name, importance);
        // Register the channel with the system; you can't change the importance
        // or other notification behaviors after this
        notificationManager.createNotificationChannel(channel);
      }
  }
    // Create a Notification channel


  @SuppressLint("WakelockTimeout")
  @NonNull
  @Override
  public Result doWork() {



    String progress = "Started data mining job...";
    setForegroundAsync(createForegroundInfo(progress));

    PowerManager powerManager = (PowerManager) context.getSystemService(POWER_SERVICE);
    //PowerManager.WakeLock wakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,
    //  "MyApp::MyWakelockTag");

    PowerManager.WakeLock wakeLock1 = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,
      "MyApp::MyWakelockTag2");
    wakeLock1.acquire();



    abortCalc.set(false);
    dbscanresume();
    kmresume();

    String fn = "";
    boolean doexport = false;
    boolean dolog = false;
    String logfn = "";

    try {

      doexport = getInputData().getBoolean("doexport", false);
      dolog = getInputData().getBoolean("doexport", false);
      fn = getInputData().getString("fn");
      logfn = getInputData().getString("logfn");

      final boolean appendresults = getInputData().getBoolean("appendresults", true );

      final String xmlmode = getInputData().getString("mode");
      if (xmlmode == null){
        throw new IndexOutOfBoundsException( "mode not specified" );
      }

      final int xmlclusi = getInputData().getInt("clusi", -1 );
      if (xmlclusi==-1){
        throw new IndexOutOfBoundsException( "clustersize not specified" );
      }

      final int xmlfeatures = getInputData().getInt("features", -1);
      if (xmlfeatures <= 0) {
        throw new IndexOutOfBoundsException( "features<=0" );
      }

      final int xmlcluno = getInputData().getInt("cluno", -1);
      if (xmlcluno <= 0) {
        throw new IndexOutOfBoundsException( "features<=0" );
      }

      final float xmlkmeps = getInputData().getFloat("kmeps", -1);
      if (xmlkmeps <= 0) {
        throw new IndexOutOfBoundsException( "kmeps<=0" );
      }

      final float xmldbscaneps = getInputData().getFloat("dbscaneps", -1);
      if (xmldbscaneps < 0) {
        throw new IndexOutOfBoundsException( "dbscaneps <= 0" );
      }

      final int xmldbscanneig = getInputData().getInt("kk", -1);
      if (xmldbscanneig < 0) {
        throw new IndexOutOfBoundsException( "kk<=0" );
      }

      boolean GPUfound = getInputData().getBoolean("GPUfound", false);
      final String GPUpath = getInputData().getString("GPUPath");

      final int cores = getInputData().getInt("cores", -1);
      if (cores <= 0) {
        throw new IndexOutOfBoundsException( "cores <= 0" );
      }

      final int passes = getInputData().getInt("passes", 0);
      if (passes <= 0) {
        throw new IndexOutOfBoundsException( "passes <= 0");
      }

      if (oclwrap.getOclWrapper().loadOpenCL(GPUpath) < -1) {
        GPUfound = false;
      }

      Random ran = new Random();
      LinkedList<dmitem> theitems = new LinkedList<>();

      if (xmlmode.equals("fixed")) {

        for (int features = 1; features <= 4; features *= 2) {

          final float dbscaneps = (xmldbscaneps == 0) ? (float) (Math.sqrt(features)) : xmldbscaneps;
          final int kk = (xmldbscanneig == 0) ? 10 * features : xmldbscanneig;

          for (int clusterno = 2; clusterno <= 8; clusterno += 2) {

            int[] clusi = new int[clusterno];

            for (int clustersi = 128; clustersi <= 2048; clustersi *= 2) {

              Arrays.fill(clusi, clustersi);

              for (int pi = 0; pi < passes; pi++) {
                theitems.add(new dmitem(clusterno, clusi, features, xmlkmeps,
                  dbscaneps, kk));
              }

            }
          }
        }
      }
      else {

        int clusterno = xmlcluno;

        if (clusterno == 0) {
          clusterno = ran.nextInt(8) + 1;
        }

        int[] clusi = new int[clusterno];

        for (int i2 = 0; i2 < clusterno; i2++) {

          if (xmlclusi == 0) {
            clusi[i2] = (ran.nextInt(16) + 1) * 128;
          } else {
            clusi[i2] = xmlclusi;
          }

        }

        int features = xmlfeatures;
        if (features == 0) {
          features = ran.nextInt(8) + 1;
        }

        final float dbscaneps = (xmldbscaneps == 0) ? (float) (Math.sqrt(features)) : xmldbscaneps;
        final int kk = (xmldbscanneig == 0) ? 10 * features : xmldbscanneig;

        for (int pi = 0; pi < passes; pi++) {

          theitems.add(new dmitem(clusterno, clusi, features, xmlkmeps,
            dbscaneps, kk));

        }

      }


      Collections.shuffle(theitems);

      if (doexport) {
        LinkToFile.DMwriteToFileheader(fn, appendresults);
      }


      LinkedList<Integer> meth = new LinkedList<>(Arrays.asList(0, 1, 2, 3, 4));

      if (!GPUfound) {
        meth.remove(2);
      }

      final int themaxsize = theitems.size();
      int cnt = 0;

      while (theitems.size()>0) {

        cnt++;

        setProgressAsync(
          new Data.Builder()
            .putInt("PROGRESS", ((cnt-1)*100/themaxsize))
            .putInt("SIZE",themaxsize)
            .putInt("DONE",cnt)
            .build()
        );


        if (abortCalc.get()) {
          break;
        }

        dmitem dmi = theitems.pollFirst();

        Collections.shuffle(meth);

        float[] rf = createRandomClusters( dmi.clusi, dmi.features );

        final int clusu = Arrays.stream( dmi.clusi ).sum();

        String s =
          "----------------------------------------------\n" +
            "Mode: " + xmlmode + "\n" +
            "Clusters generated: " + dmi.clusi.length + "\n" +
            "Data items: " + clusu + "\n" +
            "Features: " + dmi.features + "\n";

        for (int i2 = 0; i2 < dmi.clusi.length; i2++) {
          s += "Cluster #" + i2 + ": " + dmi.clusi[i2] + "\n";
        }

        if (dolog) {
          logToFile(logfn, s, true);
        }


        short[] b0 = new short[clusu];
        Arrays.fill( b0, (short) -1);
        short[] b1 = new short[clusu];
        Arrays.fill( b1, (short) -1);
        short[] b2 = new short[clusu];
        Arrays.fill( b2, (short) -1);
        short[] b3 = new short[clusu];
        Arrays.fill( b3, (short) -1);
        short[] b4 = new short[clusu];
        Arrays.fill( b4, (short) -1);
        short[] b5 = new short[clusu];
        Arrays.fill( b5, (short) -1);
        short[] b6 = new short[clusu];
        Arrays.fill( b6, (short) -1);
        short[] b7 = new short[clusu];
        Arrays.fill( b7, (short) -1);
        short[] b8 = new short[clusu];
        Arrays.fill( b8, (short) -1);
        short[] b9 = new short[clusu];
        Arrays.fill( b9, (short) -1);

        double[] wct = new double[16];
        Arrays.fill(wct, -1);


        for (int i2 = 0; i2 < meth.size(); i2++) {

          if (abortCalc.get()) {
            break;
          }

          switch (meth.get(i2)) {

            case 0: {

              //Log.i( "--->", "(" + (cnt-1) + ") KMEANS 0");
              //wakeLock.acquire( WAKELOCKTIMEOUT );

              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                beginAsyncSection( "KMEANS - JAVA", 0 );
              }
              long t1 = System.nanoTime();
              short cluno = kmeans.kmeans_st(b5, rf, dmi.kmeps, dmi.clusterno, dmi.features);
              t1 = System.nanoTime() - t1;
              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                endAsyncSection( "KMEANS - JAVA", 0 );
              }

              //wakeLock.release();

              wct[0] = ((double) t1) / 1000000000;
              String s1 = "";

              if (cluno>=0){
                s1 = compileprogressoutput( fn,
                  0, 0, 1, dmi.clusterno, b5, wct[0] );
              }
              else {
                s1 = "KMEANS_JAVA aborted (" + cluno + ")";
              }

              if (dolog) {
                logToFile(logfn, s1, true);
              }

              break;
            }

            case 1: {
              //Log.i( "--->", "(" + (cnt-1) + ") KMEANS 1");
              //wakeLock.acquire( WAKELOCKTIMEOUT );

              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                beginAsyncSection( "KMEANS - C", 0 );
              }
              long t1 = System.nanoTime();
              short cluno = kmeans.kmeans_c(b6, rf, dmi.kmeps, dmi.clusterno, dmi.features);
              t1 = System.nanoTime() - t1;
              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                endAsyncSection( "KMEANS - C", 0 );
              }

              //wakeLock.release();

              wct[1] = ((double) t1) / 1000000000;

              String s1 = "";

              if (cluno>=0){
                s1 = compileprogressoutput( fn,
                  0, 1, 1, dmi.clusterno, b6, wct[1] );
              }
              else {
                s1 = "KMEANS_C aborted (" + cluno + ")";
              }

              if (dolog) {
                logToFile(logfn, s1, true);
              }

              break;
            }
            case 2: {
              //Log.i( "--->", "(" + (cnt-1) + ") KMEANS 2");
              //wakeLock.acquire( WAKELOCKTIMEOUT );

              long[] ej = new long[1];

              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                beginAsyncSection( "KMEANS - C+GPU", 0 );
              }
              long t1 = System.nanoTime();
              short cluno = kmeans.kmeans_c_gpu(b7, rf, dmi.kmeps, dmi.clusterno, dmi.features, ej );
              t1 = System.nanoTime() - t1;
              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                endAsyncSection( "KMEANS - C+GPU", 0 );
              }

              //wakeLock.release();

              wct[2] = ((double) t1) / 1000000000;
              wct[10] = ((double) ej[0])/ 1000000000;

              String s1 = "";

              if (cluno>=0){
                s1 = compileprogressoutput( fn,
                  0, 2, 1, dmi.clusterno, b7, wct[2] );
              }
              else {
                s1 = "KMEANS_C_GPU aborted (" + cluno + ")";
              }

              if (dolog) {
                logToFile(logfn, s1, true);
              }

              break;
            }
            case 3: {
              //Log.i( "--->", "(" + (cnt-1) + ") KMEANS 3");
              //wakeLock.acquire( WAKELOCKTIMEOUT );

              long[] ej = new long[1];

              long t1 = 0;
              short cluno = 0;

              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                beginAsyncSection( "KMEANS - JAVA (mt)", 0 );
              }

              try {

                t1 = System.nanoTime();
                cluno = kmeans.kmeans_threads(b8, rf, dmi.kmeps, dmi.clusterno, dmi.features, cores, ej );
                t1 = System.nanoTime() - t1;

              } catch (Exception e) {
                if (dolog) {
                  logToFile(logfn, "KMEANS_JAVA_THREADS: Threads have been interrupted", true);
                }
                wct[3] = -1;
                wct[11] = -1;
                t1 = -1;
              }

              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                endAsyncSection( "KMEANS - JAVA (mt)", 0 );
              }

              //wakeLock.release();

              if (t1>=0) {
                wct[3] = ((double) t1) / 1000000000;
                wct[11] = ((double) ej[0]) / 1000000000;
                String s1 = "";

                if (cluno >= 0) {
                  s1 = compileprogressoutput(fn,
                    0, 3, cores, dmi.clusterno, b8, wct[3]);
                } else {
                  s1 = "KMEANS_JAVA_THREADS aborted (" + cluno + ")";
                }

                if (dolog) {
                  logToFile(logfn, s1, true);
                }
              }

              break;
            }
            case 4: {
              //Log.i( "--->", "(" + (cnt-1) + ") KMEANS 4");
              //wakeLock.acquire( WAKELOCKTIMEOUT );

              long[] ej = new long[1];

              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                beginAsyncSection( "KMEANS - C (mt)", 0 );
              }
              long t1 = System.nanoTime();
              short cluno = kmeans.kmeans_c_phtreads(b9, rf, dmi.kmeps, dmi.clusterno, dmi.features, cores, ej );
              t1 = System.nanoTime() - t1;
              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                endAsyncSection( "KMEANS - C (mt)", 0 );
              }

              //wakeLock.release();

              wct[4] = ((double) t1) / 1000000000;
              wct[12] = ((double) ej[0])/ 1000000000;
              String s1 = "";

              if (cluno>=0){
                s1 = compileprogressoutput( fn,
                  0, 4, cores, dmi.clusterno, b9, wct[4] );
              }
              else {
                s1 = "KMEANS_C_PTHREADS aborted (" + cluno + ")";
              }

              if (dolog) {
                logToFile(logfn, s1, true);
              }

              break;
            }
          }
        }


        for (int i2 = 0; i2 < meth.size(); i2++) {

          if (abortCalc.get()) {
            break;
          }

          switch (meth.get(i2)) {

            case 0: {
              //Log.i( "--->", "(" + (cnt-1) + ") DBSCAN 0");
              //wakeLock.acquire( WAKELOCKTIMEOUT );

              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                beginAsyncSection( "DBSCAN - JAVA", 0 );
              }
              long t1 = System.nanoTime();
              short cluno = dbscan.dbscan_st( b0, rf, dmi.dbscaneps, dmi.kk, dmi.features);
              t1 = System.nanoTime() - t1;
              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                endAsyncSection( "DBSCAN - JAVA", 0 );
              }

              //wakeLock.release();

              wct[5] = ((double) t1) / 1000000000;
              String s1 = "";

              if (cluno>=0){
                s1 = compileprogressoutput( fn,
                  1, 0, 1, cluno + 1, b0, wct[5] );
              }
              else {
                s1 = "DBSCAN_JAVA aborted (" + cluno + ")";
              }

              if (dolog) {
                logToFile(logfn, s1, true);
              }

              break;
            }
            case 1: {
              //Log.i( "--->", "(" + (cnt-1) + ") DBSCAN 1");
              //wakeLock.acquire( WAKELOCKTIMEOUT );

              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                beginAsyncSection( "DBSCAN - C", 0 );
              }
              long t1 = System.nanoTime();
              short cluno = dbscan.dbscan_c(b1, rf, dmi.dbscaneps, dmi.kk, dmi.features);
              t1 = System.nanoTime() - t1;
              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                endAsyncSection( "DBSCAN - C", 0 );
              }

              //wakeLock.release();

              wct[6] = ((double) t1) / 1000000000;
              String s1 = "";

              if (cluno>=0){
                s1 = compileprogressoutput( fn,
                  1, 1, 1, cluno + 1, b1, wct[6] );
              }
              else {
                s1 = "DBSCAN_C aborted (" + cluno + ")";
              }

              if (dolog) {
                logToFile(logfn, s1, true);
              }

              break;
            }
            case 2: {
              //Log.i( "--->", "(" + (cnt-1) + ") DBSCAN 2");
              //wakeLock.acquire( WAKELOCKTIMEOUT );

              long[] ej = new long[1];

              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                beginAsyncSection( "DBSCAN - C+GPU", 0 );
              }
              long t1 = System.nanoTime();
              short cluno = dbscan.dbscan_c_gpu(b2, rf, dmi.dbscaneps, dmi.kk, dmi.features, ej );
              t1 = System.nanoTime() - t1;
              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                endAsyncSection( "DBSCAN - C+GPU", 0 );
              }

              //wakeLock.release();

              wct[7] = ((double) t1) / 1000000000;
              wct[13] = ((double) ej[0])/ 1000000000;
              String s1 = "";

              if (cluno>=0){
                s1 = compileprogressoutput( fn,
                  1, 2, 1, cluno + 1, b2, wct[7] );
              }
              else {
                s1 = "DBSCAN_C_GPU aborted (" + cluno + ")";
              }

              if (dolog) {
                logToFile(logfn, s1, true);
              }

              break;

            }
            case 3: {
              //Log.i( "--->", "(" + (cnt-1) + ") DBSCAN 3");
              //wakeLock.acquire( WAKELOCKTIMEOUT );

              long[] ej = new long[1];

              long t1 = 0;
              short cluno = 0;

              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                beginAsyncSection( "DBSCAN - JAVA (mt)", 0 );
              }

              try {
                t1 = System.nanoTime();
                cluno = dbscan.dbscan_threads(b3, rf, dmi.dbscaneps, dmi.kk, dmi.features, cores, ej );
                t1 = System.nanoTime() - t1;
              } catch (Exception e) {
                if (dolog) {
                  logToFile(logfn, "DBSCAN_JAVA_THREADS: Threads have been interrupted", true);
                }
                wct[8] = -1;
                wct[14] = -1;
                t1 = -1;
              }

              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                endAsyncSection( "DBSCAN - JAVA (mt)", 0 );
              }

              //wakeLock.release();

              if (t1>=0) {

                wct[8] = ((double) t1) / 1000000000;
                wct[14] = ((double) ej[0]) / 1000000000;
                String s1 = "";

                if (cluno >= 0) {
                  s1 = compileprogressoutput(fn,
                    1, 3, cores, cluno + 1, b3, wct[8]);
                } else {
                  s1 = "DBSCAN_JAVA_THREADS aborted (" + cluno + ")";
                }

                if (dolog) {
                  logToFile(logfn, s1, true);
                }
              }

              break;

            }
            case 4: {
              //Log.i( "--->", "(" + (cnt-1) + ") DBSCAN 4");
              //wakeLock.acquire( WAKELOCKTIMEOUT );

              long[] ej = new long[1];

              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                beginAsyncSection( "DBSCAN - C (mt)", 0 );
              }
              long t1 = System.nanoTime();
              short cluno = dbscan.dbscan_c_phtreads(b4, rf, dmi.dbscaneps, dmi.kk, dmi.features, cores, ej);
              t1 = System.nanoTime() - t1;
              if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                endAsyncSection( "DBSCAN - C (mt)", 0 );
              }

              //wakeLock.release();

              wct[9] = ((double) t1) / 1000000000;
              wct[15] = ((double) ej[0])/ 1000000000;
              String s1 = "";

              if (cluno>=0){
                s1 = compileprogressoutput( fn,
                  1, 4, cores, cluno + 1, b4, wct[9] );
              }
              else {
                s1 = "DBSCAN_C_THREADS aborted (" + cluno + ")";
              }

              if (dolog) {
                logToFile(logfn, s1, true);
              }

              break;
            }
          }
        }


        if (abortCalc.get()) {
          break;
        }
        else {


          int zz = 0;

          if (GPUfound) {
            if ((!Arrays.equals(b0, b1)) ||
              (!Arrays.equals(b1, b2)) ||
              (!Arrays.equals(b2, b3)) ||
              (!Arrays.equals(b3, b4)) ||
              (!Arrays.equals(b4, b0))) {
              zz = 1;
            }
          }
          else {
            if ((!Arrays.equals(b0, b1)) ||
              (!Arrays.equals(b1, b3)) ||
              (!Arrays.equals(b3, b4)) ||
              (!Arrays.equals(b4, b0))) {
              zz = 1;
            }
          }

          if (doexport) {
            DMwriteToFile(fn, cores, wct, clusu, dmi.clusterno, dmi.features, zz);
          }


        }
      }
    } catch (Exception e) {

      String s = e.getMessage();
      if (s==null){
        s= "none";
      }

      StringWriter sw = new StringWriter();
      PrintWriter pw = new PrintWriter(sw);
      e.printStackTrace(pw);
      String sStackTrace = sw.toString();

      if (doexport) {
        DMwriteToFile(fn, "Exception caught: " + e.toString() + "(" + s + ")" );
      }

      if (dolog) {
        logToFile(logfn, "Exception caught: " + e.toString() + "(" + s + ")\n" + sStackTrace, true);
      }
    }

    wakeLock1.release();

    notificationManager.cancelAll();

    return Result.success();
  }


}


