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
      abort();
    default:
      printf("Unknown SIGNAL is captured\n"); 
      abort();
  }
}


/*
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
      exit(EXIT_SUCCESS);
    }

    bytes_amount = bytes_amount + *bytes_amount_ptr;
    pkts_amount++;

    printf("Bytes received from queue: %d\n", *bytes_amount_ptr );

    printf("Success msg get\n");

  }

  exit(EXIT_SUCCESS);
}
*/

int main ( int argc, char *argv[] ) {

  signal(SIGINT, sig_handler);

  //******************************************************************************
  // Parsing passed arguments and fill filter settings
  //******************************************************************************

  // Required parameters dictionary
  const char *settings [5] = {
  "-dstip",
  "-srcip",
  "-dstport",
  "-srcport",
  "-if"
  };

  struct settings_struct filter_settings;

  // NULL-terminate 'interface name' to check later
  *filter_settings.iface_name = 0x00;
  filter_settings.filter_mask = 0x00000000;

  parse_args( settings, filter_settings, argc, argv );

  //******************************************************************************
  // Open RAW socket
  //******************************************************************************

  raw_socket = socket( AF_INET, SOCK_RAW, 17 );

  if( raw_socket == -1 ) {
    perror("socket()");
    exit(EXIT_SUCCESS);
  }

  printf("Success raw socket\n");

  int ret;

  ret = setsockopt( raw_socket, SOL_SOCKET, SO_BINDTODEVICE, filter_settings.iface_name, strlen(filter_settings.iface_name) );

  if( ret == -1 ) {
    perror("setsockopt");
    close( raw_socket );
    exit(EXIT_SUCCESS);
  }

  //******************************************************************************
  // Open thread message queue
  //******************************************************************************

  /*
  const char mq_name [11] = "/test-msg-q";

  struct mq_attr ma;

  ma.mq_flags   = 0;
  ma.mq_maxmsg  = 10;
  ma.mq_msgsize = 4;
  ma.mq_curmsgs = 0;

  // FIXME: O_WRONLY can conflict with set 'mode' 0220
  mq_fd = mq_open( mq_name, O_WRONLY | O_CREAT, 0220, &ma );

  if( mq_fd == (mqd_t)-1 ) {
    perror("mq_open");
    close( raw_socket );
    exit(EXIT_SUCCESS);
  }
  */

  //******************************************************************************
  // Read RAW socket
  //******************************************************************************

  u_int32_t   bytes_amount;
  u_int32_t*  bytes_amount_ptr = &bytes_amount;

  char eth_buf [1000];

  while( 1 ) {
    bytes_amount = recv( raw_socket, eth_buf, 1000, MSG_WAITALL );//PEEK );

    if( bytes_amount == -1 ) {
      perror("socket.recv");
      close( raw_socket );
      //mq_close( mq_fd );
      //mq_unlink( mq_name );
      exit(EXIT_SUCCESS);
    }

    // Skip packet if out of filter
    if( parse_and_check_pkt_fields( filter_settings, eth_buf ) )
      continue;

    // Send captured bytes amount to 'statistic collecting' thread
    //ret = mq_send( mq_fd, (char*)bytes_amount_ptr, 4, 0 );

    if( ret == -1 ) {
      perror("mq_send");
      close( raw_socket );
      //mq_close( mq_fd );
      //mq_unlink( mq_name );
      exit(EXIT_SUCCESS);
    }

    printf("Success msg send\n");

  }

  // FIXME: Do really need if only SIGINT can terminate process?!
  //******************************************************************************
  // Close RAW socket
  //******************************************************************************

  close( raw_socket );

  exit(EXIT_SUCCESS);
}
