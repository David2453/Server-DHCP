#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#include "server_connection.h"
#include "dhcp_structure.h"

int setup_server_socket()
{
    int server_socket;
    struct sockaddr_in server_addr;

    server_socket=socket(AF_INET, SOCK_DGRAM, 0);
    if(server_socket < 0)
    {
        perror("Connection failed!");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    server_addr.sin_port=htons(BOOTPS);

    if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Couldn't connect to server port!");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    return server_socket;
}

void handle_client(int server_socket)
{
    struct sockaddr_in client_addr;
    socklen_t client_len=sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    
    ssize_t recv_bytes=recvfrom(server_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_len);
    if(recv_bytes < 0)
    {
        perror("Error: Couldn't receive a message from client!");
        return;
    }

    printf("Message received from the client: %s\n", buffer);

    const char *response="Hello! I'm the server. I received your message! ";
    ssize_t send_bytes= sendto(server_socket, response, strlen(response), 0, (struct sockaddr*)&client_addr, client_len);
    if (send_bytes <0 )
    {
        perror("Error: Couldn't send the message to client!");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    dhcp_message *request=(dhcp_message*)buffer;
    if(request->op==BOOTREQUEST)
    {
        if(request->xid)
        {
            printf("DHCP Discover received from client with MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                   request->chaddr[0], request->chaddr[1], request->chaddr[2],
                   request->chaddr[3], request->chaddr[4], request->chaddr[5]);
            //mesajul offer
            dhcp_message offer;
            memset(&offer, 0, sizeof(dhcp_message));
            offer.op = BOOTREPLY;
            offer.htype = request->htype;
            offer.hlen = request->hlen; 
            offer.xid = request->xid;
            //exemplu adrese IP oferite clientului
            offer.yiaddr = inet_addr("192.168.1.100");
            offer.siaddr = inet_addr("192.168.1.1");
             memcpy(offer.chaddr, request->chaddr, ETHERNET_LEN);

            ssize_t send_bytes = sendto(server_socket, &offer, sizeof(dhcp_message), 0, 
                                        (struct sockaddr*)&client_addr, client_len);
            if (send_bytes < 0)
            {
                perror("Error: Couldn't send DHCP Offer to client!");
                close(server_socket);
                exit(EXIT_FAILURE);
            }

            printf("DHCP Offer sent to client: IP address %s\n", inet_ntoa(*(struct in_addr *)&offer.yiaddr));
        }
    }
    else
        printf("Unexpected DHCP message received.\n");
    
    
}

int main()
{
    int server_socket=setup_server_socket();
    printf("Server is open and it listen on %d port.\n", BOOTPS);

    while(1)
    {
        handle_client(server_socket);
        
    }

    close (server_socket);
    return 0;
}