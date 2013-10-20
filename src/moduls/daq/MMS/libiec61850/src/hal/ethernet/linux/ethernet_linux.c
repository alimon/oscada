/*
 *  ethernet_linux.c
 *
 *  Copyright 2013 Michael Zillgith
 *
 *  This file is part of libIEC61850.
 *
 *  libIEC61850 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  libIEC61850 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libIEC61850.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  See COPYING file for the complete license text.
 */

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "ethernet.h"

struct sEthernetSocket {
    int rawSocket;
    bool isBind;
    struct sockaddr_ll socketAddress;
};

static int
getInterfaceIndex(int sock, char* deviceName)
{
    struct ifreq ifr;

    strncpy(ifr.ifr_name, deviceName, IFNAMSIZ);

    if (ioctl(sock, SIOCGIFINDEX, &ifr) == -1) {
        perror("SIOCGIFINDEX");
        exit(1);
    }

    return ifr.ifr_ifindex;
}


void
Ethernet_getInterfaceMACAddress(char* interfaceId, uint8_t* addr)
{
    struct ifreq buffer;

    int sock = socket(PF_INET, SOCK_DGRAM, 0);

    memset(&buffer, 0x00, sizeof(buffer));

    strcpy(buffer.ifr_name, interfaceId);

    ioctl(sock, SIOCGIFHWADDR, &buffer);

    close(sock);

    int i;

    for(i = 0; i < 6; i++ )
    {
        addr[i] = (unsigned char)buffer.ifr_hwaddr.sa_data[i];
    }
}


EthernetSocket
Ethernet_createSocket(char* interfaceId, uint8_t* destAddress)
{
    EthernetSocket ethernetSocket = calloc(1, sizeof(struct sEthernetSocket));

    ethernetSocket->rawSocket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if (ethernetSocket->rawSocket == -1) {
        printf("Error creating raw socket!\n");
        return NULL;
    }

    ethernetSocket->socketAddress.sll_family = PF_PACKET;
    ethernetSocket->socketAddress.sll_protocol = htons(ETH_P_IP);

    ethernetSocket->socketAddress.sll_ifindex = getInterfaceIndex(ethernetSocket->rawSocket, interfaceId);

    ethernetSocket->socketAddress.sll_hatype =  ARPHRD_ETHER;
    ethernetSocket->socketAddress.sll_pkttype = PACKET_OTHERHOST;

    ethernetSocket->socketAddress.sll_halen = ETH_ALEN;

    memset(ethernetSocket->socketAddress.sll_addr, 0, 8);

    if (destAddress != NULL)
        memcpy(ethernetSocket->socketAddress.sll_addr, destAddress, 6);

    ethernetSocket->isBind = false;

    return ethernetSocket;
}

void
Ethernet_setProtocolFilter(EthernetSocket ethSocket, uint16_t etherType)
{
    ethSocket->socketAddress.sll_protocol = htons(etherType);
}


/* non-blocking receive */
int
Ethernet_receivePacket(EthernetSocket self, uint8_t* buffer, int bufferSize)
{
    if (self->isBind == false) {
        bind(self->rawSocket, (struct sockaddr*) &self->socketAddress, sizeof(self->socketAddress));
        //TODO check return value
        self->isBind = true;
    }

    int bytesReceived = recvfrom(self->rawSocket, buffer, bufferSize, MSG_DONTWAIT, 0, 0);
}

void
Ethernet_sendPacket(EthernetSocket ethSocket, uint8_t* buffer, int packetSize)
{
    sendto(ethSocket->rawSocket, buffer, packetSize,
                0, (struct sockaddr*) &(ethSocket->socketAddress), sizeof(ethSocket->socketAddress));
}

void
Ethernet_destroySocket(EthernetSocket ethSocket)
{
    close(ethSocket->rawSocket);
    free(ethSocket);
}

