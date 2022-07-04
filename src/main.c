#define _GNU_SOURCE

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

// Global variable to stop threads after Ctrl+C
int stop = 0;

// SIGINT handler action function
void sig_handler( int signo, siginfo_t *info, void *ptr ) {
  stop = signo;
}

// SIGINT handler set function
int set_sig_handler( void ) {
  struct sigaction action;

  action.sa_flags     = SA_SIGINFO;
  action.sa_sigaction = sig_handler;

  if( sigaction(SIGINT, &action, NULL) == -1 ){
    perror("sigaction");
    return -1;
  }
  return 0;
}

// Main function
int main ( int argc, char *argv[] ) {

  int error_occured = 0;

  //******************************************************************************
  // Parse and check utility parameters
  //******************************************************************************

  struct settings_struct filter_settings;
  memset(&filter_settings, 0, sizeof(struct settings_struct));

  switch(parse_args(&filter_settings, argc, argv)) {
    case(1):
      return 0;
    case(0):
      break;
    default:
      return -1;
  }

  //******************************************************************************
  // Define SIGINT signal behavior
  //******************************************************************************

  if(set_sig_handler() == -1) {
    return -1;
  }

  //******************************************************************************
  // Open RAW socket and bind it to interface
  //******************************************************************************

  int raw_socket;

  if((raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP))) == -1 ) {
    perror("socket");
    return -1;
  }

  // Get required interface index for RAW socket purpose
  struct ifreq if_dev_info;

  memset(&if_dev_info, 0, sizeof(struct ifreq));
  strncpy( if_dev_info.ifr_name, filter_settings.iface_name, IFNAMSIZ-1 );

  if(ioctl(raw_socket, SIOCGIFINDEX, &if_dev_info) == -1) {
    perror("ioctl");
    error_occured = 1;
    goto close_raw_socket;
  }

  struct sockaddr_ll sock_dev_info;

  memset(&sock_dev_info, 0, sizeof( struct sockaddr_ll ));

  sock_dev_info.sll_family   = AF_PACKET;
  sock_dev_info.sll_ifindex  = if_dev_info.ifr_ifindex;

  if((bind(raw_socket, (struct sockaddr*) &sock_dev_info, sizeof(struct sockaddr_ll))) == -1) {
    perror("bind");
    error_occured = 1;
    goto close_raw_socket;
  }

//******************************************************************************
// Open pipe for inter-thread communication
//******************************************************************************

  int pipefd[2];

  if(pipe2(pipefd, O_NONBLOCK) == -1) {
    perror("pipe2");
    error_occured = 1;
    goto close_raw_socket;
  }

  //******************************************************************************
  // Allocate receive buffer
  //******************************************************************************

  char *eth_buf;
  
  if((eth_buf = (char*) malloc(ETH_BUF_SIZE)) == NULL) {
    error_occured = 1;
    goto close_pipe_fd;
  }

  //******************************************************************************
  // Create and start statisctic accumulation thread
  //******************************************************************************

  pthread_t accum_thread;

  if(pthread_create(&accum_thread, NULL, accum_stat, &pipefd[0])) {
    printf("Pthred open failed\n");
    error_occured = 1;
    goto free_mem;
  }

  printf("Statistic accumulation started!\n");

  //******************************************************************************
  // Read RAW socket
  //******************************************************************************

  while( stop == 0 ) {

    int bytes_amount;

    bytes_amount = recv( raw_socket, eth_buf, 2048, 0 );

    if( bytes_amount == -1 ) {
      if( errno != EINTR )
        perror("recv");
    } else {
      if(parse_packet(&filter_settings, eth_buf, bytes_amount) != -1) {
        if(write(pipefd[1], &bytes_amount, 4) == -1) {
          perror("write");
          error_occured = 1;
          goto wait_pthread;
        }
      }
    }
  }

  wait_pthread:
    pthread_join(accum_thread, NULL);
  free_mem:
    free(eth_buf);
  close_pipe_fd:
    if(close(pipefd[1])) {
      perror("close");
    }
  close_raw_socket:
    if(close(raw_socket)) {
      perror("close");
    }

  if(error_occured)
    return -1;

  return 0;
}
