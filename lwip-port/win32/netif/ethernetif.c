/*
  ethernetif.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#include "lwip/opt.h"
#include "lwip/sys.h"

#include "netif/etharp.h"
#include "ethernetif.h"

#include "lwip_port.h"

static void low_level_init(struct netif *netif)
{
	u8_t* pMac;
	struct ethernetif *ethernetif = netif->state;

	netif->hwaddr_len = ETHARP_HWADDR_LEN;
	pMac = GetMyMAC();

	netif->hwaddr[0] = pMac[0];
	netif->hwaddr[1] = pMac[1];
	netif->hwaddr[2] = pMac[2];
	netif->hwaddr[3] = pMac[3];
	netif->hwaddr[4] = pMac[4];
	netif->hwaddr[5] = pMac[5];

	netif->mtu = 1500;
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP | NETIF_FLAG_LINK_UP;
}

err_t low_level_output(struct netif *netif, struct pbuf *p)
{
	struct pbuf *q;
	int l = 0;
	u8_t* buffer = NULL;

	SYS_ARCH_DECL_PROTECT(sr);
	SYS_ARCH_PROTECT(sr);

	buffer = NIC_GetBuffer(1600);
	if (buffer != NULL)
	{
		for (q = p; q != NULL; q = q->next)
		{
			MEMCPY((u8_t*)&buffer[l], q->payload, q->len);
			l = l + q->len;
		}
		NIC_Send(buffer, l);
	}

	SYS_ARCH_UNPROTECT(sr);

	return ERR_OK;
}

pbuf_t* low_level_input(struct netif *netif)
{
	struct pbuf *p, *q;
	int len = 0;
	int l = 0;
	struct packet_wrapper* pkt = NULL;
	u8_t *buffer;

	p = NULL;

	pkt = DMA_pop();
	if (pkt != NULL)
	{
		len = pkt->len;
		buffer = (u8_t *)pkt->packet;

		p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
		if (p != NULL) 
		{
			for (q = p; q != 0; q = q->next) 
			{
				MEMCPY((u8_t*)q->payload, (u8_t*)&buffer[l], q->len);
				l = l + q->len;
			}
		}
	}
	if (pkt != NULL)
		DMA_free(pkt);

	return p;
}

err_t ethernetif_input(struct netif *inp)
{
	err_t err;
	struct pbuf *p;
	SYS_ARCH_DECL_PROTECT(sr);

	SYS_ARCH_PROTECT(sr);
		p = low_level_input(inp);
	SYS_ARCH_UNPROTECT(sr);

	if (p == NULL) 
		return ERR_MEM;

	err = inp->input(p, inp);
	if (err != ERR_OK) 
	{
		LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
		pbuf_free(p);
		p = NULL;
	}

	return err;
}

err_t ethernetif_init(struct netif *netif)
{
	netif->output = etharp_output; //defined: lwip\src\core\ipv4\etharp.c
	netif->linkoutput = low_level_output;

	low_level_init(netif);

	return ERR_OK;
}
