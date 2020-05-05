/*
  cgi_web.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#include "../http_cgi.h"

typedef struct
{
	short _valid;	//>0: valid
	short _source; 	//0=buffer or 1=file
	
	LWIP_FIL*  _fp;
	char* _buffer;
	
	long  _total;
	long  _offset;
	
	short _ssi; 		//1=SSI
	short _ssiState; 	//0=idle, 1=searching start, 2=start found, 3=collecting name, 4=searching end, 5=end found
	short _cacheOffset; //
	short _tagLength; 	//
	
	char  _tagName[64];
	char  _cache[2*TCP_MSS]; //pre-read buffer
}SSI_Context;

typedef struct 
{
	char *extension;
	char *content_type;
}TypeHeader;

static const TypeHeader contentTypes[] = {
	{ "html",  "text/html"},
	{ "htm",   "text/html"},

	{ "shtml", "text/html"},
	{ "shtm",  "text/html"},
	{ "ssi",   "text/html"},
	{ "css",   "text/css"},

	{ "json",  "application/json"},
	{ "js",    "application/javascript"},

	{ "gif",   "image/gif"},
	{ "png",   "image/png"},
	{ "jpg",   "image/jpeg"},
	{ "bmp",   "image/bmp"},
	{ "ico",   "image/x-icon"},

	{ "class", "application/octet-stream"},
	{ "cls",   "application/octet-stream"},
	{ "swf",   "application/x-shockwave-flash"},
	{ "ram",   "application/javascript"},
	{ "pdf",   "application/pdf"},

	{ "xml",   "text/xml"},
	{ "xsl",   "text/xml"},

	{ "",    "text/plain"}
};

static const char tagStart[] = "<!--#";
static const char tagEnd[]   = "-->";

/////////////////////////////////////////////////////////////////////////////////////////////////////

extern void LogPrint(int level, char* format, ... );
extern int Strnicmp(char *str1, char *str2, int n);
extern int ReplaceTag(REQUEST_CONTEXT* context, char* tagName, char* appendTo, int maxAllowed);

/////////////////////////////////////////////////////////////////////////////////////////////////////

void Web_OnAuthHeaders(REQUEST_CONTEXT* context);
int  Web_OnAuthData(REQUEST_CONTEXT* context, char* buffer, int size);
int  Web_OnAuthCheck(REQUEST_CONTEXT* context);

void Web_OnRequestReceived(REQUEST_CONTEXT* context);
	
void Web_SetResponseHeaders(REQUEST_CONTEXT* context, char* HttpCodeInfo);
int  Web_LoadContentToSend(REQUEST_CONTEXT* context);
void Web_OnAllSent(REQUEST_CONTEXT* context);
void Web_OnFinished(REQUEST_CONTEXT* context);

struct CGI_Mapping g_cgiWebAuth = {
	"/auth/*", //char* path;
	CGI_OPT_AUTHENTICATOR | CGI_OPT_GET_ENABLED | CGI_OPT_POST_ENABLED,// unsigned long options;

	NULL, //void (*OnCancel)(REQUEST_CONTEXT* context);
	NULL, //void (*OnHeaderReceived)(REQUEST_CONTEXT* context, char* header_line);
	Web_OnAuthHeaders, //Web_OnHeadersReceived, //void (*OnHeadersReceived)(REQUEST_CONTEXT* context);
	Web_OnAuthData, //int  (*OnContentReceived)(REQUEST_CONTEXT* context, char* buffer, int size);
	Web_OnRequestReceived, //void (*OnRequestReceived)(REQUEST_CONTEXT* context);

	Web_SetResponseHeaders, //void (*SetResponseHeader)(REQUEST_CONTEXT* context, char* HttpCode);
	Web_LoadContentToSend, //int  (*LoadContentToSend)(REQUEST_CONTEXT* context);

	Web_OnAllSent, //int  (*OnAllSent)(REQUEST_CONTEXT* context);
	Web_OnFinished, //void (*OnFinished)(REQUEST_CONTEXT* context);

	NULL //struct CGI_Mapping* next;
};

struct CGI_Mapping g_cgiWebApp = {
	"/app/*", //char* path;
	CGI_OPT_AUTH_REQUIRED | CGI_OPT_GET_ENABLED | CGI_OPT_POST_ENABLED,// unsigned long options;

	NULL, //void (*OnCancel)(REQUEST_CONTEXT* context);
	NULL, //void (*OnHeaderReceived)(REQUEST_CONTEXT* context, char* header_line);
	NULL, //Web_OnHeadersReceived, //void (*OnHeadersReceived)(REQUEST_CONTEXT* context);
	NULL, //int  (*OnContentReceived)(REQUEST_CONTEXT* context, char* buffer, int size);
	Web_OnRequestReceived, //void (*OnRequestReceived)(REQUEST_CONTEXT* context);
	
	Web_SetResponseHeaders, //void (*SetResponseHeader)(REQUEST_CONTEXT* context, char* HttpCode);
	Web_LoadContentToSend, //int  (*LoadContentToSend)(REQUEST_CONTEXT* context);
	
	Web_OnAllSent, //int  (*OnAllSent)(REQUEST_CONTEXT* context);
	Web_OnFinished, //void (*OnFinished)(REQUEST_CONTEXT* context);
	
	NULL //struct CGI_Mapping* next;
};

int CheckUser(char* u, char* p)
{
	int success = 0;
	if ((stricmp(u, "admin") == 0))// && (stricmp(p, "password") == 0))
		success = 1;
	else if ((stricmp(u, "user") == 0))// && (stricmp(p, "password") == 0))
		success = 1;
	return success;
}

void Web_OnAuthHeaders(REQUEST_CONTEXT* context)
{
	memset(context->ctxResponse._appContext, 0, sizeof(context->ctxResponse._appContext));
}

int Web_OnAuthData(REQUEST_CONTEXT* context, char* buffer, int size)
{
	int len = strlen(context->ctxResponse._appContext);
	int max = sizeof(context->ctxResponse._appContext);
	strncpy(context->ctxResponse._appContext + len, buffer, (size < max-len-2) ? size : (max - len - 2));
	return size;
}

int Web_OnAuthCheck(REQUEST_CONTEXT* context)
{
	if (context->ctxResponse._appContext[0] != 0)
	{
		int success, i;
		char* u;
		char* p;
		char szToken[32];

		LogPrint(0, "POST data: %s\r\n", context->ctxResponse._appContext);
		u = strstr(context->ctxResponse._appContext, "username=");
		p = strstr(context->ctxResponse._appContext, "password=");
		if ((u != NULL) && (p != NULL))
		{
			u += 9; p += 9;

			i = 0;
			while (u[i] != 0)
			{
				if (u[i] == '&')
				{
					u[i] = 0;
					break;
				}
				i++;
			}

			i = 0;
			while (p[i] != 0)
			{
				if (p[i] == '&')
				{
					p[i] = 0;
					break;
				}
				i++;
			}

			if (CheckUser(u, p) > 0)
			{
				success = SessionCreate(context, szToken);
				if (success > 0)
				{
					context->ctxResponse._authorized = CODE_OK;
					strcpy(context->ctxResponse._token, szToken);
					return 1;
				}
			}
		}
	}
	return 0;
}

void Web_OnRequestReceived(REQUEST_CONTEXT* context)
{
	int success;
	char szTemp[256];

	SSI_Context* ctxSSI = (SSI_Context*)context->ctxResponse._appContext;
	if ((context->handler != NULL) && ((context->handler->options & CGI_OPT_AUTHENTICATOR) != 0))
	{
		if (stricmp(context->_requestPath, WEB_LOGOUT_PAGE) == 0)
		{ //logout
			SessionKill(context, 1);

			strcpy(context->_responsePath, WEB_DEFAULT_PAGE);
			context->ctxResponse._authorized = 0;
			context->ctxResponse._token[0] = 0;
			context->_result = CODE_REDIRECT;
			return;
		}

		if ((context->_requestMethod == METHOD_POST) && (stricmp(context->_requestPath, WEB_DEFAULT_PAGE) == 0))
		{ //login
			success = Web_OnAuthCheck(context);
			memset(ctxSSI, 0, sizeof(SSI_Context)); //clear after Web_OnAuthCheck
			if (success > 0)
			{
				strcpy(context->_responsePath, WEB_APP_PAGE);
				context->ctxResponse._authorized = CODE_OK;
				context->_result = CODE_REDIRECT;
				return;
			}
			else
			{
				strcpy(context->_responsePath, WEB_DEFAULT_PAGE);
				context->ctxResponse._authorized = 0;
				context->_result = CODE_REDIRECT;
				return;
			}
		}
	}

	if (sizeof(context->ctxResponse._appContext) < sizeof(SSI_Context))
	{
		ctxSSI->_valid = 0;
		context->_result = -500;
		return;
	}
	
	memset(ctxSSI, 0, sizeof(SSI_Context));

	ctxSSI->_valid = 1;
	ctxSSI->_source = 0;
	ctxSSI->_fp = NULL;
	ctxSSI->_buffer = NULL;
	
	ctxSSI->_ssi = 0;
	ctxSSI->_ssiState = 0;
	ctxSSI->_cacheOffset = 0;
	ctxSSI->_tagLength = 0;
	
	if ((context->_responsePath[0] == '/') && (context->_responsePath[1] == 0))
	{ //default path
		strcpy(context->_responsePath, WEB_ABS_ROOT, WEB_DEFAULT_PAGE);
		char* ext = strstr(WEB_DEFAULT_PAGE, ".");
		if (ext != NULL)
			strcpy(context->_extension, ext+1);
	}

	if (context->_responsePath[0] == '/')
		LWIP_sprintf(szTemp, "%s%s", g_szWebAbsRoot, context->_responsePath+1);
	else
		LWIP_sprintf(szTemp, "%s%s", g_szWebAbsRoot, context->_responsePath);
	ctxSSI->_fp = LWIP_fopen(szTemp, "r");
	if (ctxSSI->_fp != NULL)
	{
		context->_file2Get = ctxSSI->_fp;
		context->ctxResponse._dwTotal = LWIP_fsize(ctxSSI->_fp);
		LogPrint(LOG_DEBUG_ONLY, "File opened: %s, len=%d", szTemp, context->ctxResponse._dwTotal);
	}

	if (Strnicmp(context->_extension, "shtml", 5) == 0)
		ctxSSI->_ssi = 1;
	else if (Strnicmp(context->_extension, "shtm", 4) == 0)
		ctxSSI->_ssi = 1;
	else if (Strnicmp(context->_extension, "ssi", 3) == 0)
		ctxSSI->_ssi = 1;

	if (ctxSSI->_ssi > 0)
		context->handler->options |= CGI_OPT_CHUNKED;

	if (ctxSSI->_fp == NULL)
		context->ctxResponse._dwTotal = LWIP_fsize(ctxSSI->_fp);
}

char* GetContentType(REQUEST_CONTEXT* context)
{
	int i = 0; 
	int extLen = strlen(context->_extension);
	while(1)
	{
		int typeLen = strlen(contentTypes[i].extension);
		if (typeLen == 0)
			return contentTypes[i].content_type;
		
		if (typeLen == extLen)
		{
			if (Strnicmp(context->_extension, contentTypes[i].extension, typeLen) == 0)
				return contentTypes[i].content_type;
		}
		i ++;
	}
}

void Web_SetResponseHeaders(REQUEST_CONTEXT* context, char* HttpCodeInfo)
{
	SSI_Context* ctxSSI = (SSI_Context*)context->ctxResponse._appContext;
	if (ctxSSI->_valid == 0)
	{
		LWIP_sprintf(context->ctxResponse._sendBuffer, (char*)header_generic, HttpCodeInfo, GetContentType(context), 0, "close"); //length
	}
	else
	{
		LWIP_sprintf(context->ctxResponse._sendBuffer, header_chunked, HttpCodeInfo, GetContentType(context), header_nocache, "close");
	}

	if ((context->_requestMethod == METHOD_GET) && 
		(context->handler != NULL) &&
		((context->handler->options & CGI_OPT_AUTHENTICATOR) != 0))
	{ //clear cookie with login page
		char szCookie[128];
		LWIP_sprintf(szCookie, "X-Auth-Token: SID=\r\nSet-Cookie: SID=; Path=/; HttpOnly; max-age=3600\r\n");
		strcat(context->ctxResponse._sendBuffer, szCookie);
	}
	strcat(context->ctxResponse._sendBuffer, CRLF);
}

int  Web_LoadContentToSend(REQUEST_CONTEXT* context)
{
	int needSend = 0;
	SSI_Context* ctxSSI = (SSI_Context*)context->ctxResponse._appContext;

	if ((context->_requestMethod == METHOD_GET) || (context->_requestMethod == METHOD_POST))
	{
		int size;
		char szSize[64];
		int size2Send;
		int prefixLen;
		
		int offset = 10; //space for chunk size
		int dataCount = 0;
		char szCRLF[4];
		
		if (context->ctxResponse._dwOperStage == 0)
		{ //first chunk
			context->ctxResponse._dwOperIndex = 0;
			context->ctxResponse._dwOperStage = 1;
			memset(szCRLF, 0, 4);
		}
		else
		{
			strcpy(szCRLF, CRLF);
		}
		
		if (context->ctxResponse._dwOperStage == 1)
		{ //block data as a chunk
			dataCount = (context->ctxResponse._dwTotal - context->ctxResponse._dwOperIndex); //available data remained
			if (dataCount > 0)
			{
				if (ctxSSI->_ssi == 0)
				{ //non-SSI
					if (dataCount > context->ctxResponse._sendMaxBlock - 50)
						dataCount = context->ctxResponse._sendMaxBlock - 50;
					
					if (ctxSSI->_fp == NULL)
						memcpy(context->ctxResponse._sendBuffer+offset, ctxSSI->_buffer + context->ctxResponse._dwOperIndex, dataCount);
					else
					{
						unsigned int bytes = 0;
						if (0 != LWIP_fread(ctxSSI->_fp, context->ctxResponse._sendBuffer+offset, dataCount, &bytes))
						{
							context->_result = -500;
							return 0;
						}
						dataCount = bytes;
						//dataCount = ff_fread(context->ctxResponse._sendBuffer+offset, dataCount, 1, ctxSSI->_fp);
					}
					context->ctxResponse._dwOperIndex += dataCount;
				}
				else
				{ //SSI
					int lastBlock = 0;
					int consumed, canRead, totalToParse;
				
					char* pCache = ctxSSI->_cache; //cache for tag extracting
					
					canRead = dataCount; //remain data
					if (canRead > context->ctxResponse._sendMaxBlock - ctxSSI->_cacheOffset - 50)
						canRead = context->ctxResponse._sendMaxBlock - ctxSSI->_cacheOffset - 50;
					else
						lastBlock = 1;
					
					if (ctxSSI->_fp == NULL)
					{
						char* pSrc = ctxSSI->_buffer + context->ctxResponse._dwOperIndex; //const memory
						memcpy(pCache + ctxSSI->_cacheOffset, pSrc, canRead); //read to cache, then parse data in cache
					}
					else
					{
						unsigned int bytes = 0;
						if (0 != LWIP_fread(ctxSSI->_fp, pCache + ctxSSI->_cacheOffset, canRead, &bytes))
						{
							context->_result = -500;
							return 0;
						}
						//canRead = ff_fread(pCache + ctxSSI->_cacheOffset, canRead, 1, ctxSSI->_fp);
						canRead = bytes;
					}
					totalToParse = ctxSSI->_cacheOffset + canRead;
					
					LogPrint(LOG_DEBUG_ONLY, "Tag: append data: state=%d, off=%d, new=%d", ctxSSI->_ssiState, ctxSSI->_cacheOffset, canRead);
					
					consumed = 0;  	//consumed source
					dataCount = 0; 	//data that will be sent
					if (totalToParse >= 8)
					{
						while(consumed <= totalToParse - 8)
						{
							if (ctxSSI->_ssiState == 0) //0=idle, 1=searching start, 2=searching end
								ctxSSI->_ssiState = 1;
							
							if (ctxSSI->_ssiState == 1)	//1=searching start
							{
								if ((pCache[consumed] == '<') && (pCache[consumed+1] == '!') && 
									(pCache[consumed+2] == '-') && (pCache[consumed+3] == '-') && (pCache[consumed+4] == '#')) //"<!--#"
								{
									LogPrint(LOG_DEBUG_ONLY, "Tag start found");
									
									ctxSSI->_ssiState = 2; //start found
									consumed += 5;
								}
								else
								{ //copy to send buffer
									context->ctxResponse._sendBuffer[offset+dataCount] = pCache[consumed];
									dataCount ++;
									
									consumed ++;
								}
							}
							else if (ctxSSI->_ssiState == 2)	//start found, copy name and searching end
							{
								if ((pCache[consumed] == '-') && (pCache[consumed+1] == '-') && (pCache[consumed+2] == '>')) //"-->"
								{ //end found
									int nAppend;
									LogPrint(LOG_DEBUG_ONLY, "Tag end: %s", ctxSSI->_tagName);
									
									nAppend = ReplaceTag(context, ctxSSI->_tagName, context->ctxResponse._sendBuffer+offset+dataCount, context->ctxResponse._sendMaxBlock - dataCount - 50);
									dataCount += nAppend;
									consumed += 3;
									
									ctxSSI->_ssiState = 0; //end found
									ctxSSI->_tagLength = 0;
									memset(ctxSSI->_tagName, 0, sizeof(ctxSSI->_tagName));
								}
								else
								{ //copy tag name
									ctxSSI->_tagName[ctxSSI->_tagLength++] = pCache[consumed];
									ctxSSI->_tagName[ctxSSI->_tagLength] = 0;
									
									consumed ++;
								}
							}
						}
						LogPrint(LOG_DEBUG_ONLY, "Tag cache remain: consumed=%d, left=%d", consumed, totalToParse - consumed);
						
						memmove(pCache, pCache + consumed, totalToParse - consumed); //shift consumed out
						ctxSSI->_cacheOffset = totalToParse - consumed;
						
						if (lastBlock > 0)
						{
							context->ctxResponse._dwOperIndex += canRead;
							canRead = 0;
						}
					}
					
					if (lastBlock > 0)
					{
						LogPrint(LOG_DEBUG_ONLY, "Tag final remain: off=%d, read=%d", ctxSSI->_cacheOffset, canRead);
						
						memcpy(context->ctxResponse._sendBuffer+offset+dataCount, pCache, ctxSSI->_cacheOffset + canRead);
						dataCount += ctxSSI->_cacheOffset + canRead;
						
						ctxSSI->_cacheOffset = 0;
					}
					context->ctxResponse._dwOperIndex += canRead;
				}
				context->ctxResponse._sendBuffer[offset+dataCount] = 0;
			}

			if (context->ctxResponse._dwOperIndex >= context->ctxResponse._dwTotal)
			{
				LogPrint(LOG_DEBUG_ONLY, "Web content end reached, @%d", context->_sid);
				
				context->ctxResponse._dwOperStage = STAGE_END;
				context->ctxResponse._dwOperIndex = 0;
			}
		}
	
		size2Send = dataCount;
		
		LWIP_sprintf(szSize, "%s%X\r\n", szCRLF, dataCount); //chunk size
		prefixLen = strlen(szSize);
		memcpy(context->ctxResponse._sendBuffer+offset-prefixLen, szSize, prefixLen); //copy chunk size to the head
		size2Send += prefixLen;
		
		if (context->ctxResponse._dwOperStage == STAGE_END)
		{ //last block, end chunk
			strcpy(szSize, "\r\n0\r\n\r\n");
			memcpy(context->ctxResponse._sendBuffer+offset-prefixLen + size2Send, szSize, 7); //copy to the tail
			size2Send += 7;
		}
		
		memmove(context->ctxResponse._sendBuffer, context->ctxResponse._sendBuffer+offset-prefixLen, size2Send); //move to the head
		context->ctxResponse._bytesLeft = size2Send;
		needSend = 1;
		
		LogPrint(LOG_DEBUG_ONLY, "Web response sending: %d @%d", dataCount, context->_sid);
	}
	
	return needSend;
}

void Web_OnAllSent(REQUEST_CONTEXT* context)
{
	SSI_Context* ctxSSI = (SSI_Context*)context->ctxResponse._appContext;
	if ((ctxSSI != NULL) && (ctxSSI->_fp != NULL))
	{
		LogPrint(LOG_DEBUG_ONLY, "Web file sent and closed");
		
		LWIP_fclose(ctxSSI->_fp);
		ctxSSI->_fp = NULL;
	}
}

void Web_OnFinished(REQUEST_CONTEXT* context)
{
	SSI_Context* ctxSSI = (SSI_Context*)context->ctxResponse._appContext;
	if ((ctxSSI != NULL) && (ctxSSI->_fp != NULL))
	{
		LogPrint(LOG_DEBUG_ONLY, "Web file finished and closed");
		
		LWIP_fclose(ctxSSI->_fp);
		ctxSSI->_fp = NULL;
	}
}
