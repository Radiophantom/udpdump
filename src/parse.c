#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>
#include <string.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <parse.h>

#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ether.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <errno.h>

int parse_args( struct settings_struct *filter_settings, int argc, char *argv[] ) {
  int c;
  int option_index = 0;

  while(1) {
    static struct option long_options[] = {
      {"dst-ip",   required_argument, 0, 0 },
      {"src-ip",   required_argument, 0, 0 },
      {"dst-port", required_argument, 0, 0 },
      {"src-port", required_argument, 0, 0 },
      {"if",       required_argument, 0, 0 },
      {0,          0,                 0, 0 }
    };

    c = getopt_long_only(argc, argv, "", long_options, &option_index);

    struct in_addr ip_struct;
    long port;
    char *port_char_ptr;

    if(c == -1) {
      break;
    } else if (c == 0) {
      switch( option_index ) {
        case( 0 ):
          if(inet_aton(optarg, &ip_struct) == -1){
            printf("Invalid \"%s\" argument value: %s\n", long_options[option_index].name, optarg);
            exit(EXIT_FAILURE);
          }
          filter_settings -> dst_ip = (uint32_t)ip_struct.s_addr;
          printf("%X\n", filter_settings -> dst_ip);
          filter_settings -> filter_mask |= DST_IP_FILTER_EN;
          break;
        case( 1 ):
          if(inet_aton(optarg, &ip_struct) == -1){
            printf("Invalid \"%s\" argument value: %s\n", long_options[option_index].name, optarg);
            exit(EXIT_FAILURE);
          }
          filter_settings -> src_ip = (uint32_t)ip_struct.s_addr;
          printf("%X\n", filter_settings -> src_ip);
          filter_settings -> filter_mask |= SRC_IP_FILTER_EN;
          break;
        case( 2 ):
          errno = 0;
          port = strtol(optarg, &port_char_ptr, 0);
          if(errno != 0) {
            perror("strtol");
            exit(EXIT_FAILURE);
          };
          if(*port_char_ptr != '\0'){
            printf("Invalid \"%s\" argument value: %s\n", long_options[option_index].name, optarg);
            exit(EXIT_FAILURE);
          };
          if( port > 65535 ) {
            printf("Invalid \"%s\" argument value: %s\n", long_options[option_index].name, optarg);
            exit(EXIT_FAILURE);
          };
          filter_settings -> dst_port = (uint16_t)port;
          filter_settings -> filter_mask |= DST_PORT_FILTER_EN;
          break;
        case( 3 ):
          errno = 0;
          port = strtol(optarg, &port_char_ptr, 0);
          if(errno != 0)
           perror("strtol");
          if(*port_char_ptr != '\0'){
            printf("Invalid \"%s\" argument value: %s\n", long_options[option_index].name, optarg);
            exit(EXIT_FAILURE);
          };
          if( port > 65535 ) {
            printf("Invalid \"%s\" argument value: %s\n", long_options[option_index].name, optarg);
            exit(EXIT_FAILURE);
          };
          filter_settings -> filter_mask |= SRC_PORT_FILTER_EN;
          break;
        case( 4 ):
          strcpy(filter_settings -> iface_name, optarg);
          break;
        default:
          exit(EXIT_FAILURE);
      }
    } else {
      exit(EXIT_FAILURE);
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

int parse_and_check_pkt_fields( struct settings_struct *filter_settings, char *eth_buf, int eth_buf_len ) {

  struct ethhdr *eth_hdr;
  struct iphdr  *ip_hdr;
  struct udphdr *udp_hdr;

  eth_hdr = (struct ethhdr*)eth_buf;
  //eth_buf += sizeof(struct ethhdr);
  ip_hdr  = (struct iphdr* )(eth_buf + sizeof(struct ethhdr));
  //eth_buf += sizeof(struct iphdr);
  udp_hdr = (struct udphdr*)(eth_buf + sizeof(struct ethhdr) + sizeof(struct iphdr));

  eth_buf = eth_buf + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr);

  if(ntohs(eth_hdr -> h_proto) != ETHERTYPE_IP)
    return -1;
  if(ip_hdr -> version != 4)//IPVERSION)
    return -1;
  if(ip_hdr -> protocol != 17)//SOL_UDP)
    return -1;

  if(filter_settings -> filter_mask & SRC_IP_FILTER_EN)
    if(filter_settings -> src_ip != ip_hdr -> saddr)
      return -1;

  if(filter_settings -> filter_mask & DST_IP_FILTER_EN)
    if(filter_settings -> dst_ip != ip_hdr -> daddr)
      return -1;

  if(filter_settings -> filter_mask & SRC_PORT_FILTER_EN)
    if(filter_settings -> src_port != udp_hdr -> source)
      return -1;

  if(filter_settings -> filter_mask & DST_PORT_FILTER_EN)
    if(filter_settings -> dst_port != udp_hdr -> dest)
      return -1;

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
      printf("UDP data:\n");
      for( int i = 0; i < ntohs(udp_hdr -> len)-8; i++ )
        printf("%X", *eth_buf++);
      printf("\n");
#endif

  return 0;
}
