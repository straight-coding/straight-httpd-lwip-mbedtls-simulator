/*
  ethernetif.h
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#ifndef _ETHERNETIF_H
#define _ETHERNETIF_H

#include "lwip/err.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"

typedef struct pbuf pbuf_t;
typedef struct netif netif_t;

err_t  ethernetif_input(struct netif *inp);
err_t  ethernetif_init(struct netif *net);
pbuf_t* low_level_input(struct netif *netif);
err_t  low_level_output(struct netif *netif, struct pbuf *p);

#endif
