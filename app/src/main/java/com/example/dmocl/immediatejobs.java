package com.example.dmocl;

import android.os.Handler;

public abstract class immediatejobs {

  class jobschedresponse {
  }

  protected void notifyResult(
    final Result<canceljobs.jobschedresponse> result,
    final RepositoryCallback<canceljobs.jobschedresponse> callback,
    final Handler resultHandler
  ) {

    resultHandler.post(new Runnable() {
      @Override
      public void run() {
        callback.onComplete(result);
      }
    });
  }
}
