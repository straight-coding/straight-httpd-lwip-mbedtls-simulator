/*
  cgi_form.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: June 11, 2020
*/

#include "../http_cgi.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

extern void LogPrint(int level, char* format, ... );
extern void LoadConfig4Edit(void);
extern void AppyConfig(void);

/////////////////////////////////////////////////////////////////////////////////////////////////////

static void Form_OnHeadersReceived(REQUEST_CONTEXT* context);
static void Form_OnRequestReceived(REQUEST_CONTEXT* context);
static void Form_SetResponseHeaders(REQUEST_CONTEXT* context, char* HttpCodeInfo);
static void Form_AllSent(REQUEST_CONTEXT* context);

struct CGI_Mapping g_cgiForm = {
	"/app/form.shtml", //char* path;
	CGI_OPT_GET_ENABLED | CGI_OPT_POST_ENABLED,// unsigned long options;

	NULL, //void (*OnCancel)(REQUEST_CONTEXT* context);

	NULL, //int (*OnHeaderReceived)(REQUEST_CONTEXT* context, char* header_line);
	Form_OnHeadersReceived, //void (*OnHeadersReceived)(REQUEST_CONTEXT* context);
	WEB_OnFormReceived, //int  (*OnContentReceived)(REQUEST_CONTEXT* context, char* buffer, int size);
	Form_OnRequestReceived, //void (*OnRequestReceived)(REQUEST_CONTEXT* context);

	Form_SetResponseHeaders, //void (*SetResponseHeader)(REQUEST_CONTEXT* context, char* HttpCode);
	WEB_ReadContent, //int  (*ReadContent)(REQUEST_CONTEXT* context, char* buffer, int maxSize);
	Form_AllSent, //int  (*OnAllSent)(REQUEST_CONTEXT* context);
	NULL, //void (*OnFinished)(REQUEST_CONTEXT* context);

	NULL //struct CGI_Mapping* next;
};

static void Form_OnHeadersReceived(REQUEST_CONTEXT* context)
{
	LoadConfig4Edit();

	WEB_OnFormHeaders(context);
}

static void Form_OnRequestReceived(REQUEST_CONTEXT* context)
{
	WEB_RequestReceived(context);

	AppyConfig(); //apply the new values for the following function WEB_ReadContent

	memset(context->ctxResponse._sendBuffer, 0, sizeof(context->ctxResponse._sendBuffer));
	context->ctxResponse._bytesLeft = 0;
}

static void Form_SetResponseHeaders(REQUEST_CONTEXT* context, char* HttpCodeInfo)
{ //clear send buffer for response use
	WEB_AppendHeaders(context, HttpCodeInfo);
}

static void Form_AllSent(REQUEST_CONTEXT* context)
{
	WEB_AllSent(context);

	//form done
}
