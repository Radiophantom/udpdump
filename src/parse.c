#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <parse.h>

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

int find_args( const char *settings [], char *argv ) {
  for( int i = 0; i < 5; i++ )
    if( strcmp( settings[i], argv ) == 0 )
      return i;
  return -1;
}

int parse_args( const char *settings [], struct settings_struct *filter_settings, int argc, char *argv[] ) {
  int arg_ind;

  for( int i = 1; i < argc; i = i+2 ) {

    if( ( arg_ind = find_args( settings, argv[i] ) ) == -1 ) {
      printf("Unknown option \"%s\"\n", argv[i]);
      return 1;
    }

    if( i > argc - 2 ) {
      printf("\"%s\" option argument missed\n", argv[i]);
      return 1;
    }

    switch( arg_ind ) {
      case( 0 ):
        if( parse_ip( argv[i+1], &filter_settings -> dst_ip ) == -1 ) {
          printf("Invalid \"%s\" argument value: %s\n", argv[i], argv[i+1]);
          exit(EXIT_FAILURE);
        }
        filter_settings -> filter_mask |= DST_IP_FILTER_EN;
        break;
      case( 1 ):
        if( parse_ip( argv[i+1], &filter_settings -> src_ip ) == -1 ) {
          printf("Invalid \"%s\" argument value: %s\n", argv[i], argv[i+1]);
          exit(EXIT_FAILURE);
        }
        filter_settings -> filter_mask |= SRC_IP_FILTER_EN;
        break;
      case( 2 ):
        if( parse_port( argv[i+1], &filter_settings -> dst_port ) == -1 ) {
          printf("Invalid \"%s\" argument value: %s\n", argv[i], argv[i+1]);
          exit(EXIT_FAILURE);
        }
        filter_settings -> filter_mask |= DST_PORT_FILTER_EN;
        break;
      case( 3 ):
        if( parse_port( argv[i+1], &filter_settings -> src_port ) == -1 ) {
          printf("Invalid \"%s\" argument value: %s\n", argv[i], argv[i+1]);
          exit(EXIT_FAILURE);
        }
        filter_settings -> filter_mask |= SRC_PORT_FILTER_EN;
        break;
      case( 4 ):
        strcpy( filter_settings -> iface_name, argv[i+1] );
        break;
    }
  }

  if( strlen(filter_settings -> iface_name) == 0 ) {
    printf("Interface name must be set\n");
    return 1;
  }

#ifdef DEBUG
  if( filter_settings -> filter_mask & DST_IP_FILTER_EN )
    printf("Destination ip is %x\n",    filter_settings -> dst_ip    );
  if( filter_settings -> filter_mask & SRC_IP_FILTER_EN )
    printf("Source ip is %x\n",         filter_settings -> src_ip    );
  if( filter_settings -> filter_mask & DST_PORT_FILTER_EN )
    printf("Destination port is %0d\n", filter_settings -> dst_port  );
  if( filter_settings -> filter_mask & SRC_PORT_FILTER_EN )
    printf("Source port is %0d\n",      filter_settings -> src_port  );
  printf("Interface name is %s\n",    filter_settings -> iface_name);
#endif

  return 0;
}

/*
int parse_and_check_pkt_fields( struct settings_struct *filter_settings, char *eth_buf ) {

  u_int32_t *ip;
  u_int16_t *port;

  // Skip some header fields till IP-fields
  eth_buf = eth_buf + 12 + 2;

  // Cast to IP 32-bit fields
  ip = (u_int32_t*)eth_buf;

  if( filter_settings -> filter_mask & SRC_IP_FILTER_EN )
    if( filter_settings -> src_ip != ntohl(*ip++) )
      return 1;

  if( filter_settings -> filter_mask & DST_IP_FILTER_EN )
    if( filter_settings -> dst_ip != ntohl(*ip++) )
      return 1;

  // Cast to UDP 16-bit fields
  port = (u_int16_t*)ip;

  if( filter_settings -> filter_mask & SRC_PORT_FILTER_EN )
    if( filter_settings -> src_port != ntohs(*port++) )
      return 1;

  if( filter_settings -> filter_mask & DST_PORT_FILTER_EN )
    if( filter_settings -> dst_port != ntohs(*port++) )
      return 1;

  return 0;
}
*/
