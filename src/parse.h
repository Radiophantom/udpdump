
int parse_port ( char *str_ip, u_int16_t *port );
int parse_ip ( char *str_ip, u_int32_t *ip );
int parse_args( char *settings [5], struct settings_struct *filter_settings, int argc, char *argv[] );
int parse_and_check_pkt_fields( struct settings_struct *filter_settings, char *eth_buf );

