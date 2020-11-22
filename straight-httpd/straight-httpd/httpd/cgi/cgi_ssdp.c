/*
  cgi_ssdp.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#include "../http_cgi.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

extern void LogPrint(int level, char* format, ... );
extern unsigned long GetMyIP(void);

/////////////////////////////////////////////////////////////////////////////////////////////////////

static void SSDP_SendHeader(REQUEST_CONTEXT* context, char* HttpCodeInfo);
static int  SSDP_Sending(REQUEST_CONTEXT* context, char* buffer, int maxSize);

struct CGI_Mapping g_cgiSSDP = {
	"/upnp_device.xml", //char* path;
	CGI_OPT_GET_ENABLED | CGI_OPT_CHUNK_ENABLED,// unsigned long options;

	NULL, //void (*OnCancel)(REQUEST_CONTEXT* context);
	NULL, //int (*OnHeaderReceived)(REQUEST_CONTEXT* context, char* header_line);
	NULL, //void (*OnHeadersReceived)(REQUEST_CONTEXT* context);
	NULL, //int  (*OnContentReceived)(REQUEST_CONTEXT* context, char* buffer, int size);
	NULL, //void (*OnRequestReceived)(REQUEST_CONTEXT* context);
	
	SSDP_SendHeader, //void (*SetResponseHeader)(REQUEST_CONTEXT* context, char* HttpCode);
	SSDP_Sending, //int  (*ReadContent)(REQUEST_CONTEXT* context, char* buffer, int maxSize);
	
	NULL, //int  (*OnAllSent)(REQUEST_CONTEXT* context);
	NULL, //void (*OnFinished)(REQUEST_CONTEXT* context);
	
	NULL //struct CGI_Mapping* next;
};

static void SSDP_SendHeader(REQUEST_CONTEXT* context, char* HttpCodeInfo)
{
	if (context->_requestMethod == METHOD_GET)
	{
		LWIP_sprintf(context->ctxResponse._sendBuffer, (char*)header_chunked, "200 OK", "text/xml; charset=\"utf-8\"", header_nocache, CONNECTION_HEADER);
	}
}

static int SSDP_Sending(REQUEST_CONTEXT* context, char* buffer, int maxSize)
{
	int off = 0;
	int available = 128;
	if (context->_requestMethod == METHOD_GET)
	{
		char szIP[64];
		unsigned long ip = GetMyIP();
		memset(szIP, 0, sizeof(szIP));
		LWIP_sprintf(szIP, "%d.%d.%d.%d",
			(int)((ip & 0xFF000000) >> 24), 
			(int)((ip & 0x00FF0000) >> 16), 
			(int)((ip & 0x0000FF00) >> 8), 
			(int)((ip & 0x000000FF) >> 0));

		buffer[0] = 0;
		
		if (context->ctxResponse._dwOperStage == STAGE_END)
			return -1; //already done
		
		if (context->ctxResponse._dwOperStage == 0)
		{
			if (off > maxSize-available)
				goto block_end;
			strcat(buffer, "<?xml version=\"1.0\"?>\r\n");
				off = strlen(buffer);
			context->ctxResponse._dwOperStage++;
		}
		if (context->ctxResponse._dwOperStage == 1)
		{
			if (off > maxSize-available)
				goto block_end;
			strcat(buffer, "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">\r\n");
				off = strlen(buffer);
			context->ctxResponse._dwOperStage++;
		}
		if (context->ctxResponse._dwOperStage == 2)
		{
			if (off > maxSize-available)
				goto block_end;
			strcat(buffer, "<specVersion><major>1</major><minor>0</minor></specVersion>\r\n");
				off = strlen(buffer);
			context->ctxResponse._dwOperStage++;
		}
		if (context->ctxResponse._dwOperStage == 3)
		{
			if (off > maxSize-available)
				goto block_end;
			LWIP_sprintf(buffer +off, "<URLBase>https://%s</URLBase>\r\n", szIP);
				off = strlen(buffer);
			context->ctxResponse._dwOperStage++;
		}
		if (context->ctxResponse._dwOperStage == 4)
		{
			if (off > maxSize-available)
				goto block_end;
			strcat(buffer, "<device>\r\n");
				off = strlen(buffer);
			context->ctxResponse._dwOperStage++;
		}
		if (context->ctxResponse._dwOperStage == 5)
		{
			if (off > maxSize-available)
				goto block_end;
			LWIP_sprintf(buffer + off, "<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>\r\n", "");
				off = strlen(buffer);
			context->ctxResponse._dwOperStage++;
		}
		if (context->ctxResponse._dwOperStage == 6)
		{
			if (off > maxSize-available)
				goto block_end;
			LWIP_sprintf(buffer +off, "<friendlyName>%s</friendlyName>\r\n", GetDeviceName());
				off = strlen(buffer);
			context->ctxResponse._dwOperStage++;
		}
		if (context->ctxResponse._dwOperStage == 7)
		{
			if (off > maxSize-available)
				goto block_end;
			LWIP_sprintf(buffer +off, "<presentationURL>https://%s</presentationURL>\r\n", szIP);
				off = strlen(buffer);
			context->ctxResponse._dwOperStage++;
		}
		if (context->ctxResponse._dwOperStage == 8)
		{
			if (off > maxSize-available)
				goto block_end;
			LWIP_sprintf(buffer +off, "<manufacturer>%s</manufacturer>\r\n", GetVendor());
				off = strlen(buffer);
			context->ctxResponse._dwOperStage++;
		}
		if (context->ctxResponse._dwOperStage == 9)
		{
			if (off > maxSize-available)
				goto block_end;
			LWIP_sprintf(buffer +off, "<manufacturerURL>%s</manufacturerURL>\r\n", GetVendorURL());
				off = strlen(buffer);
			context->ctxResponse._dwOperStage++;
		}
		if (context->ctxResponse._dwOperStage == 10)
		{
			if (off > maxSize-available)
				goto block_end;
			LWIP_sprintf(buffer +off, "<modelDescription>%s</modelDescription>\r\n", GetModel());
				off = strlen(buffer);
			context->ctxResponse._dwOperStage++;
		}
		if (context->ctxResponse._dwOperStage == 11)
		{
			if (off > maxSize-available)
				goto block_end;
			LWIP_sprintf(buffer +off, "<modelName>%s</modelName>\r\n", GetModel());
				off = strlen(buffer);
			context->ctxResponse._dwOperStage++;
		}
		if (context->ctxResponse._dwOperStage == 12)
		{
			if (off > maxSize-available)
				goto block_end;
			LWIP_sprintf(buffer +off, "<modelNumber>%s (%s)</modelNumber>\r\n", GetDeviceName(), GetDeviceVersion()); //MODEL_NUMBER
				off = strlen(buffer);
			context->ctxResponse._dwOperStage++;
		}
		if (context->ctxResponse._dwOperStage == 13)
		{
			if (off > maxSize-available)
				goto block_end;
			LWIP_sprintf(buffer +off, "<modelURL>%s</modelURL>\r\n", GetModelURL());
				off = strlen(buffer);
			context->ctxResponse._dwOperStage++;
		}
		if (context->ctxResponse._dwOperStage == 14)
		{
			if (off > maxSize-available)
				goto block_end;
			LWIP_sprintf(buffer +off, "<serialNumber>%s</serialNumber>\r\n", GetDeviceSN());
				off = strlen(buffer);
			context->ctxResponse._dwOperStage++;
		}
		if (context->ctxResponse._dwOperStage == 15)
		{
			if (off > maxSize-available)
				goto block_end;
			LWIP_sprintf(buffer +off, "<UDN>uuid:%s</UDN>\r\n", GetDeviceUUID());
				off = strlen(buffer);
			context->ctxResponse._dwOperStage++;
		}
		if (context->ctxResponse._dwOperStage == 16)
		{
			if (off > maxSize-available)
				goto block_end;
			strcat(buffer, "</device>\r\n</root>\r\n");
				off = strlen(buffer);
			context->ctxResponse._dwOperStage ++;
		}

		//context->ctxResponse._dwTotal = off;
		//context->ctxResponse._dwOperIndex = context->ctxResponse._dwTotal;
		context->ctxResponse._dwOperStage = STAGE_END;
	}
block_end:
	LogPrint(LOG_DEBUG_ONLY, "CGI_SSDP_PROFILE response sending: %d @%d", off, context->_sid);
	return off;
}
