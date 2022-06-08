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

#define SERVER_QUEUE_NAME "/udpdump-accum-stat-util-q"

extern int stop;

// Statistic accumulation thread
void *accum_stat( void *pipefd ) {
  //******************************************************************************
  // Mask unwanted signals
  //******************************************************************************

  sigset_t mask;
  sigemptyset( &mask );
  sigaddset( &mask, SIGINT );

  //if(stop) {
  //  pthread_exit();
  //} else {
  //  pthread_sigmask(SIG_BLOCK, &mask, NULL);
  //}

  //******************************************************************************
  // Open pipe to read statistic from main thread
  //******************************************************************************

  int pfd = *(int*)pipefd;

  u_int32_t msg_value;
  long  stat_var = 0;

  //******************************************************************************
  // Open msg queue for inter-thread communication
  //******************************************************************************

  mqd_t server_mq_fd, client_mq_fd;
  struct mq_attr attr;

  char client_msg_queue_name [100];

  attr.mq_flags   = O_NONBLOCK;
  attr.mq_maxmsg  = 10;
  attr.mq_msgsize = 100;
  attr.mq_curmsgs = 0;

  if((server_mq_fd = mq_open( SERVER_QUEUE_NAME, O_RDONLY | O_CREAT | O_NONBLOCK, 0777, &attr )) == (mqd_t)-1) {
    perror("mq_open");
    exit(EXIT_FAILURE);
  }

  //******************************************************************************
  // Main process loop
  //******************************************************************************

  while( stop == 0 ) {
    if(read(pfd, &msg_value, 4) != -1) {
      stat_var += msg_value;
    } else if( errno != EAGAIN ) {
      perror("read");
      exit(EXIT_FAILURE);
    }
    if(mq_receive(server_mq_fd, client_msg_queue_name, 100, NULL) != (mqd_t)-1) {
      if((client_mq_fd = mq_open(client_msg_queue_name, O_WRONLY)) != -1) {
        if(mq_send(client_mq_fd, (char*)&stat_var, sizeof(long), 0) == -1) {
          perror("mq_send");
          exit(EXIT_FAILURE);
        }
        stat_var = 0;
      }
    } else if(errno != EAGAIN) {
      perror("mq_receive");
      exit(EXIT_FAILURE);
    }
  }
  
  if(mq_close(server_mq_fd) == (mqd_t)-1) {
    perror("mq_close");
    exit(EXIT_FAILURE);
  }
  if(mq_unlink(SERVER_QUEUE_NAME) == -1) {
    perror("mq_unlink");
    exit(EXIT_FAILURE);
  }

  pthread_exit( NULL );
}

