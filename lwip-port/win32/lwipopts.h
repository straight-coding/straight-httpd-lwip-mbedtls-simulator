/*
  @file    lwipopts.h
  @brief   lwIP Options Configuration.
*/

#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

#ifndef WIN32
#include <includes.h>
#endif

/*----------------Thread Priority---------------------------------------------*/
#ifndef TCPIP_THREAD_PRIO
#define TCPIP_THREAD_PRIO		1
#endif
#undef  DEFAULT_THREAD_PRIO
#define DEFAULT_THREAD_PRIO		2

/**
 * SYS_LIGHTWEIGHT_PROT==1: if you want inter-task protection for certain
 * critical regions during buffer allocation, deallocation and memory
 * allocation and deallocation.
 */
#define SYS_LIGHTWEIGHT_PROT    		1
#define LWIP_TCPIP_CORE_LOCKING_INPUT   0

/**
 * NO_SYS==1: Provides VERY minimal functionality. Otherwise,
 * use lwIP facilities.
 */
#define NO_SYS                  0

/* ---------- Memory options ---------- */
/* MEM_ALIGNMENT: should be set to the alignment of the CPU for which
   lwIP is compiled. 4 byte alignment -> define MEM_ALIGNMENT to 4, 2
   byte alignment -> define MEM_ALIGNMENT to 2. */
#undef  MEM_ALIGNMENT
#define MEM_ALIGNMENT           4


/* MEM_SIZE: the size of the heap memory. If the application will send
a lot of data that needs to be copied, this should be set high. */
#undef MEM_SIZE
#define MEM_SIZE                (8192*16) 

/* MEMP_NUM_PBUF: the number of memp struct pbufs. If the application
   sends a lot of data out of ROM (or other static memory), this
   should be set high. */
#undef MEMP_NUM_PBUF
#define MEMP_NUM_PBUF           10
/* MEMP_NUM_UDP_PCB: the number of UDP protocol control blocks. One
   per active UDP "connection". */
#undef MEMP_NUM_UDP_PCB
#define MEMP_NUM_UDP_PCB        6
/* MEMP_NUM_TCP_PCB: the number of simulatenously active TCP
   connections. */
#undef MEMP_NUM_TCP_PCB
#define MEMP_NUM_TCP_PCB        12
/* MEMP_NUM_TCP_PCB_LISTEN: the number of listening TCP
   connections. */
#undef MEMP_NUM_TCP_PCB_LISTEN
#define MEMP_NUM_TCP_PCB_LISTEN 6
/* MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP
   segments. */
#undef MEMP_NUM_TCP_SEG
#define MEMP_NUM_TCP_SEG        32
/* MEMP_NUM_SYS_TIMEOUT: the number of simulateously active
   timeouts. */

#define MEMP_NUM_SYS_TIMEOUT    10


/* ---------- Pbuf options ---------- */
/* PBUF_POOL_SIZE: the number of buffers in the pbuf pool. */
#undef PBUF_POOL_SIZE
#define PBUF_POOL_SIZE          128

/* PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool. */
#undef PBUF_POOL_BUFSIZE
#define PBUF_POOL_BUFSIZE       1500 /* LWIP_MEM_ALIGN_SIZE(TCP_MSS+40+PBUF_LINK_HLEN) */


/* ---------------------------------
   ---------- ARP options ----------
   ---------------------------------*/
/*etharp.h */
#define ETH_PAD_SIZE				0 

/* ---------- TCP options ---------- */
#define LWIP_TCP                1

#undef TCP_TTL
#define TCP_TTL                 255

/* Controls if TCP should queue segments that arrive out of
   order. Define to 0 if your device is low on memory. */
#undef TCP_QUEUE_OOSEQ
#define TCP_QUEUE_OOSEQ         0

//#define LWIP_WND_SCALE          1
//#define TCP_RCV_SCALE           1

/**
 * TCPIP_MBOX_SIZE: The mailbox size for the tcpip thread messages
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when tcpip_init is called.
 */
#undef TCPIP_MBOX_SIZE
#define TCPIP_MBOX_SIZE					MAX_QUEUE_ENTRIES

/**
 * DEFAULT_TCP_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_TCP. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created.
 */
#undef DEFAULT_TCP_RECVMBOX_SIZE
#define DEFAULT_TCP_RECVMBOX_SIZE       MAX_QUEUE_ENTRIES


/**
 * DEFAULT_ACCEPTMBOX_SIZE: The mailbox size for the incoming connections.
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when the acceptmbox is created.
 */
#undef DEFAULT_ACCEPTMBOX_SIZE
#define DEFAULT_ACCEPTMBOX_SIZE         MAX_QUEUE_ENTRIES

/* TCP Maximum segment size. http://lwip.wikia.com/wiki/Tuning_TCP */
#undef TCP_MSS
#define TCP_MSS                 536	//(1500 - 40)	  /* TCP_MSS = (Ethernet MTU - IP header size - TCP header size) */

/* TCP sender buffer space (bytes). */
#undef TCP_SND_BUF
#define TCP_SND_BUF             (2 * TCP_MSS)

/* TCP sender buffer space (pbufs). This must be at least = 2 * TCP_SND_BUF/TCP_MSS for things to work. */
#undef TCP_SND_QUEUELEN
#define TCP_SND_QUEUELEN        (4 * TCP_SND_BUF)/TCP_MSS

/* TCP receive window. */
#undef TCP_WND
#define TCP_WND                 (4 * TCP_MSS)


/* ---------- ICMP options ---------- */
#define LWIP_ICMP               1


/* ---------- DHCP options ---------- */
/* Define LWIP_DHCP to 1 if you want DHCP configuration of
   interfaces. DHCP is not implemented in lwIP 0.5.1, however, so
   turning this on does currently not work. */
#define LWIP_DHCP				1


/* ---------- UDP options ---------- */
#define LWIP_UDP                1
#undef UDP_TTL
#define UDP_TTL                 255
#undef DEFAULT_UDP_RECVMBOX_SIZE
#define DEFAULT_UDP_RECVMBOX_SIZE       MAX_QUEUE_ENTRIES

/* -----------RAW options -----------*/
#undef DEFAULT_RAW_RECVMBOX_SIZE
#define DEFAULT_RAW_RECVMBOX_SIZE       MAX_QUEUE_ENTRIES
#define DEFAULT_ACCEPTMBOX_SIZE         MAX_QUEUE_ENTRIES

/* ---------- Statistics options ---------- */
#undef LWIP_STATS
#define LWIP_STATS				0
#define LWIP_PROVIDE_ERRNO		1


/*
   --------------------------------------
   ---------- Checksum options ----------
   --------------------------------------
*/

/* 
The STM32F107 allows computing and verifying the IP, UDP, TCP and ICMP checksums by hardware:
 - To use this feature let the following define uncommented.
 - To disable it and process by CPU comment the  the checksum.
*/
//#define CHECKSUM_BY_HARDWARE 


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


/*
   ----------------------------------------------
   ---------- Sequential layer options ----------
   ----------------------------------------------
*/
/**
 * LWIP_NETCONN==1: Enable Netconn API (require to use api_lib.c)
 */
#define LWIP_NETCONN                    0
#define LWIP_NETCONN_FULLDUPLEX         0

/*
   ------------------------------------
   ---------- Socket options ----------
   ------------------------------------
*/
/**
 * LWIP_SOCKET==1: Enable Socket API (require to use sockets.c)
 */
#define LWIP_SOCKET                     0
#define LWIP_COMPAT_MUTEX               0


/*##############################################################################################*/
#define LWIP_DEBUG						1

#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_ALL //LWIP_DBG_LEVEL_WARNING	//LWIP_DBG_LEVEL_ALL
#define LWIP_DBG_TYPES_ON               LWIP_DBG_ON

#define ETHARP_DEBUG                    LWIP_DBG_OFF
#define NETIF_DEBUG                     LWIP_DBG_OFF
#define PBUF_DEBUG                      LWIP_DBG_OFF
#define API_LIB_DEBUG                   LWIP_DBG_OFF
#define API_MSG_DEBUG                   LWIP_DBG_OFF
#define SOCKETS_DEBUG                   LWIP_DBG_OFF
#define ACD_DEBUG						LWIP_DBG_OFF
#define ICMP_DEBUG                      LWIP_DBG_OFF
#define IGMP_DEBUG                      LWIP_DBG_OFF
#define INET_DEBUG                      LWIP_DBG_OFF
#define IP_DEBUG                        LWIP_DBG_OFF
#define IP_REASS_DEBUG                  LWIP_DBG_OFF
#define RAW_DEBUG                       LWIP_DBG_OFF
#define MEM_DEBUG                       LWIP_DBG_OFF
#define MEMP_DEBUG                      LWIP_DBG_OFF
#define SYS_DEBUG                       LWIP_DBG_OFF
#define TIMERS_DEBUG                    LWIP_DBG_ON
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
#define TCPIP_DEBUG                     LWIP_DBG_OFF
#define SLIP_DEBUG                      LWIP_DBG_OFF
#define DHCP_DEBUG                      LWIP_DBG_ON
#define AUTOIP_DEBUG                    LWIP_DBG_OFF
#define DNS_DEBUG                       LWIP_DBG_OFF

#define LWIP_IPV4 						1
#define LWIP_IPV6						0
#define LWIP_UDP						1
#define LWIP_TCP						1
#define LWIP_DNS                        0
#define LWIP_RAW						1
#define LWIP_AUTOIP						0
#define LWIP_IGMP						1

#define LWIP_TCP_SACK_OUT               0

#define TCP_LISTEN_BACKLOG              4

#define LWIP_SINGLE_NETIF				1

#define LWIP_NETBIOS_RESPOND_NAME_QUERY	1

#define LWIP_RAND() 					((u32_t)rand())

#define LWIP_CHECKSUM_ON_COPY           1
#define LWIP_TCPIP_TIMEOUT              1

#define LWIP_ALTCP						0
#define LWIP_ALTCP_TLS					0
#define LWIP_ALTCP_TLS_MBEDTLS          0
#define HTTPD_ENABLE_HTTPS				0

#define ALTCP_MBEDTLS_DEBUG                           LWIP_DBG_ON
//#define ALTCP_MBEDTLS_SESSION_CACHE_TIMEOUT_SECONDS   (3*60)

#define LWIP_HTTPD_SUPPORT_11_KEEPALIVE		1
#define LWIP_HTTPD_CGI_SSI					1
#define LWIP_HTTPD_CGI						1
#define LWIP_HTTPD_SSI						1

#define LWIP_HTTPD_MAX_TAG_NAME_LEN 		16
#define LWIP_HTTPD_SSI_RAW					0
#define LWIP_HTTPD_FILE_STATE         		0
#define LWIP_HTTPD_SSI_INCLUDE_TAG          0

//#define LWIP_HTTPD_DYNAMIC_FILE_READ  		0
//#define LWIP_HTTPD_FILE_STATE         		0
//#define LWIP_HTTPD_FS_ASYNC_READ			0

#define LWIP_HTTPD_SUPPORT_POST   			1
#define HTTPD_DEBUG         				LWIP_DBG_OFF
#define HTTPD_DEBUG_POST       				LWIP_DBG_OFF
#define REST_DEBUG         					LWIP_DBG_ON

#define HTTPD_USE_MEM_POOL  				1
#define MEMP_NUM_PARALLEL_HTTPD_CONNS		8
#define MEMP_NUM_PARALLEL_HTTPD_SSI_CONNS 	8

#define LWIP_HTTPD_POST_MANUAL_WND			1
#define HTTPD_MAX_RETRIES                   4
#define LWIP_HTTPD_DYNAMIC_HEADERS 			1

#define LWIP_SO_SNDTIMEO					1
#define LWIP_SO_RCVTIMEO					1
#define LWIP_SO_RCVBUF						1

#define HTTPD_SERVER_AGENT 					"Straight httpd/1.1"

#define PACK_STRUCT_USE_INCLUDES			1

/*##############################################################################################*/

void LwipLogPrint(char* format, ... );

#endif /* __LWIPOPTS_H__ */

