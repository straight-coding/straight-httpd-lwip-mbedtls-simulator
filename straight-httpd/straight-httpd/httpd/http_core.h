/*
  http_core.h
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#ifndef _HTTP_CORE_H_
#define _HTTP_CORE_H_

#include <string.h>

#include "lwip/opt.h"		//only for TCP_MSS definition
#include "lwip/sys.h"

#ifndef WIN32
#include "log.h"
#include "bsp.h"
#endif

#include "arch/port.h"		//for device information, network address
#include "arch/sys_arch.h"	//for semaphore, mutex, mbox, file, time, tick, and log functions

#include "../utils.h"		//for popular pure c functions, such as ston, Strnicmp, DecodeB64, ...
#include "http_session.h"
#include "http_fs.h"

#if (LWIP_ALTCP_TLS > 0)
#define ALWAYS_REDIRECT_HTTPS	1
#else
#define ALWAYS_REDIRECT_HTTPS	0
#endif

#define LOG_DEBUG_ONLY			0 //max level of debug output

#define METHOD_GET			1 //request method GET
#define METHOD_POST			2 //request method POST

#define CODE_OK				200

#define CODE_NOTMODIFIED	-304	//Not Modified (RFC 7232)
#define CODE_REDIRECT		-307	//Temporary Redirect (since HTTP/1.1)

#define CODE_UNAUTHORIZED	-401	//Unauthorized (RFC 7235)
#define CODE_FORBIDDEN		-403	//Forbidden
#define CODE_NOTFOUND		-404	//Not Found

#define CODE_ENTITYTOOLARGE	-413	//Payload Too Large (RFC 7231)
#define CODE_URITOOLONG		-414	//URI Too Long (RFC 7231)

#define CONNECTION_HEADER	"keep-alive" //"close"
/////////////////////////////////////////////////////////////////////////////////////////////////////
// Function replacement
#ifndef WIN32
#define LWIP_GetTickCount 	BSP_GetTickCount
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////
// HTTP receiving FSM state

#define HTTP_STATE_IDLE					0 //nothing received

#define HTTP_STATE_HEADER_RECEIVING		1 //something received ==> waiting request header
#define HTTP_STATE_HEADER_DONE			2 //request header completed

#define HTTP_STATE_CHUNK_LEN_RECEIVING	3 //if chunked header exists and no content-length, waiting chunk size
#define HTTP_STATE_CHUNK_LEN_DONE		4 //chunk size received
#define HTTP_STATE_CHUNK_RECEIVING		5 //receiving chunk data
#define HTTP_STATE_CHUNK_DONE			6 //current chunk completely received

#define HTTP_STATE_BODY_RECEIVING		7 //receiving request body with content-length
#define HTTP_STATE_BODY_DONE			8 //body completely received

#define HTTP_STATE_SENDING_HEADER		9  //sending response header
#define HTTP_STATE_SENDING_BODY			10 //sending response body

#define HTTP_STATE_REQUEST_END			11 //response completely sent out

#define SERVER_HEADER	"straight/1.1"
#define DATE_HEADER		1

/////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_CONNECTIONS 				4		//max concurrent socket connections
#define MAX_REQ_BUF_SIZE				TCP_MSS	//length of the request header is up to MAX_REQ_BUF_SIZE bytes
#define MAX_APP_CONTEXT_SIZE			512		//reserved buffer for app/cgi layer, such as SSI_Context peocessing

#define TO_RECV							60*1000
#define TO_SENT							60*1000

/////////////////////////////////////////////////////////////////////////////////////////////////////

#define HTTP_PROC_CALLER_RECV			1 //triggered by tcp/recv event
#define HTTP_PROC_CALLER_POLL			2 //triggered by poll/timer
#define HTTP_PROC_CALLER_SENT			3 //triggered by tcp/sent event

#define STAGE_END						0xFFFFFFFF //response may be split into multi-steps because of memory limitation

/////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _RESPONSE_CONTEXT
{
	char 	_token[128];		//for http_core.c
	int 	_authorized;		//for http_core.c
	
	unsigned long _dwOperStage;	//[0, STAGE_END], for app layer, major progress
	
	int 	_dwOperIndex;   //[0, _dwTotal], for app layer, minor progress inside a stage
	int 	_dwTotal;		//max. of _dwOperIndex, for app layer, total progress of a stage
	
	int 	_sendMaxBlock;	//TCP_MSS, for http_core.c
	
	int 	_sending;		//=0 if it is the first response block, for http_core.c
	int 	_total2Send;	//total bytes to send
	int 	_totalSent;		//total bytes already sent

	unsigned long long _tLastSent;		//tick of the last sent, for http_core.c
	unsigned long _nSendTimeout; 	//60*1000, for http_core.c
	
	int 	_bytesLeft;				//remaining data length, for http_core.c
	char 	_sendBuffer[2*TCP_MSS];	//remaining data, for http_core.c
	
	char	_appContext[MAX_APP_CONTEXT_SIZE]; //additional context for app layer to save anything, e.g. file read/write info
}RESPONSE_CONTEXT;

typedef struct _REQUEST_CONTEXT
{
	int		_sid;			//session index, for http_core.c
	int 	_state;			//HTTP FSM, for http_core.c
	int 	_peer_closing;	//half close, for http_core.c
	
	unsigned long long _tRequestStart;	//tick of the request began, for http_core.c
	unsigned long long _tLastReceived;	//tick of the last received, for http_core.c
	unsigned long _nReceiveTimeout; //60*1000, for http_core.c

	unsigned long  _https;		//is it HTTPS request? for redirecting from http to https
	unsigned long  _ipRemote;	//keep user's IP when logged in
	unsigned short _portRemote; //peer port
	unsigned short _portLocal;  //local port: 80 or 443
	int _checkAlive;			//keep-alive request, for http_core.c

	sys_mutex_t* _pMutex; 		//not used because of single thread processing
	int 	_killing;			//notification from other tasks
	
	int			_bufOffset;		//for http_core.c
	struct pbuf* _reqBufList;	//for http_core.c
	
	struct altcp_pcb* _pcb; 	//for non-blocking mode
	
	int _requestMethod;		//GET/POST, for http_core.c
	int _contentLength;		//length header, for http_core.c

	int _keepalive;			//keep-alive header, for http_core.c
	int _chunked;			//chunk header, for http_core.c
	int _expect00;			//Expect 100 header, for http_core.c

	time_t _ifModified;		//for hrader: If-Modified-Since

	long _rangeFrom;		//included		[from,to)
	long _rangeTo;			//not included	[from,to)

	int _contentReceived;	//count while receiving, for http_core.c
	int _result;			//200 for success, 0 for pending, negative for failure, for http_core.c

	SESSION* _session;		//for session management

	struct CGI_Mapping* handler;	//matched CGI handler
	long _options;	//working options copied from default CGI_Mapping, which may be changed by privilege
	RESPONSE_CONTEXT ctxResponse;	
	
	char _ver[4];			//HTTP version
	long _posQuestion;		//parameters start position in _requestPath
	char _extension[8];		//extension of the requested path
	char _requestPath[128];	//full path
	char _responsePath[128]; //use to redirect

	void* _fileHandle;	//file handle for GET, POST, and DIR operations
	
	int  request_length;	//[0, max_level]
	int  max_level; 		//MAX_REQ_BUF_SIZE
	char http_request_buffer[MAX_REQ_BUF_SIZE + 20]; //receiving buffer, max space to hold the request
}REQUEST_CONTEXT;

void PrintLwipStatus(void); //kill the odlest TIME_WAIT pcb and print active count

int  IsKilling(REQUEST_CONTEXT* context, int reset);	//read killing flag, and clear it
void SetupHttpContext(void); //initialize context for connections

struct altcp_pcb* HttpdInit(int tls, unsigned long port);
int HttpdStop(struct altcp_pcb *pcbListen);

#endif
