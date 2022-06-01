#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <mqueue.h>
#include <fcntl.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <string.h>

#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ether.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <net/ethernet.h>

#include <main.h>
#include <accum_stat.h>
#include <parse.h>

#define DEBUG

// Global variable to stop threads after Ctrl+C
int stop = 0;

// SIGINT handler action function
void sig_handler( int signo, siginfo_t *info, void *ptr ) {
  stop = 1;
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
    perror("socket");
    exit(EXIT_FAILURE);
  }

#ifdef DEBUG
  printf("Success raw socket\n");
#endif

  // Get required interface index for RAW socket purpose
  struct ifreq if_dev_info;

  memset(&if_dev_info, 0, sizeof(struct ifreq));
  strncpy( if_dev_info.ifr_name, filter_settings.iface_name, IFNAMSIZ-1 );

  if(ioctl(raw_socket, SIOCGIFINDEX, &if_dev_info) == -1) {
    perror("ioctl");
    goto exit_and_close_socket;
  }

  struct sockaddr_ll sock_dev_info;

  memset(&sock_dev_info, 0, sizeof( struct sockaddr_ll ));

  sock_dev_info.sll_family   = AF_PACKET;
  //sock_dev_info.sll_pkttype  = PACKET_OTHERHOST;
  sock_dev_info.sll_ifindex  = if_dev_info.ifr_ifindex;

  ret = bind( raw_socket, (struct sockaddr*) &sock_dev_info, sizeof( struct sockaddr_ll ) );

  if( ret == -1 ) {
    perror("bind");
    goto exit_and_close_socket;
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

  char msg_queue_name [12] = "/test-msg-q\0";

  mq_fd = mq_open( msg_queue_name, O_WRONLY | O_CREAT, 0200, &attr );

  if( mq_fd == (mqd_t)-1 ) {
    perror("mq_open");
    goto exit_and_close_socket;
  }

#ifdef DEBUG
  printf("Main thread opened msg queue\n");
#endif

  //******************************************************************************
  // Create 'stat' accum thread
  //******************************************************************************

  pthread_t accum_thread;

  pthread_create( &accum_thread, NULL, accum_stat, msg_queue_name );

  //******************************************************************************
  // Read RAW socket
  //******************************************************************************

  while( stop == 0 ) {

    int bytes_amount;

    char *eth_buf = (char*) malloc( 65536 );
    char *eth_buf_ptr;

    if( eth_buf == NULL )
      goto exit_and_close_mqueue;

    bytes_amount = recv( raw_socket, eth_buf, 65536, 0 );

    struct ethhdr *eth_hdr;
    struct iphdr  *ip_hdr;
    struct udphdr *udp_hdr;

    if( bytes_amount == -1 ) {
      perror("recv");
    } else {
      printf("Msg captured\n");
      eth_buf_ptr = eth_buf;
      for( int i = 0; i < bytes_amount; i++ )
        printf("%X", eth_buf[i]);
      printf("\n");
      eth_hdr = (struct ethhdr*)eth_buf_ptr;
      eth_buf_ptr += sizeof(struct ethhdr);
      ip_hdr  = (struct iphdr*)eth_buf_ptr;
      eth_buf_ptr += sizeof(struct iphdr);
      udp_hdr  = (struct udphdr*)eth_buf_ptr;
      //parse_and_check_pkt_fields( &filter_settings, eth_buf, bytes_amount );
#ifdef DEBUG
      printf("DST Mac:%X%X%X%X%X%X\n", eth_hdr -> h_dest[0],
                                       eth_hdr -> h_dest[1],
                                       eth_hdr -> h_dest[2],
                                       eth_hdr -> h_dest[3],
                                       eth_hdr -> h_dest[4],
                                       eth_hdr -> h_dest[5] );
      printf("SRC Mac:%X%X%X%X%X%X\n", eth_hdr -> h_source[0],
                                       eth_hdr -> h_source[1],
                                       eth_hdr -> h_source[2],
                                       eth_hdr -> h_source[3],
                                       eth_hdr -> h_source[4],
                                       eth_hdr -> h_source[5] );
      printf("IP Proto:%X\n",  ntohs(eth_hdr -> h_proto));
      printf("IP Protocol:%d\n", ip_hdr -> protocol);
      printf("IP SRC:%X\n", ntohl(ip_hdr -> saddr));
      printf("IP DST:%X\n", ntohl(ip_hdr -> daddr));
      printf("\n");
#endif
      ret = mq_send( mq_fd, (char*)&bytes_amount, 4, 0 );
      if( ret == -1 ) {
        perror("mq_send");
        free(eth_buf);
        goto exit_and_close_mqueue;
      }
    }
  }

  pthread_join( accum_thread, NULL );

  exit_and_close_mqueue:
    if( mq_close( mq_fd ) )
      perror("mq_close");
    if( mq_unlink( msg_queue_name ) )
      perror("mq_unlink");
  exit_and_close_socket:
    if( close( raw_socket ) )
      perror("close");

  exit(EXIT_SUCCESS);
}
