/*
  lwip_port.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#include "lwip_port.h"

unsigned char g_byMyMAC[] = { '\x00', '\x00', '\x00', '\x20', '\xAA', '\x74' };

unsigned long g_dwIP = 0;		// 0xC0A80563;
unsigned long g_dwSubnet = 0;	// 0xFFFFFF00;
unsigned long g_dwGateway = 0;	// 0xC0A80501;

const char* g_szVendor = "Straight";
const char* g_szVendorURL = "https://github.com/straight-coding/";

const char* g_szModel = "straight-httpd";
const char* g_szModelURL = "https://github.com/straight-coding/straight-httpd";

const char* g_szDeviceName = "Virtual Device";
const char* g_szDeviceVersion = "v1.0.0";
const char* g_szDeviceSN = "2020-04-10";
const char* g_szDeviceUUID = "A5757C42-0234-4600-8E0F-8551B7EAEB7A";

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
	g_dwIP = addr;
}

void SetGateway(unsigned long addr)
{
	g_dwGateway = addr;
}

void SetSubnet(unsigned long addr)
{
	g_dwSubnet = addr;
}
