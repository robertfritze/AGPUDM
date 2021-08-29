package com.example.dmocl;


import android.content.Context;
import android.os.Handler;
import android.widget.Button;
import android.widget.TextView;

import androidx.work.Constraints;
import androidx.work.Data;
import androidx.work.ExistingWorkPolicy;
import androidx.work.OneTimeWorkRequest;
import androidx.work.Operation;
import androidx.work.WorkContinuation;
import androidx.work.WorkManager;
import androidx.work.WorkRequest;

import com.google.common.util.concurrent.ListenableFuture;

import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedList;
import java.util.Random;
import java.util.concurrent.CancellationException;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executor;
import java.util.concurrent.TimeUnit;

import static com.example.dmocl.LinkToFile.logToFile;

public class submitjobs extends immediatejobs {


  private Executor executor;
  private final Handler resultHandler;
  private Context context;
  private String gpupath;
  private boolean doexport;
  private boolean dolog;
  private boolean appendresults;
  private String logfn;
  private String fn;
  private String xmlmode;
  private int xmlcluno;
  private int passes;
  private int xmlclusi;
  private int xmlfeatures;
  private float xmlkmeps;
  private float xmldbscaneps;
  private int xmldbscanneig;
  private boolean GPUfound;
  private int threads;
  private TextView jobinfo;
  private Button startbutton;

  public submitjobs(
    Handler resultHandler,
    Executor executor,
    Context context,
    TextView jobinfo,
    Button startbutton,

    String GPUpath,
    boolean doexport,
    boolean dolog,
    boolean appendresults,
    String logfn,
    String fn,
    String mode,
    int clusterno,
    int passes,
    int clusi,
    int features,
    String kmeanseps,
    String dbscaneps,
    int dbscanneigh,
    boolean GPUfound,
    int cores

  ) {
    this.resultHandler = resultHandler;
    this.executor = executor;
    this.context = context;
    this.startbutton = startbutton;
    this.jobinfo = jobinfo;
    this.gpupath = GPUpath;
    this.doexport = doexport;
    this.dolog = dolog;
    this.appendresults = appendresults;
    this.logfn = logfn;
    this.fn = fn;
    this.xmlmode = mode;
    this.xmlcluno = clusterno;
    this.passes = passes;
    this.xmlclusi = clusi;
    this.xmlfeatures = features;
    this.threads = cores;

    try {
      this.xmlkmeps = Float.parseFloat(kmeanseps);
    } catch (Exception e) {
      this.xmlkmeps = -1;
    }

    try {
      this.xmldbscaneps = Float.parseFloat(dbscaneps);
    } catch (Exception e) {
      this.xmldbscaneps = -1;
    }

    this.xmldbscanneig = dbscanneigh;
    this.GPUfound = GPUfound;
  }

  public void startcalculations() {


    if (threads < 0) {

      String s =
        "Invalid value for threads (" + threads + ").\n" +
          "Allowed values are 0 <= threads (0=maximum number of cores)\n";

      if (dolog) {
        logToFile(logfn, s, true);
      }

    } else {


      if (threads == 0) {
        threads = Runtime.getRuntime().availableProcessors();
      }


      if (!xmlmode.equals("fixed") && !xmlmode.equals("dynamic")) {

        String s =
          "Invalid mode in xml file (" + xmlmode + ").\n" +
            "Allowed values are \"fixed\" and \"dynamic\".\n";

        if (dolog) {
          logToFile(logfn, s, true);
        }

      } else {


        if ((xmlcluno < 0) | (xmlcluno > 50)) {

          String s =
            "Invalid clusters value in xml file (" + xmlcluno + ")\n" +
              "Allowed values are 0 <= x < 50 (0=random number)\n";

          if (dolog) {
            logToFile(logfn, s, true);
          }

        } else {

          if (passes <= 0) {

            String s = "Invalid passes value in xml file (" + passes + ")\n" +
              "Allowed values are values > 0\n";

            if (dolog) {
              logToFile(logfn, s, true);
            }

          } else {

            if ((xmlclusi < 0) | (xmlclusi > 100000)) {

              String s = "Invalid cluster size in xml file (" + xmlclusi + ")\n" +
                "Allowed values are values 0 <= x <= 100000 (0=random value)\n";

              if (dolog) {
                logToFile(logfn, s, true);
              }

            } else {

              if ((xmlfeatures < 0) | (xmlfeatures > 10)) {

                String s =
                  "Invalid feature value xml file (" + xmlfeatures + ")\n" +
                    "Allowed values are 0 <= x <= 10 (0=random value)\n";

                if (dolog) {
                  logToFile(logfn, s, true);
                }

              } else {

                int weiter = 0;

                try {

                  if (xmlkmeps <= 0) {
                    String s = "Invalid kmeans eps value xml file\n" +
                      "Allowed values are values 0.0 < x\n";

                    if (dolog) {
                      logToFile(logfn, s, true);
                    }

                    weiter = 1;

                  }
                } catch (Exception e) {

                  String s = "Invalid kmeans eps value in xml file\n" +
                    "Allowed values are values 0.0 < x\n";

                  if (dolog) {
                    logToFile(logfn, s, true);
                  }

                  weiter = 1;
                }

                if (weiter == 0) {

                  int weiter1 = 0;

                  float xmldbscaneps = 0;

                  try {

                    if (xmldbscaneps < 0) {
                      String s = "Invalid dbscan eps value xml file\n" +
                        "Allowed values are values 0 <= x (0.0=sqrt(features))\n";

                      if (dolog) {
                        logToFile(logfn, s, true);
                      }

                      weiter1 = 1;
                    }


                  } catch (Exception e) {

                    String s = "Invalid dbscan eps value xml file\n" +
                      "Allowed values are values 0 <= x (0.0=sqrt(features))\n";

                    if (dolog) {
                      logToFile(logfn, s, true);
                    }

                    weiter1 = 1;
                  }

                  if (weiter1 == 0) {


                    if (xmldbscanneig < 0) {

                      String s =
                        "Invalid neigh value in xml file (" + xmldbscanneig + ")\n" +
                          "Allowed values are values 0 <= x (0=10*features)\n";

                      if (dolog) {
                        logToFile(logfn, s, true);
                      }

                    } else {

                      Constraints constraints = new Constraints.Builder()
                        //.setRequiresCharging(true)
                        //.setRequiresDeviceIdle(true)
                        .build();

                      OneTimeWorkRequest myWorkRequest =
                        new OneTimeWorkRequest.Builder(dataminingtask.class)
                          .setConstraints(constraints)
                          .addTag("AndroidOCL")
                          .setInputData(
                            new Data.Builder()
                              .putBoolean("appendresults",appendresults)
                              .putString("mode",xmlmode)
                              .putInt("passes", passes)
                              .putInt("features", xmlfeatures)
                              .putInt("clusi", xmlclusi)
                              .putFloat("kmeps", xmlkmeps)
                              .putFloat("dbscaneps", xmldbscaneps)
                              .putInt("kk", xmldbscanneig)
                              .putInt("cluno", xmlcluno )
                              .putBoolean("GPUfound", GPUfound)
                              .putString("GPUPath", gpupath)
                              .putInt("cores", threads)
                              .putBoolean("doexport", doexport)
                              .putString("fn", fn)
                              .putBoolean("dolog", dolog)
                              .putString("logfn", logfn)
                              .build()
                          )
                          .keepResultsForAtLeast(1, TimeUnit.MILLISECONDS)
                          .build();

                      WorkContinuation wc = WorkManager.getInstance(context).beginUniqueWork(
                        "AndroidDMOCL",
                        ExistingWorkPolicy.REPLACE,
                        myWorkRequest
                      );


                      Operation opstate = wc.enqueue();

                      ListenableFuture<Operation.State.SUCCESS> lfos = null;
                      int item = 0;

                      try {
                        lfos = opstate.getResult();
                        lfos.get();
                      } catch (CancellationException | ExecutionException | InterruptedException e) {
                        item = 1;
                      }

                      jobinfo.post(new Runnable() {
                        @Override
                        public void run() {
                          jobinfo.setText("");
                        }
                      });

                      if (lfos != null) {
                        if (!lfos.isCancelled()) {
                          if (item == 0) {
                            startbutton.post(new Runnable() {
                              @Override
                              public void run() {
                                startbutton.setTextColor(context.getResources().getColor(R.color.colorBlack));
                                startbutton.setText(context.getResources().getString(R.string.startbuttoncancel));
                                startbutton.setFocusable(true);
                                startbutton.setEnabled(true);
                              }
                            });

                          } else {
                            item = 1;
                          }
                        } else {
                          item = 1;
                        }
                      } else {
                        item = 1;
                      }

                      if (item == 1) {

                        startbutton.post(new Runnable() {
                          @Override
                          public void run() {
                            startbutton.setTextColor(context.getResources().getColor(R.color.colorRed));
                            startbutton.setText(context.getResources().getString(R.string.startbuttonfail));
                            startbutton.setFocusable(false);
                            startbutton.setEnabled(false);
                          }
                        });
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  public void startsubmitjobs(
    final RepositoryCallback<jobschedresponse> callback
  ) {

    executor.execute(new Runnable() {
      @Override
      public void run() {

        startcalculations();

        Result.SubmitDone<jobschedresponse> result = new Result.SubmitDone<>();
        notifyResult(result, callback, resultHandler);

      }
    });
  }
}



