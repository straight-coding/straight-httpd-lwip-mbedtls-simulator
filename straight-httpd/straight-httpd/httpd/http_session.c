/*
  http_session.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: June 11, 2020
*/
#include "lwip/opt.h"

#include "lwip/altcp.h"

#include "http_cgi.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

extern void LogPrint(int level, char* format, ... );
extern int  Strnicmp(char *str1, char *str2, int n);

/////////////////////////////////////////////////////////////////////////////////////////////////////

sys_mutex_t		g_sessionMutex;
SESSION			g_httpSessions[MAX_SESSIONS];

/////////////////////////////////////////////////////////////////////////////////////////////////////
void SetupSession(void)
{
	memset(g_httpSessions, 0, sizeof(g_httpSessions)); //only when the task started
	if (sys_mutex_new(&g_sessionMutex) != ERR_OK) {}
}

int SessionCreate(REQUEST_CONTEXT* context, char* outCookie)
{
	int result = 0;
	*outCookie = 0;

	sys_mutex_lock(&g_sessionMutex);
	for (int i = 0; i < MAX_SESSIONS; i++)
	{
		if (g_httpSessions[i]._token[0] != 0)
			continue;

		memset(&g_httpSessions[i], 0, sizeof(SESSION));

		LWIP_sprintf(outCookie, "SID=%08lX", LWIP_GetTickCount());

		g_httpSessions[i]._tLastReceived = LWIP_GetTickCount();
		if (g_httpSessions[i]._tLastReceived == 0)
			g_httpSessions[i]._tLastReceived = 1;

		g_httpSessions[i]._nUserIP = context->_pcb->remote_ip.addr;
		strcpy(g_httpSessions[i]._token, outCookie);

		LogPrint(0, "Session created @ %s from %08lX", g_httpSessions[i]._token, g_httpSessions[i]._nUserIP);

		result = 1;
		break;
	}
	sys_mutex_unlock(&g_sessionMutex);

	if (result == 0)
		LogPrint(0, "Session full, from %08lX", context->_pcb->remote_ip.addr);

	return result;
}

void SessionKill(REQUEST_CONTEXT* context, int matchIP) //called by IsContextTimeout
{
	sys_mutex_lock(&g_sessionMutex);
	for (int i = 0; i < MAX_SESSIONS; i++)
	{
		if (g_httpSessions[i]._token[0] == 0)
			continue;

		if (strstr(context->ctxResponse._token, g_httpSessions[i]._token) != NULL)
		{
			if ((matchIP == 0) || (context->_pcb->remote_ip.addr == g_httpSessions[i]._nUserIP))
			{
				LogPrint(0, "Session killed @ %s from %08lX", g_httpSessions[i]._token, g_httpSessions[i]._nUserIP);
				memset(&g_httpSessions[i], 0, sizeof(SESSION));
			}
			break;
		}
	}
	sys_mutex_unlock(&g_sessionMutex);
}

int SessionTypes(char* extension)
{
	if ((Strnicmp(extension, "css", 3) == 0) ||
		(Strnicmp(extension, "jpg", 3) == 0) ||
		(Strnicmp(extension, "png", 3) == 0) ||
		(Strnicmp(extension, "ico", 3) == 0) ||
		(Strnicmp(extension, "bmp", 3) == 0) ||
		(Strnicmp(extension, "svg", 3) == 0) ||
		(Strnicmp(extension, "gif", 3) == 0) ||
		(Strnicmp(extension, "woff", 4) == 0) ||
		(Strnicmp(extension, "ttf", 3) == 0) ||
		(Strnicmp(extension, "otf", 3) == 0) ||
		(Strnicmp(extension, "tiff", 4) == 0))
	{
		return 0;
	}
	else
	{
		return 1;
	}

	if ((extension[0] == 0) ||
		(Strnicmp(extension, "htm", 3) == 0) ||
		(Strnicmp(extension, "html", 4) == 0) ||
		(Strnicmp(extension, "shtml", 5) == 0) ||
		(Strnicmp(extension, "ssi", 3) == 0) ||
		(Strnicmp(extension, "cgi", 3) == 0) ||
		(Strnicmp(extension, "json", 4) == 0))
	{
		return 1;
	}
	return 0;
}

SESSION* GetSession(char* token)
{
	if ((token != NULL) && (token[0] != 0))
	{
		for (int i = 0; i < MAX_SESSIONS; i++)
		{
			if (g_httpSessions[i]._token[0] == 0)
				continue;

			if (strstr(token, g_httpSessions[i]._token) != NULL)
				return &g_httpSessions[i];
		}
	}
	return NULL;
}

int SessionCheck(REQUEST_CONTEXT* context) //called when header 'X-Auth-Token' or 'Cookie' received, also called after all headers received
{
	int result = CODE_OK;

	sys_mutex_lock(&g_sessionMutex);
	if (context == NULL)
	{
		unsigned long recvElapsed;
		unsigned long sendElapsed;

		for (int i = 0; i < MAX_SESSIONS; i++)
		{
			if (g_httpSessions[i]._token[0] == 0)
				continue;

			recvElapsed = LWIP_GetTickCount();
			sendElapsed = recvElapsed;

			if (recvElapsed == 0) recvElapsed = 1;
			if (sendElapsed == 0) sendElapsed = 1;

			if (g_httpSessions[i]._tLastReceived != 0)
			{
				if (recvElapsed >= g_httpSessions[i]._tLastReceived)
					recvElapsed -= g_httpSessions[i]._tLastReceived;
				else
					recvElapsed += (0xFFFFFFFF - g_httpSessions[i]._tLastReceived);
			}
			else
				recvElapsed = 0;

			if (g_httpSessions[i]._tLastSent != 0)
			{
				if (sendElapsed >= g_httpSessions[i]._tLastSent)
					sendElapsed -= g_httpSessions[i]._tLastSent;
				else
					sendElapsed += (0xFFFFFFFF - g_httpSessions[i]._tLastSent);
			}
			else
				sendElapsed = 0;

			if (((sendElapsed < recvElapsed) ? sendElapsed : recvElapsed) > TO_SESSION)
			{
				LogPrint(0, "Session killed (timeout) @ %s from %08lX", g_httpSessions[i]._token, g_httpSessions[i]._nUserIP);

				memset(&g_httpSessions[i], 0, sizeof(SESSION));
				result = CODE_OK;
			}
		}
	}
	else if (context->handler != NULL)
	{
		context->_session = GetSession(context->ctxResponse._token);
		if ((context->_options & CGI_OPT_AUTH_REQUIRED) != 0)
		{ //this scope needs authentication
			if (SessionTypes(context->_extension) > 0)
			{
				result = CODE_REDIRECT; //Temporary Redirect (since HTTP/1.1)

				//context->_session = GetSession(context->ctxResponse._token);
				if (context->_session != NULL)
					result = CODE_OK;
				if (result == CODE_REDIRECT)
				{
					context->ctxResponse._authorized = 0;
					strcpy(context->_responsePath, WEB_DEFAULT_PAGE);
				}
			}
		}
	}
	sys_mutex_unlock(&g_sessionMutex);

	return result;
}

void SessionClearAll() //called at the very beginning of AppThread
{
	sys_mutex_lock(&g_sessionMutex);
		memset(g_httpSessions, 0, sizeof(g_httpSessions));
	sys_mutex_unlock(&g_sessionMutex);
}

void SessionReceived(REQUEST_CONTEXT* context) //called by OnHttpReceive
{
	sys_mutex_lock(&g_sessionMutex);
	if ((context->_session != NULL) && (context->handler != NULL))
	{
		if ((context->_options & CGI_OPT_AUTH_REQUIRED) != 0)
		{
			context->_session->_tLastReceived = LWIP_GetTickCount();
			if (context->_session->_tLastReceived == 0)
				context->_session->_tLastReceived++;
		}
	}
	sys_mutex_unlock(&g_sessionMutex);
}

void SessionSent(REQUEST_CONTEXT* context) //called by OnHttpSent
{
	sys_mutex_lock(&g_sessionMutex);
	if ((context->_session != NULL) && (context->handler != NULL))
	{
		if ((context->_options & CGI_OPT_AUTH_REQUIRED) != 0)
		{
			context->_session->_tLastSent = LWIP_GetTickCount();
			if (context->_session->_tLastSent == 0)
				context->_session->_tLastSent++;
		}
	}
	sys_mutex_unlock(&g_sessionMutex);
}
