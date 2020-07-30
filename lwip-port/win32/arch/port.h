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

typedef struct _DeviceConfig
{
	unsigned long nDhcpEnabled;// = 1;// 0 or 1;
	unsigned long dwIP;// = 0;		// 0xC0A80563;
	unsigned long dwSubnet;// = 0;	// 0xFFFFFF00;
	unsigned long dwGateway;// = 0;	// 0xC0A80501;

	long nLog;// = 0;
	long nSessionTimeout;// = 3 * 60 * 1000;

	char szColor[16];// = { 0 };
	char szDate[16];// = { 0 };
	char szFont[16];// = { 0 };
	char szLocation[16];// = { 0 };
}DeviceConfig;

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

void LoadConfig4Edit();
void AppyConfig();

unsigned char* GetMyMAC(void);
int FillMAC(void* context, char* buffer, int maxSize);

unsigned long GetMyIP(void);
int FillIP(void* context, char* buffer, int maxSize);
void SetMyIPLong(unsigned long addr);
void SetMyIP(char* addr);

unsigned long GetGateway(void);
int FillGateway(void* context, char* buffer, int maxSize);
void SetGatewayLong(unsigned long addr);
void SetGateway(char* addr);

unsigned long GetSubnet(void);
int FillSubnet(void* context, char* buffer, int maxSize);
void SetSubnetLong(unsigned long addr);
void SetSubnet(char* addr);

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

