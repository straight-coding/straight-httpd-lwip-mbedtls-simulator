/*
  cgi_upgrade.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: May 04, 2020
*/

#include "../http_cgi.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

extern void LogPrint(int level, char* format, ... );

/////////////////////////////////////////////////////////////////////////////////////////////////////

void FW_OnCancel(REQUEST_CONTEXT* context);
void FW_OnHeadersReceived(REQUEST_CONTEXT* context);
int  FW_OnContentReceived(REQUEST_CONTEXT* context, char* buffer, int size);
void FW_AllReceived(REQUEST_CONTEXT* context);

struct CGI_Mapping g_cgiUpgrade = {
	"/app/upgrade.cgi",
	CGI_OPT_AUTH_REQUIRED | CGI_OPT_POST_ENABLED,// unsigned long options;
	
	FW_OnCancel, //void (*OnCancel)(REQUEST_CONTEXT* context);

	NULL, //void (*OnHeaderReceived)(REQUEST_CONTEXT* context, char* header_line);
	FW_OnHeadersReceived, //void (*OnHeadersReceived)(REQUEST_CONTEXT* context);
	FW_OnContentReceived, //int  (*OnContentReceived)(REQUEST_CONTEXT* context, char* buffer, int size);
	FW_AllReceived, //void (*OnRequestReceived)(REQUEST_CONTEXT* context);

	NULL, //void (*SetResponseHeader)(REQUEST_CONTEXT* context, char* HttpCode);
	NULL, //int  (*LoadContentToSend)(REQUEST_CONTEXT* context, int caller);
	
	NULL, //int  (*OnAllSent)(REQUEST_CONTEXT* context);
	NULL, //void (*OnFinished)(REQUEST_CONTEXT* context);
	
	NULL //struct CGI_Mapping* next;
};

long Dev_IsBusy(void)
{
	return 0;
}
long Dev_IsOnline(void)
{
	return 1;
}
long FW_Start(void* context, char* szFileName, long nFileSize)
{
	return 1; //ready to receive f/w
}
long FW_GetFreeSize(u32_t askForSize)
{
	return askForSize; //how many it can accept (etc. accept all)
}
long FW_Received(u8_t* pData, u32_t dwLen)
{
	return dwLen;  //real count consumed (etc. consume all)
}
void FW_AllReceived(REQUEST_CONTEXT* context)
{ //to upgrade after f/w completely received
	LogPrint(0, "Post upgrade done: length=%d @%d", context->_contentLength, context->_sid);
}
void FW_OnCancel(REQUEST_CONTEXT* context)
{ //to cancel upgrade because of connection error
	LogPrint(0, "Post upgrade canceled: length=%d @%d", context->_contentLength, context->_sid);
}

void FW_OnHeadersReceived(REQUEST_CONTEXT* context)
{
	if (context->_requestMethod == METHOD_GET)
	{
		context->_result = -404; //404 not found
	}
	else if (context->_requestMethod == METHOD_POST)
	{
		if (Dev_IsBusy())
		{
			context->_result = -500; //forbidden, need abort
			LogPrint(0, "Failed to start upgrade, Dev_IsBusy @%d", context->_sid);
		}
		else
		{
			if (FW_Start(context, context->_requestPath, (long)context->_contentLength) <= 0) //2=PRINT_MODE_ONLINE
			{
				context->_result = -500; //forbidden, need abort
				LogPrint(0, "FW_Start failed, len=%d @%d", context->_contentLength, context->_sid);
			}
			else
			{
				LogPrint(0, "FW_Start success, len=%d @%d", context->_contentLength, context->_sid);
			}
		}
	}
}

int  FW_OnContentReceived(REQUEST_CONTEXT* context, char* buffer, int size)
{
	int consumed = 0;
	int maxToConsume = context->_contentLength - context->_contentReceived;
	if (maxToConsume > size)
		maxToConsume = size;
	
	if (maxToConsume > 0)
	{
		if (context->_requestMethod == METHOD_POST)
		{ //processing while receiving
			int nFree = 0;
			if (IsKilling(context, 1)) //get and reset
			{
				context->_result = -500;
				return 0;
			}

			nFree = FW_GetFreeSize(maxToConsume);
			if (nFree > 0)
			{
				maxToConsume = nFree;
				if (maxToConsume > 0)
				{
					int eaten = FW_Received((u8_t*)buffer, maxToConsume);
					if (eaten > 0)
					{
						consumed = eaten;
						
						context->_tLastReceived = LWIP_GetTickCount();
						if (context->_tLastReceived == 0)
							context->_tLastReceived ++;
					}
					LogPrint(LOG_DEBUG_ONLY, "Enqueued for parser %d, free=%d, @%d", eaten, nFree, context->_sid);
				}
			}
			else
			{ //blocked by processor
				context->_tLastReceived = LWIP_GetTickCount();
				if (context->_tLastReceived == 0)
					context->_tLastReceived ++;

				LogPrint(0, "Blocked by Dev: received %d, need %d, but free %d @%d", size, maxToConsume, nFree, context->_sid);
			}
		}
	}
	
	return consumed;
}
