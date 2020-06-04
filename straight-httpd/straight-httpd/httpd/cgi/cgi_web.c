/*
  cgi_web.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#include "../http_cgi.h"

static const char tagStart[] = "<!--#";
static const char tagEnd[]   = "-->";

/////////////////////////////////////////////////////////////////////////////////////////////////////

extern void LogPrint(int level, char* format, ... );
extern int Strnicmp(char *str1, char *str2, int n);
extern int ReplaceTag(REQUEST_CONTEXT* context, char* tagName, char* appendTo, int maxAllowed);

/////////////////////////////////////////////////////////////////////////////////////////////////////

void Web_OnAuthHeaders(REQUEST_CONTEXT* context);
int  Web_OnAuthData(REQUEST_CONTEXT* context, char* buffer, int size);
void Web_OnRequestReceived(REQUEST_CONTEXT* context);
	
void Web_SetResponseHeaders(REQUEST_CONTEXT* context, char* HttpCodeInfo);
int  Web_ReadContent(REQUEST_CONTEXT* context, char* buffer, int maxSize);
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
	Web_ReadContent, //int  (*ReadContent)(REQUEST_CONTEXT* context, char* buffer, int maxSize);

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
	Web_ReadContent, //int  (*ReadContent)(REQUEST_CONTEXT* context, char* buffer, int maxSize);
	
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

int AuthCheck(REQUEST_CONTEXT* context)
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

void Web_OnAuthHeaders(REQUEST_CONTEXT* context)
{
	memset(context->ctxResponse._appContext, 0, sizeof(context->ctxResponse._appContext));
}

int Web_OnAuthData(REQUEST_CONTEXT* context, char* buffer, int size)
{
	if (size > 0)
	{
		int len = strlen(context->ctxResponse._appContext);
		int max = sizeof(context->ctxResponse._appContext);
		strncpy(context->ctxResponse._appContext + len, buffer, (size < max - len - 2) ? size : (max - len - 2));
	}
	return size;
}

void Web_OnRequestReceived(REQUEST_CONTEXT* context)
{
	int success;
	char szTemp[256];

	SSI_Context* ctxSSI = (SSI_Context*)context->ctxResponse._appContext;
	if ((context->handler != NULL) && ((context->_options & CGI_OPT_AUTHENTICATOR) != 0))
	{
		if (stricmp(context->_requestPath, WEB_SESSION_CHECK) == 0)
		{ //session exists?
			context->_options &= ~(CGI_OPT_AUTHENTICATOR);

			if (context->_session == NULL)
				context->_result = CODE_UNAUTHORIZED; ////Unauthorized (RFC 7235)
			else
				context->_result = CODE_OK;
			return;
		}

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
			success = AuthCheck(context);
			memset(ctxSSI, 0, sizeof(SSI_Context)); //clear after OnAuthCheck
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
		LogPrint(0, "sizeof(_appContext) error=%d, @%d", context->_result, context->_sid);
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

	if (Strnicmp(context->_extension, "shtml", 5) == 0)
		ctxSSI->_ssi = 1;
	else if (Strnicmp(context->_extension, "shtm", 4) == 0)
		ctxSSI->_ssi = 1;
	else if (Strnicmp(context->_extension, "ssi", 3) == 0)
		ctxSSI->_ssi = 1;

	if ((ctxSSI->_ssi > 0) && (context->handler != NULL))
		context->_options |= CGI_OPT_CHUNK_ENABLED;

	if (context->_responsePath[0] == '/')
		LWIP_sprintf(szTemp, "%s%s", g_szWebAbsRoot, context->_responsePath+1);
	else
		LWIP_sprintf(szTemp, "%s%s", g_szWebAbsRoot, context->_responsePath);

	ctxSSI->_fp = LWIP_fopen(szTemp, "rb");
	if (ctxSSI->_fp != NULL)
	{
		context->_fileHandle = ctxSSI->_fp;
		context->ctxResponse._dwTotal = LWIP_fsize(ctxSSI->_fp);
		LogPrint(LOG_DEBUG_ONLY, "File opened: %s, len=%d, SSI=%d @%d", szTemp, context->ctxResponse._dwTotal, ctxSSI->_ssi, context->_sid);
	}
	else
	{
		ctxSSI->_valid = 0;
		context->_result = -404;
		LogPrint(0, "Failed to open %s, @%d", szTemp, context->_sid);
		return;
	}
}

void Web_SetResponseHeaders(REQUEST_CONTEXT* context, char* HttpCodeInfo)
{
	SSI_Context* ctxSSI = (SSI_Context*)context->ctxResponse._appContext;

	int chunkHeader = 0;
	if (ctxSSI->_ssi > 0)
		chunkHeader = 1;
	else if ((context->handler != NULL) && ((context->_options & CGI_OPT_CHUNK_ENABLED) != 0))
		chunkHeader = 1;

	if (context->_rangeTo > context->_rangeFrom)
	{
		long size = 0;
		if (context->_rangeTo > context->ctxResponse._dwTotal)
			context->_rangeTo = context->ctxResponse._dwTotal;
		size = context->_rangeTo - context->_rangeFrom;

		LWIP_sprintf(context->ctxResponse._sendBuffer, header_range, context->_rangeFrom, context->_rangeTo-1, context->ctxResponse._dwTotal, GetContentType(context), size, "close");

		context->ctxResponse._dwTotal = size;
		LWIP_fseek(context->_fileHandle, context->_rangeFrom);
	}
	else if (chunkHeader > 0)
		LWIP_sprintf(context->ctxResponse._sendBuffer, header_chunked, HttpCodeInfo, GetContentType(context), header_nocache, "close");
	else
		LWIP_sprintf(context->ctxResponse._sendBuffer, (char*)header_generic, HttpCodeInfo, GetContentType(context), context->ctxResponse._dwTotal, "close");

	if ((context->_requestMethod == METHOD_GET) && 
		(context->handler != NULL) &&
		((context->_options & CGI_OPT_AUTHENTICATOR) != 0))
	{ //clear cookie with login page
		char szCookie[128];
		LWIP_sprintf(szCookie, "X-Auth-Token: SID=\r\nSet-Cookie: SID=; Path=/; HttpOnly; max-age=3600\r\n");
		strcat(context->ctxResponse._sendBuffer, szCookie);
	}
	strcat(context->ctxResponse._sendBuffer, CRLF);
	LogPrint(LOG_DEBUG_ONLY, context->ctxResponse._sendBuffer);
}

int Web_ReadContent(REQUEST_CONTEXT* context, char* buffer, int maxSize)
{
	int outputCount = -1; //output count
	SSI_Context* ctxSSI = (SSI_Context*)context->ctxResponse._appContext;
	char* pCache = ctxSSI->_cache; //cache for tag extracting

	if ((context->_requestMethod == METHOD_GET) || (context->_requestMethod == METHOD_POST))
	{
		int maxRead = (context->ctxResponse._dwTotal - context->ctxResponse._dwOperIndex); //available data remained
		if (maxRead == 0)
		{
			if ((ctxSSI->_ssi > 0) && (ctxSSI->_cacheOffset > 0))
			{
				outputCount = ctxSSI->_cacheOffset;
				ctxSSI->_cacheOffset = 0;
				memcpy(buffer, ctxSSI->_cache, outputCount); //read to cache, then parse data in cache

				context->ctxResponse._dwOperStage = STAGE_END;
				return outputCount;
			}
			context->ctxResponse._dwOperStage = STAGE_END;
			return -1; //file already finished, no more data
		}

		outputCount = 0;
		if (ctxSSI->_ssi == 0)
		{ //non-SSI
			if (maxRead > maxSize)
				maxRead = maxSize;
				
			if (ctxSSI->_fp == NULL) //content in memory -> ctxSSI->_buffer
				memcpy(buffer, ctxSSI->_buffer + context->ctxResponse._dwOperIndex, maxRead);
			else
			{
				unsigned int bytes = 0;
				if (0 != LWIP_fread(ctxSSI->_fp, buffer, maxRead, &bytes))
				{
					context->_result = -500;
					LogPrint(0, "Non-SSI LWIP_fread error=%d, @%d", context->_result, context->_sid);
					return 0;
				}
				maxRead = bytes;
			}
			context->ctxResponse._dwOperIndex += maxRead;
			outputCount += maxRead;
		}
		else
		{ //SSI
			int ready2Send = 0;
			int consumed = 0;

			while(context->ctxResponse._dwOperIndex < context->ctxResponse._dwTotal)
			{
				maxRead = (context->ctxResponse._dwTotal - context->ctxResponse._dwOperIndex);
				if (maxRead > sizeof(ctxSSI->_cache) - ctxSSI->_cacheOffset)
					maxRead = sizeof(ctxSSI->_cache) - ctxSSI->_cacheOffset;

				if (ctxSSI->_fp == NULL)
				{
					char* pSrc = ctxSSI->_buffer + context->ctxResponse._dwOperIndex; //const memory
					memcpy(pCache + ctxSSI->_cacheOffset, pSrc, maxRead); //read to cache, then parse data in cache
				}
				else
				{
					unsigned int bytes = 0;
					if (0 != LWIP_fread(ctxSSI->_fp, pCache + ctxSSI->_cacheOffset, maxRead, &bytes))
					{
						context->_result = -500;
						LogPrint(0, "SSI LWIP_fread error=%d, @%d", context->_result, context->_sid);
						return 0;
					}
					maxRead = bytes;
				}
				//LogPrint(LOG_DEBUG_ONLY, "SSI read to cache: %d", maxRead);
				context->ctxResponse._dwOperIndex += maxRead;		//update next index to read
				ctxSSI->_cacheOffset += maxRead;

				//to parse data in ctxSSI->_cache
				consumed = 0;
				while(consumed < ctxSSI->_cacheOffset)
				{
					if (ctxSSI->_ssiState == 0) //0=idle, 1=searching start, 2=searching end
						ctxSSI->_ssiState = 1;

					if (ctxSSI->_ssiState == 1)
					{ //1=searching TAG start
						if (pCache[consumed] != tagStart[0]) //'<'
						{ //copy to send buffer
							buffer[outputCount] = pCache[consumed];

							outputCount++;
							consumed++;
							if (outputCount >= maxSize)
							{
								ready2Send = 1;
								break; //send buffer if full
							}
						}
						else if (consumed <= ctxSSI->_cacheOffset - strlen(tagStart))
						{ //detect start TAG
							if (Strnicmp(pCache+consumed, tagStart, strlen(tagStart)) == 0)
							{ //start found, "<!--#"
								LogPrint(LOG_DEBUG_ONLY, "Tag start found");

								ctxSSI->_ssiState = 2; //start found

								consumed += 5;
								ready2Send = 1;
								break; //tag found, then send out, and leave buffer space for the coming TAG content
							}
							else
							{ //not a TAG
								buffer[outputCount] = pCache[consumed];

								outputCount++;
								consumed++;
								if (outputCount >= maxSize)
								{
									ready2Send = 1;
									break; //send buffer if full
								}
							}
						}
						else
							break; //TAG start flag incomplete
					}
					else if (ctxSSI->_ssiState == 2) //start found, copy name and searching end
					{
						if (pCache[consumed] != tagEnd[0])
						{ //copy to send buffer
							ctxSSI->_tagName[ctxSSI->_tagLength++] = pCache[consumed];
							ctxSSI->_tagName[ctxSSI->_tagLength] = 0;
							consumed++;
						}
						else if (consumed <= ctxSSI->_cacheOffset - strlen(tagEnd))
						{ //detect end TAG
							if (Strnicmp(pCache + consumed, tagEnd, strlen(tagEnd)) == 0)
							{ //end found, "-->"
								int nAppend;
								LogPrint(LOG_DEBUG_ONLY, "Tag end: %s", ctxSSI->_tagName);

								nAppend = ReplaceTag(context, ctxSSI->_tagName, buffer + outputCount, maxSize - outputCount - 50);
								outputCount += nAppend;
								consumed += 3;

								ctxSSI->_ssiState = 0; //end found
								ctxSSI->_tagLength = 0;
								memset(ctxSSI->_tagName, 0, sizeof(ctxSSI->_tagName));
							}
							else
							{
								ctxSSI->_tagName[ctxSSI->_tagLength++] = pCache[consumed];
								ctxSSI->_tagName[ctxSSI->_tagLength] = 0;
								consumed++;
							}
						}
						else
							break; //TAG end flag incomplete
					}
				}

				ctxSSI->_cacheOffset -= consumed;
				if (ctxSSI->_cacheOffset > 0)
					memmove(pCache, pCache + consumed, ctxSSI->_cacheOffset); //shift consumed out

				if (ready2Send > 0)
					break; //ready to send buffer
			}
			//LogPrint(LOG_DEBUG_ONLY, "SSI final remain: off=%d, read=%d", ctxSSI->_cacheOffset, canRead);
		}

		if (context->ctxResponse._dwOperIndex >= context->ctxResponse._dwTotal)
		{
			if ((ctxSSI->_ssi > 0) && (ctxSSI->_cacheOffset > 0))
				context->ctxResponse._dwOperStage++;
			else
			{
				LogPrint(LOG_DEBUG_ONLY, "Web content end reached, @%d", context->_sid);
				context->ctxResponse._dwOperStage = STAGE_END;
			}
		}
		else
			context->ctxResponse._dwOperStage++;

		LogPrint(LOG_DEBUG_ONLY, "Web response sending: %d bytes, %ld/%ld @%d", 
			outputCount, 
			context->ctxResponse._dwOperIndex,
			context->ctxResponse._dwTotal,
			context->_sid);
	}
	return outputCount;
}

void Web_OnAllSent(REQUEST_CONTEXT* context)
{ //all response sent out
	SSI_Context* ctxSSI = (SSI_Context*)context->ctxResponse._appContext;
	if ((ctxSSI != NULL) && (ctxSSI->_fp != NULL))
	{
		LogPrint(LOG_DEBUG_ONLY, "Web file sent and closed");
		
		LWIP_fclose(ctxSSI->_fp);
		ctxSSI->_fp = NULL;
	}
}

void Web_OnFinished(REQUEST_CONTEXT* context)
{ //connection closed
	SSI_Context* ctxSSI = (SSI_Context*)context->ctxResponse._appContext;
	if ((ctxSSI != NULL) && (ctxSSI->_fp != NULL))
	{
		LogPrint(LOG_DEBUG_ONLY, "Web file finished and closed");
		
		LWIP_fclose(ctxSSI->_fp);
		ctxSSI->_fp = NULL;
	}
}
