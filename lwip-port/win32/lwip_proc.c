/*
  lwip_proc.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#include "lwip/init.h"

#include "lwip/dhcp.h"
#include "lwip/priv/tcpip_priv.h"

#include "netif/ethernetif.h"
#include "lwip_port.h"

extern sys_mbox_t tcpip_mbox;
struct netif main_netif;
long g_tcpipReady = 0;

struct tcpip_msg eth_msg;
void NotifyFromEthISR(void)
{
	eth_msg.type = ETHNET_INPUT;
	sys_mbox_trypost_fromisr(&tcpip_mbox, &eth_msg);
}

ip_addr_t main_netif_ipaddr, main_netif_netmask, main_netif_gw;

extern void LwipLinkUp(void);
extern void LwipLinkDown(void);

void LwipInit(int reboot)
{
	lwip_init();

	if (sys_mbox_new(&tcpip_mbox, TCPIP_MBOX_SIZE) != ERR_OK) 
	{
		LWIP_ASSERT("failed to create tcpip_thread mbox", 0);
	}
	
	if (sys_mutex_new(&lock_tcpip_core) != ERR_OK) 
	{
		LWIP_ASSERT("failed to create lock_tcpip_core", 0);

		LWIP_MARK_TCPIP_THREAD();
	}
	
	main_netif_ipaddr.addr = PP_HTONL(GetMyIP());
	main_netif_netmask.addr = PP_HTONL(GetSubnet());
	main_netif_gw.addr = PP_HTONL(GetGateway());

	netif_set_flags(&main_netif, NETIF_FLAG_IGMP);
	
	netif_add(&main_netif, &main_netif_ipaddr, &main_netif_netmask, &main_netif_gw, NULL, ethernetif_init, tcpip_input);
	netif_set_default(&main_netif);
	
	LwipLinkUp();
}

void LwipStop(void)
{
}

extern void tcpip_thread_handle_msg(struct tcpip_msg *msg);
unsigned long g_nTimerLastCheck = 0;
int tcpip_inloop(u32_t maxWait)
{
	struct tcpip_msg *msg;
	u32_t 		sleeptime;
	int			needRecall = 0;

	unsigned long now = sys_now();
	if (now == 0)
		now ++;
	
	LOCK_TCPIP_CORE();

	now = msDiff(now, g_nTimerLastCheck);
	if ((g_nTimerLastCheck == 0) || (now >= 100))
	{ //check timeout every 100ms
		sleeptime = sys_timeouts_sleeptime(); /* Return the time left before the next timeout is due. If no timeouts are enqueued, returns 0xffffffff */
		if (sleeptime == 0)
		{ //timeout right now
			sys_check_timeouts();
			needRecall = 1; //next pending
		}
		else
		{ //SYS_TIMEOUTS_SLEEPTIME_INFINITE, or near in the future
			g_nTimerLastCheck = now;
		}
	}
	UNLOCK_TCPIP_CORE();

	if (needRecall == 0)
	{
		if (SYS_MBOX_EMPTY != sys_arch_mbox_fetch(&tcpip_mbox, (void **)&msg, maxWait))
		{
			if (msg != NULL)
			{
				LOCK_TCPIP_CORE();
					tcpip_thread_handle_msg(msg);
				UNLOCK_TCPIP_CORE();

				needRecall = 1;
			}
		}
	}
	return needRecall;
}

void LwipLinkUp(void)
{
	netif_set_link_up(&main_netif);	
	netif_set_up(&main_netif);
	
	main_netif_ipaddr.addr = PP_HTONL(GetMyIP());
	main_netif_netmask.addr = PP_HTONL(GetSubnet());
	main_netif_gw.addr = PP_HTONL(GetGateway());
	
	netif_set_ipaddr(&main_netif, &main_netif_ipaddr);
	netif_set_netmask(&main_netif, &main_netif_netmask);
	netif_set_gw(&main_netif, &main_netif_gw);
	
	if (dhcp_start(&main_netif) != ERR_OK) {
         LWIP_DEBUGF(LWIP_DBG_ON, ("DHCP failed"));
	}  
}

void OnDhcpFinished(void)
{
	SetMyIP(PP_HTONL(main_netif.ip_addr.addr));
	SetGateway(PP_HTONL(main_netif.gw.addr));
	SetSubnet(PP_HTONL(main_netif.netmask.addr));
	
	netif_set_flags(&main_netif, NETIF_FLAG_IGMP);
		
	LWIP_DEBUGF(REST_DEBUG, ("ip: %08lX, gateway: %08lX, subnet: %08lX", GetMyIP(), GetGateway(), GetSubnet()));
	
	g_tcpipReady = 1;
}

void LwipLinkDown(void)
{
	g_tcpipReady = 0;
	
	dhcp_stop(&main_netif);
	
	netif_set_down(&main_netif);
	netif_set_link_down(&main_netif);	
}
