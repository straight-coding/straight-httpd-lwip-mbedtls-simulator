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

int urgentCount = 0;
int nonUrgent = 0;
int ethCount = 0;

int noTimerCount = 0;
int timer0Count = 0;
int timer1Count = 0;
int eventCount = 0;

struct tcpip_msg eth_msg;
void NotifyFromEthISR(void)
{
	eth_msg.type = ETHNET_INPUT;
	sys_mbox_trypost_fromisr(&tcpip_mbox, &eth_msg);

	ethCount++;
}

ip_addr_t main_netif_ipaddr, main_netif_netmask, main_netif_gw;

extern void LwipLinkUp(void);
extern void LwipLinkDown(void);

void LwipInit(int reboot)
{
	struct netif* nif;

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
	
	nif = netif_add(&main_netif,
					&main_netif_ipaddr, 
					&main_netif_netmask, 
					&main_netif_gw, 
					NULL, 
					ethernetif_init, 
					tcpip_input);
	netif_set_default(&main_netif);
	
	LwipLinkUp();
}

void LwipStop(void)
{
}

extern void tcpip_thread_handle_msg(struct tcpip_msg *msg);
unsigned long g_nTimerLastCheck = 0;
int tcpip_inloop(void)
{
	struct tcpip_msg *msg;
	u32_t 		sleeptime;
	u32_t ret;

	int waitMS = 100; //if not urgent
	int waitMin = 10; //if urgent

	LOCK_TCPIP_CORE();

	sleeptime = sys_timeouts_sleeptime(); /* Return the time left before the next timeout is due. If no timeouts are enqueued, returns 0xffffffff */
	if (sleeptime == SYS_TIMEOUTS_SLEEPTIME_INFINITE)
	{ //timer queue is empty
		noTimerCount++;
	}
	else if (sleeptime == 0)
	{ //timeout
		sys_check_timeouts();

		sleeptime = sys_timeouts_sleeptime(); /* Return the time left before the next timeout is due */
		timer0Count++;
	}
	else
	{
		timer1Count++;
	}

	if (sleeptime != SYS_TIMEOUTS_SLEEPTIME_INFINITE)
		waitMS = sleeptime; //next timer will come soon, in urgent
	if (waitMS == 0)
		waitMS = waitMin;

	UNLOCK_TCPIP_CORE();

	if (sleeptime == 0)
		return 1;

	ret = sys_arch_mbox_fetch(&tcpip_mbox, (void **)&msg, waitMS);
	if ((SYS_MBOX_EMPTY != ret) && (msg != NULL))
	{
		LOCK_TCPIP_CORE();
			tcpip_thread_handle_msg(msg);
		UNLOCK_TCPIP_CORE();

		eventCount++;
		waitMS = waitMin;
	}
	else
	{ //nothing happened
		waitMS = 100; //not urgent
		if (sleeptime == 0)
			waitMS = waitMin; //in urgent
	}

	if (waitMS == waitMin)
	{
		urgentCount++;
		return 1;
	}

	nonUrgent++;
	return 0;
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
	
	if (dhcp_start(&main_netif) != ERR_OK) 
	{
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
