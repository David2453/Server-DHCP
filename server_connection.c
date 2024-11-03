#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#include "server_connection.h"

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
}

int main()
{
    int server_socket=setup_server_socket();
    printf("Server is open and it listen on %d port", BOOTPS);

    while(1)
    {
        handle_client(server_socket);
        
    }

    close (server_socket);
    return 0;
}