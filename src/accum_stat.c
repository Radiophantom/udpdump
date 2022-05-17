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

//#define DEBUG
/*
static void collect_stat ( union sigval sv ) {
  
  void *buf;


}
*/

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

int parse_ip ( char *str_ip ) {
  int ip = 0;

  for( int i = 0; i < 3; i++ ) {
      ip = ( ip << 8 ) | atoi(str_ip);
      // Find next '.' divider
      while( *str_ip != '.' )
        str_ip++;
      str_ip++;
    }
  ip = ( ip << 8 ) | atoi(str_ip);
  return ip;
}

int main ( int argc, char *argv[] ) {

  signal(SIGINT, sig_handler);

  //******************************************************************************
  // Parsing passed arguments and fill filter settings
  //******************************************************************************

  // Required parameters dictionary
  char *settings [5] = {
  "-dstip",
  "-srcip",
  "-dstport",
  "-srcport",
  "-dev"
  };

  struct {
    u_int32_t dst_ip;
    u_int32_t src_ip;
    u_int16_t dst_port;
    u_int16_t src_port;
    char      dev_name [100];
  } settings_struct;

  if( argc != 11 ) {
    printf("Invalid arguments amount\n");
    exit(EXIT_SUCCESS);
  }

  for( int i = 1; i < 11; i = i+2 ) {
    for( int j = 0; j < 5; j++ ) {
      int res;
      res = strcmp( settings[j], argv[i] );
      if( res == 0 ) {
        if( j == 0 )
          settings_struct.dst_ip = parse_ip( argv[i+1] );
        else if ( j == 1 )
          settings_struct.src_ip = parse_ip( argv[i+1] );
        else if ( j == 2 )
          settings_struct.dst_port = atoi( argv[i+1] );
        else if ( j == 3 )
          settings_struct.src_port = atoi( argv[i+1] );
        else if ( j == 4 ) {
          strcpy( settings_struct.dev_name, argv[i+1] );
        }
        break;
      }
      else if ( j == 4 ) {
        printf("Unrecognized parameter \"%s\"\n", argv[i]);
        exit(EXIT_SUCCESS);
      }
    }
  }

  printf("Destination ip is %X\n", settings_struct.dst_ip);
  printf("Source ip is %X\n", settings_struct.src_ip);
  printf("Destination port is %d\n", settings_struct.dst_port);
  printf("Source port is %d\n", settings_struct.src_port);
  printf("Device name is %s\n", settings_struct.dev_name);

  //******************************************************************************
  // Open RAW socket processing
  //******************************************************************************

  raw_socket = socket( AF_INET, SOCK_RAW, 17 );

  if( raw_socket == -1 ) {
    perror("socket()");
    exit(EXIT_SUCCESS);
  }

  printf("Success raw socket\n");

  int ret;

  ret = setsockopt( raw_socket, SOL_SOCKET, SO_BINDTODEVICE, settings_struct.dev_name, strlen(settings_struct.dev_name) );

  if( ret == -1 ) {
    perror("setsockopt");
    clean_all();
    exit(EXIT_SUCCESS);
  }

  /*
  u_int32_t buf_len;

  ret = getsockopt( raw_socket, SOL_SOCKET, SO_BINDTODEVICE, settings_struct.dev_name, &buf_len );

  if( ret == -1 ) {
    perror("Get raw socket options");
    exit(EXIT_SUCCESS);
  }
  */

  struct mq_attr ma;

  ma.mq_flags = 0;
  ma.mq_maxmsg = 10;
  ma.mq_msgsize = 4;
  ma.mq_curmsgs = 0;

  mq_fd = mq_open( "/test-msg-q", O_RDWR | O_CREAT, 0770, &ma );

  if( mq_fd == (mqd_t)-1 ) {
    perror("mq_open");
    clean_all();
    exit(EXIT_SUCCESS);
  }

  //******************************************************************************
  // Read RAW socket
  //******************************************************************************

  u_int32_t   bytes_amount;
  u_int32_t*  bytes_amount_ptr  = &bytes_amount;

  char eth_buf [1000];

  while( 1 ) {
    bytes_amount = recv( raw_socket, eth_buf, 1000, MSG_WAITALL );//PEEK );

    if( bytes_amount == -1 ) {
      perror("socket recv()");
      clean_all();
      exit(EXIT_SUCCESS);
    }

    u_int32_t *ip;
    u_int16_t *port;

    ip = (u_int32_t*)eth_buf;
    ip = ip + 3;

    if( settings_struct.src_ip != ntohl(*ip) )
      continue;

    ip++;

    if( settings_struct.dst_ip != ntohl(*ip) )
      continue;

    ip++;

    port = (u_int16_t*)ip;

    if( settings_struct.src_port != ntohs(*port) )
      continue;

    port++;

    if( settings_struct.dst_port != ntohs(*port) )
      continue;

    #ifdef DEBUG

      printf("Bytes captured: %0d\n", bytes_amount);

      printf("UDP bytes has been captured: %d\n", bytes_amount-20-8);

      char* udp_data;

      port = port + 3;

      udp_data = (char*)port;

      printf("UDP data:\n");
     
      for( int i = 0; i < bytes_amount-20-8; i++ ) {
        printf("%c", *udp_data++);
      }
      printf("\n");
    #endif

    ret = mq_send( mq_fd, (char*)bytes_amount_ptr, 4, 0 );

    if( ret == -1 ) {
      perror("mq_send");
      clean_all();
      exit(EXIT_SUCCESS);
    }

    printf("Success msg send\n");

    char buf [10] = "nonenone\0";

    ret = mq_receive( mq_fd, buf, 4, NULL );
    if( ret == -1 ) {
      perror("mq_receive");
      clean_all();
      exit(EXIT_SUCCESS);
    }

    printf("Bytes received from queue: %d\n", *bytes_amount_ptr );

    printf("Success msg get\n");

  }

  //******************************************************************************
  // Close RAW socket
  //******************************************************************************

  close( raw_socket );

  exit(EXIT_SUCCESS);
}
