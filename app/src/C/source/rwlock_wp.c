/*!
 \file rwlock_wp.c
 \brief A writer preferred reader/writer lock
 \details
 This file implements a writer preferred reader/writer lock. The lock is reentrant for the
 readers and exclusive for the writers. Once a writer is waiting all readers that have
 acquired a reader lock are allowed to finish but new readers have to queue up until the
 writer has finished.
 \copyright Copyright Robert Fritze 2021
 \license MIT
 \version 1.0
 \author Robert Fritze
 \date 11.9.2021
 */


#include "rwlock_wp.h"

// see header file for details
void rwlockwp_reader_acquire( volatile struct rwlockwp* rwl ){

  pthread_mutex_lock( &rwl->g );              // acquire lock

                                       // wait while there are writers
                                       // waiting or active
  while ((rwl->num_writers_waiting>0) || (rwl->writer_active>0)){
    pthread_cond_wait( &rwl->c, &rwl->g );
  }

  rwl->num_reader_active++;                // one more reader

  pthread_mutex_unlock( &rwl->g );      // release lock

} // rwlockwp_reader_acquire

// see header file for details
void rwlockwp_reader_release( volatile struct rwlockwp* rwl ){

  pthread_mutex_lock( &rwl->g );    // acquire lock

  rwl->num_reader_active--;          // one reader less

  if (rwl->num_reader_active == 0){    // was the last active reader?

    pthread_cond_broadcast( &rwl->c );   // wake up all waiting writers

  }

  pthread_mutex_unlock( &rwl->g );     // release lock

} // rwlockwp_reader_release



// see header file for details
void rwlockwp_writer_acquire( volatile struct rwlockwp* rwl ){

  pthread_mutex_lock( &rwl->g );         // acquire lock
  rwl->num_writers_waiting++;          // one more writer WAITING

                         // wait until there are no more active
                         // readers and no more active writers
  while ((rwl->num_reader_active>0) || (rwl->writer_active>0)){
    pthread_cond_wait( &rwl->c, &rwl->g );
  }

  rwl->num_writers_waiting--;       // not any more waiting
  rwl->writer_active = 1;            // ... BUT ACTIVE

  pthread_mutex_unlock( &rwl->g );   // release lock

}  // rwlockwp_writer_acquire



// see header file for details
void rwlockwp_writer_release( volatile struct rwlockwp* rwl ){

  pthread_mutex_lock( &rwl->g );    // acquire lock

  rwl->writer_active = 0;            // writer is not any more active

  pthread_cond_broadcast( &rwl->c );   // wake up waiting readers and writers

  pthread_mutex_unlock( &rwl->g );   // release lock

} // rwlockwp_writer_release


