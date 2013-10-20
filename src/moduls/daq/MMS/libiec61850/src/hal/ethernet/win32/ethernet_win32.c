/*
 *  ethernet_win32.c
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

#include "stack_config.h"

#if CONFIG_INCLUDE_ETHERNET_WINDOWS == 1

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <malloc.h>

#include <winsock2.h>
#include <iphlpapi.h>

#pragma comment (lib, "IPHLPAPI.lib")

#include "ethernet.h"

#define HAVE_REMOTE

#include "pcap.h"

struct sEthernetSocket {
    pcap_t* rawSocket;
    struct bpf_program etherTypeFilter;
};

#ifdef __GNUC__ /* detect MINGW */

#define MAX_ADAPTER_ADDRESS_LENGTH      8

typedef struct _IP_ADAPTER_ADDRESSES {
	union {
		ULONGLONG Alignment;
		struct {
			ULONG Length;
			DWORD IfIndex;
		};
	};
	struct _IP_ADAPTER_ADDRESSES *Next;
	PCHAR AdapterName;
	void* FirstUnicastAddress;
	void* FirstAnycastAddress;
	void* FirstMulticastAddress;
	void* FirstDnsServerAddress;
	PWCHAR DnsSuffix;
	PWCHAR Description;
	PWCHAR FriendlyName;
	BYTE PhysicalAddress[MAX_ADAPTER_ADDRESS_LENGTH];
	DWORD PhysicalAddressLength;
	DWORD Flags;
	DWORD Mtu;
	DWORD IfType;
} IP_ADAPTER_ADDRESSES, *PIP_ADAPTER_ADDRESSES;

typedef ULONG (WINAPI* pgetadaptersaddresses)(ULONG family, ULONG flags, PVOID reserved, PIP_ADAPTER_ADDRESSES AdapterAddresses,
		PULONG SizePointer);


static pgetadaptersaddresses GetAdaptersAddresses;

static bool dllLoaded = false;

static void
loadDLLs()
{
	HINSTANCE hDll = LoadLibrary("iphlpapi.dll");

	if (hDll == NULL) {
	    printf("Error loading iphlpapi.dll!\n");
	    return;
	}


	GetAdaptersAddresses = (pgetadaptersaddresses) GetProcAddress(hDll,
			"GetAdaptersAddresses");

	if (GetAdaptersAddresses == NULL)
			printf("Error loading GetAdaptersAddresses from iphlpapi.dll (%d)\n", GetLastError());
}

#endif /* __GNUC__ */


static char*
getInterfaceName(int interfaceIndex)
{
    pcap_t *fp;
    char errbuf[PCAP_ERRBUF_SIZE];
    char* interfaceName = NULL;

    pcap_if_t *devices;
    pcap_if_t *device;

    /* Get the ethernet device list */
    if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &devices, errbuf) == -1)
    {
        printf("pcap_findalldevs_ex: %s\n", errbuf);
        return NULL;
    }

    bool ifaceFound = false;

    /* Search device list for requested interface) */
    int i = 0;
    for(device = devices; device != NULL; device= device->next)
    {
        if (i == interfaceIndex) {
            interfaceName = (char*) malloc(strlen(device->name) + 1);
            strcpy(interfaceName, device->name);
            printf("Use interface (%s)\n", interfaceName);
            ifaceFound = true;
            break;
        }

        i++;
    }

    if (!ifaceFound)
    {
        printf("No ethernet interfaces found! Make sure WinPcap is installed.\n");
        return NULL;
    }

    pcap_freealldevs(devices);

    return interfaceName;
}

void
getAdapterMacAddress(char* pcapAdapterName, uint8_t* macAddress)
{
	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	ULONG outBufLen = 0;

	pAddresses = (IP_ADAPTER_ADDRESSES*) malloc(65000);

	if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW) {
		free(pAddresses);
		pAddresses = (IP_ADAPTER_ADDRESSES*) malloc(outBufLen);
	}

	if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAddresses, &outBufLen) == NO_ERROR) {
		PIP_ADAPTER_ADDRESSES pAddress = pAddresses;

		while (pAddress != NULL) {

			DWORD addressLength = pAddress->PhysicalAddressLength;

			if (addressLength == 6) {

				int i;

				printf("Adapter %s: ", pAddress->AdapterName);

				for (i = 0; i < addressLength; i++) {
					printf("%02x ", pAddress->PhysicalAddress[i]);
				}

				if (strstr(pcapAdapterName, pAddress->AdapterName) != 0) {
					printf(" requested found!");

					for (i = 0; i < addressLength; i++) {
						macAddress[i] = pAddress->PhysicalAddress[i];
					}
				}

				printf("\n");
			}

			pAddress = pAddress->Next;
		}

		free(pAddresses);
	}
	else {
		printf("Error getting device addresses!\n");
	}
}



void
Ethernet_getInterfaceMACAddress(char* interfaceId, uint8_t* addr)
{
#ifdef __GNUC__
    if (!dllLoaded) {
    	loadDLLs();
    	dllLoaded = true;
    }
#endif

    char* endPtr;

    long interfaceIndex = strtol(interfaceId, &endPtr, 10);

    if (*endPtr != NULL) {
    	printf("Ethernet_getInterfaceMACAddress: invalid interface number %s\n", interfaceId);
    	return;
    }

    char* interfaceName = getInterfaceName((int) interfaceIndex);

	getAdapterMacAddress(interfaceName, addr);
}


EthernetSocket
Ethernet_createSocket(char* interfaceId, uint8_t* destAddress)
{
    pcap_t *pcapSocket;
    char errbuf[PCAP_ERRBUF_SIZE];

    int interfaceIndex = atoi(interfaceId);

    char* interfaceName = getInterfaceName(interfaceIndex);

    if ((pcapSocket = pcap_open_live(interfaceName, 65536, PCAP_OPENFLAG_PROMISCUOUS, 10, errbuf)) == NULL)
    {
        printf("Open ethernet socket failed for device %s\n", interfaceName);
        return NULL;
    }

    EthernetSocket ethernetSocket = (EthernetSocket) calloc(1, sizeof(struct sEthernetSocket));

    ethernetSocket->rawSocket = pcapSocket;

    return ethernetSocket;
}

void
Ethernet_destroySocket(EthernetSocket ethSocket)
{
    pcap_close(ethSocket->rawSocket);
    free(ethSocket);
}

void
Ethernet_sendPacket(EthernetSocket ethSocket, uint8_t* buffer, int packetSize)
{
    if (pcap_sendpacket(ethSocket->rawSocket, buffer, packetSize) != 0)
        printf("Error sending the packet: %s\n", pcap_geterr(ethSocket->rawSocket));
}

void
Ethernet_setProtocolFilter(EthernetSocket ethSocket, uint16_t etherType)
{
	char* filterString = (char*) alloca(100);

	sprintf(filterString, "(ether proto 0x%04x) or (vlan and ether proto 0x%04x)", etherType, etherType);

	if (pcap_compile(ethSocket->rawSocket, &(ethSocket->etherTypeFilter), filterString, 1, 0) < 0) {
		printf("Compiling packet filter failed!\n");
		return;
	}

	if (pcap_setfilter(ethSocket->rawSocket, &(ethSocket->etherTypeFilter)) < 0) {
		printf("Setting packet filter failed!\n");
	}
}

int
Ethernet_receivePacket(EthernetSocket self, uint8_t* buffer, int bufferSize)
{
	struct pcap_pkthdr* header;
	uint8_t* packetData;

	int pcapCode = pcap_next_ex(self->rawSocket, &header, (const unsigned char**) &packetData);

	if (pcapCode > 0) {
		int packetSize = header->caplen;

		if (packetSize > bufferSize)
			packetSize = bufferSize;

		memcpy(buffer, packetData, packetSize);

		return packetSize;
	}
	else {
		if (pcapCode < 0)
			printf("winpcap error\n");

		return 0;
	}
}

#endif
