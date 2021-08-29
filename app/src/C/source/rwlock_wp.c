//
// Created by robert on 07.07.20.
//

#include "rwlock_wp.h"


void rwlockwp_reader_acquire( volatile struct rwlockwp* rwl ){

  pthread_mutex_lock( &rwl->g );

  while ((rwl->num_writers_waiting>0) || (rwl->writer_active>0)){
    pthread_cond_wait( &rwl->c, &rwl->g );
  }

  rwl->num_reader_active++;

  pthread_mutex_unlock( &rwl->g );

}

void rwlockwp_reader_release( volatile struct rwlockwp* rwl ){

  pthread_mutex_lock( &rwl->g );

  rwl->num_reader_active--;

  if (rwl->num_reader_active == 0){

    pthread_cond_broadcast( &rwl->c );

  }

  pthread_mutex_unlock( &rwl->g );

}

void rwlockwp_writer_acquire( volatile struct rwlockwp* rwl ){

  pthread_mutex_lock( &rwl->g );
  rwl->num_writers_waiting++;

  while ((rwl->num_reader_active>0) || (rwl->writer_active>0)){
    pthread_cond_wait( &rwl->c, &rwl->g );
  }

  rwl->num_writers_waiting--;
  rwl->writer_active = 1;

  pthread_mutex_unlock( &rwl->g );

}

void rwlockwp_writer_release( volatile struct rwlockwp* rwl ){


  pthread_mutex_lock( &rwl->g );

  rwl->writer_active = 0;

  pthread_cond_broadcast( &rwl->c );

  pthread_mutex_unlock( &rwl->g );

}

