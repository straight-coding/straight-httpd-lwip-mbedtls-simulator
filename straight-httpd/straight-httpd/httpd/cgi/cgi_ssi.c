/*
  cgi_ssi.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#include "../http_cgi.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

static int FillMAC(REQUEST_CONTEXT* context, char* buffer, int maxSize);
static int FillIP(REQUEST_CONTEXT* context, char* buffer, int maxSize);
static int FillGateway(REQUEST_CONTEXT* context, char* buffer, int maxSize);
static int FillSubnet(REQUEST_CONTEXT* context, char* buffer, int maxSize);

/////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	char* _tagName; //in shtml files, tag format: <!--#TAG_NAME-->
	char* (*_tagGetter)(void); //get a null terminated string from global memory or flash
	int (*_tagProducer)(REQUEST_CONTEXT* context, char* buffer, int maxSize); //fill info to the provided buffer
}SSI_Tag;

static SSI_Tag g_tagsBuiltin[] = {
	{"DEV_VENDOR",		GetVendor, NULL},
	{"DEV_VENDOR_URL",	GetVendorURL, NULL},

	{"DEV_MODEL",		GetModel, NULL},
	{"DEV_MODEL_URL",	GetModelURL, NULL},

	{"DEV_MAC",			NULL, FillMAC},
	{"DEV_IP",			NULL, FillIP},
	{"DEV_GATEWAY",		NULL, FillGateway},
	{"DEV_SUBNET",		NULL, FillSubnet},

	{"DEV_NAME",		GetDeviceName, NULL},
	{"DEV_SN",			GetDeviceSN, NULL},
	{"DEV_UUID",		GetDeviceUUID, NULL},
	{"DEV_VERSION",		GetDeviceVersion, NULL},

	{NULL,	NULL, NULL}
};

int ReplaceTag(REQUEST_CONTEXT* context, char* tagName, char* appendTo, int maxAllowed)
{
	int i = 0;
	int valueLen = 0;

	char szTemp[64];
	memset(szTemp, 0, sizeof(szTemp));

	//check built-in tags first
	while (g_tagsBuiltin[i]._tagName != NULL)
	{
		if (strcmp(g_tagsBuiltin[i]._tagName, tagName) == 0)
		{
			if (g_tagsBuiltin[i]._tagGetter != NULL)
			{
				strncpy(szTemp, g_tagsBuiltin[i]._tagGetter(), sizeof(szTemp) - 1);
				break;
			}
			else if (g_tagsBuiltin[i]._tagProducer != NULL)
			{
				g_tagsBuiltin[i]._tagProducer(context, szTemp, sizeof(szTemp) - 1);
				break;
			}
		}
		i++;
	}

	if (szTemp[0] == 0)
	{ //replace your customized tags here

	}

	if (szTemp[0] != 0)
	{
		LogPrint(LOG_DEBUG_ONLY, "  Tag value: %s", szTemp);
		
		valueLen = strlen(szTemp);
		if (valueLen >= maxAllowed)
			valueLen = maxAllowed;
		strncpy(appendTo, szTemp, valueLen);
	}
	return valueLen;
}

int FillMAC(REQUEST_CONTEXT* context, char* buffer, int maxSize)
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

int FillIP(REQUEST_CONTEXT* context, char* buffer, int maxSize)
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

int FillGateway(REQUEST_CONTEXT* context, char* buffer, int maxSize)
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

int FillSubnet(REQUEST_CONTEXT* context, char* buffer, int maxSize)
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
