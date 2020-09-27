/*
  lwip_api.c
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

static struct altcp_tls_config* cfg = NULL;
static size_t privkey_len = 0;
static size_t privkey_pass_len = 0;
static size_t cert_len = 0;

static const char *privkey_pass = "straight";

static const char *privkey = "-----BEGIN ENCRYPTED PRIVATE KEY-----\n"\
	"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\n"\
	"Pdc=\n"\
	"-----END ENCRYPTED PRIVATE KEY-----\n";

static const char *cert = "-----BEGIN CERTIFICATE-----\n"\
	"MIIFEDCCAvigAwIBAgIQ+UjKCIcDNLRKcHf+9tlCFjANBgkqhkiG9w0BAQ0FADAR\n"\
	"MQ8wDQYDVQQDEwZDQVJvb3QwHhcNMjAwNjA2MDcwMDAwWhcNMzAwNjA2MDcwMDAw\n"\
	"WjAaMRgwFgYDVQQDEw9zZXJ2ZXIuc3RyYWlnaHQwggIiMA0GCSqGSIb3DQEBAQUA\n"\
	"A4ICDwAwggIKAoICAQDRA3XspjYrcSXowTn59X/fchaHiiSto9H/si7juqHLPKwM\n"\
	"GuUsremif7bWSuwOeNlix8VlDAC0F8YvwIsVgpj2Okf13K8qWvwMrbnxf9sbF0Gr\n"\
	"iFDhguK3mJOv31hXCV0D5ApRk3VY2XXhgB39GBEapw/MEylwAEQJEHrKNmhUlMPO\n"\
	"b/14jhVx/CqDGcec3d58PjsCqbkaKK599rS6D9hBmiaYAc1j5yJ6X50tbNBuChvY\n"\
	"nmkHg72Wzeo8XMA91YXNeYbCuKnjaHfNCh3qtjyq1TjD5KNMWTm7qTjMEC9v7PFV\n"\
	"9jaVbcT/mvIzR/I0lGj7Hvhx0n3SeM2HppKC+TinlWJl8JON3up9Jcc7GPJmFKsk\n"\
	"DvRUxkbiEE00gnzPKwNxo3Jv6Dkp2iG13NWR1zS7C5X4K5C0ToSDJnG64CqhgmO7\n"\
	"cvHi7Bp2SDGAhdDAFC36ZuuAtl4CyonUIMXwmbnEIHURDFyr1wwCq6cUonRTyqS2\n"\
	"jQ4h2ujf2r4Fw5SuVYG1hnGaga17MWE6SmdftHKN1TF5jdCQb07jQBxccvK2kZL7\n"\
	"S+wUnZc3xevs82X+yZf5mwpDwIXDY3sZ444Snq+6Mi5DoPTZ9qe7zhD1jY0Ob5sJ\n"\
	"u9FYMMewlGjHl327Xf1TcBst+nOOdMpo+vOQriN+jI6oIZQEQCBuFSXVaCXQcQID\n"\
	"AQABo1swWTATBgNVHSUEDDAKBggrBgEFBQcDATBCBgNVHQEEOzA5gBCSAdfUvW/H\n"\
	"oozRFOuOwv7DoRMwETEPMA0GA1UEAxMGQ0FSb290ghCEbjEbwaNTmUlAMsisbhAO\n"\
	"MA0GCSqGSIb3DQEBDQUAA4ICAQACqqPdUx+/MWlG71RS4L4ru2GwHYBZZVF68VCN\n"\
	"DhSj2xt54FNzaW/G0OS6qCItosiYeRM+bZjDKngGmIZ8pJtGkhaqO35vnkR9jajY\n"\
	"gsH87yx/fpkObN+2Av4R6gq+e01Z+54ldBWTyIDqumIi0oKSoKBSi4hZHn14eheK\n"\
	"w0K21PzfUuPRYioECwxQOw6oAmsTrydlOdUi9vn1ZwRmlnnnAQgaRjkCAk9TymRW\n"\
	"vBlD1jbUTkONBRJSR73ry7oWvqldzAnnTh9J7v6Fy2NAJsvpT7XDavra/CEerla8\n"\
	"kaa5RRtbRqQG6wnFHpt9xTsY5n5bgEuIqwETfW34XvtbpcQ1geiDQf7vE9nJ/a0/\n"\
	"E1Z9JMzXdIzfYJ4CGDJVYCear3Pqt6vWkcYIIjmOdzpSPsImbcMGFNyIpzkxeqms\n"\
	"1g10oEe5dbcs58rkKmyU1o1gwzQrEhgzHCRObjsHLkCX2S+h7Zwunu2EN/bVOW+6\n"\
	"gbXzSdAn8t32ax5WDpkBeBP4sTKVWtf3kLR9ilODe06YOH0CkWcFGa0cf97WsZcT\n"\
	"+rTHSaMjsdU8lyitPsNFBHkrlCEEFWSG7rL08IoFomj8N5Uqr7HQpsiJfB2wnJrG\n"\
	"5ytnzI1XSYQqWotGKMz4b4pPfFK9jhyh3rtndD6xElmJdfDOLi7/pnm0CK+m1kf2\n"\
	"5Rd26w==\n"\
	"-----END CERTIFICATE-----\n";

extern sys_mbox_t tcpip_mbox;

struct netif main_netif;
long g_ipIsReady = 0; //

static int urgentCount = 0;
static int nonUrgent = 0;
static int noTimerCount = 0;
static int timer0Count = 0;
static int timer1Count = 0;
static int eventCount = 0;
static int ethCount = 0;

static struct tcpip_msg eth_msg;
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
#if LWIP_ALTCP_TLS
struct altcp_tls_config* getTlsConfig(void)
{
	struct altcp_tls_config* conf;
	privkey_len = strlen(privkey) + 1;
	privkey_pass_len = strlen(privkey_pass) + 1;
	cert_len = strlen(cert) + 1;

	//mbedtls_memory_buffer_alloc_init(memory_buf, sizeof(memory_buf));
	//mbedtls_threading_set_alt(mutex_init, mutex_free, mutex_lock, mutex_unlock);

	conf = altcp_tls_create_config_server_privkey_cert((u8_t*)privkey, privkey_len, (u8_t*)privkey_pass, privkey_pass_len, (u8_t*)cert, cert_len);
	return conf;
}
#endif
void LwipInit(void)
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
	LwipLinkDown();

	sys_mbox_free(&tcpip_mbox);
	sys_mutex_free(&lock_tcpip_core);
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
	
	if (IsDhcpEnabled())
	{
		if (dhcp_start(&main_netif) == ERR_OK)
			return;
        LWIP_DEBUGF(LWIP_DBG_ON, ("DHCP failed"));
	}

	OnDhcpFinished();
}

void OnDhcpFinished(void)
{
	SetMyIPLong(PP_HTONL(main_netif.ip_addr.addr));
	SetGatewayLong(PP_HTONL(main_netif.gw.addr));
	SetSubnetLong(PP_HTONL(main_netif.netmask.addr));

	netif_set_flags(&main_netif, NETIF_FLAG_IGMP);

	LWIP_DEBUGF(REST_DEBUG, ("ip: %u.%u.%u.%u, gateway: %u.%u.%u.%u, subnet: %u.%u.%u.%u",
		((GetMyIP() >> 24) & 0xFF), ((GetMyIP() >> 16) & 0xFF),
		((GetMyIP() >> 8) & 0xFF), ((GetMyIP() & 0xFF)),
		((GetGateway() >> 24) & 0xFF), ((GetGateway() >> 16) & 0xFF),
		((GetGateway() >> 8) & 0xFF), ((GetGateway() & 0xFF)),
		((GetSubnet() >> 24) & 0xFF), ((GetSubnet() >> 16) & 0xFF),
		((GetSubnet() >> 8) & 0xFF), ((GetSubnet() & 0xFF))));

	g_ipIsReady = 1;
}

void LwipLinkDown(void)
{
	g_ipIsReady = 0;
	
	dhcp_stop(&main_netif);
	
	netif_set_down(&main_netif);
	netif_set_link_down(&main_netif);	
}
