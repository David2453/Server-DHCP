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

#define BUFFER_SIZE 512

int setup_server_socket();
void handle_client(int server_socket, ip_pool_ind *pool, binding_list *bindings, dhcp_config *config);

#endif