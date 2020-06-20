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
//get a null terminated string from global memory or flash
#define TAG_TYPE_GETTER			0x1000 //char* (*_tagGetter)(void); 
//fill info to the provided buffer
#define TAG_TYPE_PRODUCER		0x2000 //int (*_tagProducer)(REQUEST_CONTEXT* context, char* buffer, int maxSize); 
#define TAG_TYPE_BYTE			0x3000
#define TAG_TYPE_WORD			0x4000
#define TAG_TYPE_FLOAT			0x5000
#define TAG_TYPE_DWORD			0x6000
#define TAG_TYPE_DOUBLE			0x7000
#define TAG_TYPE_STRING			0x8000

typedef struct
{
	char* _tagName; //in shtml files, tag format: <!--#TAG_NAME-->
	int   _tagType;
	void* _tagHandler;
}SSI_Tag;

char g_szColor[16] = { 0 };
char g_szDate[16]  = { 0 };
char g_szTime[16]  = { 0 };

static SSI_Tag g_tagsBuiltin[] = {
	{ "DEV_VENDOR",		TAG_TYPE_GETTER, GetVendor },
	{ "DEV_VENDOR_URL",	TAG_TYPE_GETTER, GetVendorURL },
	  
	{ "DEV_MODEL",		TAG_TYPE_GETTER, GetModel },
	{ "DEV_MODEL_URL",	TAG_TYPE_GETTER, GetModelURL },
	  
	{ "DEV_MAC",		TAG_TYPE_PRODUCER, FillMAC },
	{ "DEV_IP",			TAG_TYPE_PRODUCER, FillIP },
	{ "DEV_GATEWAY",	TAG_TYPE_PRODUCER, FillGateway },
	{ "DEV_SUBNET",		TAG_TYPE_PRODUCER, FillSubnet },
	  
	{ "DEV_NAME",		TAG_TYPE_GETTER, GetDeviceName },
	{ "DEV_SN",			TAG_TYPE_GETTER, GetDeviceSN },
	{ "DEV_UUID",		TAG_TYPE_GETTER, GetDeviceUUID },
	{ "DEV_VERSION",	TAG_TYPE_GETTER, GetDeviceVersion },

	{ "VAR_COLOR",		TAG_TYPE_STRING + sizeof(g_szColor), g_szColor},
	{ "VAR_DATE",		TAG_TYPE_STRING + sizeof(g_szDate), g_szDate},
	{ "VAR_TIME",		TAG_TYPE_STRING + sizeof(g_szTime), g_szTime},

	{NULL, NULL, NULL}
};

void TAG_Setter(char* name, char* value)
{
	int i = 0;

	LogPrint(LOG_DEBUG_ONLY, "  Tag: %s=%s", name, value);

	while (g_tagsBuiltin[i]._tagName != NULL)
	{
		if (strcmp(g_tagsBuiltin[i]._tagName, name) == 0)
		{
			int type = (g_tagsBuiltin[i]._tagType & 0xF000);
			int size = (g_tagsBuiltin[i]._tagType & 0x0FFF);
			if (type == TAG_TYPE_STRING)
			{
				strncpy(g_tagsBuiltin[i]._tagHandler, value, size - 1);
				return;
			}
		}
		i++;
	}
}

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
			int type = (g_tagsBuiltin[i]._tagType & 0xF000);
			int size = (g_tagsBuiltin[i]._tagType & 0x0FFF);
			if (size > sizeof(szTemp))
				size = sizeof(szTemp);

			if (type == TAG_TYPE_GETTER)
			{
				char* (*getter)(void) = g_tagsBuiltin[i]._tagHandler;

				strncpy(szTemp, getter(), sizeof(szTemp) - 1);
				break;
			}
			else if (type == TAG_TYPE_PRODUCER)
			{
				int (*producer)(REQUEST_CONTEXT* context, char* buffer, int maxSize) = g_tagsBuiltin[i]._tagHandler;
				producer(context, szTemp, sizeof(szTemp) - 1);
				break;
			}
			else if (type == TAG_TYPE_STRING)
			{
				strncpy(szTemp, g_tagsBuiltin[i]._tagHandler, size - 1);
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
