#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <mqueue.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <linux/if_packet.h>

#include <main.h>
#include <accum_stat.h>
#include <parse.h>

#define DEBUG

#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

// Global variable to stop threads after Ctrl+C
int stop = 0;

// SIGINT handler action function
void sig_handler( int signo, siginfo_t *info, void *ptr ) {
  stop = signo;
}

// SIGINT handler set function
void set_sig_handler( void ) {
  struct sigaction action;

  action.sa_flags     = SA_SIGINFO;
  action.sa_sigaction = sig_handler;

  if( sigaction(SIGINT, &action, NULL) == -1 ){
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
}

// Main function
int main ( int argc, char *argv[] ) {

  // Some functions temp result store variable
  int ret;

  //******************************************************************************
  // Parse and check utility parameters
  //******************************************************************************

  struct settings_struct filter_settings;
  memset(&filter_settings, 0, sizeof(struct settings_struct));

  if( parse_args( settings, &filter_settings, argc, argv ) )
    exit(EXIT_FAILURE);

  //******************************************************************************
  // Define SIGINT signal behavior
  //******************************************************************************

  set_sig_handler();

  //******************************************************************************
  // Open RAW socket and bind it to interface
  //******************************************************************************

  int raw_socket;

  if((raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1 ) {
    handle_error("socket");
  }

#ifdef DEBUG
  printf("Success raw socket\n");
#endif

  // Get required interface index for RAW socket purpose
  struct ifreq if_dev_info;

  memset(&if_dev_info, 0, sizeof(struct ifreq));
  strncpy( if_dev_info.ifr_name, filter_settings.iface_name, IFNAMSIZ-1 );

  if(ioctl(raw_socket, SIOCGIFINDEX, &if_dev_info) == -1) {
    handle_error("ioctl");
  }

  struct sockaddr_ll sock_dev_info;

  memset(&sock_dev_info, 0, sizeof( struct sockaddr_ll ));

  sock_dev_info.sll_family   = AF_PACKET;
  sock_dev_info.sll_ifindex  = if_dev_info.ifr_ifindex;

  ret = bind( raw_socket, (struct sockaddr*) &sock_dev_info, sizeof( struct sockaddr_ll ) );

  if( ret == -1 ) {
    handle_error("bind");
  }

  //******************************************************************************
  // Open msg queue for inter-thread communication
  //******************************************************************************

  mqd_t mq_fd;
  struct mq_attr attr;

  attr.mq_flags   = 0;
  attr.mq_maxmsg  = 10;
  attr.mq_msgsize = 4;
  attr.mq_curmsgs = 0;

  char msg_queue_name [20] = "/udpdump-util-q";

  mq_fd = mq_open( msg_queue_name, O_WRONLY | O_CREAT, 0200, &attr );

  if( mq_fd == (mqd_t)-1 ) {
    handle_error("mq_open");
  }

#ifdef DEBUG
  printf("Main thread opened msg queue\n");
#endif

  //******************************************************************************
  // Create 'stat' accum thread
  //******************************************************************************

  pthread_t accum_thread;

  if(pthread_create(&accum_thread, NULL, accum_stat, msg_queue_name)) {
    handle_error("pthread_create");
  }

  //******************************************************************************
  // Read RAW socket
  //******************************************************************************

  char *eth_buf;
  
  if((eth_buf = (char*) malloc(2048)) == NULL)
    goto exit_and_close_mqueue;

  while( stop == 0 ) {

    int bytes_amount;

    char *eth_buf_ptr;

    bytes_amount = recv( raw_socket, eth_buf, 2048, 0 );

    if( bytes_amount == -1 ) {
      if( errno != EINTR )
        perror("recv");
    } else {
      if(parse_and_check_pkt_fields(&filter_settings, eth_buf, bytes_amount) != -1) {
        ret = mq_send( mq_fd, (char*)&bytes_amount, 4, 0 );
        if( ret == -1 ) {
          perror("mq_send");
          goto exit_and_close_mqueue;
        }
      }
    }
  }

  pthread_join( accum_thread, NULL );

  exit_and_close_mqueue:
    free(eth_buf);
    if( mq_close( mq_fd ) )
      perror("mq_close");
    if( mq_unlink( msg_queue_name ) )
      perror("mq_unlink");
  exit_and_close_socket:
    if( close( raw_socket ) )
      perror("close");

  fprintf(stderr, "Exiting via %s\n", strsignal(stop));
  printf("Main thread closed\n");
  exit(EXIT_SUCCESS);
}
