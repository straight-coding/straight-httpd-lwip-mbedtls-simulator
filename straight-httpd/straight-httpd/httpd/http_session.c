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

#if !NO_SYS
static sys_mutex_t		g_sessionMutex;
#endif
static SESSION			g_httpSessions[MAX_SESSIONS];

/////////////////////////////////////////////////////////////////////////////////////////////////////
void SetupSession(void)
{
	memset(g_httpSessions, 0, sizeof(g_httpSessions)); //only when the task started
	if (sys_mutex_new(&g_sessionMutex) != ERR_OK) {}
}

SESSION* SessionCreate(char* outCookie, unsigned long remoteIP)
{
	int i;
	SESSION* session = NULL;
	*outCookie = 0;

	sys_mutex_lock(&g_sessionMutex);
	for (i = 0; i < MAX_SESSIONS; i++)
	{
		if (g_httpSessions[i]._token[0] != 0)
			continue;

		memset(&g_httpSessions[i], 0, sizeof(SESSION));

		LWIP_sprintf(outCookie, "SID=%08lX", LWIP_GetTickCount());

		g_httpSessions[i]._tLastReceived = LWIP_GetTickCount();
		if (g_httpSessions[i]._tLastReceived == 0)
			g_httpSessions[i]._tLastReceived = 1;

		g_httpSessions[i]._nLoginIP = remoteIP;
		strcpy(g_httpSessions[i]._token, outCookie);

		LogPrint(0, "Session created @ %s from %08lX", g_httpSessions[i]._token, g_httpSessions[i]._nLoginIP);

		session = &g_httpSessions[i];
		break;
	}
	sys_mutex_unlock(&g_sessionMutex);

	if (session == NULL)
		LogPrint(0, "Session full, from %08lX", remoteIP);

	return session;
}

void SessionKill(SESSION* session) //called by IsContextTimeout
{
	if (session == NULL)
		return;

	sys_mutex_lock(&g_sessionMutex);
		if (session->_token[0] != 0)
		{
			LogPrint(0, "Session killed @ %s from %08lX", session->_token, session->_nLoginIP);
			memset(session, 0, sizeof(SESSION));
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
/*
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
	return 0;*/
}

SESSION* GetSession(char* token)
{
	int i;
	if ((token != NULL) && (token[0] != 0))
	{
		int nTotalCount = 0;
		for (i = 0; i < MAX_SESSIONS; i++)
		{
			if (g_httpSessions[i]._token[0] == 0)
				continue;

			LogPrint(0, "Compare session: [%s] ? [%s]", token, g_httpSessions[i]._token);
			nTotalCount ++;
			
			if (strstr(token, g_httpSessions[i]._token) != NULL)
			{
				LogPrint(0, "Session found: %s from %08lX", g_httpSessions[i]._token, g_httpSessions[i]._nLoginIP);
				return &g_httpSessions[i];
			}
		}
		LogPrint(0, "No such a session: %s, total=%d", token, nTotalCount);
	}
	return NULL;
}

void SessionCheck(long long tick) //called when header 'X-Auth-Token' or 'Cookie' received, also called after all headers received
{
	int i;
	sys_mutex_lock(&g_sessionMutex);
	{
		unsigned long long recvElapsed;
		unsigned long long sendElapsed;

		for (i = 0; i < MAX_SESSIONS; i++)
		{
			if (g_httpSessions[i]._token[0] == 0)
				continue;

			recvElapsed = tick;
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

			if ((GetSessionTimeout() > 0) && (((sendElapsed < recvElapsed) ? sendElapsed : recvElapsed) > (unsigned long)GetSessionTimeout()))
			{
				LogPrint(0, "Session killed (timeout) @ %s from %08lX", g_httpSessions[i]._token, g_httpSessions[i]._nLoginIP);

				memset(&g_httpSessions[i], 0, sizeof(SESSION));
			}
		}
	}
	sys_mutex_unlock(&g_sessionMutex);
}

void SessionClearAll(void) //called at the very beginning of AppThread
{
	sys_mutex_lock(&g_sessionMutex);
		memset(g_httpSessions, 0, sizeof(g_httpSessions));
	sys_mutex_unlock(&g_sessionMutex);
}

void SessionReceived(SESSION* session) //called by OnHttpReceive
{
	if (session == NULL)
		return;

	sys_mutex_lock(&g_sessionMutex);
		if (session->_token[0] != 0)
		{
			session->_tLastReceived = LWIP_GetTickCount();
			if (session->_tLastReceived == 0)
				session->_tLastReceived++;

		}
	sys_mutex_unlock(&g_sessionMutex);
}

void SessionSent(SESSION* session) //called by OnHttpSent
{
	if (session == NULL)
		return;

	sys_mutex_lock(&g_sessionMutex);
		if (session->_token[0] != 0)
		{
			session->_tLastSent = LWIP_GetTickCount();
			if (session->_tLastSent == 0)
				session->_tLastSent++;
		}
	sys_mutex_unlock(&g_sessionMutex);
}
