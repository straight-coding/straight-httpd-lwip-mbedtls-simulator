/*
  http_core.h
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#ifndef _HTTP_CORE_H_
#define _HTTP_CORE_H_

#include "lwip/opt.h"

#include "lwip_port.h"
#include "arch/sys_arch.h"

#define LOG_DEBUG_ONLY			6

#define NON_BLOCKING_SERVICE	1

#define USE_CHUNKED				1

#define METHOD_GET				1
#define METHOD_POST				2

/////////////////////////////////////////////////////////////////////////////////////////////////////

#define HTTP_STATE_IDLE					0 //nothing received

#define HTTP_STATE_HEADER_RECEIVING		1 //something received
#define HTTP_STATE_HEADER_DONE			2 //request header completed

#define HTTP_STATE_CHUNK_LEN_RECEIVING	3 //
#define HTTP_STATE_CHUNK_LEN_DONE		4 //
#define HTTP_STATE_CHUNK_RECEIVING		5 //
#define HTTP_STATE_CHUNK_DONE			6

#define HTTP_STATE_BODY_RECEIVING		7
#define HTTP_STATE_BODY_DONE			8

#define HTTP_STATE_SENDING_HEADER		9
#define HTTP_STATE_SENDING_BODY			10

#define HTTP_STATE_REQUEST_END			11

/////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_CONNECTIONS 				5
#define MAX_REQ_BUF_SIZE				4096
#define MAX_APP_CONTEXT_SIZE			4096

#define MAX_COOKIE_SIZE					64

/////////////////////////////////////////////////////////////////////////////////////////////////////

#define HTTP_PROC_CALLER_RECV			1
#define HTTP_PROC_CALLER_POLL			2
#define HTTP_PROC_CALLER_SENT			3

#define STAGE_END						0xFFFFFFFF

/////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _RESPONSE_CONTEXT
{
	char 	_cookie[128];	//for http_core.c
	int 	_authorized;	//for http_core.c
	
	int 	_cmdType;		//for cgi_command.c, ATTACH / DETACH
	unsigned long _remote_ip;	//for cgi_command.c
	
	unsigned long _dwOperStage;	//[0, STAGE_END], for app layer
	
	int 	_dwOperIndex;   //[0, _dwTotal], for app layer
	int 	_dwTotal;		//max. of _dwOperIndex, for app layer
	
	int 	_sendMaxBlock;	//TCP_MSS, for http_core.c
	
	int 	_sending;	//=0 if it is the first response block, for http_core.c
	
	unsigned long _tLastSent;		//tick of the last sent, for http_core.c
	unsigned long _nSendTimeout; 	//60*1000, for http_core.c
	
	int 	_bytesLeft;				//remaining data length, for http_core.c
	char 	_sendBuffer[2*TCP_MSS];	//remaining data, for http_core.c
	
	char	_appContext[MAX_APP_CONTEXT_SIZE]; //can be used by app layer to save anything
}RESPONSE_CONTEXT;

typedef struct _REQUEST_CONTEXT
{
	int		_sid;			//session index, for http_core.c
	int 	_state;			//HTTP FSM, for http_core.c
	int 	_peer_closing;	//half close, for http_core.c
	
	unsigned long _tRequestStart;		//tick of the request began, for http_core.c
	unsigned long _tLastReceived;		//tick of the last received, for http_core.c
	unsigned long _nReceiveTimeout; 	//60*1000, for http_core.c
	
	sys_mutex_t* _pMutex; 		//not used because of single thread processing
	int 	_killing;			//notification from other tasks
	
	int			_bufOffset;		//for http_core.c
	struct pbuf* _reqBufList;	//for http_core.c
	
	struct altcp_pcb* _pcb; 	//for non-blocking mode
	
	int _requestMethod;		//GET/POST, for http_core.c
	int _contentLength;		//length header, for http_core.c

	int _keepalive;			//keep-alive header, for http_core.c
	int _chunked;			//chunk header, for http_core.c

	int _contentReceived;	//count while receiving, for http_core.c
	int _result;			//200 for success, 0 for pending, negative for failure, for http_core.c

	struct CGI_Mapping* handler;	//matched CGI handler
	RESPONSE_CONTEXT ctxResponse;	
	
	char _ver[4];			//HTTP version
	char _extension[8];		//extension of the requested path
	char _requestPath[128];	//full path
	char _responsePath[128]; //use to redirect

	//struct fs_file  file2Get;
	//int nContentOffset;
	
	int  request_length;	//[0, max_level]
	int  max_level; 		//MAX_REQ_BUF_SIZE
	char http_request_buffer[MAX_REQ_BUF_SIZE + 20]; //receiving buffer, max space to hold the request
}REQUEST_CONTEXT;

int SetOwner(unsigned long owner_ip, char* computerid, char* setcookie); //success=200, Forbidden=-403
int ClearOwner(char* cookie);
int UpdateOwner(char* cookie);

int GetCookie(char* cookie);
int IsAuthorized(char* cookie);

void PrintLwipStatus(void);

void SetKilling(REQUEST_CONTEXT* context);				//for app to kill session
int  IsKilling(REQUEST_CONTEXT* context, int reset);

void LockSession(REQUEST_CONTEXT* context);
void UnlockSession(REQUEST_CONTEXT* context);

void SetupHttpContext(void);
REQUEST_CONTEXT* GetHttpContext(void);
void ResetHttpContext(REQUEST_CONTEXT* context);
void CloseHttpContext(REQUEST_CONTEXT* context);
void FreeHttpContext(REQUEST_CONTEXT* context);
int  IsContextTimeout(REQUEST_CONTEXT* context);

signed char sendBuffered(REQUEST_CONTEXT* context);

struct altcp_pcb* HttpdInit(unsigned long port);
int HttpdStop(struct altcp_pcb *pcbListen);

#endif
