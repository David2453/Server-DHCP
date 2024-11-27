#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

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

#include "client_connection.h"
#include "dhcp_structure.h"
#include "options.h"


int setup_client_socket()
{
    int client_socket;
    struct sockaddr_in client_addr;
    
    client_socket=socket(AF_INET, SOCK_DGRAM, 0);
    if(client_socket < 0)
    {
        perror("Couldn't connect to client socket! ");
        exit(EXIT_FAILURE);
    }

    memset(&client_addr,0,sizeof(client_addr));
    client_addr.sin_family=AF_INET;
    client_addr.sin_port=htons(BOOTPC);
    client_addr.sin_addr.s_addr=inet_addr("127.0.0.1"); //adresa IP server
    //client_addr.sin_addr.s_addr=INADDR_ANY;

    if (bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) 
    {
        perror("Couldn't connect to client port!");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    return client_socket;

}

void communicate_with_server(int client_socket)
{
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    const char *message ="Hello! I'm the client you ve been waiting for!";


    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(BOOTPS);
    server_addr.sin_addr.s_addr=inet_addr("127.0.0.1");

    ssize_t send_bytes=sendto(client_socket, message, strlen(message), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    if(send_bytes < 0)
    {
        perror("Error: Couldn't send message to server!");
        close (client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Message sent to server: %s\n", message);

    socklen_t server_len=sizeof(server_addr);
    ssize_t recv_bytes=recvfrom(client_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &server_len);

    if(recv_bytes<0)
    {
        perror("Error: Couldn't receive a message from server");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    //pentru primul masaj trnasmis de la tastatura
    buffer[recv_bytes]='\0';
    printf("Response from server: %s\n", buffer);

    dhcp_message *offer_msg=(dhcp_message*)buffer;
    struct in_addr client_ip, server_ip;
    client_ip.s_addr = offer_msg->yiaddr;
    server_ip.s_addr = offer_msg->siaddr;

    printf("Received DHCP Offer:\n");
    printf("Your(client) IP Address: %s\n", inet_ntoa(client_ip));
    printf("Server IP Address: %s\n",inet_ntoa(server_ip));

}

void get_mac_address(uint8_t *mac, const char *interface)
{
    int sock=socket(AF_INET, SOCK_DGRAM, 0);
    struct ifreq ifr;

    if (sock<0)
    {
        perror("Error: Couldn't create the socket!");
        exit(EXIT_FAILURE);
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    if(ioctl(sock, SIOCGIFHWADDR, &ifr)<0)
    {
        perror("Error: Couldn't get the MAC address!");
        close(sock);
        exit(EXIT_FAILURE);
    }

    memcpy(mac, ifr.ifr_hwaddr.sa_data, ETHERNET_LEN);
    close(sock);

}

void init_dhcp_discover(dhcp_message *message, uint8_t *mac_address)
{
    memset(message, 0, sizeof(dhcp_message));

    message->op=BOOTREQUEST;
    message->htype=ETHERNET;
    message->hlen=ETHERNET_LEN;
    message->hops=0;
    message->xid=htonl(rand());
    message->secs=0;
    message->flags=0;

    memcpy(message->chaddr, mac_address, ETHERNET_LEN);
}

void send_dhcp_discover(int client_socket, struct sockaddr_in *server_addr, dhcp_message *discover_message)
{
    ssize_t send_bytes=sendto(client_socket,discover_message, sizeof(discover_message), 0, (struct sockaddr*)server_addr, sizeof(*server_addr));

    if (send_bytes < 0)
    {
        perror("Error: Couldn't send DHCP discover message!");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("DHCP discover message sent!\n");
   
}

int main()
{
    int client_socket = setup_client_socket();

    uint8_t mac_address[ETHERNET_LEN];
    dhcp_message discover_message;

    communicate_with_server(client_socket);

    // get_mac_address(mac_address,"enp0s3");

    // init_dhcp_discover(&discover_message, mac_address);

    // struct sockaddr_in server_addr;
    // memset(&server_addr, 0, sizeof(server_addr));
    // server_addr.sin_family = AF_INET;
    // server_addr.sin_port = htons(BOOTPS);
    // server_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    // send_dhcp_discover(client_socket, &server_addr, &discover_message);

    

    close(client_socket);
    
    
    
    return 0;
}

#endif
