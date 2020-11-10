/*
  cgi_web.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#include <string.h>

#include "../http_cgi.h"

#ifdef WIN32
#define Strnicmp strnicmp
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////

extern void LogPrint(int level, char* format, ... );

/////////////////////////////////////////////////////////////////////////////////////////////////////

static void Auth_OnHeaders(REQUEST_CONTEXT* context);
static int  Auth_OnPostData(REQUEST_CONTEXT* context, char* buffer, int size);
static void Auth_OnRequestReceived(REQUEST_CONTEXT* context);
static void Auth_SetResponseHeaders(REQUEST_CONTEXT* context, char* HttpCodeInfo);

struct CGI_Mapping g_cgiAuth = {
	"/auth/*", //char* path;
	CGI_OPT_GET_ENABLED | CGI_OPT_POST_ENABLED,// unsigned long options;

	NULL, //void (*OnCancel)(REQUEST_CONTEXT* context);

	NULL, //int (*OnHeaderReceived)(REQUEST_CONTEXT* context, char* header_line);
	Auth_OnHeaders, //void (*OnHeadersReceived)(REQUEST_CONTEXT* context);
	Auth_OnPostData, //int  (*OnContentReceived)(REQUEST_CONTEXT* context, char* buffer, int size);
	Auth_OnRequestReceived, //void (*OnRequestReceived)(REQUEST_CONTEXT* context);

	Auth_SetResponseHeaders, //void (*SetResponseHeader)(REQUEST_CONTEXT* context, char* HttpCode);
	WEB_ReadContent, //int  (*ReadContent)(REQUEST_CONTEXT* context, char* buffer, int maxSize);
	WEB_AllSent, //int  (*OnAllSent)(REQUEST_CONTEXT* context);
	WEB_Finished, //void (*OnFinished)(REQUEST_CONTEXT* context);

	NULL //struct CGI_Mapping* next;
};

static int CheckUser(char* u, char* p)
{
	int success = 0;
	if ((Strnicmp(u, "admin", 5) == 0))// && (stricmp(p, "password") == 0))
		success = 1;
	else if ((Strnicmp(u, "user", 4) == 0))// && (stricmp(p, "password") == 0))
		success = 1;
	return success;
}

static int AuthCheck(REQUEST_CONTEXT* context)
{
	if (context->ctxResponse._appContext[0] != 0)
	{
		int i;
		char* u;
		char* p;
		char szNewToken[32];

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
				context->_session = SessionCreate(szNewToken, context->_ipRemote);
				if (context->_session != NULL)
				{
					strcpy(context->ctxResponse._token, szNewToken);
					return 1;
				}
			}
		}
	}
	return 0;
}

static void Auth_OnHeaders(REQUEST_CONTEXT* context)
{
	memset(context->ctxResponse._appContext, 0, sizeof(context->ctxResponse._appContext));
}

static int Auth_OnPostData(REQUEST_CONTEXT* context, char* buffer, int size)
{
	if (size > 0)
	{
		int len = strlen(context->ctxResponse._appContext);
		int max = sizeof(context->ctxResponse._appContext);
		strncpy(context->ctxResponse._appContext + len, buffer, (size < max - len - 2) ? size : (max - len - 2));
	}
	return size;
}

static void Auth_OnRequestReceived(REQUEST_CONTEXT* context)
{
	SSI_Context* ctxSSI = (SSI_Context*)context->ctxResponse._appContext;

	if (Strnicmp(context->_requestPath, WEB_SESSION_CHECK, strlen(WEB_SESSION_CHECK)) == 0)
	{ //session exists?
		if (context->_session == NULL)
			context->_result = CODE_UNAUTHORIZED; ////Unauthorized (RFC 7235)
		else
			context->_result = CODE_OK;
		return;
	}

	if (Strnicmp(context->_requestPath, WEB_LOGOUT_PAGE, strlen(WEB_LOGOUT_PAGE)) == 0)
	{ //logout
		SessionKill(context->_session);

		strcpy(context->_responsePath, WEB_DEFAULT_PAGE);
		context->ctxResponse._authorized = 0;
		context->ctxResponse._token[0] = 0;
		context->_result = CODE_REDIRECT;
		return;
	}

	if ((context->_requestMethod == METHOD_POST) && (Strnicmp(context->_requestPath, WEB_DEFAULT_PAGE, strlen(WEB_DEFAULT_PAGE)) == 0))
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

static void Auth_SetResponseHeaders(REQUEST_CONTEXT* context, char* HttpCodeInfo)
{ //append headers
	int nSize = strlen(context->ctxResponse._sendBuffer);

	if (Strnicmp(context->_requestPath, WEB_LOGOUT_PAGE, strlen(WEB_LOGOUT_PAGE)) == 0)
		LWIP_sprintf(context->ctxResponse._sendBuffer + nSize, "X-Auth-Token: SID=\r\nSet-Cookie: SID=; Path=/; HttpOnly; max-age=3600\r\n");
	else if ((context->_requestMethod == METHOD_POST) && (Strnicmp(context->_requestPath, WEB_DEFAULT_PAGE, strlen(WEB_DEFAULT_PAGE)) == 0))
	{
		if ((context->ctxResponse._authorized == CODE_OK) && (context->ctxResponse._token[0] != 0))
		{
			LWIP_sprintf(context->ctxResponse._sendBuffer + nSize, "X-Auth-Token: %s\r\nSet-Cookie: %s; Path=/; HttpOnly; max-age=3600\r\n",
				context->ctxResponse._token, context->ctxResponse._token);
		}
	}
	WEB_AppendHeaders(context, HttpCodeInfo);
}
