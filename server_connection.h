#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

#include <stdint.h>
#include<sys/types.h>
#include<netinet/in.h>

#include "dhcp_structure.h"

#define BUFFER_SIZE 512

int setup_server_socket();
void handle_client(int server_socket);

#endif