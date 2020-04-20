/*
  http_cgi.h
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#ifndef _HTTP_CGI_H_
#define _HTTP_CGI_H_

#include "http_core.h"

//the first matched folder will be the web home directory, 
//  where the default homepage <WEB_DRIVE[i] + WEB_ROOT + WEB_ROOT_FILE> is located.
#define WEB_DRIVE		"D"	//used for multiple dynamic disks ('CDEF' etc.), the first letter has the highest priority
#define WEB_ROOT		"/straight/straight-httpd/straight-httpd/straight-httpd/httpd/cncweb/"
#define WEB_AUTH_PAGE	"login.html"
#define WEB_HOME_PAGE	"index.shtml"

#define CGI_OPT_AUTHENTICATOR	0x80000000	//this is the authentication responser, response with token header named 'X-Auth-Token'
#define CGI_OPT_AUTH_REQUIRED	0x40000000	//MUST request with token header named 'X-Auth-Token'

#define CGI_OPT_GET_ENABLED		0x00080000	//GET enabled
#define CGI_OPT_POST_ENABLED	0x00040000	//POST enabled
#define CGI_OPT_LOG_ENABLED		0x00020000	//LOG enabled, not used

#define CGI_OPT_CHUNKED			0x00000800	//response with chunked data
#define CGI_OPT_GZIP			0x00000400	//response with gzip compression

struct CGI_Mapping
{
	char* path;			//constant string, request full path

	unsigned long options; //defined by CGI_OPT_xxxxxx

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

void CGI_SetupMapping(void); //setup mapping when initializing httpd context
void CGI_Append(struct CGI_Mapping* newMapping); //append single CGI mapping

//cancel notification to app layer because of any HTTP fatal errors
//  including timeout, format errors, sending failures, and stack keneral errors
void CGI_Cancel(REQUEST_CONTEXT* context);

//called by FreeHttpContext()
void CGI_Finish(REQUEST_CONTEXT* context);

//called when the first HTTP request line is completely received
void CGI_SetCgiHandler(REQUEST_CONTEXT* context);

//called when every single HTTP request header/line is received
void CGI_HeaderReceived(REQUEST_CONTEXT* context, char* header_line);

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
//	  data MUST be put in context->ctxResponse._sendBuffer, and the buffer size set to context->ctxResponse._bytesLeft in advance
int  CGI_LoadContentToSend(REQUEST_CONTEXT* context, int caller);

extern const char no_cache[];
extern const char response_header_chunked[];
extern const char response_header_generic[];
extern const char* Response_Status_Lines[];

extern char g_szWebDrive[16];
extern char g_szWebRoot[128];
extern char g_szWebRootFile[128];

int CheckWebRoot(char* drive, char* root, char* file);
void SetWebRoot(char* drive, char* root, char* file);

#endif
