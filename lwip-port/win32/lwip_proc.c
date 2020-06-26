/*
  lwip_proc.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#include "lwip/init.h"

#include "lwip/dhcp.h"
#include "lwip/priv/tcpip_priv.h"

#include "netif/ethernetif.h"
#include "arch/port.h"

#include "lwip/altcp.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"

struct altcp_tls_config* cfg = NULL;
size_t privkey_len = 0;
size_t privkey_pass_len = 0;
size_t cert_len = 0;

const char *privkey_pass = "artisman";

const char *privkey = "-----BEGIN PRIVATE KEY-----\n"\
"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCVTiXMTzWj90Fj\n"\
"49Pcyd9m7GzK7jYhQHKl12yLJLBFgJT3YUngMq9+kdvR/sq8JW5t27pQkxUzsIMS\n"\
"E6OE7HS4G+QGdFeUu7jY54IXRO7cIRsgpSh8/lxVG1JwANe7YdzccYr5cwv+8ixo\n"\
"It6rl+QJ4kCeVBGAYJ5V3lNL9usSRviF2VGUc6Q4ObpbcHfSmIAP5Y4Kcb6L6dnH\n"\
"zsCMNK/cv9kcnFBDZL/RFwP30M9rFCjHneb1v+ZQcDU7MvJQVIS6USi+a5Pse1/k\n"\
"OJXQf6lSu9FUXb6IjppncWUfrkF+dBke3Nrhyt6Cgeg+a+WOAd9HHSzsYPJiNp90\n"\
"RH2j7+/hAgMBAAECggEAEx3BWypdHNRAjBCUhLVYJC5rM4RSa+l7hF5TqHEXHJ78\n"\
"87uCIPF8ZME6GuX7gSFtxSUX8s8SLN8RuRPIoPFBdH0lsendeM6cOTFTB7Wsu4SF\n"\
"m6VpzK8olUD9Shfhhz1dcOAcwhmH1KmfI+orBl9ZNCbCzULIRt6YBziDA7vmlx1L\n"\
"DYR3Qw6SIEq7vX7E2300Cl7iX9uPjOWhnIpTYuEWan2CBqkUuukmjyEiide/3NZs\n"\
"Nm4aop7n6Jxi1pUvpXd/oCEI0HeV+5p0S7uNhrlBRg4S74pxmBee1kFa6Ixw1b/u\n"\
"4ek4CsY4tnN1/uPmChhc/Lts0xo6OZ8yM3PYP8SCHQKBgQDFxITd2CQBwTItj9KA\n"\
"hneTJaaD3/KeDOhR0ymD5Anmorl/2g0G+OejIaZpmDVTHwIfPO3wvBpmYGDq1dbk\n"\
"b0rha4iUq8YsMw92RZ08oYujXY8FnS+b6Kc5SWOqeNWGkiTUv6H2fPLj0qdVffK9\n"\
"uEPVKHnZ90zSt9UXNC1XDAtSBwKBgQDBRJgBZyKosWWzC+oLXUXTNYOSB6hNLXsQ\n"\
"HI2QdWLKlPld3JiEwMZ4uNiOkTS9mUeb/SWh8AMVuHar5zwyKim6/nbJLGn1zGJ9\n"\
"CGK/Guph7roc+PIew5+IekavJyAzZZbpw+/PrbipixXWn5aB3egMOM1RWtL6LLuJ\n"\
"8GZcZG6U1wKBgBLsdgZAS1m7odCIRY000LZM0P0nbbC/7W7+9KcBKA1gnr1kIQD8\n"\
"yjVq3+CUxu14NxzEGMSDS0dmi3+NDK35FEIzpvMK6MCL9jvL93q4voLYTfosi0Sw\n"\
"42dw5U+Hlm71Bv8wgw/x7s/r9UUR8ytCOYNpBxfbOQekvYgl3vzIU0D7AoGBALz6\n"\
"6LO+eIqBZFNuW/2ex88d7bhWGoDU7xezA830qpQylZ/tO4nbwnZ7MO4/GFYo0ne6\n"\
"UhkFys5rYEb5Rcg7qDB78AUIk9fQcaGXGI+LrxHx0DTSTFY+rPlTr1hHptn7BVUx\n"\
"zYXCdeX65XDG/fGg3e1NgZ6Cc/hC02KvGjhP0D1jAoGBAIJZVoWtKkHue2NWGHiJ\n"\
"B/AZnpXJzFe5ydonffMBHU1p/xfMUhDl9+9ar1GweQt/vTJ13MYzzCne3eUxp422\n"\
"SjBYrPGZh6/AD/ONCvXr0oCUmihhBRMDva6bgkroOcd1+y5wix8/6LORpTrmuvC8\n"\
"ccjQorMq/SRSgKBNsFjPmwzI\n"\
"-----END PRIVATE KEY-----\n";

const char *cert = "-----BEGIN CERTIFICATE-----\n"\
"MIICtDCCAZygAwIBAgIQegEvyv9aoL1BjqZLHeUhNzANBgkqhkiG9w0BAQUFADAW\n"\
"MRQwEgYDVQQDEwthcnRpc21hbi5jbjAeFw0xODA2MDMwNzAwMDBaFw0yODA2MTAw\n"\
"NzAwMDBaMBYxFDASBgNVBAMTC2FydGlzbWFuLmNuMIIBIjANBgkqhkiG9w0BAQEF\n"\
"AAOCAQ8AMIIBCgKCAQEAlU4lzE81o/dBY+PT3MnfZuxsyu42IUBypddsiySwRYCU\n"\
"92FJ4DKvfpHb0f7KvCVubdu6UJMVM7CDEhOjhOx0uBvkBnRXlLu42OeCF0Tu3CEb\n"\
"IKUofP5cVRtScADXu2Hc3HGK+XML/vIsaCLeq5fkCeJAnlQRgGCeVd5TS/brEkb4\n"\
"hdlRlHOkODm6W3B30piAD+WOCnG+i+nZx87AjDSv3L/ZHJxQQ2S/0RcD99DPaxQo\n"\
"x53m9b/mUHA1OzLyUFSEulEovmuT7Htf5DiV0H+pUrvRVF2+iI6aZ3FlH65BfnQZ\n"\
"Htza4cregoHoPmvljgHfRx0s7GDyYjafdER9o+/v4QIDAQABMA0GCSqGSIb3DQEB\n"\
"BQUAA4IBAQBIoeZKxvf1Aia/UV5shpKIe8IQNObMnwLxPyG6uVF2Uh/4bnIG3O5A\n"\
"IfikcAdKp8TXcxXhqV8Kd5zpdV5/K6Jm4SzHWP8Ch2b/udBX4931jqOO3Th4djDI\n"\
"//wgr6Vypjr4nnpVtu4mp/izdNUNYN1NmQSc79YDOG52LA39AB6gdzZ74qawMEO2\n"\
"Oeqy5S007/XYQmx3J1wSy7G2PiRMWyy6/eyiqileemjeVRp0TW/kXAxsEnJIu/Qy\n"\
"KkhhTdXTWEPFDFExVWKlI8ANqD4FLqrainF50tIet+KWJ54ABDWqP081CGAGzrlN\n"\
"kKQb3egC/xrex53GcRjfTBWC6+v7fdH5\n"\
"-----END CERTIFICATE-----\n";

extern sys_mbox_t tcpip_mbox;
struct netif main_netif;
long g_tcpipReady = 0;

int urgentCount = 0;
int nonUrgent = 0;

int noTimerCount = 0;
int timer0Count = 0;
int timer1Count = 0;
int eventCount = 0;

int ethCount = 0;
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

unsigned char memory_buf[128 * 1024];
/*
void mutex_init(mbedtls_threading_mutex_t *mutex)
{
	OS_ERR err;

	mutex->is_valid = 0;
	OSMutexCreate(&mutex->mutex, "mbedtls mutex", &err);
	if (err == OS_ERR_NONE)
	{
		mutex->is_valid = 1;
	}
}

void mutex_free(mbedtls_threading_mutex_t *mutex)
{
	OS_ERR err;
	if (mutex == NULL)
		return;

	mutex->is_valid = 0;
	OSMutexDel(&mutex->mutex, OS_OPT_DEL_ALWAYS, &err);
	if (err == OS_ERR_NONE)
	{
	}
}

int mutex_lock(mbedtls_threading_mutex_t *mutex)
{
	OS_ERR  err;
	CPU_TS  ts;

	if (mutex == NULL || !mutex->is_valid)
		return(MBEDTLS_ERR_THREADING_BAD_INPUT_DATA);

	OSMutexPend(&mutex->mutex, (OS_TICK)0, (OS_OPT)OS_OPT_PEND_BLOCKING, &ts, &err);
	return 0;
}

int mutex_unlock(mbedtls_threading_mutex_t *mutex)
{
	OS_ERR  err;
	CPU_TS  ts;

	if (mutex == NULL || !mutex->is_valid)
		return(MBEDTLS_ERR_THREADING_BAD_INPUT_DATA);

	OSMutexPost(&mutex->mutex, (OS_OPT)OS_OPT_POST_NONE, &err);
	return 0;
}
*/
void InitCert()
{
	privkey_len = strlen(privkey) + 1;
	privkey_pass_len = strlen(privkey_pass) + 1;
	cert_len = strlen(cert) + 1;

	//mbedtls_memory_buffer_alloc_init(memory_buf, sizeof(memory_buf));

	//mbedtls_threading_set_alt(mutex_init, mutex_free, mutex_lock, mutex_unlock);

	//cfg = altcp_tls_create_config_server_privkey_cert((u8_t*)privkey, privkey_len, (u8_t*)privkey_pass, privkey_pass_len, (u8_t*)cert, cert_len);
}

void LwipInit(int reboot)
{
	struct netif* nif;

	lwip_init();

	InitCert();

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

	LWIP_DEBUGF(REST_DEBUG, ("ip: %u.%u.%u.%u, gateway: %u.%u.%u.%u, subnet: %u.%u.%u.%u",
		((GetMyIP() >> 24) & 0xFF), ((GetMyIP() >> 16) & 0xFF),
		((GetMyIP() >> 8) & 0xFF), ((GetMyIP() & 0xFF)),
		((GetGateway() >> 24) & 0xFF), ((GetGateway() >> 16) & 0xFF),
		((GetGateway() >> 8) & 0xFF), ((GetGateway() & 0xFF)),
		((GetSubnet() >> 24) & 0xFF), ((GetSubnet() >> 16) & 0xFF),
		((GetSubnet() >> 8) & 0xFF), ((GetSubnet() & 0xFF))));

	g_tcpipReady = 1;
}

void LwipLinkDown(void)
{
	g_tcpipReady = 0;
	
	dhcp_stop(&main_netif);
	
	netif_set_down(&main_netif);
	netif_set_link_down(&main_netif);	
}
