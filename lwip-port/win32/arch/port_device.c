/*
  port_device.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: June 20, 2020
*/

#include <stdio.h>
#include <stdlib.h>

#include <lwip/sys.h>

static unsigned char g_byMyMAC[] = { '\x00', '\x00', '\x00', '\x20', '\xAA', '\x74' };

static unsigned long g_nDhcpEnabled = 1;// 0 or 1;
static unsigned long g_dwIP = 0;		// 0xC0A80563;
static unsigned long g_dwSubnet = 0;	// 0xFFFFFF00;
static unsigned long g_dwGateway = 0;	// 0xC0A80501;

static const char* g_szVendor = "Straight";
static const char* g_szVendorURL = "https://github.com/straight-coding/";

static const char* g_szModel = "straight-httpd";
static const char* g_szModelURL = "https://github.com/straight-coding/straight-httpd";

static const char* g_szDeviceName = "Virtual Device";
static const char* g_szDeviceVersion = "v1.0.0";
static const char* g_szDeviceSN = "2020-04-10";
static const char* g_szDeviceUUID = "A5757C42-0234-4600-8E0F-8551B7EAEB7A";

static sys_mutex_t g_devInfoMutex;

static char g_szColor[16] = { 0 };
static char g_szDate[16] = { 0 };
static char g_szFont[16] = { 0 };
static char g_szLocation[16] = { 0 };
static long g_nLog = 0;
static long g_nSessionTimeout = 3 * 60 * 1000;

void InitDevInfo(void)
{
	if (sys_mutex_new(&g_devInfoMutex) != 0) {}
}

static void lockDevInfo(void)
{
	sys_mutex_lock(&g_devInfoMutex);
}

static void unlockDevInfo(void)
{
	sys_mutex_unlock(&g_devInfoMutex);
}

char* GetVendor(void)
{
	return g_szVendor;
}

char* GetVendorURL(void)
{
	return g_szVendorURL;
}

char* GetModel(void)
{
	return g_szModel;
}

char* GetModelURL(void)
{
	return g_szModelURL;
}

char* GetDeviceName(void)
{
	return g_szDeviceName;
}

char* GetDeviceSN(void)
{
	return g_szDeviceSN;
}

char* GetDeviceUUID(void)
{
	return g_szDeviceUUID;
}

char* GetDeviceVersion(void)
{
	return g_szDeviceVersion;
}

unsigned char* GetMyMAC(void)
{
	return (unsigned char*)g_byMyMAC;
}

unsigned long IsDhcpEnabled()
{
	return g_nDhcpEnabled;
}

int FillDhcp(void* context, char* buffer, int maxSize)
{
	int len = 0;
	if (g_nDhcpEnabled > 0)
		LWIP_sprintf(buffer, "%u", 1);
	else
		LWIP_sprintf(buffer, "%u", 0);

	len = strlen(buffer);
	return (len < maxSize) ? len : maxSize;
}

void SetDhcpEnabled(char* value)
{
	g_nDhcpEnabled = (ston(value) != 0) ? 1 : 0;
}

unsigned long GetMyIP(void)
{
	return g_dwIP;
}

unsigned long GetGateway(void)
{
	return g_dwGateway;
}

unsigned long GetSubnet(void)
{
	return g_dwSubnet;
}

void SetMyIP(unsigned long addr)
{
	lockDevInfo();
		g_dwIP = addr;
	unlockDevInfo();
}

void SetGateway(unsigned long addr)
{
	lockDevInfo();
		g_dwGateway = addr;
	unlockDevInfo();
}

void SetSubnet(unsigned long addr)
{
	lockDevInfo();
		g_dwSubnet = addr;
	unlockDevInfo();
}

int FillMAC(void* context, char* buffer, int maxSize)
{
	int len = 0;
	char szTemp[32];
	unsigned char* mac = GetMyMAC();

	LWIP_sprintf(szTemp, "%02X:%02X:%02X:%02X:%02X:%02X",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	strncpy(buffer, szTemp, maxSize);
	len = strlen(buffer);

	return (len < maxSize) ? len : maxSize;
}

int FillIP(void* context, char* buffer, int maxSize)
{
	int len = 0;
	char szTemp[32];
	long ip = GetMyIP();

	LWIP_sprintf(szTemp, "%d.%d.%d.%d",
		(int)((ip & 0xFF000000) >> 24),
		(int)((ip & 0x00FF0000) >> 16),
		(int)((ip & 0x0000FF00) >> 8),
		(int)((ip & 0x000000FF) >> 0));

	strncpy(buffer, szTemp, maxSize);
	len = strlen(buffer);

	return (len < maxSize) ? len : maxSize;
}

int FillGateway(void* context, char* buffer, int maxSize)
{
	int len = 0;
	char szTemp[32];
	long gw = GetGateway();

	LWIP_sprintf(szTemp, "%d.%d.%d.%d",
		(int)((gw & 0xFF000000) >> 24),
		(int)((gw & 0x00FF0000) >> 16),
		(int)((gw & 0x0000FF00) >> 8),
		(int)((gw & 0x000000FF) >> 0));

	strncpy(buffer, szTemp, maxSize);
	len = strlen(buffer);

	return (len < maxSize) ? len : maxSize;
}

int FillSubnet(void* context, char* buffer, int maxSize)
{
	int len = 0;
	char szTemp[32];
	long mask = GetSubnet();

	LWIP_sprintf(szTemp, "%d.%d.%d.%d",
		(int)((mask & 0xFF000000) >> 24),
		(int)((mask & 0x00FF0000) >> 16),
		(int)((mask & 0x0000FF00) >> 8),
		(int)((mask & 0x000000FF) >> 0));

	strncpy(buffer, szTemp, maxSize);
	len = strlen(buffer);

	return (len < maxSize) ? len : maxSize;
}

char* GetLocation()
{
	return g_szLocation;
}

char* GetColor()
{
	return g_szColor;
}

char* GetDate()
{
	return g_szDate;
}

char* GetFont()
{
	return g_szFont;
}

int FillLog(void* context, char* buffer, int maxSize)
{
	int len = 0;

	if (g_nLog > 0)
		LWIP_sprintf(buffer, "%u", 1);
	else
		LWIP_sprintf(buffer, "%u", 0);

	len = strlen(buffer);
	return (len < maxSize) ? len : maxSize;
}

void SetLocation(char* value)
{
	memset(g_szLocation, 0, sizeof(g_szLocation));
	strncpy(g_szLocation, value, sizeof(g_szLocation) - 1);
}

void SetColor(char* value)
{
	memset(g_szColor, 0, sizeof(g_szColor));
	strncpy(g_szColor, value, sizeof(g_szColor) - 1);
}

void SetDate(char* value)
{
	memset(g_szDate, 0, sizeof(g_szDate));
	strncpy(g_szDate, value, sizeof(g_szDate) - 1);
}

void SetFont(char* value)
{
	memset(g_szFont, 0, sizeof(g_szFont));
	strncpy(g_szFont, value, sizeof(g_szFont)-1);
}

void SetLog(char* value)
{
	g_nLog = ston(value);
}

long GetSessionTimeout()
{
	return g_nSessionTimeout;
}

int FillSessionTimeout(void* context, char* buffer, int maxSize)
{
	int len = 0;

	LWIP_sprintf(buffer, "%u", g_nSessionTimeout/(60*1000));

	len = strlen(buffer);
	return (len < maxSize) ? len : maxSize;
}

void SetSessionTimeout(char* value)
{
	g_nSessionTimeout = 60*1000*ston(value);
}
