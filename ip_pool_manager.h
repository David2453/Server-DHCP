#ifndef IP_POOL_MANAGER_H
#define IP_POOL_MANAGER_H

#include <stdint.h>
#include <time.h>

#include "queue.h"
#include "options.h"


//lista dublu inlantuita

enum{
    DYNAMIC=0,
    STATIC=1,
    STATIC_OR_DYNAMIC=2
};

enum{
    EMPTY=0,
    ASSOCIATED,
    PENDING,
    EXPIRED,
    RELEASED
};

struct ip_pool_ind
{
    uint32_t ip_first;
    uint32_t ip_last;
    uint32_t ip_current;
};
typedef struct ip_pool_ind ip_pool_ind;

struct ip_binding
{
    uint32_t address;
    uint8_t cident_len;
    uint8_t cident[256];

    time_t binding_time;
    time_t lease_time;

    int status;
    int is_static;

    LIST_ENTRY(ip_binding) pointers;
};
typedef struct ip_binding ip_binding;

//macro uri
typedef LIST_HEAD(ip_binding_list, ip_binding) BINDING_LIST_HEAD;
typedef struct ip_binding_list binding_list;

void init_binding_list(binding_list *list);
ip_binding *add_binding (binding_list *list, uint32_t address, uint8_t *cident, uint8_t cident_len, int is_static);
void remove_binding (ip_binding *binding);

void update_bindings_statuses (binding_list *list);
void cleanup_expired_bindings(binding_list *list);

ip_binding *search_binding (binding_list *list, uint8_t *cident, uint8_t cident_len, int is_static, int status);
ip_binding *new_dynamic_binding (binding_list *list, ip_pool_ind *indexes, uint32_t address, uint8_t *cident, uint8_t cident_len);


#endif