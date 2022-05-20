
#define IS_IPV4   0x45
#define IS_UDP    0x17
#define IS_ETHII  0x8000

#define DST_PORT_FILTER_EN ( 1 << 0 )
#define SRC_PORT_FILTER_EN ( 1 << 1 )
#define DST_IP_FILTER_EN   ( 1 << 2 )
#define SRC_IP_FILTER_EN   ( 1 << 3 )

struct settings_struct {
  u_int32_t filter_mask;
  u_int32_t dst_ip;
  u_int32_t src_ip;
  u_int16_t dst_port;
  u_int16_t src_port;
  char      iface_name [100];
};
