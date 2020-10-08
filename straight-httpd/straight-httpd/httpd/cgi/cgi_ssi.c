/*
  cgi_ssi.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#include "../http_cgi.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////
//get a null terminated string from global memory or flash
#define TAG_GETTER			0x1000 //char* (*_tagGetter)(void); 
//convert and fill info to the provided buffer
#define TAG_PROVIDER		0x2000 //int (*_tagProvider)(REQUEST_CONTEXT* context, char* buffer, int maxSize); 
#define TAG_BYTE			0x3000
#define TAG_WORD			0x4000
#define TAG_FLOAT			0x5000
#define TAG_DWORD			0x6000
#define TAG_DOUBLE			0x7000
#define TAG_STRING			0x8000
#define TAG_SETTER			0x9000 //void (*_tagSetter)(char* value);

#define FORMATER_FLOAT		"%.3f"
#define FORMATER_DOUBLE		"%.6f"

typedef struct
{
	char* _tagName; //in shtml files, tag format: <!--#TAG_NAME-->
	int   _tagType;
	void* _tagHandler;
}SSI_Tag;

//////////////////////////////////////////////////////////////////////////////////

static SSI_Tag g_Getters[] = {
	{ "DEV_VENDOR",		TAG_GETTER, GetVendor }, //for home use
	{ "DEV_VENDOR_URL",	TAG_GETTER, GetVendorURL },
	  
	{ "DEV_MODEL",		TAG_GETTER, GetModel },
	{ "DEV_MODEL_URL",	TAG_GETTER, GetModelURL },
	  
	{ "DEV_DHCP",		TAG_PROVIDER, FillDhcp },
	{ "DEV_MAC",		TAG_PROVIDER, FillMAC },
	{ "DEV_IP",			TAG_PROVIDER, FillIP },
	{ "DEV_GATEWAY",	TAG_PROVIDER, FillGateway },
	{ "DEV_SUBNET",		TAG_PROVIDER, FillSubnet },

	{ "VAR_SESSION_TIMEOUT", TAG_PROVIDER, FillSessionTimeout },

	{ "DEV_NAME",		TAG_GETTER, GetDeviceName },
	{ "DEV_SN",			TAG_GETTER, GetDeviceSN },
	{ "DEV_UUID",		TAG_GETTER, GetDeviceUUID },
	{ "DEV_VERSION",	TAG_GETTER, GetDeviceVersion },

	{ "VAR_LOCATION",	TAG_GETTER, GetLocation },
	{ "VAR_COLOR",		TAG_GETTER, GetColor },
	{ "VAR_DATE",		TAG_GETTER, GetDate },
	{ "VAR_FONT",		TAG_GETTER, GetFont },
	{ "VAR_LOG",		TAG_PROVIDER, FillLog },

	{NULL, 0, NULL}
};

static SSI_Tag g_Setters[] = {
	{ "DEV_DHCP",		TAG_SETTER, SetDhcpEnabled },
	{ "VAR_SESSION_TIMEOUT", TAG_SETTER, SetSessionTimeout },
	{ "DEV_IP",			TAG_SETTER, SetMyIP },
	{ "DEV_GATEWAY",	TAG_SETTER, SetGateway },
	{ "DEV_SUBNET",		TAG_SETTER, SetSubnet },
	{ "VAR_LOCATION",	TAG_SETTER, SetLocation },
	{ "VAR_COLOR",		TAG_SETTER, SetColor },
	{ "VAR_DATE",		TAG_SETTER, SetDate },
	{ "VAR_FONT",		TAG_SETTER, SetFont },
	{ "VAR_LOG",		TAG_SETTER, SetLog },
	{NULL, 0, NULL}
};

void TAG_Setter(char* name, char* value)
{
	int i = 0;
	int type = 0;
	int size = 0;
	SSI_Tag* tag = NULL;

	LogPrint(LOG_DEBUG_ONLY, "  Tag: %s=%s", name, value);

	while (g_Setters[i]._tagName != NULL)
	{
		tag = &g_Setters[i++];
		if (strcmp(tag->_tagName, name) != 0)
			continue;

		if (tag->_tagHandler == NULL)
			break;

		type = (tag->_tagType & 0xF000);
		size = (tag->_tagType & 0x0FFF);

		if (type == TAG_SETTER)
		{
			void (*setter)(char*) = tag->_tagHandler;
			setter(value);
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

	while (g_Getters[i]._tagName != NULL)
	{
		tag = &g_Getters[i++];
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
