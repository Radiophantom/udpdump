#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>
#include <common.h>

int main( void ) {

  int error_occured = 0;

  //******************************************************************************
  // Open POSIX message queues
  //******************************************************************************

  // Server msg queue
  mqd_t server_mq_fd;

  if((server_mq_fd = mq_open(ACCUM_QUEUE_NAME, O_WRONLY)) == (mqd_t)-1) {
    if(errno == ENOENT){
      fprintf(stderr, "UDPDUMP server is not launched yet...\n");
    } else if(errno == EACCES){
      fprintf(stderr, "'get-stat' utility must be run under 'sudo'\n");
    } else {
      perror("mq_open");
    }
    return -1;
  }

  // Client msg queue
  struct mq_attr attr;
  mqd_t client_mq_fd;

  attr.mq_flags   = 0;
  attr.mq_maxmsg  = 10;
  attr.mq_msgsize = sizeof(long);
  attr.mq_curmsgs = 0;

  if((client_mq_fd = mq_open(DISPLAY_QUEUE_NAME, O_RDONLY | O_CREAT, 0666, &attr)) == (mqd_t)-1) {
    perror("mq_open");
    error_occured = 1;
    goto close_server_mq;
  }

  //******************************************************************************
  // Request accumulated statistic
  //******************************************************************************

  if(mq_send(server_mq_fd, DISPLAY_QUEUE_NAME, strlen(DISPLAY_QUEUE_NAME)+1, 0) == -1) {
    perror("mq_send");
    error_occured = 1;
    goto close_client_mq;
  }

  //******************************************************************************
  // Read requested statistic and print the result
  //******************************************************************************

  long stat_bytes_amount;

  printf("Waiting for server response...\n");

  if(mq_receive(client_mq_fd, (char*)&stat_bytes_amount, sizeof(long), NULL) == -1) {
    perror("mq_receive");
    error_occured = 1;
    goto close_client_mq;
  }

  printf("Accumulated bytes amount: %ld\n", stat_bytes_amount);

  //******************************************************************************
  // Close POSIX message 'queue' and others
  //******************************************************************************

close_client_mq:
  if(mq_close(client_mq_fd) == (mqd_t)-1) {
    perror("mq_close");
  }
  if(mq_unlink(DISPLAY_QUEUE_NAME) == (mqd_t)-1) {
    perror("mq_unlink");
  }
close_server_mq:
  if(mq_close(server_mq_fd) == (mqd_t)-1) {
    perror("mq_close");
  }

  if(error_occured)
    return -1;

  return 0;
}
