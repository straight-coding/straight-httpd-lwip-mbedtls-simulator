/*
  cgi_ssdp.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#include "../http_cgi.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

extern void LogPrint(int level, char* format, ... );

/////////////////////////////////////////////////////////////////////////////////////////////////////

void SSDP_SendHeader(REQUEST_CONTEXT* context, char* HttpCodeInfo);
int  SSDP_Sending(REQUEST_CONTEXT* context);
void SSDP_OnAllSent(REQUEST_CONTEXT* context);

struct CGI_Mapping g_cgiSSDP = {
	"/upnp_device.xml", //char* path;
	CGI_OPT_GET_ENABLED,// unsigned long options;

	NULL, //void (*OnCancel)(REQUEST_CONTEXT* context);
	NULL, //void (*OnHeaderReceived)(REQUEST_CONTEXT* context, char* header_line);
	NULL, //void (*OnHeadersReceived)(REQUEST_CONTEXT* context);
	NULL, //int  (*OnContentReceived)(REQUEST_CONTEXT* context, char* buffer, int size);
	NULL, //void (*OnRequestReceived)(REQUEST_CONTEXT* context);
	
	SSDP_SendHeader, //void (*SetResponseHeader)(REQUEST_CONTEXT* context, char* HttpCode);
	SSDP_Sending, //int  (*LoadContentToSend)(REQUEST_CONTEXT* context);
	
	SSDP_OnAllSent, //int  (*OnAllSent)(REQUEST_CONTEXT* context);
	NULL, //void (*OnFinished)(REQUEST_CONTEXT* context);
	
	NULL //struct CGI_Mapping* next;
};

void SSDP_SendHeader(REQUEST_CONTEXT* context, char* HttpCodeInfo)
{
	if (context->_requestMethod == METHOD_GET)
	{
		LWIP_sprintf(context->ctxResponse._sendBuffer, (char*)response_header_chunked, "200 OK", "text/xml; charset=\"utf-8\"", no_cache, "close");
	}
}

int  SSDP_Sending(REQUEST_CONTEXT* context)
{
	int needSend = 0;
	if (context->_requestMethod == METHOD_GET)
	{
		int size, i;
/*	
		if (context->ctxResponse._dwOperStage >= STAGE_END)
		{
			if (caller == HTTP_PROC_CALLER_SENT)
			{
				LogPrint(LOG_DEBUG_ONLY, "CGI_SSDP_PROFILE response done @%d", context->_sid);
				context->_state = HTTP_STATE_REQUEST_END;
			}
		}
		else*/
		{
			char szSize[64];
			int size2Send;
			int prefixLen;
			
			int len = 0;
			char szIP[64];
			u32_t ip = GetMyIP();
			
			int idx, j;
			int off = 0;
			
			context->ctxResponse._sendBuffer[10] = 0;
			
			strcat(context->ctxResponse._sendBuffer+10, "<?xml version=\"1.0\"?>\r\n");
				off = strlen(context->ctxResponse._sendBuffer+10);
			strcat(context->ctxResponse._sendBuffer+10, "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">\r\n");
				off = strlen(context->ctxResponse._sendBuffer+10);
			strcat(context->ctxResponse._sendBuffer+10, "<specVersion><major>1</major><minor>0</minor></specVersion>\r\n");
				off = strlen(context->ctxResponse._sendBuffer+10);
				
			memset(szIP, 0, sizeof(szIP));
			LWIP_sprintf(szIP, "%d.%d.%d.%d",
				(int)((ip & 0xFF000000) >> 24), 
				(int)((ip & 0x00FF0000) >> 16), 
				(int)((ip & 0x0000FF00) >> 8), 
				(int)((ip & 0x000000FF) >> 0));
			LWIP_sprintf(context->ctxResponse._sendBuffer+10+off, "<URLBase>http://%s</URLBase>\r\n", szIP);
				off = strlen(context->ctxResponse._sendBuffer+10);
			
			strcat(context->ctxResponse._sendBuffer+10, "<device>\r\n");
				off = strlen(context->ctxResponse._sendBuffer+10);

			LWIP_sprintf(context->ctxResponse._sendBuffer+10+off, "<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>\r\n", "");
				off = strlen(context->ctxResponse._sendBuffer+10);
			LWIP_sprintf(context->ctxResponse._sendBuffer+10+off, "<friendlyName>%s</friendlyName>\r\n", GetDeviceName());
				off = strlen(context->ctxResponse._sendBuffer+10);
			LWIP_sprintf(context->ctxResponse._sendBuffer+10+off, "<presentationURL>http://%s</presentationURL>\r\n", szIP);
				off = strlen(context->ctxResponse._sendBuffer+10);
			LWIP_sprintf(context->ctxResponse._sendBuffer+10+off, "<manufacturer>%s</manufacturer>\r\n", GetVendor());
				off = strlen(context->ctxResponse._sendBuffer+10);
			LWIP_sprintf(context->ctxResponse._sendBuffer+10+off, "<manufacturerURL>%s</manufacturerURL>\r\n", GetVendorURL());
				off = strlen(context->ctxResponse._sendBuffer+10);
			LWIP_sprintf(context->ctxResponse._sendBuffer+10+off, "<modelDescription>%s</modelDescription>\r\n", GetModel());
				off = strlen(context->ctxResponse._sendBuffer+10);
			LWIP_sprintf(context->ctxResponse._sendBuffer+10+off, "<modelName>%s</modelName>\r\n", GetModel());
				off = strlen(context->ctxResponse._sendBuffer+10);
			LWIP_sprintf(context->ctxResponse._sendBuffer+10+off, "<modelNumber>%s (%s)</modelNumber>\r\n", GetDeviceName(), GetDeviceVersion()); //MODEL_NUMBER
				off = strlen(context->ctxResponse._sendBuffer+10);
			LWIP_sprintf(context->ctxResponse._sendBuffer+10+off, "<modelURL>%s</modelURL>\r\n", GetModelURL());
				off = strlen(context->ctxResponse._sendBuffer+10);
			LWIP_sprintf(context->ctxResponse._sendBuffer+10+off, "<serialNumber>%s</serialNumber>\r\n", GetDeviceSN());
				off = strlen(context->ctxResponse._sendBuffer+10);
			LWIP_sprintf(context->ctxResponse._sendBuffer+10+off, "<UDN>uuid:%s</UDN>\r\n", GetDeviceUUID());
				off = strlen(context->ctxResponse._sendBuffer+10);
			
			strcat(context->ctxResponse._sendBuffer+10, "</device>\r\n");
				off = strlen(context->ctxResponse._sendBuffer+10);
			strcat(context->ctxResponse._sendBuffer+10, "</root>\r\n");
				off = strlen(context->ctxResponse._sendBuffer+10);

			size2Send = off;
			context->ctxResponse._dwOperIndex = context->ctxResponse._dwTotal;

			LWIP_sprintf(szSize, "%X\r\n", off);
			prefixLen = strlen(szSize);
			memcpy(context->ctxResponse._sendBuffer+10-prefixLen, szSize, prefixLen); //copy to the head
			size2Send += prefixLen;
			
			strcpy(szSize, "\r\n0\r\n\r\n");
			memcpy(context->ctxResponse._sendBuffer+10-prefixLen + size2Send, szSize, 7); //copy to the tail
			size2Send += 7;
			
			memmove(context->ctxResponse._sendBuffer, context->ctxResponse._sendBuffer+10-prefixLen, size2Send); //move to the head
			
			context->ctxResponse._bytesLeft = size2Send;
			needSend = 1;
			context->ctxResponse._dwOperStage = STAGE_END;
			
			LogPrint(LOG_DEBUG_ONLY, "CGI_SSDP_PROFILE response sending: %d @%d", size2Send, context->_sid);
		}
	}
	else if (context->_requestMethod == METHOD_POST)
	{ //no body to send
		context->_state = HTTP_STATE_REQUEST_END;
	}
	
	return needSend;
}

void SSDP_OnAllSent(REQUEST_CONTEXT* context)
{
}
