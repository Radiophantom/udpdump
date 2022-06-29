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
#include <common.h>

extern int stop;

// Statistic accumulation thread
void *accum_stat( void *pipefd ) {

  int error_occured = 0;

  //******************************************************************************
  // Open pipe to read statistic from main thread
  //******************************************************************************

  int pfd = *(int*)pipefd;

  u_int32_t msg_value;
  long      stat_var = 0;

  //******************************************************************************
  // Open msg queue for inter-thread communication
  //******************************************************************************

  mqd_t server_mq_fd, client_mq_fd;
  struct mq_attr attr;

  char client_msg_queue_name [100];

  attr.mq_flags   = 0;
  attr.mq_maxmsg  = 10;
  attr.mq_msgsize = 100;
  attr.mq_curmsgs = 0;

  if((server_mq_fd = mq_open( ACCUM_QUEUE_NAME, O_RDONLY | O_CREAT | O_NONBLOCK, 0777, &attr )) == (mqd_t)-1) {
    perror("mq_open");
    error_occured = 1;
    goto close_pipe_fd;
  }

  //******************************************************************************
  // Main process loop
  //******************************************************************************

  while(stop == 0) {
    if(read(pfd, &msg_value, 4) != -1) {
      stat_var += msg_value;
    } else if( errno != EAGAIN ) {
      perror("read");
      error_occured = 1;
      goto close_server_mq;
    }
    if(mq_receive(server_mq_fd, client_msg_queue_name, 100, NULL) != (mqd_t)-1) {
      if((client_mq_fd = mq_open(client_msg_queue_name, O_WRONLY)) != -1) {
        if(mq_send(client_mq_fd, (char*)&stat_var, sizeof(long), 0) == -1) {
          perror("mq_send");
          error_occured = 1;
        }
        stat_var = 0;
        if(mq_close(client_mq_fd) == (mqd_t)-1) {
          perror("mq_close");
          error_occured = 1;
          goto close_server_mq;
        }
        if(error_occured) {
          goto close_server_mq;
        }
      }
    } else if(errno != EAGAIN) {
      perror("mq_receive");
      error_occured = 1;
      goto close_server_mq;
    }
  }
  
close_server_mq:
  if(mq_close(server_mq_fd) == (mqd_t)-1) {
    perror("mq_close");
  }
  if(mq_unlink(ACCUM_QUEUE_NAME) == -1) {
    perror("mq_unlink");
  }
close_pipe_fd:
  if(close(pfd)) {
    perror("close");
  }

  if(error_occured)
    return -1;

  return 0;
}

