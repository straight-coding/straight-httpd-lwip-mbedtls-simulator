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
void InitDevInfo(void);

struct packet_wrapper* DMA_pop();
void DMA_free(struct packet_wrapper* pkt);

unsigned char* NIC_GetBuffer(int size);
long NIC_Send(unsigned char *buf, int size);
void NotifyFromEthISR();

///////////////////////////////////////////////////////////////////////////////////////////
// Network
///////////////////////////////////////////////////////////////////////////////////////////

unsigned char* GetMyMAC(void);
int FillMAC(void* context, char* buffer, int maxSize);

unsigned long GetMyIP(void);
int FillIP(void* context, char* buffer, int maxSize);
void SetMyIP(unsigned long addr);

unsigned long GetGateway(void);
int FillGateway(void* context, char* buffer, int maxSize);
void SetGateway(unsigned long addr);

unsigned long GetSubnet(void);
int FillSubnet(void* context, char* buffer, int maxSize);
void SetSubnet(unsigned long addr);

unsigned long IsDhcpEnabled();
int FillDhcp(void* context, char* buffer, int maxSize);
void SetDhcpEnabled(char* value);

void OnDhcpFinished(void);

long GetSessionTimeout();
int FillSessionTimeout(void* context, char* buffer, int maxSize);
void SetSessionTimeout(char* value);

///////////////////////////////////////////////////////////////////////////////////////////
// Device
///////////////////////////////////////////////////////////////////////////////////////////

char* GetVendor(void);
char* GetVendorURL(void);

char* GetModel(void);
char* GetModelURL(void);

char* GetDeviceName(void);
char* GetDeviceSN(void);
char* GetDeviceUUID(void);
char* GetDeviceVersion(void);

char* GetLocation();
void SetLocation(char* value);

char* GetColor();
void SetColor(char* value);

char* GetDate();
void SetDate(char* value);

char* GetFont();
void SetFont(char* value);

int FillLog(void* context, char* buffer, int maxSize);
void SetLog(char* value);

#endif

