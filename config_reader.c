#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config_reader.h"
#include "ip_pool_manager.h"
#include <arpa/inet.h>

int read_config_dhcp(const char *config_file_path,dhcp_config *config, binding_list *bindings, ip_pool_ind *pool_indexes)
{
    FILE *f=fopen(config_file_path, "r");
    if(!f)
    {
        perror("Eroare la deschiderea fisierului de config");
        return -1;
    }

    char buffer[256];
    //int mode=0; //0=cautare sectiune nou; 1=am gasit sectiunea "pool"

    while(fgets(buffer, sizeof(buffer), f))
    {
        char *line=buffer;
        //eliminare spatii
        while(*line==' ' || *line=='\t')
            line++;

        //ignorare linii goale sau comentraii
        if(*line == '\0' || *line=='#')
        {
            continue;
        }

        if(strncmp(line, "range", 5)==0)
        {
           char ip_start[16], ip_end[16];
           if(sscanf(line, "range %15s %15s", ip_start, ip_end)==2)
           {
            config->ip_pool_start=ntohl(inet_addr(ip_start));
            config->ip_pool_end=ntohl(inet_addr(ip_end));

            pool_indexes->ip_first=config->ip_pool_start;
            pool_indexes->ip_last=config->ip_pool_end;
            pool_indexes->ip_current=pool_indexes->ip_first;

            printf("Intervalul IP: %s - %s\n", ip_start, ip_end);
           }
        } else if(strncmp(line, "default-lease-time", 18)==0)
        {
            int dlease_time;
            if(sscanf(line, "default-lease-time %d", &dlease_time)==1)
            {
                config->default_lease_time=dlease_time;
                printf("Default lease time: %d sec\n", dlease_time);
            }
        } else if(strncmp(line, "max-lease-time", 14)==0)
        {
            int mlease_time;
            if (sscanf(line, "max-lease-time %d", &mlease_time) == 1)
            {
                config->max_lease_time=mlease_time;
                printf("Max lease time: %d sec\n", mlease_time);
            }
        } else if(strncmp(line, "subnet", 6)==0)
        {
            char subnet[16], netmask[6];
            if(sscanf(line, "subnet %15s netmask %15s", subnet, netmask))
            {
                config->subnet=ntohl(inet_addr(subnet));
                config->netmask=ntohl(inet_addr(netmask));
                printf("Subnet: %s, Netmask: %s\n", subnet, netmask);
            }
        } else if(strncmp(line, "option routers", 14)==0)
        {
            char router[16];
            if(sscanf(line, "options routers %15s", router)==1)
            {
                config->router=ntohl(inet_addr(router));
                printf("Options routers: %s",router);
            }
        } else if(strncmp(line, "option domain-name-servers", 26)==0)

        {
            char dns1[16], dns2[16];
            if(sscanf(line, "option domain-name-servers %15s, %15s", dns1, dns2)==2)
            {
                config->dns[0]=ntohl(inet_addr(dns1));
                config->dns[1]=ntohl(inet_addr(dns2));
                printf("Options-domain-name-servers: %s  %s",dns1, dns2);
            }
        }
    }

    fclose(f);
    return 0;

}