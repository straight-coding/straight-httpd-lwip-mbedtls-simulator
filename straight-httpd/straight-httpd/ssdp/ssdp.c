/*
  ssdp.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#include "lwip/udp.h"
#include "lwip/igmp.h"
#include "lwip/tcpip.h"
#include "lwip/inet.h"

#include "ssdp.h"

#define UPNP_CACHE_SEC			1800
#define UPNP_MULTICAST_ADDRESS  "239.255.255.250"
#define UPNP_MULTICAST_PORT		1900

#define UPNP_LOCATION_PORT		80 //to request detail information
#define UPNP_LOCATION_XML		"upnp_device.xml"

typedef enum
{
	ADVERTISE_UP = 0,
	ADVERTISE_DOWN = 1,
	MSEARCH_REPLY = 2
}SSDP_TYPES;

static void ssdp_listener(void *arg, struct udp_pcb *upcb, struct pbuf *p, ip_addr_t *addr, u16_t port);
static void ssdp_send_to(SSDP_TYPES type, ip_addr_t* ipDest, unsigned long portDest);
static void ssdp_advertise(void* arg);

static sys_mutex_t		lock_ssdp;
static ip4_addr_t 		g_ssdpGroupIP;
static ip_addr_t		g_ssdpLocalIP;
static struct udp_pcb*	g_ssdpPcb;

extern u32_t GetMyIP(void);
extern u32_t GetGateway(void);
extern u32_t GetSubnet(void);

extern void LogPrint(int level, char* format, ...);

ip_addr_t* GetLocalhost(void)
{
	g_ssdpLocalIP.addr = PP_HTONL(GetMyIP());
	return &g_ssdpLocalIP;
}

void ssdpInit(struct netif *net)
{
	if (sys_mutex_new(&lock_ssdp) != ERR_OK)
	{
	}
	sys_mutex_lock(&lock_ssdp);
	{
		net->flags |= NETIF_FLAG_IGMP;

		IP4_ADDR(&g_ssdpGroupIP, 239, 255, 255, 250);
		igmp_joingroup(GetLocalhost(), &g_ssdpGroupIP);

		g_ssdpPcb = (struct udp_pcb*) udp_new();
		if (g_ssdpPcb != NULL)
		{
			udp_bind(g_ssdpPcb, IP_ADDR_ANY, UPNP_MULTICAST_PORT);
			udp_recv(g_ssdpPcb, (udp_recv_fn)ssdp_listener, NULL);
		}
	}

	sys_mutex_unlock(&lock_ssdp);

	tcpip_timeout(60 * 1000UL, ssdp_advertise, (void*)ADVERTISE_UP);

	LWIP_DEBUGF(REST_DEBUG, ("ssdp: started"));
}

void ssdpDown(void)
{
	tcpip_untimeout(ssdp_advertise, (void*)ADVERTISE_UP);

	sys_mutex_lock(&lock_ssdp);
	{
		ip_addr_t castIp;
		ssdp_send_to(ADVERTISE_DOWN, 0, 0);

		udp_remove(g_ssdpPcb);
		g_ssdpPcb = NULL;

		IP4_ADDR(&castIp, 239, 255, 255, 250);
		igmp_leavegroup(GetLocalhost(), &castIp);
	}
	sys_mutex_unlock(&lock_ssdp);

	LWIP_DEBUGF(REST_DEBUG, ("ssdp: stopped"));
}

void ssdp_advertise(void* arg)
{
	sys_mutex_lock(&lock_ssdp);
		ssdp_send_to((SSDP_TYPES)arg, 0, 0);
	sys_mutex_unlock(&lock_ssdp);

	tcpip_timeout(60 * 1000UL, ssdp_advertise, (void*)ADVERTISE_UP);
}

void ssdp_listener(void *arg, struct udp_pcb *upcb, struct pbuf *p, ip_addr_t *addr, u16_t port)
{ //message from the control point
	u32_t i = 0;
	u32_t line = 0;
	u32_t begin = 0;
	u32_t end = 0; //not included
	u32_t len = 0;

	u32_t got_host = 0;
	u32_t got_st = 0;
	u32_t got_man = 0;
	u32_t got_mx = 0;
	long  ssdp_mx_min = 0;

	char  szBuffer[256];
	char* buff = szBuffer;

	memset(szBuffer, 0, sizeof(szBuffer));
	len = pbuf_copy_partial(p, szBuffer, sizeof(szBuffer) - 4, 0);
	pbuf_free(p);

	sys_mutex_lock(&lock_ssdp);
	if (strstr((const char*)buff, "NOTIFY "))
	{
		sys_mutex_unlock(&lock_ssdp);
		return;
	}

	for (i = 0; i < len - 1; i++)
	{
		if ((buff[i] == '\r') && (buff[i + 1] == '\n'))
		{
			buff[i] = 0;
			end = i;

			if (line == 0)
			{
				if (Strnicmp(buff + begin, "M-SEARCH", 8) != 0)
				{
					sys_mutex_unlock(&lock_ssdp);
					return;
				}
			}
			else
			{
				if (Strnicmp(buff + begin, "Host:", 5) == 0)
				{ //The host and port (HOST) the message will be sent to (multicast or unicast)
					got_host = 1;
				}
				else if (Strnicmp(buff + begin, "ST:", 3) == 0)
				{ //The search target (ST) of the service the search request is attempting to discover
					if (strstr(buff + begin + 3, "ssdp:all") != 0)
						got_st = 1;
					else if (strstr(buff + begin + 3, "upnp:rootdevice") != 0)
						got_st = 1;
				}
				else if (Strnicmp(buff + begin, "Man:", 4) == 0)
				{ //The message type (MAN), for an M-Search this is always "ssdp:discover".
					if (strstr(buff + begin + 4, "\"ssdp:discover\"") != 0)
						got_man = 1;
				}
				else if (Strnicmp(buff + begin, "MX:", 3) == 0)
				{ //The maximum wait response time (MX) in seconds
					ssdp_mx_min = ston(buff + begin + 3);
					got_mx = 1;
				}
			}

			i++;

			line++;
			begin = end + 2;
		}
	}

	if (got_host && got_st && got_man && got_mx && (ssdp_mx_min > 0))
	{ //reply
		ssdp_send_to(MSEARCH_REPLY, addr, (u32_t)port);
	}

	sys_mutex_unlock(&lock_ssdp);
}

char* htoa(ip_addr_t * addr)
{
	return inet_ntoa(*addr);
}

void ssdp_send_to(SSDP_TYPES type, ip_addr_t* ipDest, unsigned long portDest)
{
	char *NTString = "";
	char uuid_string[128];
	char msg[768];

	ip_addr_t reply_ip;
	unsigned short reply_port = UPNP_MULTICAST_PORT;
	IP4_ADDR(&reply_ip, 239, 255, 255, 250);

	LWIP_sprintf(uuid_string, "%s", GetDeviceUUID());

	memset(msg, 0, sizeof(msg));

	switch (type)
	{
		case ADVERTISE_UP:
		case ADVERTISE_DOWN:
			NTString = "NT";
			strcpy(msg, "NOTIFY * HTTP/1.1\r\n");

			//The host and port (HOST) the message will be sent to
			LWIP_sprintf(msg + strlen(msg), "HOST: %s:%d\r\n", UPNP_MULTICAST_ADDRESS, UPNP_MULTICAST_PORT);

			//The cache control (CACHE-CONTROL) value to determine for how long the message is valid
			LWIP_sprintf(msg + strlen(msg), "CACHE-CONTROL: max-age=%d\r\n", UPNP_CACHE_SEC);

			//The notification subtype (NTS), for an alive message this will always be ssdp:alive
			LWIP_sprintf(msg + strlen(msg), "NTS: %s\r\n", (type == ADVERTISE_UP ? "ssdp:alive" : "ssdp:byebye"));
			break;
		case MSEARCH_REPLY:
			ip_addr_set(&reply_ip, ipDest);
			reply_port = portDest;

			NTString = "ST";
			strcpy(msg, "HTTP/1.1 200 OK\r\n");

			//The cache control (CACHE-CONTROL) value to determine for how long the message is valid
			LWIP_sprintf(msg + strlen(msg), "CACHE-CONTROL: max-age=%d\r\n", UPNP_CACHE_SEC);

			//The EXT field is required for backwards compatibility with UPnP 1.0 but can otherwise be ignored.
			LWIP_sprintf(msg + strlen(msg), "EXT: webport=%d\r\n", UPNP_LOCATION_PORT);

			LWIP_sprintf(msg + strlen(msg), "SN: %s\r\n", GetDeviceSN());
			break;
		default:
			return;
	}

	if (type != ADVERTISE_DOWN)
	{ //The location URL (LOCATION) to allow the control point to 
		//gain more information about this service.
		LWIP_sprintf(msg + strlen(msg), "LOCATION: http://%s/%s\r\n",
			htoa((ip_addr_t *)GetLocalhost()), UPNP_LOCATION_XML);
	}

	//The server system information (SERVER) value 
	//[OS-Name] UPnP/[Version] [Product-Name]/[Product-Version].
	LWIP_sprintf(msg + strlen(msg), "SERVER: %s, UPnP/1.0\r\n", GetDeviceName());

	//The search target (ST) of the service that is responding
	//The notification type (NT) that defines the service it offers (the equivalent of ST in an M-Search message).
	LWIP_sprintf(msg + strlen(msg), "%s: upnp:rootdevice\r\n", NTString);

	//The unique service name (USN) to identify the service.
	LWIP_sprintf(msg + strlen(msg), "USN: uuid:%s::upnp:rootdevice\r\n", uuid_string);

	strcat(msg, "\r\n");
	{
		struct pbuf * udpbuf = pbuf_alloc(PBUF_TRANSPORT, 0, PBUF_REF);
		if (udpbuf != NULL)
		{
			udpbuf->payload = (void*)msg;
			udpbuf->len = udpbuf->tot_len = strlen(msg);
			udp_sendto(g_ssdpPcb, udpbuf, &reply_ip, reply_port);

			udpbuf->payload = NULL;
			pbuf_free(udpbuf);

			if (type == ADVERTISE_UP)
				LWIP_DEBUGF(SSDP_DEBUG, ("ssdp: advertise up"));
			else if (type == ADVERTISE_DOWN)
				LWIP_DEBUGF(SSDP_DEBUG, ("ssdp: advertise down"));
			else if (type == MSEARCH_REPLY)
				LWIP_DEBUGF(API_LIB_DEBUG, ("ssdp: reply ==> %s:%d", htoa((ip_addr_t *)&reply_ip), reply_port));
		}
	}
}
