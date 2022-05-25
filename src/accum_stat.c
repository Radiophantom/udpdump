#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <pthread.h>
#include <signal.h>

#include <accum_stat.h>
#include <parse.h>

int raw_socket;
mqd_t mq_fd;

void clean_all ( void ) {
  close( raw_socket );
  mq_close( mq_fd );
  mq_unlink( "/test-msg-q" );
}

void sig_handler( int signum ) {
  printf("\n"); 
  switch( signum ) {
    case SIGINT:
      printf("SIGINT captured\n"); 
      clean_all();
      exit(EXIT_SUCCESS);
    default:
      printf("Unknown SIGNAL is captured\n"); 
      exit(EXIT_SUCCESS);
  }
}

struct {
  mqd_t mq_fd;
} collect_stat_struct_t;

void collect_stat( void *arg ) {

  u_int32_t bytes_amount = 0;
  u_int32_t pkts_amount = 0;

  char buf [4] = "nonenone\0";
  u_int32_t *bytes_amount_ptr = (u_int32_t*)buf;

  int ret;

  mq_fd = mq_open( "/test-msg-q", O_RDONLY | O_CREAT, 0770, arg -> &ma );

  while( 1 ) {
    ret = mq_receive( arg -> mq_fd, buf, 4, NULL );
    if( ret == -1 ) {
      perror("mq_receive");
      clean_all();
      exit(EXIT_FAILURE);
    }

    bytes_amount = bytes_amount + *bytes_amount_ptr;
    pkts_amount++;

    printf("Bytes received from queue: %d\n", *bytes_amount_ptr );

    printf("Success msg get\n");

  }

  exit(EXIT_SUCCESS);
}
