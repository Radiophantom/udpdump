#ifndef _ACCUM_STAT_H
#define _ACCUM_STAT_H   1

struct settings_struct {
  u_int32_t filter_mask;
  u_int32_t dst_ip;
  u_int32_t src_ip;
  u_int16_t dst_port;
  u_int16_t src_port;
  int       iface_valid;
  char      iface_name [100];
};

void *accum_stat( void *msg_queue_name );

#endif /* <accum_stat.h> included. */
