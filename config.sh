#!/bin/bash

INTERFACE='enp0s3'

IP_ADDRESS=$(ip addr show $INTERFACE | awk '/inet /{ print $2 }' | cut -d/ -f1)
NETWORK_MASK=$(ip addr show $INTERFACE | awk '/inet /{ print $2 }' | cut -d/ -f2)
DEFAULT_GATEWAY=$(ip route | grep default | grep $INTERFACE | awk '{print $3}')

# pool de adrese IP
POOL_START='192.168.1.10'
POOL_END='192.168.1.100'

IP_ADDRESS='192.168.1.100'
DEFAULT_GATEWAY=${IP_ADDRESS%.*}.1
NETWORK_MASK='24'

DEFAULT_LEASE_TIME='600'
MAX_LEASE_TIME='7200'

PENDING_TIME='30'

DHCP_CONFIG_FILE='dhcp_config_file.txt'

cat <<EOL | sudo tee $DHCP_CONFIG_FILE
# Configuratie server DHCP

#Optiuni generale
option domain-name "localdomain";
option domain-name-servers 8.8.8.8, 8.8.4.4;

#Timpi de lease
default-lease-time $DEFAULT_LEASE_TIME;
max-lease-time $MAX_LEASE_TIME;

# ddns-update-style none;

# Subnet si setari IP
subnet ${IP_ADDRESS%.*}.0 netmask $(ipcalc $IP_ADDRESS/$NETWORK_MASK | grep Netmask | awk '{print $2}') {
    range $POOL_START $POOL_END;
    option routers $DEFAULT_GATEWAY;
    option subnet-mask $(ipcalc $IP_ADDRESS/$NETWORK_MASK | grep Netmask | awk '{print $2}');
    option broadcast-address ${IP_ADDRESS%.*}.255;
}

authoritative;

# Binding PENDING time
#pending-time $PENDING_TIME;

# Interfata de retea pentru serverul DHCP
INTERFACES="$INTERFACE";
EOL

# Setari suplimentare pentru interfata in /etc/default/isc-dhcp-server (pentru ISC DHCP)
echo "INTERFACESv4=\"$INTERFACE\"" | sudo tee /etc/default/isc-dhcp-server > /dev/null

# Afisare mesaj de succes
echo "Configuratia DHCP a fost generata cu succes în $DHCP_CONFIG_FILE."