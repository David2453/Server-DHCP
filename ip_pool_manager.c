#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <arpa/inet.h>
#include <stdarg.h>

#include "queue.h"
#include "ip_pool_manager.h"
#define LOG_FILE "logfile.txt"

void init_binding_list(binding_list *list)
{
    LIST_INIT(list);
}

void log_operation(const char *format, ...)
{
     FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL)
    {
        perror("Error opening log file");
        return;
    }

    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);

    fclose(log_file);

}

ip_binding *add_binding(binding_list *list, uint32_t address, uint8_t *cident, uint8_t cident_len, int is_static)
{
    ip_binding *binding=calloc(1, sizeof(*binding));
    if (binding == NULL)
    {
        perror("Error allocating memory for binding");
        return NULL;
    }

    binding->address=address;
    binding->cident_len=cident_len;
    memcpy(binding->cident, cident, cident_len);
    binding->is_static=is_static;
    binding->status=PENDING; //initial, in starea de pending
    binding->binding_time=time(NULL);

    LIST_INSERT_HEAD(list, binding, pointers);
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &binding->address, ip_str, INET_ADDRSTRLEN);
    log_operation("Added binding: IP=%s, ClientID=%.*s, Static=%d\n", ip_str, cident_len, cident, is_static);

    return binding;
}

void remove_binding(ip_binding *binding)
{
    if(binding!=NULL)
    {
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &binding->address, ip_str, INET_ADDRSTRLEN);
        log_operation("Removing binding: IP=%s\n", ip_str);

        LIST_REMOVE(binding, pointers);
        free(binding);
    }
}

void update_bindings_statuses(binding_list *list)
{
    ip_binding *binding, *binding_time;
    time_t current_time=time(NULL);

    LIST_FOREACH_SAFE(binding, list, pointers,binding_time)
    {
        if(binding->status==PENDING && (current_time - binding->binding_time>3600))
        { 
            binding->status=EXPIRED;
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &binding->address, ip_str, INET_ADDRSTRLEN);
            log_operation("Binding expired: IP=%s\n", ip_str);
        }
    }
}

void cleanup_expired_bindings(binding_list *list)
{
    ip_binding *binding, *binding_time;
    LIST_FOREACH_SAFE(binding, list, pointers, binding_time)
    {
        if (binding->status == EXPIRED)
        {
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &binding->address, ip_str, INET_ADDRSTRLEN);
            log_operation("Cleaning up expired binding: IP=%s\n", ip_str);

            remove_binding(binding);
        }
    }

}

ip_binding *search_binding(binding_list *list, const uint8_t *cident, uint8_t cident_len, int is_static, int status)
{
    ip_binding *binding, *binding_temp;
    LIST_FOREACH_SAFE(binding, list,pointers, binding_temp)
    {
     if((binding->is_static == is_static || is_static == STATIC_OR_DYNAMIC) && binding->cident_len == cident_len && memcmp(binding->cident, cident, cident_len) == 0) 
        {
            if(status == 0)
        {     
                return binding;
        }
        else if(status == binding->status)
            {
                return binding;
            }
        }
    }
    return NULL;
}

static uint32_t take_free_address(ip_pool_ind *indexes)
{
    if(indexes->ip_current<=indexes->ip_last)
    {
        uint32_t address=indexes->ip_current;
        indexes->ip_current=htonl(ntohl(indexes->ip_current)+1);
        return address;
    }

    return 0;
}


ip_binding *new_dynamic_binding(binding_list *list, ip_pool_ind *indexes, uint32_t address, uint8_t *cident, uint8_t cident_len)
{
    ip_binding *binding, *binding_temp;
    ip_binding *found_binding=NULL;
    if(address!=0)
    {
        LIST_FOREACH_SAFE(binding, list, pointers, binding_temp)
        {
            if(binding->address==address)
            {
                found_binding=binding;
                
            }
        }
    }

    if(found_binding!=NULL && !found_binding->is_static && found_binding->status!=PENDING && found_binding->status!=ASSOCIATED)
    {
        return found_binding;
    }
    else{
        uint32_t new_address = take_free_address(indexes);
        if(new_address!=0)
        {
            return add_binding(list, new_address, cident, cident_len, 0);
        }
        else
        {
                LIST_FOREACH_SAFE(binding, list, pointers, binding_temp)
                {
                    if(!binding->is_static && found_binding->status != PENDING && found_binding->status != ASSOCIATED)
                        return binding;
                }
            return NULL;
        }
    }

    return NULL;
}
