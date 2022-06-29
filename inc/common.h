#ifndef _COMMON_H
#define _COMMON_H 1

#define ACCUM_QUEUE_NAME   "/udpdump-accum-stat-util-q"
#define DISPLAY_QUEUE_NAME "/udpdump-get-stat-util-q"

struct settings_struct {
  u_int32_t filter_mask;
  u_int32_t dst_ip;
  u_int32_t src_ip;
  u_int16_t dst_port;
  u_int16_t src_port;
  int       iface_valid;
  char      iface_name [100];
};

#endif
