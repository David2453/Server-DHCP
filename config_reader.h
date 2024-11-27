#ifndef CONFIG_READER_H
#define CONFIG_READER_H

#include <stdint.h>
#include "ip_pool_manager.h"

struct dhcp_config
{
    uint32_t ip_pool_start;
    uint32_t ip_pool_end;
    uint32_t subnet;
    uint32_t netmask;
    uint32_t router;
    uint32_t dns[2];
    int default_lease_time;
    int max_lease_time;
};
typedef struct dhcp_config dhcp_config;


int read_config_dhcp(const char *config_file_path, dhcp_config *config, binding_list *bindings,ip_pool_ind *pool_indexes);

#endif