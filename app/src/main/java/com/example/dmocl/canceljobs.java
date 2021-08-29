package com.example.dmocl;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.Observer;
import androidx.work.Operation;
import androidx.work.WorkInfo;
import androidx.work.WorkManager;

import com.google.common.util.concurrent.ListenableFuture;

import java.util.List;
import java.util.concurrent.CancellationException;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executor;


public class canceljobs extends immediatejobs {


  private Executor executor;
  private final Handler resultHandler;
  private Context context;
  private TextView jobinfo;
  private Button startbutton;


  public canceljobs(
    Handler resultHandler,
    Executor executor,
    Context context,
    TextView jobinfo,
    Button startbutton
  ) {
    this.resultHandler = resultHandler;
    this.executor = executor;
    this.context = context;
    this.jobinfo = jobinfo;
    this.startbutton = startbutton;
  }

  public void startcanceljobs(
    final RepositoryCallback<jobschedresponse> callback
  ) {

    executor.execute(new Runnable() {
      @Override
      public void run() {

        Operation opstate = WorkManager.getInstance(context).cancelUniqueWork("AndroidDMOCL");
        ListenableFuture<Operation.State.SUCCESS> lfos = null;
        int item=0;

        try {
          lfos = opstate.getResult();
          lfos.get();
        }
        catch( CancellationException | ExecutionException | InterruptedException e ) {
          item=1;
        }

        jobinfo.post(new Runnable(){
          @Override
          public void run(){
            jobinfo.setText( "" );
          }
        });

        if (lfos != null) {
          //if (!lfos.isCancelled()) {
            if (item==0) {
              startbutton.post(new Runnable(){
                @Override
                public void run(){
                  startbutton.setTextColor( context.getResources().getColor(R.color.colorBlack));
                  startbutton.setText( context.getResources().getString( R.string.startbuttonsubmit ) );
                  startbutton.setFocusable(true);
                  startbutton.setEnabled(true);
                }
              });

            }
            else {
              item = 1;
            }
          //}
          //else{
          //  item = 1;
          //}
        }
        else {
          item = 1;
        }

        if (item==1) {

          startbutton.post(new Runnable(){
            @Override
            public void run(){
              startbutton.setTextColor( context.getResources().getColor(R.color.colorRed));
              startbutton.setText( context.getResources().getString( R.string.startbuttonfail ) );
              startbutton.setFocusable(false);
              startbutton.setEnabled(false);
            }
          });

        }


//          Handler handler = new Handler(Looper.getMainLooper());
//          handler.post( new Runnable() {
//            public void run() {


/*
                WorkManager instance = WorkManager.getInstance();
                WorkManager.getInstance(context).pruneWork();
                ListenableFuture<List<WorkInfo>> statuses = instance.getWorkInfosForUniqueWork("AndroidDMOCL");
                int running = 0;
                int waiting = 0;
                try {
                  List<WorkInfo> workInfoLi = statuses.get();
                  for (WorkInfo workInfo : workInfoLi) {
                    WorkInfo.State state2 = workInfo.getState();
                    running += (state2 == WorkInfo.State.RUNNING) ? 1 : 0;
                    waiting += (state2 == WorkInfo.State.ENQUEUED | state2 == WorkInfo.State.BLOCKED) ? 1 : 0;
                  }
                } catch (Exception e) {
                  throw new MainActivity.WorkManagerNoInformationException();
                }

                if ((running + waiting) > 0) {
                  startbutton.setText(context.getResources().getString(R.string.startbuttoncancel));
                  jobinfo.setText(context.getResources().getString(R.string.jobinfobanner, running, waiting));
                } else {
                  startbutton.setText(context.getResources().getString(R.string.startbuttonsubmit));
                  jobinfo.setText(context.getResources().getString(R.string.nocalcsubm));
                }

                startbutton.setEnabled(true);
                startbutton.setFocusable(true);
                startbutton.setTextColor(context.getResources().getColor(R.color.colorBlack));


//          }

//          });
        } catch (Exception e) {
          e.printStackTrace();
        }


        LiveData<Operation.State> ldos = opstate.getState();

        Handler handler = new Handler(Looper.getMainLooper());
        handler.post( new Runnable() {
          public void run() {

            ldos.observeForever( new Observer<Operation.State>(){

              @Override
              public void onChanged(@Nullable Operation.State state){

                jobinfo.setText( "" );
                startbutton.setText( context.getResources().getString( R.string.startbuttonsubmit) );                startbutton.setEnabled(true);
                startbutton.setFocusable(true);
                startbutton.setEnabled(true);

                ldos.removeObserver( this );
              }
            }
          );
          }
        });
*/
        //WorkManager.getInstance(context).pruneWork();

        Result.CancelDone<jobschedresponse> result = new Result.CancelDone<>();
        notifyResult( result, callback, resultHandler );

      }
    });
  }


}


