#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>

#define SERVER_QUEUE_NAME "/udpdump-accum-stat-util-q"

int main( void ) {

  //******************************************************************************
  // Open POSIX message queues
  //******************************************************************************

  // Server msg queue
  mqd_t server_mq_fd;

  if((server_mq_fd = mq_open(SERVER_QUEUE_NAME, O_WRONLY)) == (mqd_t)-1) {
    perror("mq_open");
    exit(EXIT_FAILURE);
  }
  printf("Server msg queue opened\n");

  // Client msg queue
  char msg_queue_name [] = "/udpdump-get-stat-util-q";

  struct mq_attr attr;
  mqd_t client_mq_fd;

  attr.mq_flags   = 0;
  attr.mq_maxmsg  = 10;
  attr.mq_msgsize = sizeof(long);
  attr.mq_curmsgs = 0;

  if((client_mq_fd = mq_open(msg_queue_name, O_RDONLY | O_CREAT, 0666, &attr)) == (mqd_t)-1) {
    perror("mq_open");
    exit(EXIT_FAILURE);
  }
  printf("Util msg queue opened\n");

  //******************************************************************************
  // Request accumulated statistic
  //******************************************************************************
  
  if(mq_send(server_mq_fd, msg_queue_name, strlen(msg_queue_name)+1, 0) == -1) {
    perror("mq_send");
    // FIXME: Add mq_unlink
    //mq_unlink();
    exit(EXIT_FAILURE);
  }
  printf("Request send to server\n");

  //******************************************************************************
  // Read requested statistic and print the result
  //******************************************************************************

  long stat_bytes_amount;
  
  if(mq_receive(client_mq_fd, (char*)&stat_bytes_amount, sizeof(long), NULL) == -1) {
    perror("mq_receive");
    exit(EXIT_FAILURE);
  }
  printf("Accumulated bytes amount: %ld\n", stat_bytes_amount);

  //******************************************************************************
  // Close POSIX message 'queue' and others
  //******************************************************************************

  if(mq_close(client_mq_fd) == (mqd_t)-1) {
    perror("mq_close");
    exit(EXIT_FAILURE);
  }
  if(mq_unlink(msg_queue_name) == (mqd_t)-1) {
    perror("mq_unlink");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}
