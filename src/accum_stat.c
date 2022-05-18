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

#define DST_PORT_FILTER_EN ( 1 << 0 )
#define SRC_PORT_FILTER_EN ( 1 << 1 )
#define DST_IP_FILTER_EN   ( 1 << 2 )
#define SRC_IP_FILTER_EN   ( 1 << 3 )

/*
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

int parse_and_check_pkt_fields( char *eth_buf ) {

  u_int32_t *ip;
  u_int16_t *port;

  // Skip some header fields till IP-fields
  eth_buf = eth_buf + 12;

  // Cast to IP 32-bit fields
  ip = (u_int32_t*)eth_buf;

  if( settings_struct.src_ip_filter_en )
    if( settings_struct.src_ip != ntohl(*ip++) )
      return 1;

  if( settings_struct.dst_ip_filter_en )
    if( settings_struct.dst_ip != ntohl(*ip++) )
      return 1;

  // Cast to UDP 16-bit fields
  port = (u_int16_t*)ip;

  if( settings_struct.src_port_filter_en )
    if( settings_struct.src_port != ntohs(*port++) )
      return 1;

  if( settings_struct.dst_port_filter_en )
    if( settings_struct.dst_port != ntohs(*port++) )
      return 1;

  return 0;
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

int parse_port ( char *str_ip, u_int16_t *port ) {
  int port_int;
  if( strlen( str_ip ) > 5 )
    return -1;
  port_int = atoi( str_ip );
  if( port_int > 65535 )
    return -1;
  *port = (u_int16_t)port_int;
  return 0;
}

int parse_ip ( char *str_ip, u_int32_t *ip ) {

  char *next_str_ip = str_ip;
  u_int32_t ip_byte;

  // Parse first 3 IP fields
  for( int i = 0; i < 3; i++ ) {
    // Find next '.' divider
    for( int j = 0; j < 4; j++ ) {
      if( *next_str_ip++ == '.' ) {
        if( j == 0 )
          return -1;
        break;
      }
      if( j == 3 )
        return -1;
    }
    if( ( ip_byte = atoi( str_ip) ) > 255 )
      return -1;
    *ip = ( *ip << 8 ) | ip_byte;
    str_ip = next_str_ip;
  }

  // Parse last 4th IP field
  for( int j = 0; j < 4; j++ ) {
    if ( *next_str_ip == '.' ) {
      return -1;
    }
    if ( *next_str_ip++ == '\0' ) {
      if( j == 0 )
        return -1;
      break;
    }
    if( j == 3 )
      return -1;
  }

  if( ( ip_byte = atoi( str_ip) ) > 255 )
    return -1;
  *ip = ( *ip << 8 ) | ip_byte;
  return 0;
}

int main ( int argc, char *argv[] ) {

//  signal(SIGINT, sig_handler);

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

  int parse_args( char *argv ) {
    for( int i = 0; i < 5; i++ )
      if( strcmp( settings[i], argv ) == 0 )
        return i;
    return -1;
  }

  u_int32_t filter_mask = 0x00000000;

  struct {
    u_int32_t dst_ip;
    u_int32_t src_ip;
    u_int16_t dst_port;
    u_int16_t src_port;
    char      iface_name [100];
  } settings_struct;

  // NULL-terminate 'interface name' to check later
  *settings_struct.iface_name = 0x00;

  int arg_ind;

  for( int i = 1; i < argc; i = i+2 ) {

    if( ( arg_ind = parse_args( argv[i] ) ) == -1 ) {
      printf("Unknown option \"%s\"\n", argv[i]);
      exit(EXIT_SUCCESS);
    }

    if( i > argc - 2 ) {
      printf("\"%s\" option argument missed\n", argv[i]);
      exit(EXIT_SUCCESS);
    }

    switch( arg_ind ) {
      case( 0 ):
        if( parse_ip( argv[i+1], &settings_struct.dst_ip ) == -1 ) {
          printf("Invalid \"%s\" argument value: %s\n", argv[i], argv[i+1]);
          exit(EXIT_SUCCESS);
        }
        filter_mask |= DST_IP_FILTER_EN;
        break;
      case( 1 ):
        if( parse_ip( argv[i+1], &settings_struct.src_ip ) == -1 ) {
          printf("Invalid \"%s\" argument value: %s\n", argv[i], argv[i+1]);
          exit(EXIT_SUCCESS);
        }
        filter_mask |= SRC_IP_FILTER_EN;
        break;
      case( 2 ):
        if( parse_port( argv[i+1], &settings_struct.dst_port ) == -1 ) {
          printf("Invalid \"%s\" argument value: %s\n", argv[i], argv[i+1]);
          exit(EXIT_SUCCESS);
        }
        filter_mask |= DST_PORT_FILTER_EN;
        break;
      case( 3 ):
        if( parse_port( argv[i+1], &settings_struct.src_port ) == -1 ) {
          printf("Invalid \"%s\" argument value: %s\n", argv[i], argv[i+1]);
          exit(EXIT_SUCCESS);
        }
        filter_mask |= SRC_PORT_FILTER_EN;
        break;
      case( 4 ):
        strcpy( settings_struct.iface_name, argv[i+1] );
        break;
    }
  }

  if( strlen(settings_struct.iface_name) == 0 ) {
    printf("Interface name must be set\n");
    exit(EXIT_SUCCESS);
  }

  if( filter_mask & DST_IP_FILTER_EN )
    printf("Destination ip is %x\n",    settings_struct.dst_ip    );
  if( filter_mask & SRC_IP_FILTER_EN )
    printf("Source ip is %x\n",         settings_struct.src_ip    );
  if( filter_mask & DST_PORT_FILTER_EN )
    printf("Destination port is %0d\n", settings_struct.dst_port  );
  if( filter_mask & SRC_PORT_FILTER_EN )
    printf("Source port is %0d\n",      settings_struct.src_port  );
  printf("Interface name is %s\n",    settings_struct.iface_name);

  /*
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

  ret = setsockopt( raw_socket, SOL_SOCKET, SO_BINDTODEVICE, settings_struct.dev_name, strlen(settings_struct.dev_name) );

  if( ret == -1 ) {
    perror("setsockopt");
    close( raw_socket );
    exit(EXIT_SUCCESS);
  }

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
    exit(EXIT_SUCCESS);
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
      mq_close( mq_fd );
      mq_unlink( mq_name );
      exit(EXIT_SUCCESS);
    }

    // Skip packet if out of filter
    if( parse_and_check_pkt_fields( eth_buf ) )
      continue;

    // Send captured bytes amount to 'statistic collecting' thread
    ret = mq_send( mq_fd, (char*)bytes_amount_ptr, 4, 0 );

    if( ret == -1 ) {
      perror("mq_send");
      close( raw_socket );
      mq_close( mq_fd );
      mq_unlink( mq_name );
      exit(EXIT_SUCCESS);
    }

    printf("Success msg send\n");

  }

  // FIXME: Do really need if only SIGINT can terminate process?!
  //******************************************************************************
  // Close RAW socket
  //******************************************************************************

  close( raw_socket );
  */

  exit(EXIT_SUCCESS);
}
