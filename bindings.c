#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

#include "queue.h"
#include "bindings.h"

void init_binding_list(binding_list *list)
{
    LIST_INIT(list);
}

address_binding *add_binding(binding_list *list, uint32_t address, uint8_t *cident, uint8_t cident_len, int is_static)
{
    address_binding *binding = calloc(1, sizeof(*binding));

    binding->address = address;
    binding->cident_len = cident_len;
    memcpy(binding->cident, cident, cident_len);

    binding->is_static = is_static;

    LIST_INSERT_HEAD(list, binding, pointers);

    return binding;
}

void update_bindings_statuses (binding_list *list)
{
    address_binding *binding, *binding_temp;
    
    LIST_FOREACH_SAFE(binding, list, pointers, binding_temp)
    {
	    if(binding->binding_time + binding->lease_time < time(NULL)) 
        {
	        binding->status = EXPIRED;
	    }
    }
}

address_binding *search_binding (binding_list *list, uint8_t *cident, uint8_t cident_len, int is_static, int status)
{
    address_binding *binding, *binding_temp;
	
    LIST_FOREACH_SAFE(binding, list, pointers, binding_temp) 
    {
        if((binding->is_static == is_static || is_static == STATIC_OR_DYNAMIC) && binding->cident_len == cident_len && memcmp(binding->cident, cident, cident_len) == 0) 
       {

            if(status == 0)
            return binding;
            else if(status == binding->status)
            return binding;
	    }
    }

    return NULL;
}

static uint32_t take_free_address (pool_indexes *indexes)
{
    if(indexes->current <= indexes->last) 
    {

        uint32_t address = indexes->current;	
        indexes->current = htonl(ntohl(indexes->current) + 1);
        return address;

    } 
    else
	    return 0;
}

address_binding *new_dynamic_binding (binding_list *list, pool_indexes *indexes, uint32_t address, uint8_t *cident, uint8_t cident_len)
{
    address_binding *binding, *binding_temp;
    address_binding *found_binding = NULL;

    if (address != 0) 
    {
        LIST_FOREACH_SAFE(binding, list, pointers, binding_temp) 
        {
	    
            //se cauta un binding cu adresa IP solicitata
            if(binding->address == address) 
            {
                found_binding = binding;
                break;
            }
        }
    }

    if(found_binding != NULL &&
       !found_binding->is_static &&
       found_binding->status != PENDING &&
       found_binding->status != ASSOCIATED) 
    {

        // adresa IP solicitata este valabila 
        return found_binding;
        
    } else {

            /* the requested IP address is already in use, or no address has been
                requested, or the address requested has never been allocated
                (we do not support this last case and just return the next
                available address!). */

            uint32_t address = take_free_address(indexes);

            if(address != 0)
                return add_binding(list, address, cident, cident_len, 0);

            else { //se cauta orice adresa IP care ar putea fi expirata
            
                LIST_FOREACH_SAFE(binding, list, pointers, binding_temp)
                {
                    if(!binding->is_static &&
                    found_binding->status != PENDING &&
                    found_binding->status != ASSOCIATED)
                        return binding;
                }

                //nu mai sunt adrese IP valabile
                return NULL;
        }	
    }

    
    return NULL;
}