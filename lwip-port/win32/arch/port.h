/*
  port.h
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#ifndef _PORT_H_
#define _PORT_H_

///////////////////////////////////////////////////////////////////////////////////////////
// Network
///////////////////////////////////////////////////////////////////////////////////////////

#define MAX_DMA_QUEUE	1024
struct packet_wrapper
{
	long len;
	unsigned char* packet;	/* allocated */
	void* next;				/* next packet */
};

void LwipInit(int reboot);
extern int tcpip_inloop(void);

struct packet_wrapper* DMA_pop();
void DMA_free(struct packet_wrapper* pkt);

unsigned char* NIC_GetBuffer(int size);
long NIC_Send(unsigned char *buf, int size);
void NotifyFromEthISR();

///////////////////////////////////////////////////////////////////////////////////////////
// Device
///////////////////////////////////////////////////////////////////////////////////////////

unsigned char* GetMyMAC(void);
unsigned long GetMyIP(void);
unsigned long GetGateway(void);
unsigned long GetSubnet(void);

void  SetMyIP(unsigned long addr);
void  SetGateway(unsigned long addr);
void  SetSubnet(unsigned long addr);
	
char* GetVendor(void);
char* GetVendorURL(void);

char* GetModel(void);
char* GetModelURL(void);

char* GetDeviceName(void);
char* GetDeviceSN(void);
char* GetDeviceUUID(void);
char* GetDeviceVersion(void);

void  OnDhcpFinished(void);

#endif

