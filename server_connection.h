#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

#define LOG_FILENAME "logfile.txt"

#include <stdint.h>
#include<sys/types.h>
#include<netinet/in.h>

#include "dhcp_structure.h"
#include "ip_pool_manager.h"
#include "config_reader.h"
#include "options.h"

typedef struct ClientTask{

    int server_socket;
    ip_pool_ind* pool;
    struct ip_binding_list* bindings;
    dhcp_config* config;
    char buffer[BUFFER_SIZE];
    socklen_t client_len;
    ssize_t recv_bytes;
    struct sockaddr_in client_addr;
}ClientTask;


#define BUFFER_SIZE 512

int setup_server_socket();
void handle_client(ClientTask* task);

#endif