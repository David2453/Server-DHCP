#ifndef DHCP_STRUCTURE_H
#define DHCP_STRUCTURE_H

#include <stdint.h>

enum ports {
    BOOTPS = 67,
    BOOTPC = 68
};

enum op_types {
    BOOTREQUEST = 1,
    BOOTREPLY   = 2,   
};

enum hardware_address_types {
    ETHERNET     = 0x01,
    ETHERNET_LEN = 0x06,
};

enum{
    DHCP_HEADER_SIZE=236
};

struct dhcp_message
{
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    
    uint32_t xid;
    
    uint16_t secs;
    uint16_t flags;
    
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    
    uint8_t chaddr[16];
    
    uint8_t sname[64];
    
    uint8_t file[128];
    
    uint8_t options[312];

};

typedef struct dhcp_message dhcp_message;

#endif