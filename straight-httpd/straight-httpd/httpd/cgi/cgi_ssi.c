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
#define TAG_GETTER			0x1000 //char* (*_tagGetter)(void); 
//fill info to the provided buffer
#define TAG_PROVIDER		0x2000 //int (*_tagProvider)(REQUEST_CONTEXT* context, char* buffer, int maxSize); 
#define TAG_BYTE			0x3000
#define TAG_WORD			0x4000
#define TAG_FLOAT			0x5000
#define TAG_DWORD			0x6000
#define TAG_DOUBLE			0x7000
#define TAG_STRING			0x8000

#define FORMATER_FLOAT		"%.3f"
#define FORMATER_DOUBLE		"%.6f"

typedef struct
{
	char* _tagName; //in shtml files, tag format: <!--#TAG_NAME-->
	int   _tagType;
	void* _tagHandler;
}SSI_Tag;

char g_szColor[16] = { 0 };
char g_szDate[16]  = { 0 };
char g_szFont[16] = { 0 };
char g_szLocation[16] = { 0 };
long g_nLog = 0;

static SSI_Tag g_tagsBuiltin[] = {
	{ "DEV_VENDOR",		TAG_GETTER, GetVendor }, //for home use
	{ "DEV_VENDOR_URL",	TAG_GETTER, GetVendorURL },
	  
	{ "DEV_MODEL",		TAG_GETTER, GetModel },
	{ "DEV_MODEL_URL",	TAG_GETTER, GetModelURL },
	  
	{ "DEV_MAC",		TAG_PROVIDER, FillMAC },
	{ "DEV_IP",			TAG_PROVIDER, FillIP },
	{ "DEV_GATEWAY",	TAG_PROVIDER, FillGateway },
	{ "DEV_SUBNET",		TAG_PROVIDER, FillSubnet },
	  
	{ "DEV_NAME",		TAG_GETTER, GetDeviceName },
	{ "DEV_SN",			TAG_GETTER, GetDeviceSN },
	{ "DEV_UUID",		TAG_GETTER, GetDeviceUUID },
	{ "DEV_VERSION",	TAG_GETTER, GetDeviceVersion },

	{ "VAR_LOCATION",	TAG_STRING + sizeof(g_szLocation), g_szLocation}, //for form use
	{ "VAR_COLOR",		TAG_STRING + sizeof(g_szColor), g_szColor},
	{ "VAR_DATE",		TAG_STRING + sizeof(g_szDate),  g_szDate},
	{ "VAR_LOG",		TAG_DWORD + sizeof(g_nLog),  &g_nLog},
	{ "VAR_FONT",		TAG_STRING + sizeof(g_szFont),  g_szFont},

	{NULL, NULL, NULL}
};

void TAG_Setter(char* name, char* value)
{
	int i = 0;
	int type = 0;
	int size = 0;
	SSI_Tag* tag = NULL;

	LogPrint(LOG_DEBUG_ONLY, "  Tag: %s=%s", name, value);

	while (g_tagsBuiltin[i]._tagName != NULL)
	{
		tag = &g_tagsBuiltin[i++];
		if (strcmp(tag->_tagName, name) != 0)
			continue;

		if (tag->_tagHandler == NULL)
			break;

		type = (tag->_tagType & 0xF000);
		size = (tag->_tagType & 0x0FFF);

		if (type == TAG_STRING)
		{
			strncpy(tag->_tagHandler, value, size - 1);
		}
		else if (type == TAG_BYTE)
		{
			long nValue = ston(value);
			((unsigned char*)tag->_tagHandler)[0] = (unsigned char)nValue;
		}
		else if (type == TAG_WORD)
		{
			long nValue = ston(value);
			((unsigned short*)tag->_tagHandler)[0] = (unsigned short)nValue;
		}
		else if (type == TAG_DWORD)
		{
			long nValue = ston(value);
			((unsigned long*)tag->_tagHandler)[0] = (unsigned long)nValue;
		}
		else if (type == TAG_FLOAT)
		{
			float fValue = atof(value);
			((float*)tag->_tagHandler)[0] = fValue;
		}
		else if (type == TAG_DOUBLE)
		{
			double fValue = atof(value);
			((double*)tag->_tagHandler)[0] = fValue;
		}
		break;
	}
}

int ReplaceTag(REQUEST_CONTEXT* context, char* tagName, char* appendTo, int maxAllowed)
{
	int i = 0;
	int type = 0;
	int size = 0;
	int valueLen = 0;
	SSI_Tag* tag = NULL;

	char szTemp[64];
	memset(szTemp, 0, sizeof(szTemp));

	while (g_tagsBuiltin[i]._tagName != NULL)
	{
		tag = &g_tagsBuiltin[i++];
		if (strcmp(tag->_tagName, tagName) != 0)
			continue;

		if (tag->_tagHandler == NULL)
			break;

		type = (tag->_tagType & 0xF000);
		size = (tag->_tagType & 0x0FFF);
		if (size > sizeof(szTemp)-1)
			size = sizeof(szTemp)-1;

		if (type == TAG_GETTER)
		{
			char* (*getter)(void) = tag->_tagHandler;
			strncpy(szTemp, getter(), sizeof(szTemp) - 1);
		}
		else if (type == TAG_PROVIDER)
		{
			int (*producer)(REQUEST_CONTEXT* context, char* buffer, int maxSize) = tag->_tagHandler;
			producer(context, szTemp, sizeof(szTemp) - 1);
		}
		else if (type == TAG_STRING)
			strncpy(szTemp, tag->_tagHandler, size);
		else if (type == TAG_BYTE)
			LWIP_sprintf(szTemp, "%u", ((unsigned char*)tag->_tagHandler)[0]);
		else if (type == TAG_WORD)
			LWIP_sprintf(szTemp, "%u", ((unsigned short*)tag->_tagHandler)[0]);
		else if (type == TAG_DWORD)
			LWIP_sprintf(szTemp, "%lu", ((unsigned long*)tag->_tagHandler)[0]);
		else if (type == TAG_FLOAT)
			LWIP_sprintf(szTemp, FORMATER_FLOAT, ((float*)tag->_tagHandler)[0]);
		else if (type == TAG_DOUBLE)
			LWIP_sprintf(szTemp, FORMATER_DOUBLE, ((double*)tag->_tagHandler)[0]);

		break;
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
