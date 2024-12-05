#define _POSIX_C_SOURCE 200809L

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include <fcntl.h>

#include "server_connection.h"
#include "dhcp_structure.h"
#include "ip_pool_manager.h"
#include "config_reader.h"
#include "options.h"

#define DHCP_OPTION_MSG_TYPE 53
#define DHCP_OPTIONS_MAX_LEN 276
#define CONFIG_FILENAME "/etc/dhcp/dhcpd.conf"

uint8_t get_dhcp_message_type(const uint8_t *options, size_t options_len)
{
    // size_t i = 0;
    // while (i < options_len) 

    // {
    //     uint8_t option_code=options[i];
    //     if (option_code == 0xFF) 
    //     { 
    //         break;
    //     }
    //     if(i+1>=options_len)
    //     {
    //         break;
    //     }

    //     uint8_t option_len=options[i+1];
    //     if(i+2+option_len>options_len)
    //     {
    //         break;
    //     }

    //     if (option_code == DHCP_OPTION_MSG_TYPE && option_len>=1) 
    //     {
    //         return options[i + 2]; //tip mesaj
    //     }
    //     i += 2 + option_len; //urmatoarea optiune
    // }
    // return 0; 


    size_t i = 4; //sar peste magic cookie
    while (i < options_len) 
    {
        printf("Option code: %u, Length: %u\n", options[i], options[i + 1]);
        if (options[i] == 0xFF) 
        { 
            break;
        }
        if (options[i] == DHCP_OPTION_MSG_TYPE) 
        {
            uint8_t option_len=options[i+1];
            if(option_len >= 1)
            {
                return options[i + 2]; 
            }
        }
        i += 2 + options[i + 1]; 
    }
    return 0; 
}


int setup_server_socket()
{
    int server_socket;
    struct sockaddr_in server_addr;

    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0)
    {
        perror("Connection failed!");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(BOOTPS);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Couldn't connect to server port!");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    return server_socket;
}

void print_dhcp_message(const dhcp_message *msg) {
    printf("========== DHCP Message ==========\n");
    fflush(stdout);

    printf("Operation (op): %u (%s)\n", msg->op,
           msg->op == BOOTREQUEST ? "BOOTREQUEST" : "BOOTREPLY");
    fflush(stdout);
    printf("Hardware type (htype): %u\n", msg->htype);
    fflush(stdout);
    printf("MAC address length (hlen): %u\n", msg->hlen);
    fflush(stdout);
    printf("Hops: %u\n", msg->hops);
    fflush(stdout);


    printf("Transaction ID (xid): 0x%08X\n", ntohl(msg->xid));
    fflush(stdout);

    printf("Seconds elapsed (secs): %u\n", ntohs(msg->secs));
    fflush(stdout);
    printf("Flags: 0x%04X\n", ntohs(msg->flags));
    fflush(stdout);

    struct in_addr addr;

    addr.s_addr = msg->ciaddr;
    printf("Client IP address (ciaddr): %s\n", inet_ntoa(addr));
    fflush(stdout);

    addr.s_addr = msg->yiaddr;
    printf("Your IP address (yiaddr): %s\n", inet_ntoa(addr));
    fflush(stdout);

    addr.s_addr = msg->siaddr;
    printf("Server IP address (siaddr): %s\n", inet_ntoa(addr));
    fflush(stdout);

    addr.s_addr = msg->giaddr;
    printf("Gateway IP address (giaddr): %s\n", inet_ntoa(addr));
    fflush(stdout);

    printf("Client MAC address (chaddr): ");
    for (int i = 0; i < msg->hlen && i < 16; i++) {
        printf("%02X", msg->chaddr[i]);
        if (i < msg->hlen - 1) printf(":");
    }
    printf("\n");
    fflush(stdout);

    printf("Server name (sname): %s\n", msg->sname);
    fflush(stdout);

    printf("Boot file name (file): %s\n", msg->file);
    fflush(stdout);

    printf("Options:\n");
    fflush(stdout);
    for (int i = 0; i < 312; i++) {
        if (msg->options[i] == 0xFF) { // 0xFF este terminatorul pt campul de opt
            break;
        }
        printf("  Option[%d]: 0x%02X\n", i, msg->options[i]);
        fflush(stdout);
    }

    printf("==================================\n");
    fflush(stdout);
}

int is_ip_available(ip_pool_ind *pool, binding_list *bindings, uint32_t ip_to_check)
{
    ip_binding *binding;
    if(ip_to_check < pool->ip_first || ip_to_check >pool->ip_last)
    {
        return 0;
    }

    LIST_FOREACH(binding, bindings, pointers)
    {
        if(binding->address==ip_to_check)
        {
            return 0;
        }
    }

    return 1;
}

uint32_t offer_ip_available(ip_pool_ind *pool, binding_list *bindings)
{
    uint32_t offered_ip=0;
    for(uint32_t ip=pool->ip_first; ip<=pool->ip_last;ip++)
    {
        if(is_ip_available(pool, bindings, ip))
        {
            offered_ip=ip;
            pool->ip_current=ip;
            break;
        }
    }
    return offered_ip;
}

void send_DHCP_Offer(int server_socket, const dhcp_message *request, const struct sockaddr_in *client_addr, socklen_t client_len, ip_pool_ind *pool, binding_list *bindings, dhcp_config *config)
{
    if(pool->ip_current >= pool->ip_last)
    {
        printf("NO available IP addresses in the pool\n");
        fflush(stdout);
        return;
    }

    //alocare dinamica ip
    uint32_t offered_ip=offer_ip_available(pool, bindings);
   
    if(offered_ip==0)
    {
        printf("NO available IP addresses in the pool\n");
        fflush(stdout);
        return;
    }
    
    //creare binding nou pentru ip-ul oferit
    ip_binding *new_binding=(ip_binding*)malloc(sizeof(ip_binding));
    if(!new_binding)
    {
        perror("Error allocating memory for new binding\n");
        return;
    }
    
    new_binding->address=offered_ip;
    new_binding->binding_time=time(NULL);
    new_binding->lease_time=new_binding->binding_time + config->max_lease_time;
    new_binding->status=1; //activ
    new_binding->is_static=0; //dinamic
    new_binding->cident_len=request->hlen;
    memcpy(new_binding->cident, request->chaddr, request->hlen);

    LIST_INSERT_HEAD(bindings, new_binding, pointers);

    dhcp_message offer;
    memset(&offer, 0, sizeof(dhcp_message));
    offer.op = BOOTREPLY;
    offer.htype = request->htype;
    offer.hlen = request->hlen; 
    offer.xid = request->xid;

    //adresele IP oferite dinamic
    offer.siaddr=config->router;
    offer.yiaddr=offered_ip;

//OPTIUNI SUPLIMENTARE

    offer.options[0]=DHCP_OPTION_MSG_TYPE;
    offer.options[1]=2; //DHCP Offer
    offer.options[2]=0xFF;
    // offer.options[2]=IP_ADDRESS_LEASE_TIME; //lease time
    // offer.options[3]=(uint8_t)((config->max_lease_time >> 24) & 0xFF);
    // offer.options[4] = (uint8_t)((config->max_lease_time >> 16) & 0xFF);
    // offer.options[5] = (uint8_t)((config->max_lease_time >> 8) & 0xFF);
    // offer.options[6] = (uint8_t)(config->max_lease_time & 0xFF);
    // offer.options[7]=SUBNET_MASK;
    // offer.options[8]=4; //nr bytes pt mascca de subretea
    // offer.options[9] = (uint8_t)((config->subnet>> 24) & 0xFF);
    // offer.options[10] = (uint8_t)((config->subnet>> 16) & 0xFF);
    // offer.options[11] = (uint8_t)((config->subnet >> 8) & 0xFF);
    // offer.options[12] = (uint8_t)(config->subnet & 0xFF);
    // offer.options[13]=ROUTER;
    // offer.options[14]=4; //nr bytes pt gateway/router
    // offer.options[15] = (uint8_t)((config->router>> 24) & 0xFF);
    // offer.options[16] = (uint8_t)((config->router>> 16) & 0xFF);
    // offer.options[17] = (uint8_t)((config->router>> 8) & 0xFF);
    // offer.options[18] = (uint8_t)(config->router & 0xFF);
    // offer.options[19]=DOMAIN_NAME_SERVER;
    // offer.options[20]=4; //nr bytes pt DNS
    // offer.options[21] = (uint8_t)((config->dns[0] >> 24) & 0xFF); 
    // offer.options[22] = (uint8_t)((config->dns[0] >> 16) & 0xFF);
    // offer.options[23] = (uint8_t)((config->dns[0] >> 8) & 0xFF);
    // offer.options[24] = (uint8_t)(config->dns[0] & 0xFF);

    //adresa MAC
    memcpy(offer.chaddr, request->chaddr, request->hlen);

    ssize_t send_bytes=sendto(server_socket, &offer, sizeof(dhcp_message),0,
                            (struct sockaddr*)client_addr, client_len);
    if (send_bytes < 0) 
    {
        perror("Error: Couldn't send DHCP Offer to client!");
        close(server_socket);
        exit(EXIT_FAILURE);
    } else
    {
        printf("DHCP Offer sent to client: IP address %s\n", inet_ntoa(*(struct in_addr *)&offer.yiaddr));
        fflush(stdout);
    
    }
    print_dhcp_message(&offer);
}

ip_binding *find_binding(binding_list *bindings, const uint8_t *cident, uint8_t cident_len)
{
    ip_binding *current_binding;
    LIST_FOREACH(current_binding, bindings, pointers)
    {
        if(current_binding->cident_len==cident_len && memcmp(current_binding->cident, cident, cident_len))
            return current_binding;
    }

    return NULL;
}

void send_DHCP_Ack(int server_socket, const dhcp_message *request, const struct sockaddr_in *client_addr, socklen_t client_len,ip_pool_ind *pool, binding_list *bindings, dhcp_config *config)
{
    //ip_binding *binding=find_binding(bindings, request->chaddr, request->hlen);
    ip_binding *binding=search_binding(bindings,request->chaddr,client_len,STATIC_OR_DYNAMIC,1);
    if(!binding /*|| binding->status != 1*/)
    {
        printf("No active binding for this client\n");
        return;
    } else
    {
        printf("Binding found: IP %s\n", inet_ntoa(*(struct in_addr *)&binding->address));
    }
    
    dhcp_message ack;
    memset(&ack, 0, sizeof(dhcp_message));
    ack.op = BOOTREPLY;
    ack.htype = request->htype;
    ack.hlen = request->hlen; 
    ack.xid = request->xid;
   
    ack.yiaddr=binding->address;
    ack.siaddr=config->router;

    ack.options[0]=DHCP_OPTION_MSG_TYPE;
    ack.options[1]=1; //lungime DHCP Ack
    ack.options[2]=DHCP_ACK; //ack
    ack.options[3] = IP_ADDRESS_LEASE_TIME; // Lease Time
    ack.options[4] = 4; // Lungime (4 octeți)
    ack.options[5] = (uint8_t)((config->max_lease_time >> 24) & 0xFF);
    ack.options[6] = (uint8_t)((config->max_lease_time >> 16) & 0xFF);
    ack.options[7] = (uint8_t)((config->max_lease_time >> 8) & 0xFF);
    ack.options[8] = (uint8_t)(config->max_lease_time & 0xFF);

    ack.options[9] = SUBNET_MASK; // Subnet mask (cod 1)
    ack.options[10] = 4; // Lungime
    ack.options[11] = (uint8_t)((config->subnet >> 24) & 0xFF);
    ack.options[12] = (uint8_t)((config->subnet >> 16) & 0xFF);
    ack.options[13] = (uint8_t)((config->subnet >> 8) & 0xFF);
    ack.options[14] = (uint8_t)(config->subnet & 0xFF);

    ack.options[15] = ROUTER; // Router (cod 3)
    ack.options[16] = 4; // Lungime
    ack.options[17] = (uint8_t)((config->router >> 24) & 0xFF);
    ack.options[18] = (uint8_t)((config->router >> 16) & 0xFF);
    ack.options[19] = (uint8_t)((config->router >> 8) & 0xFF);
    ack.options[20] = (uint8_t)(config->router & 0xFF);

    ack.options[21] = DOMAIN_NAME_SERVER; // DNS (cod 6)
    ack.options[22] = 4; // Lungime
    ack.options[23] = (uint8_t)((config->dns[0] >> 24) & 0xFF);
    ack.options[24] = (uint8_t)((config->dns[0] >> 16) & 0xFF);
    ack.options[25] = (uint8_t)((config->dns[0] >> 8) & 0xFF);
    ack.options[26] = (uint8_t)(config->dns[0] & 0xFF);

    ack.options[27] = 0xFF; // Terminator
    memcpy(ack.chaddr, request->chaddr, request->hlen);

    ssize_t send_bytes = sendto(server_socket, &ack, sizeof(dhcp_message), 0, 
                                (struct sockaddr*)client_addr, client_len);
    if (send_bytes < 0)
    {
        perror("Error: Couldn't send DHCP ACK to client!");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("DHCP ACK sent to client: IP address %s\n", inet_ntoa(*(struct in_addr *)&ack.yiaddr));
        fflush(stdout);
    }

    print_dhcp_message(&ack);
}

void handle_DHCP_Decline(dhcp_message *request, binding_list *bindings)
{
    printf("\n\nReceived DHCPDECLINE from client\n");
    uint32_t declined_ip=ntohl(request->yiaddr);
    uint8_t *client_id = request->chaddr;
    uint8_t client_id_len = request->hlen;

    ip_binding *binding = search_binding(bindings, client_id, client_id_len, DYNAMIC, ASSOCIATED);
    if (binding != NULL && binding->address == declined_ip)
    {
        remove_binding(binding);
        printf("IP address %u.%u.%u.%u is declined\n", (declined_ip >> 24) & 0xFF, (declined_ip >> 16) & 0xFF, 
                                                        (declined_ip >> 8) & 0xFF, declined_ip & 0xFF);
    }


}

void handle_DHCP_Release(dhcp_message *request, binding_list *bindings)
{
    printf("\n\nReceived DHCPRELEASE from client\n");
    uint32_t released_ip=ntohl(request->ciaddr);
    uint8_t *client_id = request->chaddr;
    uint8_t client_id_len = request->hlen;

    ip_binding *binding = search_binding(bindings, client_id, client_id_len, STATIC_OR_DYNAMIC, ASSOCIATED);
    if (binding != NULL && binding->address == released_ip)
    {
        binding->status=RELEASED;
        binding->lease_time=0;
        printf("IP address %u.%u.%u.%u is released by client\n", (released_ip >> 24) & 0xFF, (released_ip >> 16) & 0xFF, 
                                                        (released_ip >> 8) & 0xFF, released_ip & 0xFF);
    }


}

void handle_DHCP_Inform_Reply(int server_socket, const dhcp_message *request, const struct sockaddr_in *client_addr, socklen_t client_len,ip_pool_ind *pool, binding_list *bindings, dhcp_config *config)
{
    dhcp_message inform_reply;
    memset(&inform_reply, 0, sizeof(dhcp_message));

    inform_reply.op=BOOTREPLY;
    inform_reply.htype=request->htype;
    inform_reply.hlen=request->hlen;
    inform_reply.xid=request->xid;

    inform_reply.options[0]=DHCP_OPTION_MSG_TYPE;
    inform_reply.options[1]=8;
    inform_reply.options[2]=DOMAIN_NAME_SERVER;
    inform_reply.options[3]=4;
    inform_reply.options[4] = (uint8_t)((config->dns[0] >> 24) & 0xFF);
    inform_reply.options[5] = (uint8_t)((config->dns[0] >> 16) & 0xFF);
    inform_reply.options[6] = (uint8_t)((config->dns[0] >> 8) & 0xFF);
    inform_reply.options[7] = (uint8_t)(config->dns[0] & 0xFF);
    inform_reply.options[25]=0xFF;

    memcpy(inform_reply.chaddr, request->chaddr, request->hlen);

    ssize_t send_bytes = sendto(server_socket, &inform_reply, sizeof(dhcp_message), 0, (struct sockaddr *)client_addr, client_len);
    if (send_bytes < 0) {
        perror("Error sending DHCPINFORM Reply");
        close(server_socket);
        exit(EXIT_FAILURE);
    } else {
        printf("DHCPINFORM Reply sent successfully.\n");
        fflush(stdout);
    }

    print_dhcp_message(&inform_reply);
}

void send_DHCP_NACK(int server_socket, const dhcp_message *request, const struct sockaddr_in *client_addr, socklen_t client_len,ip_pool_ind *pool, binding_list *bindings, dhcp_config *config)
{
    dhcp_message nack;
    memset(&nack, 0, sizeof(dhcp_message));

    nack.op = BOOTREPLY;
    nack.htype = request->htype;
    nack.hlen = request->hlen;
    nack.xid = request->xid;

    nack.options[0] = DHCP_OPTION_MSG_TYPE;
    nack.options[1]=1;
    nack.options[2]=6;
    nack.options[3]=0xFF;

    memcpy(nack.chaddr, request->chaddr, request->hlen);

    ssize_t sent_bytes = sendto(server_socket, &nack, sizeof(dhcp_message), 0, (struct sockaddr *)client_addr, client_len);
    if (sent_bytes < 0) {
        perror("Eroare la trimiterea DHCP NACK către client");
        close(server_socket);
        exit(EXIT_FAILURE);
    } else {
        printf("DHCP NACK trimis către client.\n");
        fflush(stdout);
    }

    print_dhcp_message(&nack);
}

void deserializare_dhcp_message(const char *buffer, dhcp_message *message)
{
    memcpy(&message->op, buffer, sizeof(message->op));
    memcpy(&message->htype, buffer + 1, sizeof(message->htype));
    memcpy(&message->hlen, buffer + 2, sizeof(message->hlen));
    memcpy(&message->hops, buffer + 3, sizeof(message->hops));

    // Conversii pentru endianness
    uint32_t xid_net;
    memcpy(&xid_net, buffer + 4, sizeof(message->xid));
    message->xid = ntohl(xid_net);

    memcpy(&message->secs, buffer + 8, sizeof(message->secs));
    memcpy(&message->flags, buffer + 10, sizeof(message->flags));
    memcpy(&message->ciaddr, buffer + 12, sizeof(message->ciaddr));
    memcpy(&message->yiaddr, buffer + 16, sizeof(message->yiaddr));
    memcpy(&message->siaddr, buffer + 20, sizeof(message->siaddr));
    memcpy(&message->giaddr, buffer + 24, sizeof(message->giaddr));
    memcpy(message->chaddr, buffer + 28, sizeof(message->chaddr));
    memcpy(message->sname, buffer + 44, sizeof(message->sname));
    memcpy(message->file, buffer + 108, sizeof(message->file));

    size_t options_len=DHCP_OPTIONS_MAX_LEN;
    memcpy(message->options, buffer + 236, options_len);

//setarea markerului de sfarsit
    for(size_t i=0;i<options_len;i++)
    {
        if(message->options[i]==0xFF)
        {
            return;
        }
    }

    if (options_len < DHCP_OPTIONS_MAX_LEN) 
    {
        message->options[options_len] = 0xFF;
    }
}

void handle_client(int server_socket, ip_pool_ind *pool, binding_list *bindings, dhcp_config *config)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    ssize_t recv_bytes = recvfrom(server_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_len);
    if (recv_bytes < 0)
    {
        perror("Error: Couldn't receive a message from client!");
        return;
    }

    printf("Message received from the client: %s\n", buffer);
    fflush(stdout);

    // const char *response = "Hello! I'm the server. I received your message! ";
    // ssize_t send_bytes = sendto(server_socket, response, strlen(response), 0, (struct sockaddr*)&client_addr, client_len);
    // if (send_bytes < 0)
    // {
    //     perror("Error: Couldn't send the message to client!");
    //     close(server_socket);
    //     exit(EXIT_FAILURE);
    // }

    // Convorbire prin mesaje DHCP

    dhcp_message *request = (dhcp_message*)buffer;
    //apelare functie de deserializare
    //dhcp_message *request;
    //deserializare_dhcp_message(buffer, request);

    uint8_t message_type=get_dhcp_message_type(request->options, sizeof(request->options));
    printf("DHCP Message type: %u\n", message_type);
    print_dhcp_message(request);
    fflush(stdout);
    

    // if (request->op == BOOTREQUEST)
    // {
    //     if (request->xid)
    //     {
    //         printf("DHCP Discover received from client with MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
    //                request->chaddr[0], request->chaddr[1], request->chaddr[2],
    //                request->chaddr[3], request->chaddr[4], request->chaddr[5]);
    //         fflush(stdout);

    //         // Mesajul offer
    //         dhcp_message offer;
    //         memset(&offer, 0, sizeof(dhcp_message));
    //         offer.op = BOOTREPLY;
    //         offer.htype = request->htype;
    //         offer.hlen = request->hlen; 
    //         offer.xid = request->xid;
    //         // Exemplu adrese IP oferite clientului
    //         offer.yiaddr = inet_addr("192.168.1.100");
    //         offer.siaddr = inet_addr("192.168.1.1");
    //         memcpy(offer.chaddr, request->chaddr, ETHERNET_LEN);

    //         ssize_t send_bytes = sendto(server_socket, &offer, sizeof(dhcp_message), 0, 
    //                                     (struct sockaddr*)&client_addr, client_len);
    //         if (send_bytes < 0)
    //         {
    //             perror("Error: Couldn't send DHCP Offer to client!");
    //             close(server_socket);
    //             exit(EXIT_FAILURE);
    //         }

    //         printf("DHCP Offer sent to client: IP address %s\n", inet_ntoa(*(struct in_addr *)&offer.yiaddr));
    //         fflush(stdout);
    //     }
    // }
    // else
    // {
    //     printf("Unexpected DHCP message received.\n");
    //     fflush(stdout);
    // }


    if(request->op==BOOTREQUEST)
    {
        if(request->xid)
        {
            switch(message_type)
            {
                case 1: //DISCOVER
                    printf("DHCP Discover received from client with MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                            request->chaddr[0], request->chaddr[1], request->chaddr[2],
                            request->chaddr[3], request->chaddr[4], request->chaddr[5]);
                    fflush(stdout);
                    
                    send_DHCP_Offer(server_socket,request,&client_addr,client_len,pool, bindings,config);
                    break;

                case 3: //REQUEST
                    printf("DHCP Request received from client with MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                            request->chaddr[0], request->chaddr[1], request->chaddr[2],
                            request->chaddr[3], request->chaddr[4], request->chaddr[5]);
                    fflush(stdout);

                    send_DHCP_Ack(server_socket, request, &client_addr, client_len,pool, bindings,config);
                    break;
                
                case 5: //DECLINE
                    handle_DHCP_Decline(request,bindings);
                    break;
           
                case 7: //RELEASE
                    handle_DHCP_Release(request, bindings);
                    break;
                case 8: //INFORM_REPLY
                    handle_DHCP_Inform_Reply(server_socket, request, &client_addr, client_len,pool, bindings,config);
                    break;
                case 6: //DHCP_NACK
                    send_DHCP_NACK(server_socket, request, &client_addr, client_len,pool, bindings,config);
                    break;
                default:
                    printf("Unknown DHCP message type: %u\n", message_type);
                    break;
            }
        }
    }


}

void init_logfile()
{
    int fd = open(LOG_FILENAME, O_APPEND | O_RDWR | O_CREAT, 0640);
    if (fd < 0)
    {
        perror("Error while opening file.\n");
        exit(2);
    }
    if (dup2(fd, STDOUT_FILENO) < 0)
    {
        perror("Error dup-ing file descriptor.\n");
        exit(3);
    }
    printf("\n------------------------Start of Server------------------------\n\n\n\n\n");
    fflush(stdout);
}

void execute_bashScript_config()
{
    int ret=system("sudo bash /home/andreea/PSO/Proiect/Server-DHCP-AndreeasBranch");
    if(ret==-1)
    {
        perror("Eroare la executarea scriptului\n");
    }
    else{
        printf("Scriptul a fost executat cu succes.\n");
    }
}

int main()
{
    binding_list bindings;
    ip_pool_ind pool_indexes;
    dhcp_config server_config;
    init_binding_list(&bindings);

    init_logfile();

    //configurare fisier de config
    execute_bashScript_config();

    //citire fisier de config
    if(read_config_dhcp(CONFIG_FILENAME, &server_config, &bindings, &pool_indexes)!=0)
    {
        printf("Error reading the config file %s\n\n", CONFIG_FILENAME);
        exit(EXIT_FAILURE);
    }
    printf("Server configuration loaded successfully\n");
    fflush(stdout);

    int server_socket = setup_server_socket();
    printf("Server is open and it listens on %d port.\n", BOOTPS);
    fflush(stdout);

    while (1)
    {
        handle_client(server_socket, &pool_indexes, &bindings, &server_config);
    }

    close(server_socket);
    return 0;
}
