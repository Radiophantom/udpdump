#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <pthread.h>
#include <accum_stat.h>
#include <signal.h>

extern int stop;

// Statistic accumulation thread
void *accum_stat( void *msg_queue_name ) {
  sigset_t mask;
  sigemptyset( &mask );
  sigaddset( &mask, SIGINT );
  pthread_sigmask(SIG_BLOCK, &mask, NULL);

  mqd_t mq_fd;

  mq_fd = mq_open( msg_queue_name, O_RDONLY | O_NONBLOCK );

  if( mq_fd == (mqd_t)-1 ) {
    perror("mq_open");
    pthread_exit( NULL );
  }

  u_int32_t msg_value;
  int       ret;

  while( stop == 0 ) {
    ret = mq_receive( mq_fd, (char*)&msg_value, 4, NULL );
    if( ret != -1 ) {
      printf("Message value: %0d\n", msg_value);
    } else if( errno != EAGAIN ) {
      perror("mq_timedreceive");
      if( mq_close( mq_fd ) )
        perror("mq_close");
      pthread_exit( NULL );
    }
  }
  
  if( mq_close( mq_fd ) )
    perror("mq_close");
  pthread_exit( NULL );
}

