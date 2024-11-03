#ifndef DHCPSERVER_H
#define DHCPSERVER_H

#include <stdint.h>
#include <time.h>

#include "dhcp_structure.h"
#include "bindings.h"
#include "options.h"

struct address_pool
{
    uint32_t server_id;
    uint32_t netmask;
    uint32_t gateway;

    char device[16];

    pool_indexes indexes;   //delimitare adrese valabile

    time_t lease_time;
    time_t pending_time;

    dhcp_option_list options;

    binding_list bindings;
};

typedef struct address_pool address_pool;

struct dhcp_msg {
    dhcp_message hdr;
    dhcp_option_list opts;
};

typedef struct dhcp_msg dhcp_msg;

#endif