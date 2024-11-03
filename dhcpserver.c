#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/errno.h>
#include <time.h>
#include <ctype.h>
#include <regex.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <net/if.h>

#include "dhcpserver.h"
#include "dhcp_structure.h"
#include "bindings.h"
#include "options.h"
#include "logging.h"


address_pool pool;

char *str_ip (uint32_t ip)
{
    struct in_addr addr;
    memcpy(&addr, &ip, sizeof(ip));
    return inet_ntoa(addr);
}

char *str_mac (uint8_t *mac)
{
    static char str[128];

    sprintf(str, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
	    mac[0], mac[1], mac[2],
	    mac[3], mac[4], mac[5]);

    return str;
}

char *str_status (int status)
{
    switch(status) {
    case EMPTY:
	    return "empty";
    case PENDING:
	    return "pending";
    case ASSOCIATED:
	    return "associated";
    case RELEASED:
	    return "released";
    case EXPIRED:
	    return "expired";
    default:
	    return NULL;
    }
}

void add_arp_entry (int s, uint8_t *mac, uint32_t ip)
{
    struct arpreq ar;
    struct sockaddr_in *sock;

    memset(&ar, 0, sizeof(ar));
   
    sock = (struct sockaddr_in *) &ar.arp_pa;
    sock->sin_family = AF_INET;
    sock->sin_addr.s_addr = ip;

    memcpy(ar.arp_ha.sa_data, mac, 6);
    ar.arp_flags = ATF_COM; 

    strncpy(ar.arp_dev, pool.device, sizeof(ar.arp_dev));
    
    if (ioctl(s, SIOCSARP, (char *) &ar) < 0)  {
	perror("error adding entry to arp table");
    };    
}

void delete_arp_entry (int s, uint8_t *mac, uint32_t ip)
{
    struct arpreq ar;
    struct sockaddr_in *sock;

    memset(&ar, 0, sizeof(ar));
    
    sock = (struct sockaddr_in *) &ar.arp_pa;
    sock->sin_family = AF_INET;
    sock->sin_addr.s_addr = ip;

    strncpy(ar.arp_dev, pool.device, sizeof(ar.arp_dev));

    if(ioctl(s, SIOCGARP, (char *) &ar) < 0)  {
	if (errno != ENXIO) {
	    perror("error getting arp entry");
	    return;
	}
    };
    
    if(ip == 0 || memcmp(mac, ar.arp_ha.sa_data, 6) == 0) { 
	if(ioctl(s, SIOCDARP, (char *) &ar) < 0) {
	    perror("error removing arp table entry");
	}
    }
}

int send_dhcp_reply	(int s, struct sockaddr_in *client_sock, dhcp_msg *reply)
{
    size_t len, ret;

    len = serialize_option_list(&reply->opts, reply->hdr.options,
				sizeof(reply->hdr) - DHCP_HEADER_SIZE);

    len += DHCP_HEADER_SIZE;
    
    client_sock->sin_addr.s_addr = reply->hdr.yiaddr; 

    if(reply->hdr.yiaddr != 0) {
	add_arp_entry(s, reply->hdr.chaddr, reply->hdr.yiaddr);
    }

    if ((ret = sendto(s, reply, len, 0, (struct sockaddr *)client_sock, sizeof(*client_sock))) < 0) {
	perror("sendto failed");
	return -1;
    }

    return ret;
}

uint8_t expand_request (dhcp_msg *request, size_t len)
{
    init_option_list(&request->opts);
    
    if (request->hdr.hlen < 1 || request->hdr.hlen > 16)
	    return 0;

    if(parse_options_to_list(&request->opts, (dhcp_option *)request->hdr.options,
			     len - DHCP_HEADER_SIZE) == 0)
	    return 0;
    
    dhcp_option *type_opt = search_option(&request->opts, DHCP_MESSAGE_TYPE);
    
    if (type_opt == NULL)
	    return 0;

    uint8_t type = type_opt->data[0];
    
    return type;
}

int init_reply (dhcp_msg *request, dhcp_msg *reply)
{
    memset(&reply->hdr, 0, sizeof(reply->hdr));

    init_option_list(&reply->opts);
    
    reply->hdr.op = BOOTREPLY;

    reply->hdr.htype = request->hdr.htype;
    reply->hdr.hlen  = request->hdr.hlen;

    reply->hdr.xid   = request->hdr.xid;
    reply->hdr.flags = request->hdr.flags;
     
    reply->hdr.giaddr = request->hdr.giaddr;
    
    memcpy(reply->hdr.chaddr, request->hdr.chaddr, request->hdr.hlen);

    return 1;
}

void fill_requested_dhcp_options (dhcp_option *requested_opts, dhcp_option_list *reply_opts)
{
    uint8_t len = requested_opts->len;
    uint8_t *id = requested_opts->data;

    int i;
    for (i = 0; i < len; i++) {
	    
	if(id[i] != 0) {
	    dhcp_option *opt = search_option(&pool.options, id[i]);

	    if(opt != NULL)
		append_option(reply_opts, opt);
	}
	    
    }
}

int fill_dhcp_reply (dhcp_msg *request, dhcp_msg *reply,
		 address_binding *binding, uint8_t type)
{
    static dhcp_option type_opt, server_id_opt;

    type_opt.id = DHCP_MESSAGE_TYPE;
    type_opt.len = 1;
    type_opt.data[0] = type;
    append_option(&reply->opts, &type_opt);

    server_id_opt.id = SERVER_IDENTIFIER;
    server_id_opt.len = 4;
    memcpy(server_id_opt.data, &pool.server_id, sizeof(pool.server_id));
    append_option(&reply->opts, &server_id_opt);
    
    if(binding != NULL) {
	reply->hdr.yiaddr = binding->address;
    }
    
    if (type != DHCP_NAK) {
	dhcp_option *requested_opts = search_option(&request->opts, PARAMETER_REQUEST_LIST);

	if (requested_opts)
	    fill_requested_dhcp_options(requested_opts, &reply->opts);
    }
    
    return type;
}


void message_dispatcher(int s, struct sockaddr_in server_sock)
{
    while (1) 
    {
        struct sockaddr_in client_sock;
        socklen_t slen = sizeof(client_sock);
        size_t len;

        dhcp_msg request;
        dhcp_msg reply;

        uint8_t type;

        if((len = recvfrom(s, &request.hdr, sizeof(request.hdr), 0, (struct sockaddr *)&client_sock, &slen)) < DHCP_HEADER_SIZE + 5) {
            continue; 
        }

        if(request.hdr.op != BOOTREQUEST)
            continue;

        if((type = expand_request(&request, len)) == 0) {
            log_error("%s.%u: invalid request received\n",
                inet_ntoa(client_sock.sin_addr), ntohs(client_sock.sin_port));
            continue;
        }

        init_reply(&request, &reply);
    }
}


int main(int argc, char *argv[])
{
    int s;
    struct protoent *pp;
    struct servent *ss;
    struct sockaddr_in server_sock;

    memset(&pool, 0, sizeof(pool));

    init_binding_list(&pool.bindings);
    init_option_list(&pool.options);

    parse_args(argc, argv, &pool);

    //setare server
    if ((ss = getservbyname("bootps", "udp")) == 0) 
    {
        fprintf(stderr, "server: getservbyname() error\n");
        exit(1);
    }

     if ((pp = getprotobyname("udp")) == 0) 
    {
        fprintf(stderr, "server: getprotobyname() error\n");
        exit(1);
    }

    if ((s = socket(AF_INET, SOCK_DGRAM, pp->p_proto)) == -1) 
    {
        perror("server: socket() error");
        exit(1);
    }

    server_sock.sin_family = AF_INET;
    server_sock.sin_addr.s_addr = htonl(INADDR_ANY);
    server_sock.sin_port = ss->s_port;

    if (bind(s, (struct sockaddr *) &server_sock, sizeof(server_sock)) == -1) 
    {
        perror("server: bind()");
        close(s);
        exit(1);
    }

    printf("dhcp server: listening on %d\n", ntohs(server_sock.sin_port));

    message_dispatcher(s, server_sock);

    close(s);

    return 0;
}