/*
  http_cgi.h
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#ifndef _HTTP_CGI_H_
#define _HTTP_CGI_H_

#include "http_core.h"

#define WEB_ROOT "/webapp/"

struct CGI_Mapping
{
	char* path;			//request full path
	int   authRequired; //> 0: MUST request with cookie
	int   getEnabled; 	//1=GET enabled
	int   postEnabled; 	//1=POST enabled
	int   logEnabled; 	//1=LOG enabled, not used
	int   chunked;		//> 0: response with chunked data
	int   gzip;			//> 0: response with gzip compression

	void (*OnCancel)(REQUEST_CONTEXT* context);
	void (*OnHeaderReceived)(REQUEST_CONTEXT* context, char* header_line);
	void (*OnHeadersReceived)(REQUEST_CONTEXT* context);
	int  (*OnContentReceived)(REQUEST_CONTEXT* context, char* buffer, int size);
	void (*OnRequestReceived)(REQUEST_CONTEXT* context);
	
	void (*SetResponseHeader)(REQUEST_CONTEXT* context, char* HttpCode);
	int  (*LoadContentToSend)(REQUEST_CONTEXT* context);
	
	void (*OnAllSent)(REQUEST_CONTEXT* context);
	void (*OnFinished)(REQUEST_CONTEXT* context);
	
	struct CGI_Mapping* next;
};

void CGI_SetupMapping(void);
void CGI_Append(struct CGI_Mapping* newMapping);

//cancel notification to app layer because of HTTP fatal error
//  including timeout, format error, sending failure, stack keneral error
void CGI_Cancel(REQUEST_CONTEXT* context);

//called by FreeHttpContext()
void CGI_Finish(REQUEST_CONTEXT* context);

//called when the first HTTP request line is received
void CGI_SetCgiHandler(REQUEST_CONTEXT* context);

//called when single HTTP request header is received
void CGI_HeaderReceived(REQUEST_CONTEXT* context, char* header_line);

//called when all HTTP request headers are received
void CGI_HeadersReceived(REQUEST_CONTEXT* context);

//called when HTTP request body is received (may be partial)
int  CGI_ContentReceived(REQUEST_CONTEXT* context, char* buffer, int size);

//called when HTTP request body is completely received
void CGI_RequestReceived(REQUEST_CONTEXT* context);

//set response headers: content-type, content-length, connection, and http code and status info
void CGI_SetResponseHeader(REQUEST_CONTEXT* context, char* HttpCodeInfo);

//load response body chunk by chunk
//  there are two-level progress initialized as 0: 
// 	    context->ctxResponse._dwOperStage = 0; //main progress, set context->ctxResponse._dwOperStage=STAGE_END if it is the last data block
//	    context->ctxResponse._dwOperIndex = 0; //sub-progress, for app layer internal use
//  return 1 if data is ready to send
//	    data MUST be put in context->ctxResponse._sendBuffer, and the buffer size is context->ctxResponse._bytesLeft
int  CGI_LoadContentToSend(REQUEST_CONTEXT* context, int caller);

extern const char no_cache[];
extern const char response_header_chunked[];
extern const char response_header_generic[];
extern const char* Response_Status_Lines[];

extern char webRoot[128];
extern char webRootFile[128];
int CheckWebRoot(char drive, char* root, char* file);
void UpdateWebRoot(void);

#endif
