/*
  cgi_form.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: June 11, 2020
*/

#include "../http_cgi.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

extern void LogPrint(int level, char* format, ... );

/////////////////////////////////////////////////////////////////////////////////////////////////////

void Form_OnHeaders(REQUEST_CONTEXT* context);
int  Form_OnContentReceived(REQUEST_CONTEXT* context, char* buffer, int size);
void Form_OnRequestReceived(REQUEST_CONTEXT* context);
	
int ExtractFormData(char* buffer, int nSize, int nTotalRemain);
void Form_SetResponseHeaders(REQUEST_CONTEXT* context, char* HttpCodeInfo);

struct CGI_Mapping g_cgiForm = {
	"/form/*", //char* path;
	CGI_OPT_GET_ENABLED | CGI_OPT_POST_ENABLED,// unsigned long options;

	NULL, //void (*OnCancel)(REQUEST_CONTEXT* context);

	NULL, //int (*OnHeaderReceived)(REQUEST_CONTEXT* context, char* header_line);
	Form_OnHeaders, //void (*OnHeadersReceived)(REQUEST_CONTEXT* context);
	Form_OnContentReceived, //int  (*OnContentReceived)(REQUEST_CONTEXT* context, char* buffer, int size);
	Form_OnRequestReceived, //void (*OnRequestReceived)(REQUEST_CONTEXT* context);

	Form_SetResponseHeaders, //void (*SetResponseHeader)(REQUEST_CONTEXT* context, char* HttpCode);
	WEB_ReadContent, //int  (*ReadContent)(REQUEST_CONTEXT* context, char* buffer, int maxSize);
	WEB_AllSent, //int  (*OnAllSent)(REQUEST_CONTEXT* context);
	WEB_Finished, //void (*OnFinished)(REQUEST_CONTEXT* context);

	NULL //struct CGI_Mapping* next;
};
/*
int Form_Setter(REQUEST_CONTEXT* context)
{
	if (context->ctxResponse._appContext[0] != 0)
	{
		int success, i;
		char* u;
		char* p;
		char szToken[32];

		LogPrint(0, "POST data: %s\r\n", context->ctxResponse._appContext);
		u = strstr(context->ctxResponse._appContext, "username=");
		p = strstr(context->ctxResponse._appContext, "password=");
		if ((u != NULL) && (p != NULL))
		{
			u += 9; p += 9;

			i = 0;
			while (u[i] != 0)
			{
				if (u[i] == '&')
				{
					u[i] = 0;
					break;
				}
				i++;
			}

			i = 0;
			while (p[i] != 0)
			{
				if (p[i] == '&')
				{
					p[i] = 0;
					break;
				}
				i++;
			}
		}
	}
	return 0;
}
*/
void Form_OnHeaders(REQUEST_CONTEXT* context)
{
	memset(context->ctxResponse._sendBuffer, 0, sizeof(context->ctxResponse._sendBuffer));
	context->ctxResponse._bytesLeft = 0;
}

void Form_OnRequestReceived(REQUEST_CONTEXT* context)
{
	SSI_Context* ctxSSI = (SSI_Context*)context->ctxResponse._appContext;

	if ((context->_requestMethod == METHOD_POST) && (stricmp(context->_requestPath, WEB_DEFAULT_PAGE) == 0))
	{ //login
		int success = AuthCheck(context);
		memset(ctxSSI, 0, sizeof(SSI_Context)); //clear after OnAuthCheck
		if (success > 0)
		{
			strcpy(context->_responsePath, WEB_APP_PAGE);
			context->ctxResponse._authorized = CODE_OK;
			context->_result = CODE_REDIRECT;
			return;
		}
		else
		{
			strcpy(context->_responsePath, WEB_DEFAULT_PAGE);
			context->ctxResponse._authorized = 0;
			context->_result = CODE_REDIRECT;
			return;
		}
	}

	WEB_RequestReceived(context);
}

void Form_SetResponseHeaders(REQUEST_CONTEXT* context, char* HttpCodeInfo)
{
	WEB_SetResponseHeaders(context, HttpCodeInfo);
}

int Form_OnContentReceived(REQUEST_CONTEXT* context, char* buffer, int size)
{ //use send buffer to parse form data
	int consumed = context->_contentLength - context->_contentReceived;
	if (consumed > size)
		consumed = size;

	if (consumed > 0)
	{
		if (context->_requestMethod == METHOD_POST)
		{ //processing while receiving
			int nFree = 0;
			if (IsKilling(context, 1)) //get and reset
			{
				context->_result = -500;
				LogPrint(0, "IsKilling error=%d, @%d", context->_result, context->_sid);
				return 0;
			}

			nFree = sizeof(context->ctxResponse._sendBuffer) - context->ctxResponse._bytesLeft - 32;
			if (consumed > nFree)
			{
				consumed = nFree;
				if (consumed > 0)
				{
					int nParsed;
					int nRemain = context->_contentLength - context->_contentReceived - consumed;

					memcpy(context->ctxResponse._sendBuffer + context->ctxResponse._bytesLeft, buffer, consumed);
					context->ctxResponse._bytesLeft += consumed;

					context->_tLastReceived = LWIP_GetTickCount();
					if (context->_tLastReceived == 0)
						context->_tLastReceived++;

					nParsed = ExtractFormData(context->ctxResponse._sendBuffer, context->ctxResponse._bytesLeft, nRemain);
					if (nParsed > 0)
					{
						context->ctxResponse._bytesLeft -= nParsed;
						if (context->ctxResponse._bytesLeft > 0)
							memcpy(context->ctxResponse._sendBuffer, context->ctxResponse._sendBuffer + nParsed, context->ctxResponse._bytesLeft);
					}
					//LogPrint(LOG_DEBUG_ONLY, "File accepted %d, free=%d, @%d", eaten, nFree, context->_sid);
				}
			}
		}
	}
	return consumed;
}

int ExtractFormData(char* buffer, int nSize, int nTotalRemain)
{
	return nSize;
}
