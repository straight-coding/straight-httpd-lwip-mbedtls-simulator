/*
  http_web.h
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: June 10, 2020
*/

#ifndef _HTTP_WEB_H_
#define _HTTP_WEB_H_

#include "http_cgi.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

extern void LogPrint(int level, char* format, ... );
extern int Strnicmp(char *str1, char *str2, int n);
extern int ReplaceTag(REQUEST_CONTEXT* context, char* tagName, char* appendTo, int maxAllowed);

/////////////////////////////////////////////////////////////////////////////////////////////////////
void WEB_OnFormHeaders(REQUEST_CONTEXT* context);
int  WEB_OnFormReceived(REQUEST_CONTEXT* context, char* buffer, int size);

void WEB_RequestReceived(REQUEST_CONTEXT* context);
void WEB_AppendHeaders(REQUEST_CONTEXT* context, char* HttpCodeInfo);

int  WEB_ReadContent(REQUEST_CONTEXT* context, char* buffer, int maxSize);
void WEB_AllSent(REQUEST_CONTEXT* context);
void WEB_Finished(REQUEST_CONTEXT* context);

#endif
