#define _POSIX_C_SOURCE 200809L

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include <fcntl.h>
#include<pthread.h>


#include "dhcp_structure.h"
#include "server_connection.h"
#include "ip_pool_manager.h"
#include "config_reader.h"
#include "options.h"

#define THREAD_NUM 1
#define DHCP_OPTION_MSG_TYPE 53
#define DHCP_OPTIONS_MAX_LEN 276
#define CONFIG_FILENAME "dhcp_config_file.txt"



// int server_socket, ip_pool_ind *pool, binding_list *bindings, dhcp_config *config


ClientTask taskQueue[256];
int taskCount = 0;


//
pthread_mutex_t mutexQueue;
pthread_cond_t condQueue;

pthread_mutex_t pool_mutex;
pthread_mutex_t bindings_mutex;

//

void init()
{

}

void submitTask(ClientTask task) {
    pthread_mutex_lock(&mutexQueue);
    taskQueue[taskCount] = task;
    taskCount++;
    pthread_mutex_unlock(&mutexQueue);
    pthread_cond_signal(&condQueue);
}
// 1 2 3 4 5
// 2 3 4 5

void* startThread(void* args) {
    while (1) {
        ClientTask task;

        pthread_mutex_lock(&mutexQueue);
        while (taskCount == 0) {
            pthread_cond_wait(&condQueue, &mutexQueue);
        }

        task = taskQueue[0];
        int i;
        for (i = 0; i < taskCount - 1; i++) {
            taskQueue[i] = taskQueue[i + 1];
        }
        taskCount--;
        pthread_mutex_unlock(&mutexQueue);
        handle_client(&task);
    }
}



uint8_t get_dhcp_message_type(const uint8_t *options, size_t options_len)
{
    
    size_t i = 4; //sar peste magic cookie
    while (i < options_len) 
    {
        //printf("Option code: %u, Length: %u\n", options[i], options[i + 1]);
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
    int broadcast = 1;
    if(setsockopt(server_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast))<0)
    {
        perror("Setarea permisiunilor de BROADCAST a esuat: ");
        exit(-1);
    }

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Couldn't connect to server port!");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    return server_socket;
}

void print_dhcp_message(const dhcp_message *msg) {
    uint8_t message_type=get_dhcp_message_type(msg->options, sizeof(msg->options));
    printf("\n\n\n\n\n========== DHCP Message ==========\n");
    fflush(stdout);

    switch(message_type)
    {
        case 1:
        printf("DHCP DISCOVER received from client with MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                            msg->chaddr[0], msg->chaddr[1], msg->chaddr[2],
                            msg->chaddr[3], msg->chaddr[4], msg->chaddr[5]);
        fflush(stdout);
        break;

        case 2:
        printf("DHCP OFFER sent to client with MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                            msg->chaddr[0], msg->chaddr[1], msg->chaddr[2],
                            msg->chaddr[3], msg->chaddr[4], msg->chaddr[5]);
        fflush(stdout);
        break;

        case 3:
        printf("DHCP REQUEST received from client with MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                            msg->chaddr[0], msg->chaddr[1], msg->chaddr[2],
                            msg->chaddr[3], msg->chaddr[4], msg->chaddr[5]);
        fflush(stdout);
        break;

        case 4:
        printf("DHCP DECLINE received from client with MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                            msg->chaddr[0], msg->chaddr[1], msg->chaddr[2],
                            msg->chaddr[3], msg->chaddr[4], msg->chaddr[5]);
        fflush(stdout);
        break;

        case 5:
        printf("DHCP ACK sent to client with MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                            msg->chaddr[0], msg->chaddr[1], msg->chaddr[2],
                            msg->chaddr[3], msg->chaddr[4], msg->chaddr[5]);
        fflush(stdout);
        break;

        case 6:
        printf("DHCP NACK sent to client with MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                            msg->chaddr[0], msg->chaddr[1], msg->chaddr[2],
                            msg->chaddr[3], msg->chaddr[4], msg->chaddr[5]);
        fflush(stdout);
        break;

        case 7:
        printf("DHCP RELEASE received from client with MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                            msg->chaddr[0], msg->chaddr[1], msg->chaddr[2],
                            msg->chaddr[3], msg->chaddr[4], msg->chaddr[5]);
        fflush(stdout);
        break;

        case 8:
        printf("DHCP INFORM received from client with MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                            msg->chaddr[0], msg->chaddr[1], msg->chaddr[2],
                            msg->chaddr[3], msg->chaddr[4], msg->chaddr[5]);
        fflush(stdout);
        break;

        default:
        break;

    }

    printf("DHCP Message type: %u\n", message_type);
    

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

uint32_t offer_ip_available(ip_pool_ind *pool, binding_list *bindings) {
    uint32_t offered_ip = 0;
    pthread_mutex_lock(&pool_mutex); 
    for (uint32_t ip = pool->ip_current + 1; ip <= pool->ip_last; ip++) {
        if (is_ip_available(pool, bindings, ip)) {
            offered_ip = ip;
            pool->ip_current = ip;
            break;
        }
    }
    pthread_mutex_unlock(&pool_mutex);
    return offered_ip;
}

void display_binding_list(const binding_list *list) {
    if (list == NULL) {
        printf("Lista este NULL.\n");
        return;
    }

    const ip_binding *item;
    int index = 0;

    LIST_FOREACH(item, list, pointers) {
        printf("Element %d:\n", index++);
        printf("  Address: %u.%u.%u.%u\n",
               (item->address >> 24) & 0xFF,
               (item->address >> 16) & 0xFF,
               (item->address >> 8) & 0xFF,
               item->address & 0xFF);
        printf("  CID Identifier Length: %u\n", item->cident_len);
        printf("  CID Identifier: ");
        for (int i = 0; i < item->cident_len; i++) {
            printf("%02X", item->cident[i]);
        }
        printf("\n");
        printf("  Binding Time: %s", ctime(&item->binding_time));
        printf("  Lease Time: %s", ctime(&item->lease_time));
        printf("  Status: %d\n", item->status);
        printf("  Is Static: %s\n", item->is_static ? "Yes" : "No");
        printf("----------------------\n");
    }

    if (index == 0) {
        printf("Lista este goalĂ.\n");
    }
}



void send_DHCP_Offer(int server_socket, const dhcp_message *request,  struct sockaddr_in *client_addr, socklen_t client_len, ip_pool_ind *pool, binding_list *bindings, dhcp_config *config) {
    if (!request || !client_addr) {
        perror("Invalid request or client address!");
        return;
    }
    
    if (pool->ip_current >= pool->ip_last) {
        printf("No available IP addresses in the pool.\n");
        fflush(stdout);
        return;
    }

    uint32_t offered_ip = offer_ip_available(pool, bindings);
    if (offered_ip == 0) {
        printf("No available IP addresses in the pool.\n");
        fflush(stdout);
        return;
    }

    ip_binding *new_binding = (ip_binding *)malloc(sizeof(ip_binding));
    if (!new_binding) {
        perror("Error allocating memory for new binding");
        return;
    }
    
    new_binding->address = offered_ip;
    new_binding->binding_time = time(NULL);
    new_binding->lease_time = new_binding->binding_time + config->max_lease_time;
    new_binding->status = 1; // activ
    new_binding->is_static = 0; // dinamic
    new_binding->cident_len = request->hlen;
    memcpy(new_binding->cident, request->chaddr, request->hlen);

    LIST_INSERT_HEAD(bindings, new_binding, pointers);

    printf("+++++Lista binding-urilor+++++.\n");
    display_binding_list(bindings);

    dhcp_message offer;
    memset(&offer, 0, sizeof(dhcp_message));
    offer.op = BOOTREPLY;
    offer.htype = request->htype;
    offer.hlen = request->hlen;
    offer.xid = request->xid;
    offer.secs = 0; // Secunde - poate fi lăsat 0
    offer.flags = request->flags; // Copiază steagurile din cerere
    offer.yiaddr = htonl(offered_ip); // Adresa oferită clientului
    offer.siaddr = htonl(config->router); // Adresa serverului (opțional)
    offer.giaddr = 0; // Gateway IP (0 în acest caz)
    memset(offer.sname, 0, sizeof(offer.sname)); // Server Name
    memset(offer.file, 0, sizeof(offer.file)); // Boot File Name

    // Opțiuni configurate corect
    int opt_idx = 0;
    offer.options[opt_idx++] = 0x63;
    offer.options[opt_idx++] = 0x82;
    offer.options[opt_idx++] = 0x53;
    offer.options[opt_idx++] = 0x63;

    offer.options[opt_idx++] = DHCP_OPTION_MSG_TYPE;
    offer.options[opt_idx++] = 1;
    offer.options[opt_idx++] = DHCP_OFFER;

    offer.options[opt_idx++] = IP_ADDRESS_LEASE_TIME;
    offer.options[opt_idx++] = 4;
    offer.options[opt_idx++] = (uint8_t)((config->max_lease_time >> 24) & 0xFF);
    offer.options[opt_idx++] = (uint8_t)((config->max_lease_time >> 16) & 0xFF);
    offer.options[opt_idx++] = (uint8_t)((config->max_lease_time >> 8) & 0xFF);
    offer.options[opt_idx++] = (uint8_t)(config->max_lease_time & 0xFF);

    offer.options[opt_idx++] = 0xFF; // End of options

    

    memcpy(offer.chaddr, request->chaddr, request->hlen);
    client_addr->sin_family=AF_INET;
    client_addr->sin_port=htons(68);
    client_addr->sin_addr.s_addr=inet_addr("192.168.1.255");


    ssize_t send_bytes = sendto(server_socket, &offer, sizeof(dhcp_message), 0, (struct sockaddr *)client_addr, client_len);
    if (send_bytes < 0) {
        perror("Error sending DHCP Offer to client");
    } else {
        print_dhcp_message(&offer);
    }
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

void send_DHCP_Ack(int server_socket, const dhcp_message *request, struct sockaddr_in *client_addr, socklen_t client_len,ip_pool_ind *pool, binding_list *bindings, dhcp_config *config)
{

    if (!request || !request->chaddr) {
        printf("Invalid DHCP request\n");
        return;
    }
    //ip_binding *binding=find_binding(bindings, request->chaddr, request->hlen);
    pthread_mutex_lock(&bindings_mutex);
    ip_binding *binding=search_binding(bindings,request->chaddr,6,STATIC_OR_DYNAMIC,1);
    pthread_mutex_unlock(&bindings_mutex);

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

    ack.yiaddr = htonl(binding->address);
    ack.siaddr =  htonl(0xC0A80101); // Adresa IP 192.168.1.1

    // Magic cookie (4 octeți)
    ack.options[0] = 0x63;
    ack.options[1] = 0x82;
    ack.options[2] = 0x53;
    ack.options[3] = 0x63;

    // DHCP Message Type
    ack.options[4] = DHCP_OPTION_MSG_TYPE;
    ack.options[5] = 1; // Lungime DHCP Ack
    ack.options[6] = DHCP_ACK;

    // Lease Time
    ack.options[7] = IP_ADDRESS_LEASE_TIME; // Lease Time
    ack.options[8] = 4; // Lungime (4 octeți)
    ack.options[9] = (uint8_t)((config->max_lease_time >> 24) & 0xFF);
    ack.options[10] = (uint8_t)((config->max_lease_time >> 16) & 0xFF);
    ack.options[11] = (uint8_t)((config->max_lease_time >> 8) & 0xFF);
    ack.options[12] = (uint8_t)(config->max_lease_time & 0xFF);

    // Subnet Mask
    config->subnet = 0xFFFFFF00; // 255.255.255.0    
    ack.options[13] = SUBNET_MASK; // Subnet mask (cod 1)
    ack.options[14] = 4; // Lungime
    ack.options[15] = (uint8_t)((config->subnet >> 24) & 0xFF);
    ack.options[16] = (uint8_t)((config->subnet >> 16) & 0xFF);
    ack.options[17] = (uint8_t)((config->subnet >> 8) & 0xFF);
    ack.options[18] = (uint8_t)(config->subnet & 0xFF);

    // Router
    ack.options[19] = ROUTER; // Router (cod 3)
    ack.options[20] = 4; // Lungime
    ack.options[21] = (uint8_t)((config->router >> 24) & 0xFF);
    ack.options[22] = (uint8_t)((config->router >> 16) & 0xFF);
    ack.options[23] = (uint8_t)((config->router >> 8) & 0xFF);
    ack.options[24] = (uint8_t)(config->router & 0xFF);

    // DNS
    config->dns[0] = 0x08080808; // 8.8.8.8
    ack.options[25] = DOMAIN_NAME_SERVER; // DNS (cod 6)
    ack.options[26] = 4; // Lungime
    ack.options[27] = (uint8_t)((config->dns[0] >> 24) & 0xFF);
    ack.options[28] = (uint8_t)((config->dns[0] >> 16) & 0xFF);
    ack.options[29] = (uint8_t)((config->dns[0] >> 8) & 0xFF);
    ack.options[30] = (uint8_t)(config->dns[0] & 0xFF);

    // Terminator
    ack.options[31] = 0xFF; // Terminator

    // Adresa hardware a clientului
    memcpy(ack.chaddr, request->chaddr, request->hlen);
    //client_addr->sin_addr=htonl(0xAC100F3E);
    client_addr->sin_family=AF_INET;
    client_addr->sin_port=htons(68);
    client_addr->sin_addr.s_addr = inet_addr("192.168.1.255");


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


void handle_client(ClientTask* task )
{
    
     if (task-> recv_bytes< 0)
    {
        perror("Error: Couldn't receive a message from client!");
        return;
    }
    fflush(stdout);

    
    // Convorbire prin mesaje DHCP

    dhcp_message *request = (dhcp_message*)task->buffer;
    uint8_t message_type=get_dhcp_message_type(request->options, sizeof(request->options));

    fflush(stdout);
    
    if(request->op==BOOTREQUEST)
    {
        if(request->xid)
        {
            switch(message_type)
            {
                case 1: //DISCOVER

                    print_dhcp_message(request);
                    send_DHCP_Offer(task->server_socket,request, &(task->client_addr),task->client_len,task->pool, task->bindings,task->config);
                    break;

                case 3: //REQUEST
                    
                    print_dhcp_message(request);
                    send_DHCP_Ack(task->server_socket, request, &(task->client_addr), task->client_len,task->pool, task->bindings,task->config);
                    break;
                
                case 5: //DECLINE
                    print_dhcp_message(request);
                    handle_DHCP_Decline(request,task->bindings);
                    break;
           
                case 7: //RELEASE
                    print_dhcp_message(request);
                    handle_DHCP_Release(request, task->bindings);
                    break;
                case 8: //INFORM_REPLY
                    print_dhcp_message(request);
                    handle_DHCP_Inform_Reply(task->server_socket, request, &(task->client_addr),task->client_len,task->pool,task-> bindings,task->config);
                    break;
                case 6: //DHCP_NACK
                //6 vine de la server
                    send_DHCP_NACK(task->server_socket, request, &(task->client_addr), task->client_len,task->pool, task->bindings,task->config);
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


int main()
{
    binding_list bindings;
    ip_pool_ind pool_indexes;
    dhcp_config server_config;
    init_binding_list(&bindings);

    init_logfile();

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

    pthread_t th[THREAD_NUM];
    pthread_mutex_init(&mutexQueue, NULL);
    pthread_cond_init(&condQueue, NULL);
    pthread_mutex_init(&pool_mutex,NULL);
    pthread_mutex_init(&bindings_mutex,NULL);
    int i;
    for (i = 0; i < THREAD_NUM; i++) {
        if (pthread_create(&th[i], NULL, &startThread, NULL) != 0) {
            perror("Failed to create the thread");
        }
    }
    
    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        char buffer[BUFFER_SIZE];

        ssize_t recv_bytes = recvfrom(server_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_len);
        if (recv_bytes < 0)
         {
            perror("Error: Couldn't receive data from client");
            return -1;
        }
        ClientTask T;
        memset(&T, 0, sizeof(ClientTask));

        memcpy(T.buffer,buffer,recv_bytes);
        T.bindings=&bindings;
        T.client_len=client_len;
        T.config=&server_config;
        T.pool=&pool_indexes;
        T.recv_bytes=recv_bytes;
        T.server_socket=server_socket;
        T.client_addr=client_addr;
        submitTask(T);

    }
    pthread_mutex_destroy(&mutexQueue);
    pthread_cond_destroy(&condQueue);

    close(server_socket);
    return 0;
}
