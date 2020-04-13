/*
  straight-httpd.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#include "../../lwip-port/win32/lwip_port.h"

#define NO_LWIP		0

#pragma warning(disable:4996)

#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")

#include "pcap.h"
#pragma comment(lib, "../../pcap/lib/Packet.lib")
#pragma comment(lib, "../../pcap/lib/wpcap.lib")

pcap_t* g_hPcap = NULL;

long	g_nKeepRunning = 0;
HANDLE	g_mainThread = NULL;
DWORD WINAPI MainLoopThread(void* data);
HANDLE	g_appThread = NULL;
DWORD WINAPI AppThread(void* data);

extern long g_tcpipReady;

HANDLE	g_dmaQLock = NULL;
long	g_qLength = 0;
struct packet_wrapper* g_dmaQueue = NULL;

#if (NO_LWIP > 0)
HANDLE	g_pcapSemaphore = NULL;
void print_packet_info(const u_char *packet, long len)
{
	printf("Packet total length %d\n", len);
}

void packet_handler(u_char *args, long len, const u_char* packet)
{
	ETH_HEADER* ethHeader = (ETH_HEADER*)packet;

	if (ntohs(ethHeader->frame_type) == IPv4_PACKET)
	{
		IP_HEADER* ipHeader = (IP_HEADER*)(packet + sizeof(ETH_HEADER));
		int ip_header_length = ((ipHeader->ver_len_tos) & 0x0F);
		ip_header_length *= 4;
		printf("IP header length (IHL) in bytes: %d\n", ip_header_length);

		if ((ipHeader->ttlandprotocol & 0x0F) == IPPROTO_TCP)
		{
			TCP_HEADER* tcpHeader = (TCP_HEADER*)(packet + sizeof(ETH_HEADER) + ip_header_length);
		}
	}
	else  if (ntohs(ethHeader->frame_type) == ARP_PACKET)
	{
		ARP_HEADER* ipHeader = (ARP_HEADER*)(packet + sizeof(ETH_HEADER));
		printf("ARP\n");
	}
	else  if (ntohs(ethHeader->frame_type) == RARP_PACKET)
	{
		ARP_HEADER* ipHeader = (ARP_HEADER*)(packet + sizeof(ETH_HEADER));
		printf("Reverse ARP\n");
	}

	print_packet_info(packet, len);
	return;
}
#else
#endif

void DMA_push(const struct pcap_pkthdr* packet_header, const u_char* packet)
{
	struct packet_wrapper* newPacket = (struct packet_wrapper*)malloc(sizeof(struct packet_wrapper));
	if (newPacket == NULL)
		return;
	newPacket->packet = malloc(packet_header->caplen);
	if (newPacket->packet == NULL)
	{
		free(newPacket);
		return;
	}
	newPacket->next = NULL;
	newPacket->len = packet_header->caplen;

	memcpy(newPacket->packet, packet, packet_header->caplen);

	if (g_dmaQLock != NULL)
		WaitForSingleObject(g_dmaQLock, INFINITE);

	if (g_qLength < MAX_DMA_QUEUE)
	{
		g_qLength++;
		if (g_dmaQueue == NULL)
		{
			g_dmaQueue = newPacket;
		}
		else
		{
			struct packet_wrapper* prev = g_dmaQueue;
			while (prev->next != NULL)
				prev = prev->next;
			prev->next = newPacket;
		}

		//printf("New packet arrived: length = %d\n", g_qLength);
	}
	else
	{
		printf("Packet queue is full: length = %d\n", g_qLength);
	}

	if (g_dmaQLock != NULL)
		ReleaseMutex(g_dmaQLock);

#if (NO_LWIP > 0)
	ReleaseSemaphore(g_pcapSemaphore, 1, NULL); // not interested in previous count
#else
	NotifyFromEthISR();
#endif
}

struct packet_wrapper* DMA_pop()
{
	struct packet_wrapper* pkt = NULL;

	if (g_dmaQLock != NULL)
		WaitForSingleObject(g_dmaQLock, INFINITE);

	if (g_dmaQueue != NULL)
	{
		pkt = g_dmaQueue;
		g_dmaQueue = g_dmaQueue->next;
		pkt->next = NULL; //just the first packet

		g_qLength--;
		//printf("Packet consumed: length = %d\n", g_qLength);
	}

	if (g_dmaQLock != NULL)
		ReleaseMutex(g_dmaQLock);

	return pkt;
}

void DMA_free(struct packet_wrapper* pkt)
{
	struct packet_wrapper* prev = pkt;
	while(prev != NULL)
	{
		struct packet_wrapper* next = prev->next;

		if (prev->packet != NULL)
			free(prev->packet);
		free(prev);

		prev = next;
	}
}

unsigned char* NIC_GetBuffer(int size)
{
	unsigned char* buf = malloc(size);
	return buf;
}

long NIC_Send(unsigned char *buf, int size)
{
	long nSent = -1;

	if ((g_hPcap != NULL) && (buf != NULL))
		nSent = pcap_sendpacket(g_hPcap, buf, size);

	if (buf != NULL)
		free(buf);

	return nSent;
}

int main()
{
	int i = 0;

	bpf_u_int32 ip;
	bpf_u_int32 subnet_mask;
	struct in_addr address;

	struct bpf_program filterCompile;
	char szError[PCAP_ERRBUF_SIZE];

	pcap_if_t *allDevices, *dev;

	char szHostName[256];
	char szHostIP[128];
	char szHostSubnet[128];

	char szFilter[512];

	memset(szHostName, 0, sizeof(szHostName));
	memset(szFilter, 0, sizeof(szFilter));

	if (pcap_findalldevs(&allDevices, szError) == -1)
	{
		fprintf(stderr, "Error in pcap_findalldevs: %s\n", szError);
		exit(1);
	}

	printf("\nHere is a list of available devices on your system:\n\n");
	for (dev = allDevices; dev != NULL; dev = dev->next)
	{
		char* pStr;
		pcap_addr_t *dev_addr;

		if ((dev->flags & PCAP_IF_LOOPBACK) != 0)
			continue;
		if ((dev->flags & PCAP_IF_UP) == 0)
			continue;
		if ((dev->flags & PCAP_IF_RUNNING) == 0)
			continue;

		printf("%d. %s\n", ++i, dev->description);

		if ((dev->description == NULL) || (strlen(dev->description) <= 0))
			continue;

		if (strstr(dev->description, "NdisWan") != NULL)
			continue;

		for (dev_addr = dev->addresses; dev_addr != NULL; dev_addr = dev_addr->next) 
		{
			if (dev_addr->addr->sa_family == AF_INET && dev_addr->addr && dev_addr->netmask) 
			{
				printf("    Found a device %s on address %s with netmask %s\n", dev->name, 
					inet_ntoa(((struct sockaddr_in *)dev_addr->addr)->sin_addr), 
					inet_ntoa(((struct sockaddr_in *)dev_addr->netmask)->sin_addr));
				continue;
			}
		}

		if (-1 == pcap_lookupnet(dev->name, &ip, &subnet_mask, szError))
		{
			printf("%s\n", szError);
			continue;
		}

		address.s_addr = ip;
		pStr = inet_ntoa(address);
		if (pStr == NULL)
			continue;
		strcpy(szHostIP, pStr);

		if (strstr(szHostIP, "0.0.0.0") == szHostIP)
			continue;
		if (strstr(szHostIP, "127.0") == szHostIP)
			continue;
		if (strstr(szHostIP, "169.254") == szHostIP)
			continue;

		address.s_addr = subnet_mask;
		pStr = inet_ntoa(address);
		if (pStr == NULL)
			continue;
		strcpy(szHostSubnet, pStr);

		strcpy(szHostName, dev->name);

		printf("Selected Host: %s\n", szHostName);
		printf("     IP address: %s, %08lX\n", szHostIP, ip);
		printf("    Subnet Mask: %s\n", szHostSubnet);

		break;
	}
	pcap_freealldevs(allDevices);

#if 0
	//pcap_t* pcap_open_live(const char* device, int snaplen, int promisc, int to_ms, char* ebuf)
	g_hPcap = pcap_open_live(szHostName, 	// name of the device
		65536,		// portion of the packet to capture.
					//65536 grants that the whole packet will be captured on all the MACs.
		1,			// promiscuous mode (nonzero means promiscuous)
		10,			// read timeout
		szError);	// error buffer
#endif
	g_hPcap = pcap_open(szHostName,
		65536,
		PCAP_OPENFLAG_PROMISCUOUS | PCAP_OPENFLAG_MAX_RESPONSIVENESS,
		100,
		NULL,
		szError);
	if (g_hPcap == NULL)
	{
		fprintf(stderr, "\nUnable to open the adapter.\n");
		return -2;
	}

	/* Set promiscuous mode */
	pcap_set_promisc(g_hPcap, 1);

	pcap_setdirection(g_hPcap, PCAP_D_INOUT);

	pcap_setmode(g_hPcap, MODE_MON);// MODE_CAPT);

/*
	if (pcap_compile(g_hPcap, &filterCompile, szFilter, 0, ip) == -1)
	{
		fprintf(stderr, "Couldn't parse filter %s: %s\n", szFilter, pcap_geterr(g_hPcap));
		return(2);
	}

	if (pcap_setfilter(g_hPcap, &filterCompile) == -1)
	{
		fprintf(stderr, "Couldn't install filter %s: %s\n", szFilter, pcap_geterr(g_hPcap));
		return(2);
	}
*/
	g_dmaQLock = CreateMutex(NULL, FALSE, NULL);
#if (NO_LWIP > 0)
	g_pcapSemaphore = CreateSemaphore(NULL, 0, MAX_DMA_QUEUE, NULL);
#else
#endif
	InterlockedExchange(&g_nKeepRunning, 1);

	g_appThread  = CreateThread(NULL, 0, AppThread, NULL, 0, NULL);
	g_mainThread = CreateThread(NULL, 0, MainLoopThread, NULL, 0, NULL);
	if (g_mainThread != NULL)
		printf("Please press ESC key to quit\n ");

	sys_init();

	while(g_mainThread != NULL)
	{
		struct pcap_pkthdr* pkt_header;
		u_char*				pkt_data;

		int  result = pcap_next_ex(g_hPcap, &pkt_header, &pkt_data);
/*
	1  if the packet has been read without problems
	0  if the timeout set with pcap_open_live() has elapsed. In this case pkt_header and pkt_data don't point to a valid packet
	-1 if an error occurred
	-2 if EOF was reached reading from an offline capture
*/
		if (result == 1)
		{
			DMA_push(pkt_header, pkt_data);
		}
		else
		{
			//Sleep(10);
		}

		//check key press
		{
			int key = 0;
			if (_kbhit())
			{
				key = _getch();
				if (key == 27)
				{
					InterlockedExchange(&g_nKeepRunning, 1);
					Sleep(100);
					break;
				}
			}
		}
	}
	pcap_close(g_hPcap);

	if (g_mainThread != NULL)
		CloseHandle(g_mainThread);

	while (TRUE)
	{
		struct packet_wrapper* pkt = NULL;

		pkt = DMA_pop();
		if (pkt == NULL)
			break;
		DMA_free(pkt);
	}
}

#if (NO_LWIP > 0)
DWORD WINAPI MainLoopThread(void* data)
{
	long	nMore = 0;
	long 	nextTimer = 200;

	struct packet_wrapper* pkt = NULL;

	printf("Monitor started\n ");

	while (g_nKeepRunning)
	{
		DWORD dwWait = WaitForSingleObject(g_pcapSemaphore, 10);
		if (dwWait == WAIT_TIMEOUT)
			continue;
		if (dwWait != WAIT_OBJECT_0)
			continue;

		pkt = DMA_pop();
		if (pkt != NULL)
		{
			packet_handler(NULL, pkt->len, pkt->packet);

			DMA_free(pkt);
		}
	}

	printf("Monitor stopped\n ");
	return 0;
}
#else
DWORD WINAPI MainLoopThread(void* data)
{
	long	nNeedRecall = 0;
	long 	timeToNext = 200;

	struct packet_wrapper* pkt = NULL;

	printf("LWIP started\n ");

	LwipInit(0);

	while(g_nKeepRunning)
	{
		tcpip_inloop(10); //wait for 10ms at most
	}

	printf("LWIP stopped\n ");
	return 0;
}

extern struct netif main_netif;
struct altcp_pcb *g_pcbListen80 = NULL;
struct altcp_pcb *g_pcbListen8080 = NULL;

extern void tcp_kill_all(void);

DWORD WINAPI AppThread(void* data)
{
	long	restart = 0;

	SetupHttpContext();

	while (g_nKeepRunning)
	{
		ClearOwner(NULL);

		while (g_tcpipReady == 0)
		{
			Sleep(50);
		}

		if (restart > 0)
		{
			Sleep(10*1000);
		}

		PrintLwipStatus();

		netbiosns_set_name("lwipSimu");// GetDeviceName());
		netbiosns_init();

		ssdpInit(&main_netif);

		g_pcbListen80 = HttpdInit(80);
		g_pcbListen8080 = HttpdInit(8080);

		LogPrint(0, "App Service started\r\n");

		PrintLwipStatus();

		while(TRUE)
		{
			if (g_tcpipReady == 0)
				break;

			Sleep(50);
		}

		HttpdStop(g_pcbListen8080);
		HttpdStop(g_pcbListen80);

		LogPrint(0, "App Service stopped\r\n");

		ssdpDown();
		netbiosns_stop();

		tcp_kill_all();

		PrintLwipStatus();

		restart = 1;
	}
}

#endif
