/*!
 @file canceljobs.java
 @brief cancel submitted jobs
 @details
 The class in this file is used to cancel submitted jobs
 @copyright Copyright Robert Fritze 2021
 @license MIT
 @version 1.0
 @author Robert Fritze
 @date 11.9.2021
 */
package com.example.dmocl;

import android.content.Context;
import android.os.Handler;
import android.widget.Button;
import android.widget.TextView;

import androidx.work.Operation;
import androidx.work.WorkManager;

import com.google.common.util.concurrent.ListenableFuture;

import java.util.concurrent.CancellationException;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executor;

/*!
@brief cancel submitted jobs
@details
 The class is used to cancel submitted jobs
*/
public class canceljobs extends immediatejobs {


  private Executor executor;                  //!< for submitting background immediate tasks
  private final Handler resultHandler;        //!< reference to result handler
  private Context context;                    //!< the context of the app
  private TextView jobinfo;                   //!< reference to line in app
  private Button startbutton;                 //!< reference to start button

  /*!
  @brief default class constructor
  @details
  Constructor that copies the variables.
  @param resultHandler handler for the results
  @param executor
  @param context context of the app
  @param jobinfo reference to line on screen
  @param startbutton reference to start button
  */
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
  } // canceljobs


  /*!
  @brief cancel jobs
  @details
  Cancel jobs as immediate background task
  @param callback a reference for the result callback
  */
  public void startcanceljobs(
    final RepositoryCallback<jobschedresponse> callback
  ) {

                 // execute background immediate task
    executor.execute(new Runnable() {
      @Override
      public void run() {

                // subit cancel job to WorkManager
        Operation opstate = WorkManager.getInstance(context).cancelUniqueWork("AndroidDMOCL");
        ListenableFuture<Operation.State.SUCCESS> lfos = null;      // variable for successfull jobs
        int item=0;             // error flag

        try {
          lfos = opstate.getResult();           // get operation state
          lfos.get();
        }
        catch( CancellationException | ExecutionException | InterruptedException e ) {
          item=1;                // error
        }

                     // clear text of job info
        jobinfo.post(new Runnable(){
          @Override
          public void run(){
            jobinfo.setText( "" );
          }
        });

                      // job cancellation successfull?
        if (lfos != null) {

          if (item==0) {          // no error

                    // change startbutton test to submit
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
            item = 1;           // error
          }
        }
        else {
          item = 1;             // error
        }

        if (item==1) {           // if error?

          // disable start button
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
                     // generate class for results done
        Result.CancelDone<jobschedresponse> result = new Result.CancelDone<>();
        notifyResult( result, callback, resultHandler );   // notify results

      }
    });
  }  // startcanceljobs


} // canceljobs


