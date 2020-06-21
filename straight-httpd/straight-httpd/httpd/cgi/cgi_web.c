/*
  cgi_web.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#include "../http_cgi.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

struct CGI_Mapping g_cgiWebApp = {
	"/app/*", //char* path;
	CGI_OPT_AUTH_REQUIRED | CGI_OPT_GET_ENABLED,// unsigned long options;

	NULL, //void (*OnCancel)(REQUEST_CONTEXT* context);
	NULL, //int (*OnHeaderReceived)(REQUEST_CONTEXT* context, char* header_line);
	NULL, //void (*OnHeadersReceived)(REQUEST_CONTEXT* context);
	NULL, //int  (*OnContentReceived)(REQUEST_CONTEXT* context, char* buffer, int size);
	WEB_RequestReceived, //void (*OnRequestReceived)(REQUEST_CONTEXT* context);
	
	WEB_AppendHeaders, //void (*SetResponseHeader)(REQUEST_CONTEXT* context, char* HttpCode);
	WEB_ReadContent, //int  (*ReadContent)(REQUEST_CONTEXT* context, char* buffer, int maxSize);
	
	WEB_AllSent, //int  (*OnAllSent)(REQUEST_CONTEXT* context);
	WEB_Finished, //void (*OnFinished)(REQUEST_CONTEXT* context);
	
	NULL //struct CGI_Mapping* next;
};
