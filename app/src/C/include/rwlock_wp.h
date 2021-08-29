//
// Created by robert on 07.07.20.
//

#ifndef OPENCLAPP_RWLOCK_WP_H
#define OPENCLAPP_RWLOCK_WP_H

#include <pthread.h>

struct rwlockwp {

  pthread_mutex_t g;
  pthread_cond_t c;
  int num_writers_waiting;
  int num_reader_active;
  int writer_active;

};

#define RWLOCK_STATIC_INITIALIZER { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0, 0 }


void rwlockwp_reader_acquire( volatile struct rwlockwp* );
void rwlockwp_reader_release( volatile struct rwlockwp* );
void rwlockwp_writer_acquire( volatile struct rwlockwp* );
void rwlockwp_writer_release( volatile struct rwlockwp* );


#endif //OPENCLAPP_RWLOCK_WP_H
