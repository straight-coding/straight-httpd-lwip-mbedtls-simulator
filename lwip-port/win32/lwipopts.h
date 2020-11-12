/* lwipopts.h, lwIP Options Configuration. */

#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

#define ENABLE_HTTPS	1

/*
   if lwip is used with mbedtls package, there are three options that need to be set carefully.
	MEM_SIZE: More memory is better
	TCP_MSS:  MUST be bigger enough
	TCP_WND:  MUST be greater than MBEDTLS_SSL_MAX_CONTENT_LEN (it could not be changed less than 16384)
*/

/* ---------- Memory options ---------- */
/* MEM_LIBC_MALLOC==1: Use malloc/free/realloc provided by your C-library
 * instead of the lwip internal allocator. Can save code size if you
 * already use it. */
#undef  MEM_LIBC_MALLOC
#define MEM_LIBC_MALLOC         0

/* MEMP_MEM_MALLOC==1: Use mem_malloc/mem_free instead of the lwip pool allocator.
 * Especially useful with MEM_LIBC_MALLOC but handle with care regarding execution
 * speed (heap alloc can be much slower than pool alloc) and usage from interrupts
 * (especially if your netif driver allocates PBUF_POOL pbufs for received frames
 * from interrupt)!
 * ATTENTION: Currently, this uses the heap for ALL pools (also for private pools,
 * not only for internal pools defined in memp_std.h)! */
#undef  MEMP_MEM_MALLOC
#define MEMP_MEM_MALLOC 		1 //Only one can be selected with MEM_USE_POOLS

/* MEM_USE_POOLS==1: Use an alternative to malloc() by allocating from a set
 * of memory pools of various sizes. When mem_malloc is called, an element of
 * the smallest pool that can provide the length needed is returned.
 * To use this, MEMP_USE_CUSTOM_POOLS also has to be enabled. */
#undef  MEM_USE_POOLS
#define MEM_USE_POOLS           0 //Only one can be selected with MEMP_MEM_MALLOC

/* MEM_SIZE: the size of the heap memory. If the application will send
a lot of data that needs to be copied, this should be set high. */
//100KB to support large file downloading and uploading, and keep-alive works good
//57KB basic memory to create TLS connections, and support large file downloading and uploading, but keep-alive may fail
#undef  MEM_SIZE
#if (ENABLE_HTTPS > 0)
#define MEM_SIZE                (100*1024) 
#else
#define MEM_SIZE                (24*1024) 
#endif

/* ---------- Memory options ---------- */
/* MEM_ALIGNMENT: should be set to the alignment of the CPU for which
   lwIP is compiled. 4 byte alignment -> define MEM_ALIGNMENT to 4, 2
   byte alignment -> define MEM_ALIGNMENT to 2. */
#undef  MEM_ALIGNMENT
#define MEM_ALIGNMENT           4

/* MEMP_NUM_RAW_PCB: Number of raw connection PCBs 
   (requires the LWIP_RAW option) */
#undef MEMP_NUM_RAW_PCB
#define MEMP_NUM_RAW_PCB        0

/* MEMP_NUM_UDP_PCB: the number of UDP protocol control blocks. One
   per active UDP "connection". */
#undef MEMP_NUM_UDP_PCB
#define MEMP_NUM_UDP_PCB        2

/* MEMP_NUM_TCP_PCB: the number of simulatenously active TCP connections. */
#undef MEMP_NUM_TCP_PCB
#define MEMP_NUM_TCP_PCB        6

/* MEMP_NUM_TCP_PCB_LISTEN: the number of listening TCP connections. */
#undef MEMP_NUM_TCP_PCB_LISTEN
#define MEMP_NUM_TCP_PCB_LISTEN 6

/* MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP segments. */
#undef MEMP_NUM_TCP_SEG
#define MEMP_NUM_TCP_SEG        16

#define ETH_PAD_SIZE			0 
#define LWIP_TCP                1

/* TCP Maximum segment size. http://lwip.wikia.com/wiki/Tuning_TCP */
#undef TCP_MSS
#define TCP_MSS                 800//(1500 - 40)	  /* TCP_MSS = (Ethernet MTU - IP header size - TCP header size) */

/* TCP receive window. */
#undef TCP_WND
#if (ENABLE_HTTPS > 0)
#define TCP_WND                 (22 * TCP_MSS) //TCP_WND >= MBEDTLS_SSL_MAX_CONTENT_LEN
#else
#define TCP_WND                 (4 * TCP_MSS) //TCP_WND >= MBEDTLS_SSL_MAX_CONTENT_LEN
#endif

/* Controls if TCP should queue segments that arrive out of
   order. Define to 0 if your device is low on memory. */
#undef TCP_QUEUE_OOSEQ
#define TCP_QUEUE_OOSEQ         0

#define LWIP_TCP_SACK_OUT       0

/* TCPIP_MBOX_SIZE: The mailbox size for the tcpip thread messages
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when tcpip_init is called. */
#undef TCPIP_MBOX_SIZE
#define TCPIP_MBOX_SIZE			        64

/* DEFAULT_TCP_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_TCP. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created. */
#undef DEFAULT_TCP_RECVMBOX_SIZE
#define DEFAULT_TCP_RECVMBOX_SIZE       64

/* DEFAULT_ACCEPTMBOX_SIZE: The mailbox size for the incoming connections.
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when the acceptmbox is created. */
#undef DEFAULT_ACCEPTMBOX_SIZE
#define DEFAULT_ACCEPTMBOX_SIZE         64

/* LWIP_ICMP==1: Enable ICMP module inside the IP stack.
 * Be careful, disable that make your product non-compliant to RFC1122 */
#define LWIP_ICMP               1

/* Define LWIP_DHCP to 1 if you want DHCP configuration of
   interfaces. DHCP is not implemented in lwIP 0.5.1, however, so
   turning this on does currently not work. */
#define LWIP_DHCP				1

/* LWIP_UDP==1: Turn on UDP. */
#define LWIP_UDP                1

/* LWIP_STATS==1: Enable statistics collection in lwip_stats. */
#undef LWIP_STATS
#define LWIP_STATS				0

/* LWIP_PROVIDE_ERRNO==1: Let lwIP provide ERRNO values and the 'errno' variable.
 * If this is disabled, cc.h must either define 'errno', include <errno.h>,
 * define LWIP_ERRNO_STDINCLUDE to get <errno.h> included or
 * define LWIP_ERRNO_INCLUDE to <errno.h> or equivalent. */
#define LWIP_PROVIDE_ERRNO		1

#ifdef CHECKSUM_BY_HARDWARE
  /* CHECKSUM_GEN_IP==0: Generate checksums by hardware for outgoing IP packets.*/
  #undef CHECKSUM_GEN_IP
  #define CHECKSUM_GEN_IP                 0
  /* CHECKSUM_GEN_UDP==0: Generate checksums by hardware for outgoing UDP packets.*/
  #undef CHECKSUM_GEN_UDP
  #define CHECKSUM_GEN_UDP                0
  /* CHECKSUM_GEN_TCP==0: Generate checksums by hardware for outgoing TCP packets.*/
  #undef CHECKSUM_GEN_TCP
  #define CHECKSUM_GEN_TCP                0 
  /* CHECKSUM_CHECK_IP==0: Check checksums by hardware for incoming IP packets.*/
  #undef CHECKSUM_CHECK_IP
  #define CHECKSUM_CHECK_IP               0
  /* CHECKSUM_CHECK_UDP==0: Check checksums by hardware for incoming UDP packets.*/
  #undef CHECKSUM_CHECK_UDP
  #define CHECKSUM_CHECK_UDP              0
  /* CHECKSUM_CHECK_TCP==0: Check checksums by hardware for incoming TCP packets.*/
  #undef CHECKSUM_CHECK_TCP
  #define CHECKSUM_CHECK_TCP              0
#else
  /* CHECKSUM_GEN_IP==1: Generate checksums in software for outgoing IP packets.*/
  #define CHECKSUM_GEN_IP                 1
  /* CHECKSUM_GEN_UDP==1: Generate checksums in software for outgoing UDP packets.*/
  #define CHECKSUM_GEN_UDP                1
  /* CHECKSUM_GEN_TCP==1: Generate checksums in software for outgoing TCP packets.*/
  #define CHECKSUM_GEN_TCP                1
  /* CHECKSUM_CHECK_IP==1: Check checksums in software for incoming IP packets.*/
  #define CHECKSUM_CHECK_IP               1
  /* CHECKSUM_CHECK_UDP==1: Check checksums in software for incoming UDP packets.*/
  #define CHECKSUM_CHECK_UDP              1
  /* CHECKSUM_CHECK_TCP==1: Check checksums in software for incoming TCP packets.*/
  #define CHECKSUM_CHECK_TCP              1
#endif

/* LWIP_NETCONN==1: Enable Netconn API (require to use api_lib.c) */
#define LWIP_NETCONN                    0
#define LWIP_NETCONN_FULLDUPLEX         0

/* LWIP_SOCKET==1: Enable Socket API (require to use sockets.c) */
#define LWIP_SOCKET                     0

#define LWIP_COMPAT_MUTEX               0

/*##############################################################################################*/
#define LWIP_DEBUG						1

#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_ALL //LWIP_DBG_LEVEL_WARNING	//LWIP_DBG_LEVEL_ALL
#define LWIP_DBG_TYPES_ON               LWIP_DBG_ON

#define ICMP_DEBUG                      LWIP_DBG_OFF
#define IGMP_DEBUG                      LWIP_DBG_OFF
#define MEM_DEBUG                       LWIP_DBG_OFF
#define MEMP_DEBUG                      LWIP_DBG_OFF
#define TIMERS_DEBUG                    LWIP_DBG_OFF
#define TCP_DEBUG                       LWIP_DBG_OFF
#define TCP_INPUT_DEBUG                 LWIP_DBG_OFF
#define TCP_FR_DEBUG                    LWIP_DBG_OFF
#define TCP_RTO_DEBUG                   LWIP_DBG_OFF
#define TCP_CWND_DEBUG                  LWIP_DBG_OFF
#define TCP_WND_DEBUG                   LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG                LWIP_DBG_OFF
#define TCP_RST_DEBUG                   LWIP_DBG_OFF
#define TCP_QLEN_DEBUG                  LWIP_DBG_OFF
#define UDP_DEBUG                       LWIP_DBG_OFF
#define DHCP_DEBUG                      LWIP_DBG_OFF
#define SSDP_DEBUG                      LWIP_DBG_ON
#define REST_DEBUG         				LWIP_DBG_ON

#define ALTCP_MBEDTLS_LIB_DEBUG			LWIP_DBG_ON

#define LWIP_IPV4 						1
#define LWIP_IPV6						0
#define LWIP_UDP						1
#define LWIP_TCP						1
#define LWIP_DNS                        0
#define LWIP_RAW						0
#define LWIP_AUTOIP						0
#define LWIP_IGMP						1

#define TCP_LISTEN_BACKLOG              4

#define LWIP_SINGLE_NETIF				1

#define LWIP_NETBIOS_RESPOND_NAME_QUERY	1

#define LWIP_RAND() 					((u32_t)rand())

#define LWIP_CHECKSUM_ON_COPY           1
#define LWIP_TCPIP_TIMEOUT              1

#define LWIP_ALTCP						1

#if (ENABLE_HTTPS > 0)
#define LWIP_ALTCP_TLS					1
#define LWIP_ALTCP_TLS_MBEDTLS          1
#else
#define LWIP_ALTCP_TLS					0
#define LWIP_ALTCP_TLS_MBEDTLS          0
#endif

#define ALTCP_MBEDTLS_DEBUG             LWIP_DBG_ON
//#define ALTCP_MBEDTLS_SESSION_CACHE_TIMEOUT_SECONDS   (3*60)

#define LWIP_SO_SNDTIMEO				1
#define LWIP_SO_RCVTIMEO				1
#define LWIP_SO_RCVBUF					1

#define HTTPD_SERVER_AGENT 				"Straight httpd/1.1"

#define PACK_STRUCT_USE_INCLUDES		1
#define LWIP_NO_INTTYPES_H				1

/*##############################################################################################*/

void LwipLogPrint(char* format, ... );

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef WIN32
//C4103: alignment changed after including header, may be due to missing #pragma pack(pop)
#pragma warning(disable:4103)
#endif

#endif /* __LWIPOPTS_H__ */


