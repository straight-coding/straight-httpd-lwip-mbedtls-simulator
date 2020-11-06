/*
  http_cgi.h
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#ifndef _HTTP_CGI_H_
#define _HTTP_CGI_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_core.h"
#include "http_web.h"

#define CRLF	"\r\n"

#define WEB_DEFAULT_PAGE	"/auth/login.html"
#define WEB_SESSION_CHECK	"/auth/session_check.cgi"
#define WEB_LOGOUT_PAGE		"/auth/logout.cgi"
#define WEB_APP_PAGE		"/app/index.shtml"

#define MAX_CGI_PATH		64
#define MAX_TAG_LEN			64

#define CGI_OPT_AUTH_REQUIRED	0x40000000	//MUST request with token header named 'X-Auth-Token'

#define CGI_OPT_PREFIX_WILDCARD	0x08000000	//path with * wildcard

#define CGI_OPT_GET_ENABLED		0x00800000	//GET enabled
#define CGI_OPT_POST_ENABLED	0x00400000	//POST enabled
#define CGI_OPT_LOG_ENABLED		0x00200000	//LOG enabled, not used

#define CGI_OPT_CHUNK_ENABLED	0x00008000	//response with chunked data
#define CGI_OPT_GZIP			0x00004000	//response with gzip compression

struct CGI_Mapping
{
	char path[MAX_CGI_PATH];	//constant string, request full path
	unsigned long optionsAllowed;	//defined by CGI_OPT_xxxxxx

	void (*OnCancel)(REQUEST_CONTEXT* context);

	int  (*OnHeaderReceived)(REQUEST_CONTEXT* context, char* header_line); //return 1 if eaten
	void (*OnHeadersReceived)(REQUEST_CONTEXT* context);
	int  (*OnContentReceived)(REQUEST_CONTEXT* context, char* buffer, int size);
	void (*OnRequestReceived)(REQUEST_CONTEXT* context);
	
	void (*SetResponseHeaders)(REQUEST_CONTEXT* context, char* HttpCode);
	int  (*ReadContent)(REQUEST_CONTEXT* context, char* buffer, int maxSize);
	
	void (*OnAllSent)(REQUEST_CONTEXT* context);
	void (*OnFinished)(REQUEST_CONTEXT* context);
	
	struct CGI_Mapping* next;
};	//size of CGI_Mapping: 108

typedef struct
{
	short _valid;	//>0: valid
	short _source; 	//0=buffer or 1=file

	void* _fp;		//file handle and info
	char  _lastModified[64]; //keep file date in GMT format
	char* _buffer;		//static content

	long  _options;		//default options

	short _ssi; 		//1=SSI
	short _ssiState; 	//0=idle, 1=searching start, 2=start found, 3=collecting name, 4=searching end, 5=end found

	short _tagLength; 	//length of tag name
	char  _tagName[64];	//buffer of tag name

	short _cacheOffset; //for tag parsing
	char  _cache[MAX_TAG_LEN]; //pre-read buffer
}SSI_Context;	//size of SSI_Context: 216

void CGI_SetupMapping(void); //setup mapping when initializing httpd context

//cancel notification to app layer because of any HTTP fatal errors
//  including timeout, format errors, sending failures, and stack keneral errors
void CGI_Cancel(REQUEST_CONTEXT* context);

//called by FreeHttpContext()
void CGI_Finish(REQUEST_CONTEXT* context);

//called when the first HTTP request line is completely received
void CGI_SetCgiHandler(REQUEST_CONTEXT* context);

//called when every single HTTP request header/line is received
int CGI_HeaderReceived(REQUEST_CONTEXT* context, char* header_line);

//called when all HTTP request headers/lines are received
void CGI_HeadersReceived(REQUEST_CONTEXT* context);

//called when any bytes of the HTTP request body are received (may be partial)
int  CGI_ContentReceived(REQUEST_CONTEXT* context, char* buffer, int size);

//called when the HTTP request body is completely received
void CGI_RequestReceived(REQUEST_CONTEXT* context);

//set response headers: content-type, content-length, connection, and http code and status info
void CGI_SetResponseHeaders(REQUEST_CONTEXT* context, char* HttpCodeInfo);

//load response body chunk by chunk
//  there are two-level progresses initialized as 0: 
// 	  context->ctxResponse._dwOperStage = 0; //major progress, set _dwOperStage=STAGE_END if it is the last data block
//	  context->ctxResponse._dwOperIndex = 0; //sub-progress for each stage, for app layer internal use
//    caller: called from HTTP_PROC_CALLER_RECV (event) / HTTP_PROC_CALLER_POLL (timer) / HTTP_PROC_CALLER_SENT (event)
//  return 1 if data is ready to send
//	  data MUST be put in context->ctxResponse._sendBuffer, 
//			and the buffer size set to context->ctxResponse._bytesLeft in advance
int  CGI_LoadContentToSend(REQUEST_CONTEXT* context, int caller);

extern const char header_nocache[];
extern const char header_generic[];
extern const char header_chunked[];
extern const char header_range[];
extern const char header_gzip[];
extern const char redirect_body1[];
extern const char redirect_body2[];

#endif
