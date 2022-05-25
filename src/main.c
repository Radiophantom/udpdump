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

// Global variable to stop all threads and close handlers
bool stop = false;

void sig_handler( int signo, siginfo_t *info, void *extra ) {
  if( signo == SIGINT )
    stop = true;
}

void set_sig_handler( void ) {
  struct sigaction action;

  action.sa_flags = SA_SIGINFO;
  action.sa_sigaction = sig_handler;

  if( sigaction(SIGINT, &action, NULL) == -1 ){
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
}

int main ( int argc, char *argv[] ) {

  set_sig_handler();

  pthread_t accum_thread;

  pthread_create( &accum_thread, NULL, accum_stat, NULL );

  while( stop == false );



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
  struct settings_struct *filter_settings_ptr;

  filter_settings_ptr = &filter_settings;

  // NULL-terminate 'interface name' to check later
  *filter_settings.iface_name = 0x00;
  filter_settings.filter_mask = 0x00000000;

  parse_args( settings, filter_settings_ptr, argc, argv );

  //******************************************************************************
  // Open RAW socket
  //******************************************************************************

  raw_socket = socket( AF_INET, SOCK_RAW, 17 );

  if( raw_socket == -1 ) {
    perror("socket()");
    exit(EXIT_FAILURE);
  }

  printf("Success raw socket\n");

  int ret;

  ret = setsockopt( raw_socket, SOL_SOCKET, SO_BINDTODEVICE, filter_settings.iface_name, strlen(filter_settings.iface_name) );

  if( ret == -1 ) {
    perror("setsockopt");
    close( raw_socket );
    exit(EXIT_FAILURE);
  }

  //******************************************************************************
  // Create and start 'accumulation' thread
  //******************************************************************************

  /*
   *
   *
   *
   *
  */

  //******************************************************************************
  // Open thread message queue
  //******************************************************************************

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
    exit(EXIT_FAILURE);
  }

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
      exit(EXIT_FAILURE);
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
      exit(EXIT_FAILURE);
    }

    printf("Success msg send\n");

  }

  close( raw_socket );
  mq_close( mq_fd );
  mq_unlink( "/test-msg-q" );
  exit(EXIT_SUCCESS);
}
