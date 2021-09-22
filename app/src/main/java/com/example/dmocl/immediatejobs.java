/*!
 @file immediatejobs.java
 @brief Prototype for result notification
 @details
 A prototype for result notification.
 @copyright Copyright Robert Fritze 2021
 @license MIT
 @version 1.0
 @author Robert Fritze
 @date 11.9.2021
 */
package com.example.dmocl;

import android.os.Handler;

/*!
 @brief Prototype for result notification
 @details
 A prototype for result notification.
 */
public abstract class immediatejobs {


  /*!
   @brief A dummy class for the results
   @details
   A dummy class for the results
   */
  class jobschedresponse {
  }

  /*!
  @brief forwards the results to the result callback
  @details
  forwards the results to the result callback
  @param result result to forward
  @param callback callback function for the results
  @param resultHandler reference to the result handler
  */
  protected void notifyResult(
    final Result<canceljobs.jobschedresponse> result,
    final RepositoryCallback<canceljobs.jobschedresponse> callback,
    final Handler resultHandler
  ) {

          // call the callback function for result notification
    resultHandler.post(new Runnable() {
      @Override
      public void run() {
        callback.onComplete(result);
      }
    });

  } // notifyResult


} // immediatejobs
