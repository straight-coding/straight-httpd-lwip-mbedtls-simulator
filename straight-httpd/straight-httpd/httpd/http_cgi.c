/*
  http_cgi.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#include "lwip/opt.h"

#include "lwip_port.h"
#include "arch/sys_arch.h"

#include "http_core.h"
#include "http_cgi.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

extern void LogPrint(int level, char* format, ... );
extern int  Strnicmp(char *str1, char *str2, int n);

/////////////////////////////////////////////////////////////////////////////////////////////////////

struct CGI_Mapping* g_cgiMapping = NULL;

char g_szWebDrive[16]; // WEB_DRIVE
char g_szWebAbsRoot[128]; // WEB_ABS_ROOT, MUST start with '/' and end with '/'
char g_szWebDefaultPage[128]; // "/auth/login.html", MUST start with '/'
char g_szWebAppHomePage[128]; // "/app/index.html", MUST start with '/'

/////////////////////////////////////////////////////////////////////////////////////////////////////

void CGI_SetupMapping() //called from SetupHttpContext(), CGI handlers could be added here
{
	//extern struct CGI_Mapping g_cgiStatus;
	extern struct CGI_Mapping g_cgiSSDP; //"/upnp_device.xml"
	//extern struct CGI_Mapping g_cgiCommand;
	extern struct CGI_Mapping g_cgiLog;
	extern struct CGI_Mapping g_cgiUpgrade;
	extern struct CGI_Mapping g_cgiWebAuth; // "/auth/*"
	extern struct CGI_Mapping g_cgiWebApp;	// "/app/*", MUST be the last one

	SetWebRoot(WEB_DRIVE, WEB_ABS_ROOT, WEB_DEFAULT_PAGE); //default path
	
	//CGI_Append(&g_cgiStatus);		//"/status.json"
	CGI_Append(&g_cgiSSDP, NULL, 0);	//"/upnp_device.xml"
	//CGI_Append(&g_cgiCommand);	//"/cmd.cgi"
	CGI_Append(&g_cgiLog,     "/app/log.cgi", CGI_OPT_AUTH_REQUIRED | CGI_OPT_GET_ENABLED | CGI_OPT_CHUNKED);
	CGI_Append(&g_cgiUpgrade, "/app/upgrade.cgi", CGI_OPT_AUTH_REQUIRED | CGI_OPT_POST_ENABLED);
	CGI_Append(&g_cgiWebAuth, "/auth/*", CGI_OPT_AUTHENTICATOR | CGI_OPT_GET_ENABLED | CGI_OPT_POST_ENABLED);
	CGI_Append(&g_cgiWebApp,  "/app/*", CGI_OPT_AUTH_REQUIRED | CGI_OPT_GET_ENABLED | CGI_OPT_POST_ENABLED); //"/app/*", MUST be the last one
}

int CheckWebRoot(char* drive, char* absRoot, char* defaultPage)
{
	LWIP_FIL* fTest;

	char szTemp[128];
	memset(szTemp, 0, sizeof(szTemp));

	if (drive[0] != 0)
	{
		szTemp[0] = drive[0];
		szTemp[1] = ':';
	}
	strcat(szTemp, absRoot);
	strcat(szTemp, defaultPage+1);

	fTest = LWIP_fopen(szTemp, "r");
	if (fTest == NULL)
		return 0;

	LWIP_fclose(fTest);
	return 1;
}

void SetWebRoot(char* drive, char* absRoot, char* defaultFile)
{
	int i;
	int rootResult = 0;
	
	memset(g_szWebDrive, 0, sizeof(g_szWebDrive));
	memset(g_szWebAbsRoot, 0, sizeof(g_szWebAbsRoot));
	memset(g_szWebDefaultPage, 0, sizeof(g_szWebDefaultPage));
	memset(g_szWebAppHomePage, 0, sizeof(g_szWebAppHomePage));

	if ((drive != NULL) && (*drive != 0))
		strcpy(g_szWebDrive, drive);

	if ((absRoot != NULL) && (*absRoot != 0))
	{
		strcpy(g_szWebAbsRoot, absRoot);

		i = 0;
		while (g_szWebAbsRoot[i] != 0)
		{
			if (g_szWebAbsRoot[i] == '\\')
				g_szWebAbsRoot[i] = '/';
			i++;
		}
		if (g_szWebAbsRoot[0] != '/')
		{
			memmove(g_szWebAbsRoot + 1, g_szWebAbsRoot, strlen(g_szWebAbsRoot));
			g_szWebAbsRoot[0] = '/';
		}
		if (g_szWebAbsRoot[strlen(g_szWebAbsRoot)-1] != '/')
			strcat(g_szWebAbsRoot, "/");
	}

	if ((defaultFile != NULL) && (*defaultFile != 0))
	{
		strcpy(g_szWebDefaultPage, defaultFile);

		i = 0;
		while (g_szWebDefaultPage[i] != 0)
		{
			if (g_szWebDefaultPage[i] == '\\')
				g_szWebDefaultPage[i] = '/';
			i++;
		}

		if (g_szWebDefaultPage[0] != '/')
		{
			memmove(g_szWebDefaultPage + 1, g_szWebDefaultPage, strlen(g_szWebDefaultPage));
			g_szWebDefaultPage[0] = '/';
		}
	}

	{
		strcpy(g_szWebAppHomePage, WEB_APP_PAGE);

		i = 0;
		while (g_szWebAppHomePage[i] != 0)
		{
			if (g_szWebAppHomePage[i] == '\\')
				g_szWebAppHomePage[i] = '/';
			i++;
		}

		if (g_szWebAppHomePage[0] != '/')
		{
			memmove(g_szWebAppHomePage + 1, g_szWebAppHomePage, strlen(g_szWebAppHomePage));
			g_szWebAppHomePage[0] = '/';
		}
	}

	for (i = 0; i <= strlen(g_szWebDrive); i++)
	{
		rootResult = CheckWebRoot(g_szWebDrive + i, g_szWebAbsRoot, g_szWebDefaultPage);
		if (rootResult > 0)
		{
			LogPrint(LOG_DEBUG_ONLY, "WebRoot: %c:%s%s", g_szWebDrive[i], g_szWebAbsRoot, g_szWebDefaultPage+1);
			if (g_szWebDrive[i] != 0)
			{
				memmove(g_szWebAbsRoot + 2, g_szWebAbsRoot, strlen(g_szWebAbsRoot));
				g_szWebAbsRoot[0] = g_szWebDrive[i];
				g_szWebAbsRoot[1] = ':';
			}
			break;
		}
		if (g_szWebDrive[i] == 0)
			break;
	}
}

void CGI_Append(struct CGI_Mapping* newMapping, const char* ovwPath, u32_t ovwOptions)
{
	int i = 0;
	struct CGI_Mapping* next;
	if (ovwPath != NULL)
	{
		memset(newMapping->path, 0, sizeof(newMapping->path));
		strncpy(newMapping->path, ovwPath, sizeof(newMapping->path)-1);
	}

	if (ovwOptions != 0)
		newMapping->options = ovwOptions;

	while (newMapping->path[i] != 0)
	{
		if (newMapping->path[i] == '*')
		{
			newMapping->path[i] = 0;
			newMapping->options |= CGI_OPT_PREFIX_WILDCARD;
			break;
		}
		i++;
	}

	newMapping->next = NULL;
	if (g_cgiMapping == NULL)
	{
		g_cgiMapping = newMapping;
		return;
	}
	
	next = g_cgiMapping;
	while(next != NULL)
	{
		if (next->next == NULL)
		{
			next->next = newMapping;
			break;
		}
		next = next->next;
	}
}

void CGI_Cancel(REQUEST_CONTEXT* context) //cancel notification to app layer because of HTTP fatal error, including timeout, format error, sending failure, stack keneral error
{
	if (context->handler != NULL)
	{
		if (context->handler->OnCancel != NULL)
			context->handler->OnCancel(context);
	}
}

void CGI_Finish(REQUEST_CONTEXT* context) //called by FreeHttpContext()
{
	if (context->handler != NULL)
	{
		if (context->handler->OnFinished != NULL)
			context->handler->OnFinished(context);
	}
}

void CGI_SetCgiHandler(REQUEST_CONTEXT* context) //called when the first HTTP request line is received
{
	int nReqPathLen = strlen(context->_requestPath);
	struct CGI_Mapping* next = g_cgiMapping;
	
	while(next != NULL)
	{
		int nCgiPathLen = strlen(next->path);
		if ((next->options & CGI_OPT_PREFIX_WILDCARD) != 0)
		{ //check prefix match
			if (nReqPathLen >= nCgiPathLen)
			{
				if (Strnicmp(context->_requestPath, next->path, nCgiPathLen) == 0)	//such as "/auth/*", "/app/*"
				{
					context->handler = next;
					return;
				}
			}
		}
		else
		{ //check completely match
			if (nReqPathLen == nCgiPathLen)
			{ //same length
				if (Strnicmp(context->_requestPath, next->path, nCgiPathLen) == 0)	//such as "/status.json"
				{
					context->handler = next;
					return;
				}
			}
		}
		next = next->next;
	}
	context->handler = NULL;

	LogPrint(0, "No CGI for %s", context->_requestPath);
}

void CGI_HeaderReceived(REQUEST_CONTEXT* context, char* header_line) //called when single HTTP request header is received
{
	LogPrint(LOG_DEBUG_ONLY, "Header: %s, @%d", header_line, context->_sid);
	
	if (context->handler != NULL)
	{
		if (context->handler->OnHeaderReceived != NULL)
			context->handler->OnHeaderReceived(context, header_line);
	}
}

void CGI_HeadersReceived(REQUEST_CONTEXT* context) //called when all HTTP request headers are received
{
	if (context->handler != NULL)
	{
		if ((context->handler->options & CGI_OPT_AUTH_REQUIRED) != 0)
		{
			if (context->ctxResponse._authorized != CODE_OK)
			{
				context->_result = context->ctxResponse._authorized; //403 forbidden
				return;
			}
		}
		
		if (context->_requestMethod == METHOD_GET) 
		{
			if ((context->handler->options & CGI_OPT_GET_ENABLED) != 0)
			{
				if (context->handler->OnHeadersReceived != NULL)
					context->handler->OnHeadersReceived(context);
				return;
			}
		}
		else if (context->_requestMethod == METHOD_POST)
		{
			if ((context->handler->options & CGI_OPT_POST_ENABLED) != 0)
			{
				if (context->handler->OnHeadersReceived != NULL)
					context->handler->OnHeadersReceived(context);
				return;
			}
		}
	}
	
	if (context->_result < 0)
		return;
	
	if ((SessionTypes(context->_extension) > 0) && (context->handler == NULL))
	{
		LogPrint(0, "No CGI for: %s, @%d", context->_requestPath, context->_sid);
		strcpy(context->_responsePath, WEB_DEFAULT_PAGE);
		context->ctxResponse._authorized = 0;
		context->_result = CODE_REDIRECT; //redirect
	}
	else if (context->_requestMethod == METHOD_GET)
	{
		LogPrint(0, "Not found: %s, @%d", context->_requestPath, context->_sid);
		context->_result = -404; //404 not found
	}
	else if (context->_requestMethod == METHOD_POST)
	{
		LogPrint(0, "Unsuported service @%d", context->_sid);
		context->_result = -500; //forbidden, need abort
	}
	else
	{
		LogPrint(0, "Unsuported method @%d", context->_sid);
		context->_result = -500; //forbidden, need abort
	}
}

int CGI_ContentReceived(REQUEST_CONTEXT* context, char* buffer, int size) //called when HTTP request body is received (may be partial)
{
	int consumed = 0;
	
	if (context->handler != NULL)
	{
		if (context->handler->OnContentReceived != NULL)
			return context->handler->OnContentReceived(context, buffer, size);
	}
	
	return consumed;
}

void CGI_RequestReceived(REQUEST_CONTEXT* context) //called when HTTP request body is completely received
{
	if (context->handler != NULL)
	{
		if (context->handler->OnRequestReceived != NULL)
			context->handler->OnRequestReceived(context);
		return;
	}
}

void CGI_SetResponseHeaders(REQUEST_CONTEXT* context, char* HttpCodeInfo) //set response headers: content-type, length, connection, and http code
{
	if (context->_result < 0)
	{
		if (context->_result == CODE_REDIRECT)
		{
			LWIP_sprintf(context->ctxResponse._sendBuffer, (char*)header_generic, "200 OK", "text/html", strlen(redirect_body1)+ strlen(redirect_body2) + strlen(context->_responsePath), "close");//, context->_responsePath);
			if ((context->ctxResponse._authorized == CODE_OK) && (context->ctxResponse._token[0] != 0))
			{
				char szCookie[128];
				LWIP_sprintf(szCookie, "X-Auth-Token: %s\r\nSet-Cookie: %s; Path=/; HttpOnly; max-age=3600\r\n", context->ctxResponse._token, context->ctxResponse._token);
				strcat(context->ctxResponse._sendBuffer, szCookie);
			}
			strcat(context->ctxResponse._sendBuffer, CRLF);
			strcat(context->ctxResponse._sendBuffer, redirect_body1);
			strcat(context->ctxResponse._sendBuffer, context->_responsePath);
			strcat(context->ctxResponse._sendBuffer, redirect_body2);
		}
		else
		{
			LWIP_sprintf(context->ctxResponse._sendBuffer, (char*)header_generic, HttpCodeInfo, "text/html", 0, "close");
			strcat(context->ctxResponse._sendBuffer, CRLF);
		}
		return;
	}
	
	if (context->handler != NULL)
	{
		if (context->handler->SetResponseHeaders != NULL)
		{
			context->handler->SetResponseHeaders(context, HttpCodeInfo);
			return;
		}
	}
	
	if (context->_requestMethod == METHOD_GET)
	{
		LWIP_sprintf(context->ctxResponse._sendBuffer, (char*)header_generic, HttpCodeInfo, "text/html", 0, "close");
	}
	else if (context->_requestMethod == METHOD_POST)
	{
		LWIP_sprintf(context->ctxResponse._sendBuffer, (char*)header_generic, HttpCodeInfo, "text/html", 0, "close");
	}
	else
	{
		LWIP_sprintf(context->ctxResponse._sendBuffer, (char*)header_generic, HttpCodeInfo, "text/html", 0, "close");
	}
	strcat(context->ctxResponse._sendBuffer, CRLF);
}

int CGI_LoadContentToSend(REQUEST_CONTEXT* context, int caller) //load response body chunk by chunk
{
	int hasData2Send = 0;

	if (context->_requestMethod == METHOD_GET)
	{
		if (context->ctxResponse._dwOperStage >= STAGE_END)
		{
			if ((caller == HTTP_PROC_CALLER_SENT) &&
				(context->ctxResponse._totalSent >= context->ctxResponse._total2Send))
			{ //last chunk sent
				LogPrint(LOG_DEBUG_ONLY, "Response done @%d", context->_sid);
				
				context->_state = HTTP_STATE_REQUEST_END;
				
				if (context->handler->OnAllSent != NULL)
					context->handler->OnAllSent(context);
				return hasData2Send;
			}
		}
	}
	
	if (context->handler != NULL)
	{
		if (context->handler->LoadContentToSend != NULL)
		{
			if (context->ctxResponse._dwOperStage >= STAGE_END)
				return 0; //call type: HTTP_PROC_CALLER_POLL

			return context->handler->LoadContentToSend(context);
		}
	}
	
	context->_state = HTTP_STATE_REQUEST_END; //no body to send, just end
	return hasData2Send;
}
