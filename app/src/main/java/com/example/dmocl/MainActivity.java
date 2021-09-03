package com.example.dmocl;

import android.annotation.SuppressLint;
import android.app.UiModeManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.View;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.os.HandlerCompat;
import androidx.lifecycle.Observer;
import androidx.work.Data;
import androidx.work.WorkInfo;
import androidx.work.WorkManager;

import com.google.common.util.concurrent.ListenableFuture;

import org.w3c.dom.Text;

import java.io.File;
import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicBoolean;

import static android.view.View.VISIBLE;


interface RepositoryCallback<T> {
  void onComplete(Result<T> result);
}


abstract class Result<T> {

  private Result() {
  }

  static final class CancelDone<T> extends Result<T> {

    CancelDone() {
    }
  }

  static final class SubmitDone<T> extends Result<T> {

    SubmitDone() {
    }
  }

}

class notifyuserresults implements RepositoryCallback {

  private static final Object LOCK = new Object();

  private TextView jobchangedinfo;

  notifyuserresults(
    TextView jci
  ) {
    this.jobchangedinfo = jci;
  }


  @Override
  public void onComplete(Result result) {

    if (result instanceof Result.CancelDone) {

      synchronized (LOCK) {
        jobchangedinfo.setText("");
      }

    } else if (result instanceof Result.SubmitDone) {

      synchronized (LOCK) {
        jobchangedinfo.setText("");
      }

    }
  }
}


public class MainActivity extends AppCompatActivity {


  static class WorkManagerNoInformationException extends Exception {
  }

  private final Object syncobj = new Object();    // for GPUpath
  String GPUpath = "";

  AtomicBoolean GPUfound = new AtomicBoolean(false);
  private final oclwrap owrp = oclwrap.getOclWrapper();

  ExecutorService executorService = Executors.newSingleThreadExecutor();
  Handler mainThreadHandler = HandlerCompat.createAsync(Looper.getMainLooper());


  private void updateLiveInfo( List<WorkInfo> statuses, TextView jobinfo, Button startbutton )
    throws WorkManagerNoInformationException {

    if (statuses != null) {

      int running = 0;
      WorkInfo thefirstworkinfo = null;

      for (WorkInfo workInfo : statuses) {
        WorkInfo.State state = workInfo.getState();
        final boolean zopo = (state == WorkInfo.State.RUNNING | state == WorkInfo.State.ENQUEUED | state == WorkInfo.State.BLOCKED);

        if (zopo) {
          running++;

          if (state == WorkInfo.State.RUNNING) {
            thefirstworkinfo = workInfo;
          } else {
            if (thefirstworkinfo == null) {
              thefirstworkinfo = workInfo;
            }
          }
        }
      }

      if (running == 0) {
        jobinfo.setText(getResources().getString(R.string.nocalcsubm));
        startbutton.setText(getResources().getString(R.string.startbuttonsubmit));
      } else if (running == 1) {
        Data progress = thefirstworkinfo.getProgress();
        final int value = progress.getInt("PROGRESS", -1);
        final int done = progress.getInt("DONE", -1);
        final int size = progress.getInt("SIZE", -1);

        if (size > 0) {
          jobinfo.setText(getResources().getString(R.string.jobinfobanner,
            done, size, value));
        } else {
          jobinfo.setText(getResources().getString(R.string.jobinfobanner2));
        }

        startbutton.setText(getResources().getString(R.string.startbuttoncancel));
      } else {
        throw new WorkManagerNoInformationException();
      }
    }
    else {
      throw new WorkManagerNoInformationException();
    }
  }


  protected synchronized boolean tryLoadGPU(String gpupath) {

    owrp.unloadOpenCL();

    boolean ret = false;

    int zonki = owrp.loadOpenCL(gpupath);

    if (zonki == 0) {
      if (owrp.AndrCLGetPlatformCnt() > 0) {

        if (owrp.AndrCLGetDeviceCnt(0) > 0) {

          ret = true;
        }
      }
    }

    if (!ret) {
      owrp.unloadOpenCL();
    }

    return (ret);
  }

  @Override
  protected void onResume() {

    super.onResume();

    Button startbutton = findViewById(R.id.startbutton);
    TextView jobinfo = findViewById(R.id.jobstatus);

    try {

      WorkManager instance = WorkManager.getInstance();
      WorkManager.getInstance(this).pruneWork();
      ListenableFuture<List<WorkInfo>> lifustatuses = instance.getWorkInfosForUniqueWork("AndroidDMOCL");
      List<WorkInfo> statuses = lifustatuses.get();

      updateLiveInfo( statuses, jobinfo, startbutton );

    } catch (Exception e) {
      jobinfo.setText(getResources().getString(R.string.nojobinfo));
      startbutton.setText(getResources().getString(R.string.startbuttonfail));
      startbutton.setTextColor(getResources().getColor(R.color.colorRed));
      startbutton.setFocusable(false);
      startbutton.setEnabled(false);
    }
  }

  @SuppressLint({"SourceLockedOrientationActivity", "BatteryLife"})
  @Override
  protected void onCreate(Bundle savedInstanceState) {

    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);

    //Log.i( "arch", Integer.toString(oclwrap.getArchitecture()) );

    UiModeManager uiModeManager = (UiModeManager) getSystemService(UI_MODE_SERVICE);
    if (uiModeManager.getCurrentModeType() != Configuration.UI_MODE_TYPE_TELEVISION) {
      startActivity(new Intent(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS,
        Uri.parse("package:" + getPackageName())));
    }

    final int viewmode = Integer.parseInt((String) findViewById(R.id.dmoclheader).getTag());

    Button exitbutton = findViewById(R.id.exitbutton);
    Button startbutton = findViewById(R.id.startbutton);
    Button searchgpubutton = findViewById(R.id.searchgpubutton);
    TextView nachricht = findViewById(R.id.infotext);
    nachricht.setText("");
    nachricht.setVisibility(VISIBLE);
    EditText editText = findViewById(R.id.GPUpathinput);
    TextView greatinfo = findViewById(R.id.greatinfotext);
    if (viewmode == 0) {
      greatinfo.setMovementMethod(new ScrollingMovementMethod());
    }
    TextView jobinfo = findViewById(R.id.jobstatus);


    final Animation in = new AlphaAnimation(0.0f, 1.0f);
    in.setDuration(1000);

    if (viewmode == 0) {
      String greatinfostr = "";

      final boolean doexport = getResources().getBoolean(R.bool.export);
      final boolean dolog = getResources().getBoolean(R.bool.log);

      if (doexport) {
        final String fn = getApplicationContext().getExternalFilesDir(null) + File.separator +
          getResources().getString(R.string.resultfilename);

        greatinfostr += "Results are written to " + fn + "\n";

        if (getResources().getBoolean(R.bool.append)) {
          greatinfostr += "Results are appended if file exists.\n";
        } else {
          greatinfostr += "Existing file is overwritten.\n";
        }
      } else {
        greatinfostr += "Results are not exported to csv-Table.\n";
      }

      if (dolog) {
        final String logfn = getApplicationContext().getExternalFilesDir(null) + File.separator +
          getResources().getString(R.string.logfilename);

        greatinfostr += "Log information is written to " + logfn + "\n";
      } else {
        greatinfostr += "Log information will not be saved.\n";
      }


      final String xmlmode = getResources().getString(R.string.mode);
      greatinfostr += "Calculation mode = " + xmlmode + "\n";

      final int runs = getResources().getInteger(R.integer.passes);
      greatinfostr += "Number of passes: " + runs + "\n";

      if (xmlmode.equals("dynamic")) {

        final int cluno = getResources().getInteger(R.integer.clusterno);
        final int clusi = getResources().getInteger(R.integer.clustersize);
        final int feat = getResources().getInteger(R.integer.features);

        greatinfostr += "Number of clusters: " + cluno + " (0=random value)\n" +
          "Size of clusters: " + clusi + " (0=random value)\n" +
          "Features: " + feat + " (0=random value)\n";

      }

      greatinfostr += "KMEANS: eps for cluster center displacement: " +
        getResources().getString(R.string.kmeanseps) + "\n";

      float dbscaneps = Float.parseFloat(getResources().getString(R.string.dbscaneps));

      if (xmlmode.equals("dynamic") || (dbscaneps == 0)) {
        greatinfostr += "DBSCAN: eps = sqrt(features)\n";
      } else {
        greatinfostr += "DBSCAN: eps = " + dbscaneps + "\n";
      }

      final int dbscanneig = getResources().getInteger(R.integer.dbscanneigh);

      if (xmlmode.equals("dynamic") || (dbscanneig == 0)) {
        greatinfostr += "DBSCAN: k = 10*features\n";
      } else {
        greatinfostr += "DBSCAN: k = " + dbscanneig + "\n";
      }


      greatinfostr += "Bounds of values are checked when calculations are launched.";
      greatinfo.setText(greatinfostr);
    }

    synchronized (syncobj) {
      editText.setEnabled(false);
      GPUpath = getResources().getString(R.string.defaultlibOpenCLpath);
      editText.setText(GPUpath, TextView.BufferType.EDITABLE);
      GPUfound.set(tryLoadGPU(GPUpath));
    }

    if (GPUfound.get()) {

      searchgpubutton.setEnabled(false);
      oclwrap.oclinforet oir = owrp.AndrCLgetDeviceName(0, 0);

      if (oir.result == 0) {
        DecimalFormatSymbols otherSymbols = new DecimalFormatSymbols();
        otherSymbols.setDecimalSeparator('.');
        otherSymbols.setGroupingSeparator(',');
        DecimalFormat decimalFormat = new DecimalFormat("#.0", otherSymbols);
        String numberAsString = decimalFormat.format(oir.oclversion);
        nachricht.setText(getResources().getString(R.string.ocldevfound,
          oir.s, numberAsString, oir.getDevType()));

      } else {
        nachricht.setText(getResources().getString(R.string.nococlinfo));
      }

    } else {
      nachricht.setText(getResources().getString(R.string.nococldevice));
      editText.setEnabled(true);
    }


    WorkManager.getInstance(this).getWorkInfosForUniqueWorkLiveData("AndroidDMOCL")
      .observe(this, new Observer<List<WorkInfo>>() {
        @Override
        public void onChanged(@Nullable List<WorkInfo> statuses) {

          Button startbutton = findViewById(R.id.startbutton);
          TextView jobinfo = findViewById(R.id.jobstatus);

          try {
            updateLiveInfo( statuses, jobinfo, startbutton );
          } catch (Exception e) {
            jobinfo.setText(getResources().getString(R.string.nojobinfo));
            startbutton.setText(getResources().getString(R.string.startbuttonfail));
            startbutton.setTextColor(getResources().getColor(R.color.colorRed));
            startbutton.setFocusable(false);
            startbutton.setEnabled(false);
          }
        }
      });


    searchgpubutton.setOnClickListener(new View.OnClickListener() {

      @Override
      public void onClick(View v) {

        EditText editText = findViewById(R.id.GPUpathinput);
        Button searchgpubutton = findViewById(R.id.searchgpubutton);
        TextView nachricht = findViewById(R.id.infotext);

        synchronized (syncobj) {
          editText.setEnabled(false);
          GPUpath = editText.getText().toString();

          if (GPUpath.equals("")) {
            GPUpath = getResources().getString(R.string.defaultlibOpenCLpath);
          }
          GPUfound.set(tryLoadGPU(GPUpath));
        }

        if (GPUfound.get()) {

          searchgpubutton.setEnabled(false);
          oclwrap.oclinforet oir = owrp.AndrCLgetDeviceName(0, 0);

          if (oir.result == 0) {
            DecimalFormatSymbols otherSymbols = new DecimalFormatSymbols();
            otherSymbols.setDecimalSeparator('.');
            otherSymbols.setGroupingSeparator(',');
            DecimalFormat decimalFormat = new DecimalFormat("#.0", otherSymbols);
            String numberAsString = decimalFormat.format(oir.oclversion);
            nachricht.setText(getResources().getString(R.string.ocldevfound, oir.s, numberAsString,
              oir.getDevType()));

          } else {
            nachricht.setText(getResources().getString(R.string.nococlinfo));
          }

        } else {
          nachricht.startAnimation(in);
          nachricht.setText(getResources().getString(R.string.nococldevice));
          editText.setEnabled(true);
        }
      }

    });


    startbutton.setOnClickListener(new View.OnClickListener() {

      @Override
      public void onClick(View v) {

        Button startbutton = findViewById(R.id.startbutton);
        TextView jobinfo = findViewById(R.id.jobchangestatus);

        startbutton.setEnabled(false);
        startbutton.setFocusable(false);
        startbutton.setTextColor(getResources().getColor(R.color.colorGrey70));

        try {

          WorkManager instance = WorkManager.getInstance();
          WorkManager.getInstance(v.getContext()).pruneWork();
          ListenableFuture<List<WorkInfo>> statuses = instance.getWorkInfosForUniqueWork("AndroidDMOCL");

          int running = 0;

          try {
            List<WorkInfo> workInfoLi = statuses.get();
            for (WorkInfo workInfo : workInfoLi) {
              WorkInfo.State state = workInfo.getState();
              running += (state == WorkInfo.State.RUNNING | state == WorkInfo.State.ENQUEUED | state == WorkInfo.State.BLOCKED) ? 1 : 0;
            }
          } catch (Exception e) {
            throw new WorkManagerNoInformationException();
          }


          if (running > 0) {
            jobinfo.setText(getResources().getString(R.string.canellingjobs));

            canceljobs cj = new canceljobs(
              mainThreadHandler,
              executorService,
              v.getContext(),
              jobinfo,
              startbutton
            );

            cj.startcanceljobs(new notifyuserresults(
              jobinfo
            ));


          } else {

            jobinfo.setText(getResources().getString(R.string.submittingjobs));

            String gpupath = "";

            synchronized (syncobj) {
              gpupath = GPUpath;
            }


            submitjobs sj = new submitjobs(
              mainThreadHandler,
              executorService,
              v.getContext(),
              jobinfo,
              startbutton,

              gpupath,
              getResources().getBoolean(R.bool.export),
              getResources().getBoolean(R.bool.log),
              getResources().getBoolean(R.bool.append),
              getApplicationContext().getExternalFilesDir(null) + File.separator +
                getResources().getString(R.string.logfilename),
              getApplicationContext().getExternalFilesDir(null) + File.separator +
                getResources().getString(R.string.resultfilename),
              getResources().getString(R.string.mode),
              getResources().getInteger(R.integer.clusterno),
              getResources().getInteger(R.integer.passes),
              getResources().getInteger(R.integer.clustersize),
              getResources().getInteger(R.integer.features),
              getResources().getString(R.string.kmeanseps),
              getResources().getString(R.string.dbscaneps),
              getResources().getInteger(R.integer.dbscanneigh),
              GPUfound.get(),
              getResources().getInteger(R.integer.threads)
            );

            sj.startsubmitjobs(new notifyuserresults(
              jobinfo
            ));

          }

        } catch (Exception e) {
          //jobinfo.setText( getResources().getString(R.string.nojobinfo) );
          startbutton.setTextColor(getResources().getColor(R.color.colorRed));
          startbutton.setText(getResources().getString(R.string.startbuttonfail));
          startbutton.setFocusable(false);
          startbutton.setEnabled(false);
        }
      }
    });


    exitbutton.setOnClickListener(new View.OnClickListener() {

      @Override
      public void onClick(View v) {

        Button exitbutton = findViewById(R.id.exitbutton);
        Button startbutton = findViewById(R.id.startbutton);
        Button searchgpubutton = findViewById(R.id.searchgpubutton);

        startbutton.setEnabled(false);
        exitbutton.setEnabled(false);
        searchgpubutton.setEnabled(false);
        finish();

      }

    });
  }


  @Override
  protected void onDestroy() {

    super.onDestroy();

  }

}
