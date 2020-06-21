/*
  cgi_form.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: June 11, 2020
*/

#include "../http_cgi.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

extern void LogPrint(int level, char* format, ... );
extern void TAG_Setter(char* name, char* value);

/////////////////////////////////////////////////////////////////////////////////////////////////////

void Form_OnRequestReceived(REQUEST_CONTEXT* context);
	
void Form_SetResponseHeaders(REQUEST_CONTEXT* context, char* HttpCodeInfo);

struct CGI_Mapping g_cgiForm = {
	"/app/form.shtml", //char* path;
	CGI_OPT_GET_ENABLED | CGI_OPT_POST_ENABLED,// unsigned long options;

	NULL, //void (*OnCancel)(REQUEST_CONTEXT* context);

	NULL, //int (*OnHeaderReceived)(REQUEST_CONTEXT* context, char* header_line);
	WEB_OnFormHeaders, //void (*OnHeadersReceived)(REQUEST_CONTEXT* context);
	WEB_OnFormReceived, //int  (*OnContentReceived)(REQUEST_CONTEXT* context, char* buffer, int size);
	Form_OnRequestReceived, //void (*OnRequestReceived)(REQUEST_CONTEXT* context);

	Form_SetResponseHeaders, //void (*SetResponseHeader)(REQUEST_CONTEXT* context, char* HttpCode);
	WEB_ReadContent, //int  (*ReadContent)(REQUEST_CONTEXT* context, char* buffer, int maxSize);
	WEB_AllSent, //int  (*OnAllSent)(REQUEST_CONTEXT* context);
	WEB_Finished, //void (*OnFinished)(REQUEST_CONTEXT* context);

	NULL //struct CGI_Mapping* next;
};

void Form_OnRequestReceived(REQUEST_CONTEXT* context)
{
	WEB_RequestReceived(context);

	memset(context->ctxResponse._sendBuffer, 0, sizeof(context->ctxResponse._sendBuffer));
	context->ctxResponse._bytesLeft = 0;
}

void Form_SetResponseHeaders(REQUEST_CONTEXT* context, char* HttpCodeInfo)
{ //clear send buffer for response use
	WEB_AppendHeaders(context, HttpCodeInfo);
}
