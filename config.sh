#!/bin/bash

INTERFACE='enp0s3'

IP_ADDRESS=`ifconfig $INTERFACE | awk '/inet /{ print $2 }'`

NETWORK_MASK=`ifconfig $INTERFACE | awk '/netmask /{ print $4 }'`

DEFAULT_GATEWAY=`route -n | grep $INTERFACE | awk '/UG/{print $2}'`

# pool de adrese IP
POOL_START='192.168.1.100'
POOL_END='192.168.1.200'

DEFAULT_LEASE_TIME='1800'

MAX_LEASE_TIME='3600'

PENDING_TIME='30'