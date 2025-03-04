#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdint.h>
#include <time.h>

#include "queue.h"

enum dhcp_msg_type {
     DHCP_DISCOVER = 1,
     DHCP_OFFER    = 2,
     DHCP_REQUEST  = 3,
     DHCP_ACK      = 5,
     DHCP_DECLINE  = 4,
     DHCP_NAK      = 6,
     DHCP_RELEASE  = 7,
     DHCP_INFORM   = 8,
};

enum{
    PAD = 0,
    END = 255,

    SUBNET_MASK = 1,
    TIME_OFFSET = 2,
    ROUTER = 3,
    TIME_SERVER = 4,
    NAME_SERVER = 5,
    DOMAIN_NAME_SERVER = 6,
    LOG_SERVER = 7,
    COOKIE_SERVER = 8,
    LPR_SERVER = 9,
    IMPRESS_SERVER = 10,
    RESOURCE_LOCATION_SERVER = 11,
    HOST_NAME = 12,
    BOOT_FILE_SIZE = 13,
    MERIT_DUMP_FILE = 14,
    DOMAIN_NAME = 15,
    SWAP_SERVER = 16,
    ROOT_PATH = 17,
    EXTENSIONS_PATH = 18,

    //parametrii IP - host
    IP_FORWARDING = 19,
    NON_LOCAL_SOURCE_ROUTING = 20,
    POLICY_FILTER = 21,
    MAXIMUM_DATAGRAM_REASSEMBLY_SIZE = 22,
    DEFAULT_IP_TIME_TO_LIVE = 23,
    PATH_MTU_AGING_TIMEOUT = 24,
    PATH_MTU_PLATEAU_TABLE = 25,

    //parametrii IP - interfata
    INTERFACE_MTU = 26,
    ALL_SUBNETS_ARE_LOCAL = 27,
    BROADCAST_ADDRESS = 28,
    PERFORM_MASK_DISCOVERY = 29,
    MASK_SUPPLIER = 30,
    PERFORM_ROUTER_DISCOVERY = 31,
    ROUTER_SOLICITATION_ADDRESS = 32,
    STATIC_ROUTE = 33,

    //parametrii Link  - interfata
    TRAILER_ENCAPSULATION = 34,
    ARP_CACHE_TIMEOUT = 35,
    ETHERNET_ENCAPSULATION = 36,

    //parametrii TCP
    TCP_DEFAULT_TTL = 37,
    TCP_KEEPALIVE_INTERVAL = 38,
    TCP_KEEPALIVE_GARBAGE = 39,

    //parametrii application and service
    NETWORK_INFORMATION_SERVICE_DOMAIN = 40,
    NETWORK_INFORMATION_SERVERS = 41,
    NETWORK_TIME_PROTOCOL_SERVERS = 42,
    VENDOR_SPECIFIC_INFORMATION = 43,
    NETBIOS_OVER_TCP_IP_NAME_SERVER = 44,
    NETBIOS_OVER_TCP_IP_DATAGRAM_DISTRIBUTION_SERVER = 4,
    NETBIOS_OVER_TCP_IP_NODE_TYPE = 46,
    NETBIOS_OVER_TCP_IP_SCOPE = 47,
    X_WINDOW_SYSTEM_FONT_SERVER = 48,
    X_WINDOW_SYSTEM_DISPLAY_MANAGER = 49,
    NETWORK_INFORMATION_SERVICE_PLUS_DOMAIN = 64,
    NETWORK_INFORMATION_SERVICE_PLUS_SERVERS = 65,
    MOBILE_IP_HOME_AGENT = 68,
    SMTP_SERVER = 69,
    POP3_SERVER = 70,
    NNTP_SERVER = 71,
    DEFAULT_WWW_SERVER = 72,
    DEFAULT_FINGER_SERVER = 73,
    DEFAULT_IRC_SERVER = 74,
    STREETTALK_SERVER = 75,
    STREETTALK_DIRECTORY_ASSISTANCE_SERVER = 76,

    //extensii DHCP
    REQUESTED_IP_ADDRESS = 50,
    IP_ADDRESS_LEASE_TIME = 51,
    OPTION_OVERLOAD = 52,
    TFTP_SERVER_NAME = 66,
    BOOTFILE_NAME = 67,
    DHCP_MESSAGE_TYPE = 53,
    SERVER_IDENTIFIER = 54,
    PARAMETER_REQUEST_LIST = 55,
    MESSAGE = 56,
    MAXIMUM_DHCP_MESSAGE_SIZE = 57,
    RENEWAL_T1_TIME_VALUE = 58,
    REBINDING_T2_TIME_VALUE = 59,
    VENDOR_CLASS_IDENTIFIER = 60,
    CLIENT_IDENTIFIER = 61

};

//parametrii unei optiuni dhcp
struct dhcp_option
{
    uint8_t id;
    uint8_t len;
    uint8_t data[256];
};

typedef struct dhcp_option dhcp_option;

typedef TAILQ_HEAD(dhcp_option_list_, dhcp_option) DHCP_OPTION_LIST;
typedef struct dhcp_option_list_ dhcp_option_list;

//functii parsare
int parse_byte (char *s, void **p);
int parse_byte_list (char *s, void **p);
int parse_short (char *s, void **p);
int parse_short_list (char *s, void **p);
int parse_long (char *s, void **p);
int parse_string (char *s, void **p);
int parse_ip (char *s, void **p);
int parse_ip_list (char *s, void **p);
int parse_mac (char *s, void **p);

void init_option_list (dhcp_option_list *list);
uint8_t parse_option (dhcp_option *option, char *name, char *value);
dhcp_option * search_option (dhcp_option_list *list, uint8_t id);
void print_options (dhcp_option_list *list);
void append_option (dhcp_option_list *list, dhcp_option *opt);
int parse_options_to_list (dhcp_option_list *list, dhcp_option *opts, size_t len);
size_t serialize_option_list (dhcp_option_list *list, uint8_t *buf, size_t len);
void delete_option_list (dhcp_option_list *list);


#endif