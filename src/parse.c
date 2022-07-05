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

void print_help_msg() {
  fprintf(stdout, "--help [-h]  - Show help info message.\n");
  fprintf(stdout, "--if         - Interface to sniffing traffic.\n");
  fprintf(stdout, "--dst-ip     - Destination ip filter (Optional).\n");
  fprintf(stdout, "--src-ip     - Source ip filter (Optional).\n");
  fprintf(stdout, "--dst-port   - Destination UDP port filter (Optional).\n");
  fprintf(stdout, "--src-port   - Source UDP port filter (Optional).\n");
  fprintf(stdout, "\n");
  fprintf(stdout, "Example:\n");
  fprintf(stdout, "udpdump --if eth --dst-ip 192.168.1.1 --src-ip 192.168.1.2 --dst-port 50000 --src-port 50000\n");
  fprintf(stdout, "\n");
}

int get_ip_port(char *port_str, u_int16_t *ip_port) {
  long port;
  char *port_str_ptr;

  errno = 0;
  port = strtol(port_str, &port_str_ptr, 0);
  if(errno != 0) {
    perror("strtol");
    return -1;
  };
  if(*port_str_ptr != '\0'){
    return -1;
  };
  if(port > 65535) {
    return -1;
  };
  *ip_port = (uint16_t)port;
  return 0;
}

int get_ip_addr(char *ip_str, u_int32_t *ip_addr) {
  struct in_addr ip_struct;
  if(inet_aton(ip_str, &ip_struct) == 0){
    return -1;
  }
  *ip_addr = (u_int32_t)ip_struct.s_addr;
  return 0;
}

int parse_args( struct settings_struct *filter_settings, int argc, char *argv[] ) {
  int c;
  int option_index = 0;

  if(argc == 1) {
    print_help_msg();
    return 1;
  }

  while(1) {
    static struct option long_options[] = {
      {"dst-ip",   required_argument, 0,  0 },
      {"src-ip",   required_argument, 0,  0 },
      {"dst-port", required_argument, 0,  0 },
      {"src-port", required_argument, 0,  0 },
      {"if",       required_argument, 0,  0 },
      {"help",     no_argument,       0, 'h'},
      {0,          0,                 0,  0 }
    };

    c = getopt_long(argc, argv, "h", long_options, &option_index);

    if(c == -1) {
      break;
    }

    switch(c) {
      case('?'):
        return -1;
      case('h'):
        print_help_msg();
        return 1;
      default:
        switch(option_index) {
          case(0):
            if(get_ip_addr(optarg, &filter_settings -> dst_ip)) {
              fprintf(stderr, "Invalid '%s' argument value: %s\n", long_options[option_index].name, optarg);
              return -1;
            }
            filter_settings -> filter_mask |= DST_IP_FILTER_EN;
            break;
          case(1):
            if(get_ip_addr(optarg, &filter_settings -> src_ip)) {
              fprintf(stderr, "Invalid '%s' argument value: %s\n", long_options[option_index].name, optarg);
              return -1;
            }
            filter_settings -> filter_mask |= SRC_IP_FILTER_EN;
            break;
          case(2):
            if(get_ip_port(optarg, &filter_settings -> dst_port)) {
              fprintf(stderr, "Invalid '%s' argument value: %s\n", long_options[option_index].name, optarg);
              return -1;
            }
            filter_settings -> filter_mask |= DST_PORT_FILTER_EN;
            break;
          case(3):
            if(get_ip_port(optarg, &filter_settings -> src_port)) {
              fprintf(stderr, "Invalid '%s' argument value: %s\n", long_options[option_index].name, optarg);
              return -1;
            }
            filter_settings -> filter_mask |= SRC_PORT_FILTER_EN;
            break;
          case(4):
            if(strlen(optarg) > 99) {
              fprintf(stderr, "Interface name is too long\n");
              return -1;
            }
            strcpy(filter_settings -> iface_name, optarg);
            filter_settings -> iface_valid = 1;
            break;
          default:
            fprintf(stderr, "'%s' - unknown argument was passed\n", optarg);
            return -1;
        }
    }
  }

  if(optind < argc) {
    while(optind < argc) {
      fprintf(stderr, "'%s' ", argv[optind++]);
    }
    fprintf(stderr, "non-option argument was passed\n");
    return -1;
  }
      
  if(filter_settings -> iface_valid == 0) {
    fprintf(stderr, "Interface name must be set\n");
    return -1;
  }

  return 0;
}

int parse_packet( struct settings_struct *filter_settings, char *eth_buf, int eth_buf_len ) {

  struct ethhdr *eth_hdr;
  struct iphdr  *ip_hdr;
  struct udphdr *udp_hdr;

  eth_hdr = (struct ethhdr*)eth_buf;
  ip_hdr  = (struct iphdr* )(eth_buf + sizeof(struct ethhdr));
  udp_hdr = (struct udphdr*)(eth_buf + sizeof(struct ethhdr) + sizeof(struct iphdr));

  // Move pointer to UDP-data field beginning
  eth_buf = eth_buf + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr);

  // Check whether packet is UDP or not
  if(eth_hdr -> h_proto != htons(ETHERTYPE_IP))
    return -1;
  if(ip_hdr -> version != IPVERSION)
    return -1;
  if(ip_hdr -> protocol != SOL_UDP)
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

  return 0;
}
