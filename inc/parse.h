
#ifndef _PARSE_H

#define _PARSE_H 1

#include <accum_stat.h>

#define DST_PORT_FILTER_EN ( 1 << 0 )
#define SRC_PORT_FILTER_EN ( 1 << 1 )
#define DST_IP_FILTER_EN   ( 1 << 2 )
#define SRC_IP_FILTER_EN   ( 1 << 3 )

int parse_port ( char *str_ip, u_int16_t *port );
int parse_ip ( char *str_ip, u_int32_t *ip );
int parse_args( struct settings_struct *filter_settings, int argc, char *argv[] );
int parse_and_check_pkt_fields( struct settings_struct *filter_settings, char *eth_buf, int eth_buf_len );

#endif /* <parse.h> included. */
