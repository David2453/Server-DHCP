#ifndef BINDINGS_H
#define BINDINGS_H

#include <stdint.h>
#include <time.h>

#include "queue.h"
#include "options.h"

//tip de asociere
enum {
    DYNAMIC = 0,
    STATIC  = 1,
    STATIC_OR_DYNAMIC = 2
};

//binding status
enum {
    EMPTY = 0,
    ASSOCIATED,
    PENDING,
    EXPIRED,
    RELEASED
};

//adrese IP care delimiteaza pool-ul
struct pool_indexes
{
    uint32_t first;
    uint32_t last;
    uint32_t current;
};

typedef struct pool_indexes pool_indexes;

//binding - organizata ca o lista dublu inlantuita 
//definita in fisierul queue(3)

struct address_binding
{
    uint32_t address;
    uint8_t cident_len;
    uint8_t cident[256];

    time_t binding_time;
    time_t lease_time;

    int status;
    int is_static;

    LIST_ENTRY(address_binding) pointers; //din queue (3)
};

typedef struct address_binding address_binding;

typedef LIST_HEAD(binding_list_, address_binding) BINDING_LIST_HEAD;
typedef struct binding_list_ binding_list;


void init_binding_list(binding_list *list);

address_binding *add_binding(binding_list *list, uint32_t address, uint8_t *cident, uint8_t cident_len, int ist_static);
void remove_binding (address_binding *binding);

void update_bindings_statuses (binding_list *list);

address_binding *search_binding (binding_list *list, uint8_t *cident, uint8_t cident_len, int is_static, int status);
address_binding *new_dynamic_binding (binding_list *list, pool_indexes *indexes, uint32_t address, uint8_t *cident, uint8_t cident_len);


#endif