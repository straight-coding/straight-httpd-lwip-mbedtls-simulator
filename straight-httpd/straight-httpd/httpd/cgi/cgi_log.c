/*
  cgi_log.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: May 11, 2020
*/

#include "../http_cgi.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

extern void LogPrint(int level, char* format, ... );

/////////////////////////////////////////////////////////////////////////////////////////////////////

void LOG_Start(REQUEST_CONTEXT* context);
void LOG_SendHeader(REQUEST_CONTEXT* context, char* HttpCodeInfo);
int  LOG_Sending(REQUEST_CONTEXT* context);

struct CGI_Mapping g_cgiLog = {
	"/app/log.cgi",
	CGI_OPT_AUTH_REQUIRED | CGI_OPT_GET_ENABLED | CGI_OPT_CHUNKED,// unsigned long options;
	
	NULL, //void (*OnCancel)(REQUEST_CONTEXT* context);
	NULL, //void (*OnHeaderReceived)(REQUEST_CONTEXT* context, char* header_line);
	NULL, //void (*OnHeadersReceived)(REQUEST_CONTEXT* context);
	NULL, //int  (*OnContentReceived)(REQUEST_CONTEXT* context, char* buffer, int size);
	LOG_Start, //void (*OnRequestReceived)(REQUEST_CONTEXT* context);
	
	LOG_SendHeader, //void (*SetResponseHeader)(REQUEST_CONTEXT* context, char* HttpCode);
	LOG_Sending, //int  (*LoadContentToSend)(REQUEST_CONTEXT* context, int caller);
	
	NULL, //int  (*OnAllSent)(REQUEST_CONTEXT* context);
	NULL, //void (*OnFinished)(REQUEST_CONTEXT* context);
	
	NULL //struct CGI_Mapping* next;
};

void LOG_Start(REQUEST_CONTEXT* context)
{
	context->ctxResponse._dwOperStage = 10000; //total items
	context->ctxResponse._dwOperIndex = 0; //
}

void LOG_SendHeader(REQUEST_CONTEXT* context, char* HttpCodeInfo)
{
	if (context->_requestMethod == METHOD_GET)
	{
		LWIP_sprintf(context->ctxResponse._sendBuffer, (char*)header_chunked, HttpCodeInfo, "text/plain; charset=utf-8", "", "keep-alive");
	}
}

int  LOG_Sending(REQUEST_CONTEXT* context)
{ //for _dwOperStage, index of log items
	int hasData2Send = 0;

	if (context->_requestMethod == METHOD_GET)
	{
		int size, i;
		char szSize[64];
		int size2Send;
		int prefixLen;
		
		int offset = 10;
		int count = 0;
		char szCRLF[4];
		int bufferFull = 0;
/*		
		if (context->ctxResponse._dwOperStage >= STAGE_END)
		{
			if (caller == HTTP_PROC_CALLER_SENT)
			{
				LogPrint(LOG_DEBUG_ONLY, "CGI_CNC_CFG response done @%d", context->_sid);
				context->_state = HTTP_STATE_REQUEST_END;
			}
		}
		else*/
		{
			if (context->ctxResponse._dwOperStage == 0)
			{ //first chunk, system parameters
				//context->ctxResponse._dwTotal = g_stSys.nLen/sizeof(MENU_PARAS);
				if (context->ctxResponse._dwOperIndex == 0)
				{
					strcpy(context->ctxResponse._sendBuffer+offset, "{\r\n");
					count = 3;
				}
				context->ctxResponse._dwOperStage = 1;
				context->ctxResponse._dwOperIndex = 0;
				memset(szCRLF, 0, 4);
			}
			else
			{
				strcpy(szCRLF, "\r\n");
			}
			
			if ((bufferFull == 0) && (context->ctxResponse._dwOperStage == 1))
			{ //the following chunks
				while(context->ctxResponse._dwOperIndex < context->ctxResponse._dwTotal)
				{
					if (count >= sizeof(context->ctxResponse._sendBuffer) - 50 - offset)
					{
						bufferFull = 1;
						break;
					}
					
					//count = AppendParam(&g_stSys, context->ctxResponse._dwOperIndex, count, context->ctxResponse._sendBuffer+offset);
					context->ctxResponse._sendBuffer[offset+count] = 0;
					
					context->ctxResponse._dwOperIndex ++;
				}
				
				if (context->ctxResponse._dwOperIndex >= context->ctxResponse._dwTotal)
				{ //system parameters done, then variables
					//context->ctxResponse._dwTotal = g_stVar.nLen/sizeof(MENU_PARAS);
					
					context->ctxResponse._dwOperStage = 2;
					context->ctxResponse._dwOperIndex = 0;
				}
			}
				
			if ((bufferFull == 0) && (context->ctxResponse._dwOperStage == 2))
			{
				while(context->ctxResponse._dwOperIndex < context->ctxResponse._dwTotal)
				{
					if (count >= sizeof(context->ctxResponse._sendBuffer) - 50 - offset)
					{
						bufferFull = 1;
						break;
					}
					//count = AppendParam(&g_stVar, context->ctxResponse._dwOperIndex, count, context->ctxResponse._sendBuffer+offset);
					context->ctxResponse._sendBuffer[offset+count] = 0;
					
					context->ctxResponse._dwOperIndex ++;
				}

				if (context->ctxResponse._dwOperIndex >= context->ctxResponse._dwTotal)
				{
					strcat(context->ctxResponse._sendBuffer + offset, "}\r\n");
					count +=3;
					
					context->ctxResponse._dwOperStage = STAGE_END;
					context->ctxResponse._dwOperIndex = 0;
				}
			}
		
			size2Send = count;
			
			LWIP_sprintf(szSize, "%s%X\r\n", szCRLF, count); //chunk size
			prefixLen = strlen(szSize);
			memcpy(context->ctxResponse._sendBuffer+offset-prefixLen, szSize, prefixLen); //copy chunk size to the head
			size2Send += prefixLen;
			
			if (context->ctxResponse._dwOperStage >= STAGE_END)
			{ //last block, end chunk
				strcpy(szSize, "\r\n0\r\n\r\n");
				memcpy(context->ctxResponse._sendBuffer+offset-prefixLen + size2Send, szSize, 7); //copy to the tail
				size2Send += 7;
			}
			
			memmove(context->ctxResponse._sendBuffer, context->ctxResponse._sendBuffer+offset-prefixLen, size2Send); //move to the head
			context->ctxResponse._bytesLeft = size2Send;
			hasData2Send = 1;
			
			LogPrint(LOG_DEBUG_ONLY, "CGI_CNC_PARAM response sending: %d @%d", count, context->_sid);
		}
	}
	else if (context->_requestMethod == METHOD_POST)
	{ //no body to send
		context->_state = HTTP_STATE_REQUEST_END; //no body to send, just end
	}
	
	return hasData2Send;
}
