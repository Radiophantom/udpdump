#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#include <accum_stat.h>

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

  //******************************************************************************
  // Parsing passed arguments and fill filter settings
  //******************************************************************************

  // Required parameters dictionary
  char *settings [4] = {
  "-dstip",
  "-srcip",
  "-dstport",
  "-srcport"
  };

  struct {
    u_int32_t dst_ip;
    u_int32_t src_ip;
    u_int16_t dst_port;
    u_int16_t src_port;
  } settings_struct;

  if( argc != 9 ) {
    printf("Invalid arguments amount\n");
    exit(EXIT_SUCCESS);
  }

  for( int i = 1; i < 9; i = i+2 ) {
    for( int j = 0; j < 4; j++ ) {
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
        break;
      }
      else if ( j == 3 ) {
        printf("Unrecognized parameter \"%s\"\n", argv[i]);
        exit(EXIT_SUCCESS);
      }
    }
  }

  printf("Destination ip is %X\n", settings_struct.dst_ip);
  printf("Source ip is %X\n", settings_struct.src_ip);
  printf("Destination port is %d\n", settings_struct.dst_port);
  printf("Source port is %d\n", settings_struct.src_port);

  //******************************************************************************
  // Open RAW socket processing
  //******************************************************************************

  int raw_socket = socket( AF_INET, SOCK_RAW, 17 );

  if( raw_socket == -1 ) {
    perror("raw socket");
    exit(EXIT_SUCCESS);
  }

  printf("Success raw socket\n");

  char buf [100];
  u_int32_t buf_len = 100;
  u_int32_t *buf_len_ptr = &buf_len;

  const char dev_name [2] = "lo";

  int ret;

  ret = setsockopt( raw_socket, SOL_SOCKET, SO_BINDTODEVICE, dev_name, 2 );

  if( ret == -1 ) {
    perror("Set raw socket options");
    exit(EXIT_SUCCESS);
  }

  ret = getsockopt( raw_socket, SOL_SOCKET, SO_BINDTODEVICE, buf, buf_len_ptr );

  if( ret == -1 ) {
    perror("Get raw socket options");
    exit(EXIT_SUCCESS);
  }

  for( int i = 0; i < buf_len; i++ )
    printf("%c", buf[i]);

  printf("\n");

  //******************************************************************************
  // Read RAW socket
  //******************************************************************************

  int bytes_amount;

  char eth_buf [1000];

  bytes_amount = recv( raw_socket, eth_buf, 1000, MSG_PEEK );

  if( bytes_amount == -1 ) {
    perror("Raw socket peek data");
    exit(EXIT_SUCCESS);
  }

  printf("Bytes captured: %0d\n", bytes_amount);

  u_int32_t *ip;
  u_int16_t *port;

  ip = (u_int32_t*)eth_buf;
  ip = ip + 3;

  if( settings_struct.src_ip == ntohl(*ip) )
    printf("SRC IP is correct\n");
  else
    printf("SRC IP is incorrect. Expected: %X. Observed: %X\n", settings_struct.src_ip, *ip );

  ip++;

  if( settings_struct.dst_ip == ntohl(*ip) )
    printf("DST IP is correct\n");
  else
    printf("DST IP is incorrect. Expected: %X. Observed: %X\n", settings_struct.dst_ip, *ip );

  ip++;

  port = (u_int16_t*)ip;

  if( settings_struct.src_port == ntohs(*port) )
    printf("SRC PORT is correct\n");
  else
    printf("SRC PORT is incorrect. Expected: %X. Observed: %X\n", settings_struct.src_port, *port );

  port++;

  if( settings_struct.dst_port == ntohs(*port) )
    printf("DST PORT is correct\n");
  else
    printf("DST PORT is incorrect. Expected: %X. Observed: %X\n", settings_struct.dst_port, *port );

  printf("UDP bytes has been captured: %d\n", bytes_amount-20-8);

  char* udp_data;

  port = port + 3;

  udp_data = (char*)port;

  printf("UDP data:\n");
 
  for( int i = 0; i < bytes_amount-20-8; i++ ) {
    printf("%c", *udp_data++);
  }
  printf("\n");

  /*
  printf("Raw data:\n");

  for( int i = 0; i < bytes_amount; i++ )
    printf("%X", eth_buf[i]);
  printf("\n");
  */

  //******************************************************************************
  // Close RAW socket
  //******************************************************************************

  close( raw_socket );

  exit(EXIT_SUCCESS);
}
