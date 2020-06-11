/*
  cgi_web.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#include "../http_cgi.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

extern void LogPrint(int level, char* format, ... );
extern int Strnicmp(char *str1, char *str2, int n);
extern int ReplaceTag(REQUEST_CONTEXT* context, char* tagName, char* appendTo, int maxAllowed);

/////////////////////////////////////////////////////////////////////////////////////////////////////

void Web_OnAuthHeaders(REQUEST_CONTEXT* context);
int  Web_OnAuthData(REQUEST_CONTEXT* context, char* buffer, int size);
void Web_OnRequestReceived(REQUEST_CONTEXT* context);
	
void Web_SetResponseHeaders(REQUEST_CONTEXT* context, char* HttpCodeInfo);

struct CGI_Mapping g_cgiAuth = {
	"/auth/*", //char* path;
	CGI_OPT_GET_ENABLED | CGI_OPT_POST_ENABLED,// unsigned long options;

	NULL, //void (*OnCancel)(REQUEST_CONTEXT* context);
	NULL, //int (*OnHeaderReceived)(REQUEST_CONTEXT* context, char* header_line);
	Web_OnAuthHeaders, //void (*OnHeadersReceived)(REQUEST_CONTEXT* context);
	Web_OnAuthData, //int  (*OnContentReceived)(REQUEST_CONTEXT* context, char* buffer, int size);
	Web_OnRequestReceived, //void (*OnRequestReceived)(REQUEST_CONTEXT* context);

	Web_SetResponseHeaders, //void (*SetResponseHeader)(REQUEST_CONTEXT* context, char* HttpCode);
	WEB_ReadContent, //int  (*ReadContent)(REQUEST_CONTEXT* context, char* buffer, int maxSize);

	WEB_AllSent, //int  (*OnAllSent)(REQUEST_CONTEXT* context);
	WEB_Finished, //void (*OnFinished)(REQUEST_CONTEXT* context);

	NULL //struct CGI_Mapping* next;
};

int CheckUser(char* u, char* p)
{
	int success = 0;
	if ((stricmp(u, "admin") == 0))// && (stricmp(p, "password") == 0))
		success = 1;
	else if ((stricmp(u, "user") == 0))// && (stricmp(p, "password") == 0))
		success = 1;
	return success;
}

int AuthCheck(REQUEST_CONTEXT* context)
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

			if (CheckUser(u, p) > 0)
			{
				success = SessionCreate(context, szToken);
				if (success > 0)
				{
					context->ctxResponse._authorized = CODE_OK;
					strcpy(context->ctxResponse._token, szToken);
					return 1;
				}
			}
		}
	}
	return 0;
}

void Web_OnAuthHeaders(REQUEST_CONTEXT* context)
{
	memset(context->ctxResponse._appContext, 0, sizeof(context->ctxResponse._appContext));
}

int Web_OnAuthData(REQUEST_CONTEXT* context, char* buffer, int size)
{
	if (size > 0)
	{
		int len = strlen(context->ctxResponse._appContext);
		int max = sizeof(context->ctxResponse._appContext);
		strncpy(context->ctxResponse._appContext + len, buffer, (size < max - len - 2) ? size : (max - len - 2));
	}
	return size;
}

void Web_OnRequestReceived(REQUEST_CONTEXT* context)
{
	SSI_Context* ctxSSI = (SSI_Context*)context->ctxResponse._appContext;

	if (stricmp(context->_requestPath, WEB_SESSION_CHECK) == 0)
	{ //session exists?
		if (context->_session == NULL)
			context->_result = CODE_UNAUTHORIZED; ////Unauthorized (RFC 7235)
		else
			context->_result = CODE_OK;
		return;
	}

	if (stricmp(context->_requestPath, WEB_LOGOUT_PAGE) == 0)
	{ //logout
		SessionKill(context, 1);

		strcpy(context->_responsePath, WEB_DEFAULT_PAGE);
		context->ctxResponse._authorized = 0;
		context->ctxResponse._token[0] = 0;
		context->_result = CODE_REDIRECT;
		return;
	}

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

void Web_SetResponseHeaders(REQUEST_CONTEXT* context, char* HttpCodeInfo)
{
	int nSize = strlen(context->ctxResponse._sendBuffer);

	if (stricmp(context->_requestPath, WEB_LOGOUT_PAGE) == 0)
		LWIP_sprintf(context->ctxResponse._sendBuffer + nSize, "X-Auth-Token: SID=\r\nSet-Cookie: SID=; Path=/; HttpOnly; max-age=3600\r\n");
	else if ((context->_requestMethod == METHOD_POST) && (stricmp(context->_requestPath, WEB_DEFAULT_PAGE) == 0))
	{
		if ((context->ctxResponse._authorized == CODE_OK) && (context->ctxResponse._token[0] != 0))
		{
			LWIP_sprintf(context->ctxResponse._sendBuffer + nSize, "X-Auth-Token: %s\r\nSet-Cookie: %s; Path=/; HttpOnly; max-age=3600\r\n",
				context->ctxResponse._token, context->ctxResponse._token);
		}
	}
	WEB_SetResponseHeaders(context, HttpCodeInfo);
}
