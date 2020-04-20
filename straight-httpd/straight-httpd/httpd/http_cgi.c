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

char g_szWebDrive[16];
char g_szWebRoot[128];
char g_szWebRootFile[128];

/////////////////////////////////////////////////////////////////////////////////////////////////////

void CGI_SetupMapping() //called from SetupHttpContext(), CGI handlers could be added here
{
	//extern struct CGI_Mapping g_cgiStatus;
	extern struct CGI_Mapping g_cgiSSDP; //"/upnp_device.xml"
	//extern struct CGI_Mapping g_cgiCommand;
	//extern struct CGI_Mapping g_cgiParam;
	//extern struct CGI_Mapping g_cgiJob;
	extern struct CGI_Mapping g_cgiWeb;	//"/*", MUST be the last one

	SetWebRoot(WEB_DRIVE, WEB_ROOT, WEB_HOME_PAGE);
	
	//CGI_Append(&g_cgiStatus);		//"/status.json"
	CGI_Append(&g_cgiSSDP);			//"/upnp_device.xml"
	//CGI_Append(&g_cgiCommand);	//"/cmd.cgi"
	//CGI_Append(&g_cgiParam);		//"/param.json"
	//CGI_Append(&g_cgiJob);		//"/job.cgi"
	CGI_Append(&g_cgiWeb);		//"/*", MUST be the last one
}

int CheckWebRoot(char* drive, char* root, char* file)
{
	LWIP_FIL* fTest;

	char szTemp[128];
	memset(szTemp, 0, sizeof(szTemp));

	if (drive[0] != 0)
	{
		szTemp[0] = drive[0];
		szTemp[1] = ':';
	}
	strcat(szTemp, root);
	strcat(szTemp, file);

	fTest = LWIP_fopen(szTemp, "r");
	if (fTest == NULL)
		return 0;

	LWIP_fclose(fTest);
	return 1;
}

void SetWebRoot(char* drive, char* root, char* file)
{
	int i;
	int rootResult = 0;
	
	memset(g_szWebDrive, 0, sizeof(g_szWebDrive));
	memset(g_szWebRoot, 0, sizeof(g_szWebRoot));
	memset(g_szWebRootFile, 0, sizeof(g_szWebRootFile));

	if ((drive != NULL) && (*drive != 0))
		strcpy(g_szWebDrive, drive);

	if ((root != NULL) && (*root != 0))
	{
		strcpy(g_szWebRoot, root);

		i = 0;
		while (g_szWebRoot[i] != 0)
		{
			if (g_szWebRoot[i] == '\\')
				g_szWebRoot[i] = '/';
			i++;
		}
		if (g_szWebRoot[0] != '/')
		{
			memmove(g_szWebRoot+1, g_szWebRoot, strlen(g_szWebRoot));
			g_szWebRoot[0] = '/';
		}
		if (g_szWebRoot[strlen(g_szWebRoot)-1] != '/')
			strcat(g_szWebRoot, "/");
	}

	if ((file != NULL) && (*file != 0))
		strcpy(g_szWebRootFile, file);

	for (i = 0; i <= strlen(g_szWebDrive); i++)
	{
		rootResult = CheckWebRoot(g_szWebDrive + i, g_szWebRoot, g_szWebRootFile);
		if (rootResult > 0)
		{
			LogPrint(LOG_DEBUG_ONLY, "WebRoot: %c:%s%s", g_szWebDrive[i], g_szWebRoot, g_szWebRootFile);
			if (g_szWebDrive[i] != 0)
			{
				memmove(g_szWebRoot + 2, g_szWebRoot, strlen(g_szWebRoot));
				g_szWebRoot[0] = g_szWebDrive[i];
				g_szWebRoot[1] = ':';
			}
			break;
		}
		if (g_szWebDrive[i] == 0)
			break;
	}
}

void CGI_Append(struct CGI_Mapping* newMapping)
{
	struct CGI_Mapping* next;
	
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
	int nReqLen = strlen(context->_requestPath);
	struct CGI_Mapping* next = g_cgiMapping;
	
	while(next != NULL)
	{
		int nPrefix = 0;
		char szPattern[128];
		int nCgiPathLen = strlen(next->path);
		
		memset(szPattern, 0, sizeof(szPattern));
		strncpy(szPattern, next->path, sizeof(szPattern)-1);
		while(szPattern[nPrefix] != 0)
		{
			if (szPattern[nPrefix] == '*')
			{
				szPattern[nPrefix] = 0;
				break;
			}
			nPrefix ++;
		}
		
		if (nCgiPathLen == nPrefix)
		{ //full match test
			if (nReqLen == nPrefix)
			{ //same length
				if (Strnicmp(context->_requestPath, next->path, nPrefix) == 0)	//such as "/status.json"
				{
					context->handler = next;
					return;
				}
			}
		}
		else
		{ //prefix match test
			if (nReqLen >= nPrefix)
			{
				if (Strnicmp(context->_requestPath, next->path, nPrefix) == 0)	//such as "/*"
				{
					context->handler = next;
					return;
				}
			}
		}
		next = next->next;
	}
	context->handler = NULL;
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
			if (context->ctxResponse._authorized != 200)
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
	
	if (context->_requestMethod == METHOD_GET)
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
		LWIP_sprintf(context->ctxResponse._sendBuffer, (char*)response_header_generic, HttpCodeInfo, "text/html", 0, "close");
		return;
	}
	
	if (context->handler != NULL)
	{
		if (context->handler->SetResponseHeader != NULL)
		{
			context->handler->SetResponseHeader(context, HttpCodeInfo);
			return;
		}
	}
	
	if (context->_requestMethod == METHOD_GET)
	{
		LWIP_sprintf(context->ctxResponse._sendBuffer, (char*)response_header_generic, HttpCodeInfo, "text/html", 0, "close");
	}
	else if (context->_requestMethod == METHOD_POST)
	{
		LWIP_sprintf(context->ctxResponse._sendBuffer, (char*)response_header_generic, HttpCodeInfo, "text/html", 0, "keep-alive");
	}
	else
	{
		LWIP_sprintf(context->ctxResponse._sendBuffer, (char*)response_header_generic, HttpCodeInfo, "text/html", 0, "close");
	}
}

int CGI_LoadContentToSend(REQUEST_CONTEXT* context, int caller) //load response body chunk by chunk
{
	int needSend = 0;

	if (context->_requestMethod == METHOD_GET)
	{
		if (context->ctxResponse._dwOperStage >= STAGE_END)
		{
			if (caller == HTTP_PROC_CALLER_SENT)
			{ //last chunk sent
				LogPrint(LOG_DEBUG_ONLY, "Response done @%d", context->_sid);
				
				context->_state = HTTP_STATE_REQUEST_END;
				
				if (context->handler->OnAllSent != NULL)
					context->handler->OnAllSent(context);
				return needSend;
			}
		}
	}
	
	if (context->handler != NULL)
	{
		if (context->handler->LoadContentToSend != NULL)
		{
			return context->handler->LoadContentToSend(context);
		}
	}
	
	context->_state = HTTP_STATE_REQUEST_END; //no body to send, just end
	return needSend;
}
