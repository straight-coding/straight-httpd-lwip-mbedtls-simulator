/*
  http_core.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#include "lwip/opt.h"

#include "lwip/altcp.h"

#include "http_core.h"
#include "http_cgi.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

#define HTTP_TCP_PRIO         TCP_PRIO_MIN
#define HTTP_POLL_INTERVAL    4 //timer interval = 4*500ms = 2s

/////////////////////////////////////////////////////////////////////////////////////////////////////

extern void LogPrint(int level, char* format, ... );
extern long ston(u8_t* s);
extern int  Strnicmp(char *str1, char *str2, int n);

extern struct tcp_pcb *tcp_active_pcbs;
extern struct tcp_pcb *tcp_tw_pcbs;
extern void tcp_kill_timewait(void);

/////////////////////////////////////////////////////////////////////////////////////////////////////

/** Serve one HTTP connection accepted in the http thread */
signed char HttpRequestProc(REQUEST_CONTEXT* context, int caller);
signed char HttpResponse(REQUEST_CONTEXT* context, int caller);

sys_mutex_t		g_sessionMutex;
SESSION			g_httpSessions[MAX_SESSIONS];
REQUEST_CONTEXT g_httpContext[MAX_CONNECTIONS];
sys_mutex_t	    g_httpMutex[MAX_CONNECTIONS];

signed char OnHttpAccept(void *arg, struct altcp_pcb *pcb, signed char err);
signed char OnHttpReceive(void *arg, struct altcp_pcb *pcb, struct pbuf *p, signed char err);
signed char OnHttpPoll(void *arg, struct altcp_pcb *pcb);
signed char OnHttpSent(void *arg, struct altcp_pcb *pcb, u16_t len);
void  OnHttpError(void *arg, signed char err);

static const char* Response_Status_Lines[] = {
	// 1xx: Informational - Request received, continuing process 
		"000 Unknown Error",
		"100 Continue",
		"101 Switching Protocols",
	// 2xx: Success - The action was successfully received, understood, and accepted 
		"200 OK",
		"201 Created",
		"202 Accepted",
		"203 Non-Authoritative Information",
		"204 No Content",
		"205 Reset Content",
		"206 Partial Content",
	// 3xx: Redirection - Further action must be taken in order to complete the request
		"300 Multiple Choices",
		"301 Moved Permanently",
		"302 Found",
		"303 See Other",
		"304 Not Modified",
		"305 Use Proxy",
		"307 Temporary Redirect",
	// 4xx: Client Error - The request contains bad syntax or cannot be fulfilled 
		"400 Bad Request",
		"401 Unauthorized",
		"402 Payment Required",
		"403 Forbidden",
		"404 Not Found",
		"405 Method Not Allowed",
		"406 Not Acceptable",
		"407 Proxy Authentication Required",
		"408 Request Time-out",
		"409 Conflict",
		"410 Gone",
		"411 Length Required",
		"412 Precondition Failed",
		"413 Request Entity Too Large",
		"414 Request-URI Too Large",
		"415 Unsupported Media Type",
		"416 Requested range not satisfiable",
		"417 Expectation Failed",
	// 5xx: Server Error - The server failed to fulfill an apparently valid request 
		"500 Internal Server Error",
		"501 Not Implemented",
		"502 Bad Gateway",
		"503 Service Unavailable",
		"504 Gateway Time-out",
		"505 HTTP Version not supported",
		""
};

const char header_nocache[] = "Cache-Control: no-cache, no-store, must-revalidate\r\nPragma: no-cache\r\nExpires: 0\r\n";
const char header_generic[] = "HTTP/1.1 %s\r\nContent-type: %s\r\nContent-Length: %d\r\nConnection: %s\r\n";
const char header_chunked[] = "HTTP/1.1 %s\r\nContent-type: %s\r\nTransfer-Encoding: chunked\r\n%sConnection: %s\r\n";
const char redirect_body1[] = "<html><head/><body><script>location.href='";
const char redirect_body2[] = "';</script></body></html>";

/////////////////////////////////////////////////////////////////////////////////////////////////////

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

		if ((context->handler->options & CGI_OPT_AUTHENTICATOR) != 0)
		{
			//context->_session = GetSession(context->ctxResponse._token);
		}
		else if ((context->handler->options & CGI_OPT_AUTH_REQUIRED) != 0)
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
		if ((context->handler->options & CGI_OPT_AUTH_REQUIRED) != 0)
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
		if ((context->handler->options & CGI_OPT_AUTH_REQUIRED) != 0)
		{
			context->_session->_tLastSent = LWIP_GetTickCount();
			if (context->_session->_tLastSent == 0)
				context->_session->_tLastSent++;
		}
	}
	sys_mutex_unlock(&g_sessionMutex);
}

void PrintLwipStatus(void)
{
	int activeCount = 0;
	int waitCount = 0;
	struct tcp_pcb *pcb;

	tcp_kill_timewait();

	for (pcb = tcp_active_pcbs; pcb != NULL; ) //active
	{
		activeCount ++;
		pcb = pcb->next;
	}
	
	for (pcb = tcp_tw_pcbs; pcb != NULL; ) //TIME_WAIT
	{
		waitCount ++;
		pcb = pcb->next;
	}
	
	LogPrint(0, "TCP summary: active=%d, wait=%d", activeCount, waitCount);
}

void SetupHttpContext(void)
{
	int i;

	LogPrint(0, "size of REQUEST_CONTEXT: %d\r\n", sizeof(REQUEST_CONTEXT));
	LogPrint(0, "size of SESSION: %d\r\n", sizeof(SESSION));
	LogPrint(0, "size of CGI_Mapping: %d\r\n", sizeof(struct CGI_Mapping));

	memset(g_httpSessions, 0, sizeof(g_httpSessions)); //only when the task started
	memset(g_httpContext, 0, sizeof(g_httpContext)); //only when the task started
	
	for(i = 0; i < MAX_CONNECTIONS; i ++)
	{
		if (sys_mutex_new(&g_httpMutex[i]) != ERR_OK) {}
			
		g_httpContext[i]._sid = i;	
		g_httpContext[i]._pMutex = &g_httpMutex[i];
	}

	if (sys_mutex_new(&g_sessionMutex) != ERR_OK) {}
		
	CGI_SetupMapping();
}

REQUEST_CONTEXT* GetHttpContext(void)
{
	int i;
	for(i = 0; i < MAX_CONNECTIONS; i ++)
	{
		if (g_httpContext[i]._pcb == NULL)
		{
			LogPrint(0, "New connection @%d", g_httpContext[i]._sid);
			
			if (g_httpContext[i]._reqBufList != NULL)
				pbuf_free(g_httpContext[i]._reqBufList);
			g_httpContext[i]._reqBufList = NULL;
			g_httpContext[i]._bufOffset = 0;
			g_httpContext[i]._peer_closing = 0;

			ResetHttpContext(&g_httpContext[i]);

			g_httpContext[i]._tLastReceived = LWIP_GetTickCount();
			if (g_httpContext[i]._tLastReceived == 0)
				g_httpContext[i]._tLastReceived++;
			return &g_httpContext[i];
		}
	}
	LogPrint(0, "No available connection");
	return NULL;
}

int IsContextTimeout(REQUEST_CONTEXT* context) //called by OnHttpPoll
{
	unsigned long recvElapsed = LWIP_GetTickCount();
	unsigned long sendElapsed = recvElapsed;

	if (recvElapsed == 0) recvElapsed = 1;
	if (sendElapsed == 0) sendElapsed = 1;

	if (context == NULL)
		return -1;
	
	if (context->_tLastReceived != 0)
	{
		if (recvElapsed >= context->_tLastReceived)
			recvElapsed -= context->_tLastReceived;
		else
			recvElapsed += (0xFFFFFFFF - context->_tLastReceived);
	}
	else
		recvElapsed = 0;
		
	if (context->ctxResponse._tLastSent != 0)
	{
		if (sendElapsed >= context->ctxResponse._tLastSent)
			sendElapsed -= context->ctxResponse._tLastSent;
		else
			sendElapsed += (0xFFFFFFFF - context->ctxResponse._tLastSent);
	}
	else
		sendElapsed = 0;
	
	if (context->_state < HTTP_STATE_BODY_DONE) //request body done
	{ //receiving
		if (recvElapsed > context->_nReceiveTimeout)
		{
			LogPrint(0, "Receiving timeout (>=%d ms) @%d", recvElapsed, context->_sid);
			return 1;
		}
	}
	else 
	{ //sending
		if (sendElapsed > context->ctxResponse._nSendTimeout)
		{
			LogPrint(0, "Sending timeout (>=%d ms) @%d", sendElapsed, context->_sid);
			return 1;
		}
	}
	return 0;
}

void ResetHttpContext(REQUEST_CONTEXT* context)
{ //called when every request finished
	LockSession(context);
		//if (context->_reqBufList != NULL)
			//pbuf_free(context->_reqBufList);
		//context->_reqBufList = NULL;
		//context->_bufOffset = 0;

		context->_state = HTTP_STATE_IDLE;
		
		context->_tRequestStart = 0;
		context->_tLastReceived = 0;
		
		context->_requestMethod = -1;

		//context->_killing = 0;

		context->_expect00 = -1;
		context->_chunked = -1;

		context->_contentLength = -1;
		context->_contentReceived = 0;
		context->_keepalive = 0;

		context->_result = 0;
		
		memset(&context->ctxResponse, 0, sizeof(RESPONSE_CONTEXT));
		context->ctxResponse._nSendTimeout = TO_SENT;
		context->_nReceiveTimeout = TO_RECV;

		memset(context->_ver, 0, sizeof(context->_ver));
		memset(context->_extension, 0, sizeof(context->_extension));
		memset(context->_requestPath, 0, sizeof(context->_requestPath));
		memset(context->_responsePath, 0, sizeof(context->_responsePath));

		context->handler = NULL;
		if (context->_file2Get != NULL)
			LWIP_fclose(context->_file2Get);
		context->_file2Get = NULL;
	
		//can't clear right now because of pipline
		//context->request_length = 0;
		//memset(context->http_request_buffer, 0, sizeof(context->http_request_buffer));
		context->max_level = MAX_REQ_BUF_SIZE;
		
		LogPrint(LOG_DEBUG_ONLY, "Reset connection @%d", context->_sid);
		
	UnlockSession(context);
}

void CloseHttpContext(REQUEST_CONTEXT* context)
{
	signed char err;
	
	if (context == NULL)
		return;
	
	LockSession(context);
		if (context->_pcb != NULL)
		{
			altcp_arg(context->_pcb, NULL);
			
			altcp_recv(context->_pcb, NULL);
			altcp_err(context->_pcb, NULL);
			altcp_sent(context->_pcb, NULL);
			
			altcp_poll(context->_pcb, NULL, 0);

			err = altcp_close(context->_pcb);
			if (err != ERR_OK) 
			{ //_pcb not freed, try later, but detach context
				LogPrint(0, "Error %d closing %p\n", err, (void *)context->_pcb);

				altcp_poll(context->_pcb, OnHttpPoll, HTTP_POLL_INTERVAL);
			}
			else
			{ //pcb closed
				context->_pcb = NULL;
				context->_peer_closing = 0;
				LogPrint(0, "Close connection @%d", context->_sid);
			}
		}
		else
		{
			LogPrint(0, "Close connection @%d, pcb is null", context->_sid);
		}
	UnlockSession(context);

	FreeHttpContext(context);
		
	PrintLwipStatus();
}

void FreeHttpContext(REQUEST_CONTEXT* context)
{
	LockSession(context);
		CGI_Finish(context);
	
		if (context->_reqBufList != NULL)
			pbuf_free(context->_reqBufList);
		context->_reqBufList = NULL;
		context->_bufOffset = 0;

		context->_state = HTTP_STATE_IDLE;
		context->_tRequestStart = 0;
	
		//OS_MUTEX*	_pMutex;
		context->_pcb = NULL;

		context->_requestMethod = -1;
		context->_contentLength = -1;

		context->_keepalive = 0;
		context->_chunked = 0;

		context->_contentReceived = 0;
		context->_result = 0;

		memset(&context->ctxResponse, 0, sizeof(RESPONSE_CONTEXT));
	
		memset(context->_ver, 0, sizeof(context->_ver));
		memset(context->_extension, 0, sizeof(context->_extension));
		memset(context->_requestPath, 0, sizeof(context->_requestPath));
		memset(context->_responsePath, 0, sizeof(context->_responsePath));

		context->handler = NULL;
		if (context->_file2Get != NULL)
			LWIP_fclose(context->_file2Get);
		context->_file2Get = NULL;
	
		context->request_length = 0;
		context->max_level = MAX_REQ_BUF_SIZE;
		memset(context->http_request_buffer, 0, sizeof(context->http_request_buffer));
		
		LogPrint(0, "Free connection @%d", context->_sid);
	UnlockSession(context);
}

struct altcp_pcb* HttpdInit(unsigned long port)
{
	signed char err;
	struct altcp_pcb* listen = NULL;

	LogPrint(0, "HttpdInit @ %d", port);

	listen = altcp_tcp_new_ip_type(IPADDR_TYPE_ANY);
	if (listen) 
	{
		altcp_setprio(listen, HTTP_TCP_PRIO);
		
		err = altcp_bind(listen, IP_ANY_TYPE, port);
		listen = altcp_listen_with_backlog_and_err(listen, 2, &err);
		altcp_accept(listen, OnHttpAccept);
		
		return listen;
	}
	return NULL;
}

int HttpdStop(struct altcp_pcb *pcbListen)
{
	int i;
	
	altcp_close(pcbListen);
	
	for(i = 0; i < MAX_CONNECTIONS; i ++)
	{
		FreeHttpContext(&g_httpContext[i]);
	}
	PrintLwipStatus();
	
	return ERR_OK;
}


/** Function prototype for tcp accept callback functions. Called when a new
 * connection can be accepted on a listening pcb.
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param newpcb The new connection pcb
 * @param err An error code if there has been an error accepting.
 *     Only return ERR_ABRT if you have called tcp_abort from within the callback function!
 *     Return ERR_OK to continue receiving
 * 	   Otherwise, tcp_abort() will be called by stack out of this callback function
 */
signed char OnHttpAccept(void *pcbListener, struct altcp_pcb *pcbAccepted, signed char errIn) //errIn=ERR_MEM or ERR_OK
{ //arg ==> g_pcbListen
	REQUEST_CONTEXT* context = NULL;
	
	LWIP_UNUSED_ARG(errIn);
	LWIP_UNUSED_ARG(pcbListener);
	
	LogPrint(LOG_DEBUG_ONLY, "OnHttpAccept %p / %p\n", (void *)pcbAccepted, pcbListener);
	PrintLwipStatus();

	if ((errIn != ERR_OK) || (pcbAccepted == NULL)) //pcbAccepted should be not NULL
	{ //no listen or ERR_MEM, pcbAccepted == NULL
		return ERR_VAL; //abort pcbAccepted after return
	}

	/* Set priority */
	altcp_setprio(pcbAccepted, TCP_PRIO_NORMAL);
	
	/*Set TF_NODELAY*/
	tcp_nagle_disable(pcbAccepted); //set TF_NODELAY
	//altcp_nagle_enable(pcbAccepted); //clear TF_NODELAY

	/* Allocate memory for the structure that holds the state of the connection - initialized by that function. */
	context = GetHttpContext();
	if (context == NULL)
	{
		LogPrint(LOG_DEBUG_ONLY, ("http_accept: Out of memory, RST\n"));
		return ERR_MEM; //abort pcbAccepted after return
	}

	context->_pcb = pcbAccepted;
	
	/* Tell TCP that this is the structure we wish to be passed for our callbacks. */
	altcp_arg(pcbAccepted, context);

	/* Set up the various callback functions */
	altcp_recv(pcbAccepted, OnHttpReceive);
	altcp_err (pcbAccepted, OnHttpError);
	altcp_poll(pcbAccepted, OnHttpPoll, HTTP_POLL_INTERVAL);
	altcp_sent(pcbAccepted, OnHttpSent);

	return ERR_OK;
}

/** Function prototype for tcp receive callback functions. Called when data has been received.
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb which received data
 * @param p The received data (or NULL when the connection has been closed!)
 * @param err An error code if there has been an error receiving, result of tcp_process()
 *    Only return ERR_ABRT if you have called tcp_abort from within the callback function!
 *    return ERR_OK if data eaten by app layer
 *    otherwise keep data in pcb
 */
signed char OnHttpReceive(void *arg, struct altcp_pcb *pcb, struct pbuf *p, signed char err) //err always ERR_OK currently
{ //return ERR_OK if data eaten; return IN_PROGRESS if data not eaten; return ERR_ABRT if tcp_abort before return
	signed char errRet = ERR_OK;
	
	REQUEST_CONTEXT* context = (REQUEST_CONTEXT*)arg;

	if ((err != ERR_OK) || (p == NULL) || (context == NULL)) 
	{ /* error or closed by other side? */
		if (context == NULL)
		{ //no session associated
			LogPrint(0, "OnHttpReceive, no connection, err=%d", err);
			if (p != NULL)
			{
				altcp_recved(pcb, p->tot_len);
				pbuf_free(p);
			}
			return ERR_OK;
		}
		if (p == NULL)
		{ // peer colsing
			LogPrint(0, "OnHttpReceive, closed by peer, err=%d @%d", err, context->_sid);
			if (context != NULL)
			{
				if (context->_state == HTTP_STATE_IDLE)
				{ //receiving the inbound request
					CloseHttpContext(context);
					return ERR_OK;
				}
				context->_peer_closing = 1;
			}
		}
		if (err != ERR_OK)
		{
			LogPrint(0, "OnHttpReceive, unknown err=%d", err);
		}
	}
	
	LockSession(context); //lock queue
	
	context->_tLastReceived = LWIP_GetTickCount();
	if (context->_tLastReceived == 0)
		context->_tLastReceived ++;

	if (context->ctxResponse._authorized == CODE_OK)
	{ //POST large data
		SessionReceived(context); //lock used inside
	}
	
	UnlockSession(context); //unlock queue

	if (p != NULL)
	{
		LockSession(context); //lock queue
		
		if (context->_reqBufList != NULL) 
		{ //enqueue pbuf
			pbuf_cat(context->_reqBufList, p);
			LogPrint(LOG_DEBUG_ONLY, "pbuf enqueued, size=%d, total=%d, @%d", p->tot_len, context->_reqBufList->tot_len, context->_sid);
			
			UnlockSession(context); //unlock queue
			
			errRet = ERR_OK;
		}
		else if (p->tot_len > context->max_level - context->request_length)
		{ //no enough space, enqueue pbuf
			//LogPrint(LOG_DEBUG_ONLY, "pbuf rejected, size=%d @%d", p->tot_len, context->_sid);
			LogPrint(LOG_DEBUG_ONLY, "first pbuf enqueued, size=%d @%d", p->tot_len, context->_sid);
			context->_reqBufList = p;

			UnlockSession(context); //unlock queue
			
			errRet = ERR_OK;
			//errRet = ERR_INPROGRESS;
		}
		else
		{ //consumed
			LogPrint(LOG_DEBUG_ONLY, "pbuf eaten, size=%d @%d", p->tot_len, context->_sid);
			
			pbuf_copy_partial(p, context->http_request_buffer + context->request_length, p->tot_len, 0);
			
			context->request_length += p->tot_len;
			context->http_request_buffer[context->request_length] = 0;
			context->http_request_buffer[context->request_length+1] = 0;
			
			UnlockSession(context); //unlock queue
			
			altcp_recved(pcb, p->tot_len); //move tcp window
			pbuf_free(p);
			
			errRet = ERR_OK;
		}	
	}
	
	if ((errRet == ERR_OK) || (errRet == ERR_INPROGRESS))
	{
		HttpRequestProc(context, HTTP_PROC_CALLER_RECV); //ResetHttpContext() or CloseHttpContext() may be already called
		if (context->_result < 0)
		{
			LogPrint(0, "Canceling by OnHttpReceive, result=%d, @%d", context->_result, context->_sid);
			CGI_Cancel(context);
		}
		
		if (context->_result <= -500)
		{
			LogPrint(0, "Close in OnHttpReceive, result=%d, @%d", context->_result, context->_sid);
			CloseHttpContext(context);
			
			return ERR_OK;
		}
	}

	return errRet;
}

/** Function prototype for tcp poll callback functions. Called periodically as
 * specified by @see tcp_poll.
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb tcp pcb
 * @return ERR_OK: try to send some data by calling tcp_output
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
signed char OnHttpPoll(void *arg, struct altcp_pcb *pcb)
{
	signed char err = ERR_OK;
	REQUEST_CONTEXT* context = (REQUEST_CONTEXT*)arg;
	
	if (context != NULL)
	{
		LogPrint(LOG_DEBUG_ONLY, "OnHttpPoll, buf=%d, err=%d  @%d", context->request_length, err, context->_sid);
		
		if (IsContextTimeout(context) > 0)
		{
			LogPrint(0, "OnHttpPoll connection timeout @%d", context->_sid);

			CGI_Cancel(context);
			
			CloseHttpContext(context);
			
			return ERR_OK;  //not aborted
		}
		HttpRequestProc(context, HTTP_PROC_CALLER_POLL); //ResetHttpContext() or CloseHttpContext() may be already called
		if (context->_result < 0)
		{
			LogPrint(0, "Canceling by OnHttpPoll, result=%d, @%d", context->_result, context->_sid);

			CGI_Cancel(context);
		}
		
		if (context->_result <= -500)
		{
			LogPrint(0, "Close in OnHttpPoll, result=%d, @%d", context->_result, context->_sid);
			CloseHttpContext(context);

			return ERR_OK;
		}
		
		return err;
	}

	//context == NULL, pcb is closing, but failed
	altcp_poll(pcb, NULL, 0);
	err = altcp_close(pcb); //retry to close
	if (err != ERR_OK) 
	{
		LogPrint(0, "OnHttpPoll error %d closing %p again, abort", err, (void *)pcb);
		altcp_abort(pcb);
		err = ERR_ABRT;
	}
	
	PrintLwipStatus();
	return err;
}

/** Function prototype for tcp sent callback functions. Called when sent data has
 * been acknowledged by the remote side. Use it to free corresponding resources.
 * This also means that the pcb has now space available to send new data.
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb for which data has been acknowledged
 * @param len The amount of bytes acknowledged
 * @return ERR_OK: try to send some data by calling tcp_output
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
signed char OnHttpSent(void *arg, struct altcp_pcb *pcb, u16_t len)
{
	signed char err = ERR_OK;
	
	REQUEST_CONTEXT* context = (REQUEST_CONTEXT*)arg;
	if (context != NULL)
	{
		LogPrint(LOG_DEBUG_ONLY, "OnHttpSent, size=%d, err=%d  @%d", len, err, context->_sid);
		
		context->ctxResponse._totalSent += len;

		HttpRequestProc(context, HTTP_PROC_CALLER_SENT); //ResetHttpContext() or CloseHttpContext() may be already called
		if (context->_result < 0)
		{
			LogPrint(0, "Canceling by OnHttpSent, result=%d, @%d", context->_result, context->_sid);

			CGI_Cancel(context);
		}
		
		if (context->_result <= -500)
		{
			LogPrint(0, "Close in OnHttpSent, result=%d, @%d", context->_result, context->_sid);
			CloseHttpContext(context);

			return ERR_OK;
		}
		
		//context->ctxResponse._tLastSent = LWIP_GetTickCount();
		//if (context->ctxResponse._tLastSent == 0)
			//context->ctxResponse._tLastSent ++;

		if (context->ctxResponse._authorized == CODE_OK)
		{ //GET large data
			SessionSent(context); //lock used inside
		}
		
		return err;
	}

	//context == NULL, pcb is closing, but failed
	altcp_poll(pcb, NULL, 0);
	err = altcp_close(pcb); //retry to close
	if (err != ERR_OK) 
	{
		LogPrint(0, "OnHttpSent Error %d closing %p again, abort", err, (void *)pcb);
		altcp_abort(pcb);
		err = ERR_ABRT;
	}
	
	PrintLwipStatus();
	return err;
}

/** Function prototype for tcp error callback functions. Called when the pcb
 * receives a RST or is unexpectedly closed for any other reason.
 * @note The corresponding pcb is already freed when this callback is called!
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param err Error code to indicate why the pcb has been closed
 *            ERR_ABRT: aborted through tcp_abort or by a TCP timer
 *            ERR_RST: the connection was reset by the remote host
 */
void OnHttpError(void *arg, signed char err)
{ //pcb already free
	REQUEST_CONTEXT* context = (REQUEST_CONTEXT*)arg;
	LWIP_UNUSED_ARG(err);

	if (context != NULL) 
	{
		LogPrint(0, "OnHttpError, err=%d @%d\n", err, context->_sid);

		CGI_Cancel(context);
		
		FreeHttpContext(context);
	}
	else
	{
		LogPrint(0, "OnHttpError err=%d\n", err);
	}
	PrintLwipStatus();
}

void SetKilling(REQUEST_CONTEXT* context)
{
	if (context->_pMutex != NULL)
		sys_mutex_lock(context->_pMutex);
	
	context->_killing = 1;
	
	if (context->_pMutex != NULL)
		sys_mutex_unlock(context->_pMutex);
}

int IsKilling(REQUEST_CONTEXT* context, int reset)
{
	int killing = 0;
	
	if (context->_pMutex != NULL)
		sys_mutex_lock(context->_pMutex);
	
	killing = context->_killing;
	if (reset > 0)
		context->_killing = 0;
	
	if (context->_pMutex != NULL)
		sys_mutex_unlock(context->_pMutex);
	
	return killing;
}

void LockSession(REQUEST_CONTEXT* context)
{
#if (0)
	if (context->_pMutex != NULL)
		sys_mutex_lock(context->_pMutex);
#endif
}

void UnlockSession(REQUEST_CONTEXT* context)
{
#if (0)
	if (context->_pMutex != NULL)
		sys_mutex_unlock(context->_pMutex);
#endif
}

void ParseQueryString(REQUEST_CONTEXT* context)
{
	int i = 0;
	int len = 0;
	int iDot=-1; //last
	int iQuestion=-1; //first
	int iHash=-1; //first
	
	memset(context->_extension, 0, sizeof(context->_extension));
	for(i = 0; i < strlen(context->_requestPath); i ++)
	{
		if (context->_requestPath[i] == '.')
		{
			if (iQuestion < 0)
			{
				iDot = i;
				len = 0;
				context->_extension[len] = 0;
			}
		}
		else if (context->_requestPath[i] == '?')
		{
			if (iQuestion < 0)
				iQuestion = i;
		}
		else if (context->_requestPath[i] == '#')
		{
			if (iHash < 0)
				iHash = i;
		}
		else if (context->_requestPath[i] == '/')
		{
			iDot = -1;
			len = 0;
			context->_extension[len] = 0;
		}
		else
		{
			if ((iDot >= 0) && (iQuestion < 0))
			{
				context->_extension[len] = context->_requestPath[i];
				len ++;
				context->_extension[len] = 0;
			}
		}
	}
	//LogPrint(LOG_DEBUG_ONLY, "Extension: %s", context->_extension);
	/*
	if (context->file2Get.data != NULL)
	{
		int i = 0;
		char* p = (char*)context->file2Get.data;
		context->nContentOffset = 0;
		for(i = 0; i < context->file2Get.len-4; i ++)
		{
			if ((p[i+0] == 0xd) && (p[i+1] == 0xa) && 
				(p[i+2] == 0xd) && (p[i+3] == 0xa))
			{
				context->nContentOffset = i+4;
				break;
			}
		}
	}*/
}

signed char HttpRequestProc(REQUEST_CONTEXT* context, int caller) //always return ERR_OK
{ //return 0, or http error code
	char* buffer = context->http_request_buffer;
	
	int i, j;
	int afterHeader = -1;
	int size = 0;
	int consumed = 0;
	
	while(context->_state < HTTP_STATE_BODY_DONE)
	{
		LockSession(context); //unlock queue
			if (context->_reqBufList != NULL)
			{
				int nSpace = context->max_level - context->request_length;
				if (nSpace > context->_reqBufList->tot_len - context->_bufOffset)
					nSpace = context->_reqBufList->tot_len - context->_bufOffset;
				
				if (nSpace > 0)
				{
					LogPrint(LOG_DEBUG_ONLY, "Dequeued from chain: %d bytes @%d", nSpace, context->_sid);
					
					pbuf_copy_partial(context->_reqBufList, context->http_request_buffer + context->request_length, nSpace, context->_bufOffset);
					
					context->_bufOffset += nSpace;
					context->request_length += nSpace;
					
					context->http_request_buffer[context->request_length] = 0;
					context->http_request_buffer[context->request_length+1] = 0;
					
					if (context->_bufOffset == context->_reqBufList->tot_len)
					{ //completely consumed, free the whole chain
						LogPrint(LOG_DEBUG_ONLY, "Free whole chain: %d bytes @%d", context->_reqBufList->tot_len, context->_sid);
						
						altcp_recved(context->_pcb, context->_reqBufList->tot_len); //move tcp window
						pbuf_free(context->_reqBufList);
						
						context->_reqBufList = NULL;
						context->_bufOffset = 0;
					}
					else if (context->_bufOffset != 0)
					{ //check freeable buffer
						struct pbuf *p;
						struct pbuf* succeeding;

						p = context->_reqBufList;
						while(p != NULL) 
						{
							if (context->_bufOffset < p->len)
								break;
							
							succeeding = p->next;
							p->next = NULL;
							
							succeeding->tot_len = (u16_t)(p->tot_len - p->len);
							p->tot_len = p->len;
							context->_bufOffset -= p->len;
							
							LogPrint(LOG_DEBUG_ONLY, "Free pbuf from chain: %d bytes, remain=%d @%d", p->tot_len, succeeding->tot_len, context->_sid);
							
							altcp_recved(context->_pcb, p->tot_len);
							pbuf_free(p);
							
							context->_reqBufList = succeeding;
							p = succeeding;
						}
					}
				}
			}
		
			size = context->request_length;
		UnlockSession(context); //unlock queue
		
		//LogPrint(LOG_DEBUG_ONLY, "Session recv buffer: %d bytes @%d", size, context->_sid);
		if (size == 0)
			break;
		
		//processing data in the session buffer
		consumed = 0;
		if ((size > 0) && (context->_state == HTTP_STATE_IDLE))
		{
			context->_state = HTTP_STATE_HEADER_RECEIVING;
			
			context->_tRequestStart = LWIP_GetTickCount();
			if (context->_tRequestStart == 0)
				context->_tRequestStart = 1;

			context->_tLastReceived = LWIP_GetTickCount();
			if (context->_tLastReceived == 0)
				context->_tLastReceived = 1;

			context->ctxResponse._tLastSent = 0;
			
			LogPrint(LOG_DEBUG_ONLY, "new request start tick: %d @%d", context->_tRequestStart, context->_sid);
		}
		
		if (context->_state == HTTP_STATE_HEADER_RECEIVING)
		{ //parsing the whole buffer for request header
			afterHeader = 0;
			if (size >= 2)
			{ //at lease the line end
				int nLinePos = 0;
				for(i = 0; i < size-1; i ++)
				{
					if ((buffer[i] != '\r') || ((buffer[i+1] != '\n')))
						continue;
					
					//found a line
					if (i == nLinePos)
					{ //empty line
						if (context->_requestMethod < 0)
						{
							LogPrint(LOG_DEBUG_ONLY, "Method not found @%d", context->_sid);
							context->_result = -500; //first line is empty, need ABORT
						}
						
						//context->_requestHeader = 1;
						context->ctxResponse._cmdType = 0;
						context->ctxResponse._authorized = 0;
						
						context->ctxResponse._sending = 0;
						context->ctxResponse._total2Send = 0;
						context->ctxResponse._totalSent  = 0;
						context->ctxResponse._dwOperIndex = 0;
						context->ctxResponse._dwOperStage = 0;
						
						context->ctxResponse._bytesLeft = 0;
						context->ctxResponse._sendBuffer[0] = 0; //used for strlen

						afterHeader = 1;
						
						context->_state = HTTP_STATE_HEADER_DONE;
						//LogPrint(LOG_DEBUG_ONLY, "Header done @%d", context->_sid);

						nLinePos = (i + 2);
						break; //break line parser
					}
					else
					{ //valid line
						buffer[i] = 0; //for "\r\n", set "\r" to 0
						//LogPrint(LOG_DEBUG_ONLY, "%s\r\n", buffer + nLinePos);
						
						if (context->_requestMethod < 0)
						{ //first line, find method and path
							int pathLen = 0;
							int iSkip = 0;
							if ((buffer[nLinePos+0]=='G') && (buffer[nLinePos+1]=='E') && (buffer[nLinePos+2]=='T') && (buffer[nLinePos+3]==' '))
							{ //GET
								iSkip = 4;
								context->_requestMethod = METHOD_GET;
							}
							else if ((buffer[nLinePos+0]=='P') && (buffer[nLinePos+1]=='O') && (buffer[nLinePos+2]=='S') && (buffer[nLinePos+3]=='T') && (buffer[nLinePos+4]==' '))
							{ //POST
								iSkip = 5;
								context->_requestMethod = METHOD_POST;
							}
							else
							{
								context->_result = -500; //405	Method Not Allowed
								break; //break on error
							}
							
							if (iSkip > 0)
							{ //valid method and path
								for(j = nLinePos+iSkip; j < i; j ++) //i end @ "\r"
								{ //searching " HTTP/1."
									if ((buffer[j+0]==' ') && (buffer[j+1]=='H') && (buffer[j+2]=='T') && (buffer[j+3]=='T') && (buffer[j+4]=='P') && 
										(buffer[j+5]=='/') && (buffer[j+6]=='1') && (buffer[j+7]=='.'))
									{ //found " HTTP/1."
										memcpy(context->_ver, buffer+j+6, 3);
										context->_ver[3] = 0; //"1,0" or "1.1"
										break;
									}
									else
									{
										if ((pathLen == 0) && (buffer[j] == ' '))
										{ //remove the leading space
										}
										else
										{
											if (pathLen < sizeof(context->_requestPath)-2)
											{
												context->_requestPath[pathLen] = buffer[j];
												pathLen ++;
											}
										}
									}
								}
							}
							context->_requestPath[pathLen] = 0;
							
							//memset(&context->file2Get, 0, sizeof(context->file2Get));
							strcpy(context->_responsePath, context->_requestPath);
							
							CGI_SetCgiHandler(context);
							
							ParseQueryString(context); //parameters in request line
						}
						else if (Strnicmp(buffer+nLinePos, "Content-Length:", 15) == 0)
						{
							context->_contentLength = ston((u8_t*)buffer+nLinePos+15);
							context->_contentReceived = 0;
						}
						else if (Strnicmp(buffer+nLinePos, "Connection:", 11) == 0)
						{
							context->_keepalive = 0;
							if (strstr((char*)buffer+nLinePos+11, "keep-alive") != NULL)
								context->_keepalive = 1;
							else if (strstr((char*)buffer+nLinePos+11, "Keep-Alive") != NULL)
								context->_keepalive = 1;
						}
						else if (Strnicmp(buffer+nLinePos, "Transfer-Encoding:", 18) == 0)
						{
							context->_chunked = 0;
							if (strstr((char*)buffer+nLinePos+18, "chunked") != NULL)
								context->_chunked = 1;
							else if (strstr((char*)buffer+nLinePos+18, "Chunked") != NULL)
								context->_chunked = 1;
						}
						else if (Strnicmp(buffer + nLinePos, "Expect:", 7) == 0)
						{
							context->_expect00 = 0;
							if (strstr((char*)buffer + nLinePos + 7, "100-continue") != NULL)
								context->_expect00 = 1;
							else if (strstr((char*)buffer + nLinePos + 7, "100-Continue") != NULL)
								context->_expect00 = 1;
						}
						else if ((Strnicmp(buffer + nLinePos, "X-Auth-Token:", 13) == 0) ||
								 (Strnicmp(buffer + nLinePos, "Cookie:", 7) == 0))
						{
							int code = 0;
							for(j = nLinePos; j < i; j ++)
							{
								if (code == 0)
								{
									if (buffer[j] == ':')
										code = 1;
								}
								else
								{
									if (buffer[j] != ' ')
										break;
								}
							}
							memset(context->ctxResponse._token, 0, sizeof(context->ctxResponse._token));
							strncpy(context->ctxResponse._token, (char*)buffer+j, sizeof(context->ctxResponse._token)-2);
							
							code = SessionCheck(context); //lock used inside
							if (code != CODE_OK)
								context->_result = code; //-403 Forbidden
						}
						else
						{
							CGI_HeaderReceived(context, buffer+nLinePos);
						}
					}
					nLinePos = (i + 2); //pos for the next line, consumed bytes
				} //for loop through [size] bytes in buffer
				
				size -= nLinePos;
				
				LockSession(context);
				if (nLinePos > 0)
				{
					context->request_length -= nLinePos;
					if (context->request_length > 0)
						memmove(context->http_request_buffer, context->http_request_buffer+nLinePos, context->request_length);
				}
				UnlockSession(context);
			} //endif size > 0
		} //endif parsing request header in session buffer

		//error in request
		if ((context->_result < 0) && (context->_result != CODE_REDIRECT))
		{ //not redirect
			//return context->_result; //error while receiving header, illegal method or no method, or forbidden
			LogPrint(0, "Failed to parse header, %d @%d", context->_result, context->_sid);
			break; //break on error
		}
		
		if (context->_state == HTTP_STATE_HEADER_RECEIVING)
		{ //header incomplete 
			if (context->request_length >= MAX_REQ_BUF_SIZE)
			{ //request header oversized
				context->_result = -500; //500 abort
				LogPrint(0, "Request header is too large, %d bytes @%d", context->request_length, context->_sid);
				break; //break on error
			}
		}
		
		if (context->_state < HTTP_STATE_HEADER_DONE)
		{
			if (context->_reqBufList != NULL)
				continue; //load more from the queue
			break; //break if no more data
		}
		
		//header already received, preparation to receive and parse post body
		if (afterHeader > 0)
		{ //just after the complete header, or prepare to receive post data or chunks
			context->max_level = TCP_MSS;
			context->ctxResponse._authorized = SessionCheck(context); //lock used inside
			
			if (context->_chunked > 0)
			{
				context->_state = HTTP_STATE_CHUNK_LEN_RECEIVING;
			}
			else 
			{
				if (context->_contentLength > 0)
					context->_state = HTTP_STATE_BODY_RECEIVING;
				else
				{
					context->ctxResponse._tLastSent = LWIP_GetTickCount(); //start sending timeout
					if (context->ctxResponse._tLastSent == 0)
						context->ctxResponse._tLastSent++;
					context->_state = HTTP_STATE_BODY_DONE; //request done, followed with response
				}
			}
			
			//LogPrint(LOG_DEBUG_ONLY, "Header done, request: %s, response: %s @%d", context->_requestPath, context->_responsePath, context->_sid);
			LogPrint(LOG_DEBUG_ONLY, "Header done, request: %s, @%d", context->_requestPath, context->_sid);
			
			CGI_HeadersReceived(context);
		} //end @ after header
		
		if (context->_result < 0)
		{ //if error, discard the following process then response
			context->_state = HTTP_STATE_SENDING_HEADER;
			break; //break to response
		}
		
		//chunk length receiving
		if ((context->_state == HTTP_STATE_CHUNK_LEN_RECEIVING) && (size > 2))
		{
			int c;
			for(c = 0; c < size-1; c ++)
			{
				if ((buffer[c] == '\r') || ((buffer[c+1] == '\n')))
				{
					buffer[c] = 0;
					
					context->_contentLength = atol(buffer)+2; //additional CRLF
					context->_contentReceived = 0;
					context->_state = HTTP_STATE_CHUNK_RECEIVING;
					
					LockSession(context);
						context->request_length -= (c+2); //skip chunk length
						if (context->request_length > 0)
							memmove(context->http_request_buffer, context->http_request_buffer+(c+2), context->request_length);
					UnlockSession(context);
					break;
				}
			}
		}
		
		//
		if ((context->_state == HTTP_STATE_CHUNK_RECEIVING) || 
			(context->_state == HTTP_STATE_BODY_RECEIVING))
		{
			consumed = CGI_ContentReceived(context, buffer, size);
			if (context->_result <= -500)
				return ERR_OK;
			
			if (consumed <= 0)
				break;
			
			LockSession(context);
				context->_contentReceived += consumed;
				context->request_length -= consumed;
				if (context->request_length > 0)
					memmove(context->http_request_buffer, context->http_request_buffer + consumed, context->request_length);
			UnlockSession(context);
			
			if (context->_contentReceived >= context->_contentLength)
			{ //chunk or body done
				if (context->_chunked > 0)
				{
					LogPrint(LOG_DEBUG_ONLY, "Chunk received: %d @%d", context->_contentLength, context->_sid);
					
					if (context->_contentLength == 2) //0 content + CRLF
					{
						context->ctxResponse._tLastSent = LWIP_GetTickCount(); //start sending timeout
						if (context->ctxResponse._tLastSent == 0)
							context->ctxResponse._tLastSent++;
						context->_state = HTTP_STATE_BODY_DONE;
					}
					else
						context->_state = HTTP_STATE_CHUNK_LEN_RECEIVING;
					
					context->_contentLength = 0; //additional CRLF
					context->_contentReceived = 0;
				}
				else
				{
					LogPrint(LOG_DEBUG_ONLY, "Body received: %d @%d", context->_contentLength, context->_sid);
					
					context->ctxResponse._tLastSent = LWIP_GetTickCount(); //start sending timeout
					if (context->ctxResponse._tLastSent == 0)
						context->ctxResponse._tLastSent++;
					context->_state = HTTP_STATE_BODY_DONE;
				}
			}
		}
		
		//request completely received or error
		if (context->_state == HTTP_STATE_BODY_DONE) //request done, followed with response
		{
			CGI_RequestReceived(context);

			if (context->_result == 0)
				context->_result = CODE_OK;
			break;
		}
		
		LockSession(context);
		if ((context->_reqBufList == NULL) || 
			(consumed <= 0))
		{
			UnlockSession(context);
			break;
		}
		UnlockSession(context); //continue processing data 
	} //end of parsing loop
	
	//request finished, send response now
	if (context->_state >= HTTP_STATE_BODY_DONE) //after received, and HTTP_STATE_SENDING_BODY
	{
		signed char err = HttpResponse(context, caller);
		if (context->_state == HTTP_STATE_REQUEST_END)
		{
			int keepalive = context->_keepalive;
			if (context->_peer_closing > 0)
			{
				LogPrint(0, "OnClose by peer1: @%d", context->_sid);
				CloseHttpContext(context);
			}
			else if (keepalive > 0)
			{
				ResetHttpContext(context);
			}
			else
			{
				LogPrint(0, "Close without keepalive: @%d", context->_sid);
				CloseHttpContext(context);
			}
		}
	}
	else
	{ //
		if (context->_peer_closing > 0)
		{
			LogPrint(0, "OnClose by peer2: @%d", context->_sid);
			context->_result = -500;
		}
	}
	return ERR_OK; //continue the session
}

signed char sendBuffered(REQUEST_CONTEXT* context)
{
	signed char err;
	
	int size2Send = context->ctxResponse._bytesLeft;
	int max2Send = altcp_sndbuf(context->_pcb);
	if (size2Send > max2Send)
		size2Send = max2Send;
	
	do
	{
		err = altcp_write(context->_pcb, context->ctxResponse._sendBuffer, size2Send, TCP_WRITE_FLAG_COPY);
		if (err == ERR_MEM)
		{
			if ((altcp_sndbuf(context->_pcb) == 0) || 
				(altcp_sndqueuelen(context->_pcb) >= TCP_SND_QUEUELEN))
			{
				size2Send = 1;
			}
			else
			{
				size2Send /= 2;
			}
			LogPrint(0, "[altcp_write] %d bytes, err=%d, @%d", size2Send, err, context->_sid);
		}
	}while((err == ERR_MEM) && (size2Send > 1));

	if (err == ERR_OK)
	{ //size2Send sent
		context->ctxResponse._total2Send += size2Send;
		context->ctxResponse._bytesLeft -= size2Send;

		if (context->ctxResponse._bytesLeft > 0)
			memmove(context->ctxResponse._sendBuffer, context->ctxResponse._sendBuffer + size2Send, context->ctxResponse._bytesLeft);
		context->ctxResponse._sendBuffer[context->ctxResponse._bytesLeft] = 0; //used for strlen

		context->ctxResponse._tLastSent = LWIP_GetTickCount();
		if (context->ctxResponse._tLastSent == 0)
			context->ctxResponse._tLastSent++;
	}

	return err;
}

signed char HttpResponse(REQUEST_CONTEXT* context, int caller) //always return ERR_OK
{
	signed char err = ERR_OK;

	if (context->_state < HTTP_STATE_BODY_DONE)
		return err;
	if (context->_result == 0)
		return err;

	if (context->ctxResponse._bytesLeft > 0)
	{
		LogPrint(LOG_DEBUG_ONLY, "Sending remaining: %d @%d", context->ctxResponse._bytesLeft, context->_sid);
		
		err = sendBuffered(context);
		if (err != ERR_OK)
		{ //send remaining data error
			context->_result = -500;
			context->_state = HTTP_STATE_REQUEST_END;
		}
		return err;
	}
	
	 //to send response header
	if (context->ctxResponse._sending == 0)
	{
		int i;
		int statusCode = 0;
		char code[6];
		char* codeNinfo;

		context->ctxResponse._dwOperStage = 0;
		context->ctxResponse._dwOperIndex = 0;
		context->ctxResponse._tLastSent = LWIP_GetTickCount();
		if (context->ctxResponse._tLastSent == 0)
			context->ctxResponse._tLastSent = 1;

		context->ctxResponse._sendMaxBlock = TCP_MSS;
		if (context->ctxResponse._sendMaxBlock > sizeof(context->ctxResponse._sendBuffer))
			context->ctxResponse._sendMaxBlock = sizeof(context->ctxResponse._sendBuffer) - 1;
		
		statusCode = abs(context->_result);
		LWIP_sprintf(code, "%03d", statusCode % 1000);

		i = 0;
		
		codeNinfo = code;
		while(Response_Status_Lines[i][0] != 0)
		{
			if (strncmp(code, Response_Status_Lines[i], 3) == 0)
			{
				codeNinfo = (char*)Response_Status_Lines[i];
				break;
			}
			i ++;
		}
		LogPrint(LOG_DEBUG_ONLY, "Sending response header: %s @%d", codeNinfo, context->_sid);

		CGI_SetResponseHeaders(context, codeNinfo);
		
		context->ctxResponse._bytesLeft = strlen(context->ctxResponse._sendBuffer);
		
		err = sendBuffered(context);
		if (err == ERR_OK)
		{
			context->ctxResponse._sending = 1;
			LogPrint(LOG_DEBUG_ONLY, "Response header sending out @%d", context->_sid);
		}
		else
		{ //failed to send header
			context->_state = HTTP_STATE_REQUEST_END;
			context->_result = -500;
			LogPrint(0, "Failed to send response header @%d", context->_sid);
			return ERR_OK;
		}
		
		context->_state = HTTP_STATE_SENDING_HEADER;
	}
	else if (context->_state == HTTP_STATE_SENDING_HEADER)
	{ //
		if (caller == HTTP_PROC_CALLER_SENT)
		{
			LogPrint(LOG_DEBUG_ONLY, "Response header was sent @%d", context->_sid);
			if (context->_result < 0)
			{
				context->_state = HTTP_STATE_REQUEST_END;
			}
			else
			{
				context->_state = HTTP_STATE_SENDING_BODY;
			}
		}
	}
	
	//sending response body
	if (context->_state == HTTP_STATE_SENDING_BODY)
	{
		int hasData2Send = CGI_LoadContentToSend(context, caller);
		if (hasData2Send > 0)
		{
			err = sendBuffered(context);
			if (err == ERR_OK)
			{
				LogPrint(LOG_DEBUG_ONLY, "Sending response body @%d", context->_sid);
				context->ctxResponse._sending = 1;
			}
			else
			{ //failed to send body
				context->_state = HTTP_STATE_REQUEST_END;
				context->_result = -500;
				LogPrint(0, "Failed to send response body @%d", context->_sid);
			}
		}
	}
	
	return ERR_OK;
}