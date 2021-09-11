/*!
 \file rwlock_wp.h
 \brief Header file for a writer preferred reader/writer lock
 \details
 Defines the struct needed for a writer preferred reader/writer lock.
 Read- and Writer locks can be acquired and released. A static initializer for the lock is provided.
 \copyright Copyright Robert Fritze 2021
 \license MIT
 \version 1.0
 \author Robert Fritze
 \date 11.9.2021
 */

#ifndef OPENCLAPP_RWLOCK_WP_H
#define OPENCLAPP_RWLOCK_WP_H

#include <pthread.h>

                              //! A struct thats holds all necessary components for the lock
struct rwlockwp {
                              //! A mutex for the reader/writer lock
  pthread_mutex_t g;
                              //! A condition variable for the reader/writer lock
  pthread_cond_t c;
                              //! Number of writers waiting
  int num_writers_waiting;
                              //! Number of readers active
  int num_reader_active;
                              //! Number of writers active
  int writer_active;

};

                      //! A static initializer that can be used by assignment
#define RWLOCK_STATIC_INITIALIZER { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0, 0 }


/*!
 \brief Acquires the reader lock
 \details
 Acquires the reader lock. Multiple readers can acquire the lock at the same time.
 If a writer has acquired the writer lock, all new readers are blocked until the writer has
 finished.
 \param rwlockwp Pointer to the reader/writer lock
 \mt fully threadsafe
 */
void rwlockwp_reader_acquire( volatile struct rwlockwp* );

/*!
 \brief Releases the reader lock
 \details
 Releases the reader lock. If no more other readers are holding a reader lock and a writer is
 waiting, the writer will get exclusive access.
 \param rwlockwp Pointer to the reader/writer lock
 \mt fully threadsafe
 */
void rwlockwp_reader_release( volatile struct rwlockwp* );

/*!
 \brief Acquires the writer lock
 \details
 Acquires the writer lock. All new readers have to queue up. The writer is blocked until all
 reader that already hold a reader lock have finished.
 \param rwlockwp Pointer to the reader/writer lock
 \mt fully threadsafe
 */
void rwlockwp_writer_acquire( volatile struct rwlockwp* );

/*!
 \brief Releases the writer lock
 \details
 Releases the writer lock. All waiting readers will wake up.
 \param rwlockwp Pointer to the reader/writer lock
 \mt fully threadsafe
 */
void rwlockwp_writer_release( volatile struct rwlockwp* );


#endif
