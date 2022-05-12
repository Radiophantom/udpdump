#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_ip ( char *str_ip ) {
  int ip = 0;
  char *tmp;

  printf("%s\n",str_ip);

  tmp = strtok(str_ip, ".");

  printf("%s\n",tmp);
  for( int i = 0; i < 4; i++ ) {
      ip = ( ip << 8 ) | atoi(tmp);
    }
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
    uint dst_ip;
    uint src_ip;

    uint dst_port;
    uint src_port;
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
          settings_struct.src_ip = atoi( argv[i+1] );
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
  printf("Source ip is %d\n", settings_struct.src_ip);
  printf("Destination port is %d\n", settings_struct.dst_port);
  printf("Source port is %d\n", settings_struct.src_port);

  exit(EXIT_SUCCESS);
}
