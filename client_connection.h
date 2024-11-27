#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>

#include "dhcp_structure.h"

#define BUFFER_SIZE 512

int setup_client_socket();
void communicate_with_server(int client_socket);

void get_mac_address(uint8_t *mac, const char *interface);
void init_dhcp_discover(dhcp_message *message, uint8_t *mac_address);
void send_dhcp_discover(int client_socket, struct sockaddr_in *server_addr, dhcp_message *message);

