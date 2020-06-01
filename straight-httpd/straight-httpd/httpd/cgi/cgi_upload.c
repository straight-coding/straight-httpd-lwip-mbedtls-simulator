/*
  cgi_upload.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: May 04, 2020
*/

#include "../http_cgi.h"

#define UPLOAD_ROOT "D:\\temp\\"

/////////////////////////////////////////////////////////////////////////////////////////////////////

extern void LogPrint(int level, char* format, ... );

/////////////////////////////////////////////////////////////////////////////////////////////////////

void Upload_OnCancel(REQUEST_CONTEXT* context);
void Upload_OnHeaderReceived(REQUEST_CONTEXT* context, char* header_line);
void Upload_OnHeadersReceived(REQUEST_CONTEXT* context);
int  Upload_OnContentReceived(REQUEST_CONTEXT* context, char* buffer, int size);
void Upload_AllReceived(REQUEST_CONTEXT* context);

struct CGI_Mapping g_cgiUpload = {
	"/api/upload.cgi",
	CGI_OPT_AUTH_REQUIRED | CGI_OPT_POST_ENABLED,// unsigned long options;

	Upload_OnCancel, //void (*OnCancel)(REQUEST_CONTEXT* context);

	Upload_OnHeaderReceived, //void (*OnHeaderReceived)(REQUEST_CONTEXT* context, char* header_line);
	Upload_OnHeadersReceived, //void (*OnHeadersReceived)(REQUEST_CONTEXT* context);
	Upload_OnContentReceived, //int  (*OnContentReceived)(REQUEST_CONTEXT* context, char* buffer, int size);
	Upload_AllReceived, //void (*OnRequestReceived)(REQUEST_CONTEXT* context);

	NULL, //void (*SetResponseHeader)(REQUEST_CONTEXT* context, char* HttpCode);
	NULL, //int  (*ReadContent)(REQUEST_CONTEXT* context, char* buffer, int maxSize);
	
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
long Upload_Start(void* context, char* szFileName, long nFileSize)
{
	REQUEST_CONTEXT* ctx = (REQUEST_CONTEXT*)context;

	ctx->_fileHandle = LWIP_fopen(ctx->ctxResponse._appContext, "wb");
	if (ctx->_fileHandle == 0)
		return 0;

	return 1; //ready to receive f/w
}
long Upload_GetFreeSize(u32_t askForSize)
{
	return askForSize; //how many it can accept (etc. accept all)
}
long Upload_Received(REQUEST_CONTEXT* context, u8_t* pData, u32_t dwLen)
{
	if (LWIP_fwrite(context->_fileHandle, pData, dwLen) > 0) //>0:success
		return dwLen;  //real count consumed (etc. consume all)
	return 0;
}
void Upload_AllReceived(REQUEST_CONTEXT* context)
{ //to upgrade after f/w completely received
	LogPrint(0, "Post upgrade done: length=%d @%d", context->_contentLength, context->_sid);

	LWIP_fclose(context->_fileHandle);
}

void Upload_OnHeaderReceived(REQUEST_CONTEXT* context, char* header_line)
{
	if (Strnicmp(header_line, "X-File-Name:", 12) == 0)
	{
		int j;
		int len = 0;
		int code = 0;
		for (j = 0; j < strlen(header_line); j++)
		{
			if (code == 0)
			{
				if (header_line[j] == ':')
					code = 1;
			}
			else
			{
				if (header_line[j] != ' ')
					break;
			}
		}
		len = DecodeB64((unsigned char*)(header_line + j));
		LWIP_sprintf(context->ctxResponse._appContext, "%s%s", UPLOAD_ROOT, (char*)header_line + j);
	}
}

void Upload_OnCancel(REQUEST_CONTEXT* context)
{ //to cancel upgrade because of connection error
	LogPrint(0, "Post upgrade canceled: length=%d @%d", context->_contentLength, context->_sid);
}

void Upload_OnHeadersReceived(REQUEST_CONTEXT* context)
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
			if (Upload_Start(context, context->_requestPath, (long)context->_contentLength) <= 0) //2=PRINT_MODE_ONLINE
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

int Upload_OnContentReceived(REQUEST_CONTEXT* context, char* buffer, int size)
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

			nFree = Upload_GetFreeSize(maxToConsume);
			if (nFree > 0)
			{
				maxToConsume = nFree;
				if (maxToConsume > 0)
				{
					int eaten = Upload_Received(context, (u8_t*)buffer, maxToConsume);
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
