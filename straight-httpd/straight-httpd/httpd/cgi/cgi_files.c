/*
  cgi_files.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: May 30, 2020
*/

#include "../http_cgi.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

extern void LogPrint(int level, char* format, ... );

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Files_OnRequestReceived(REQUEST_CONTEXT* context);

void Files_SetResponseHeader(REQUEST_CONTEXT* context, char* HttpCodeInfo);
int  Files_ReadOneFileInfo(REQUEST_CONTEXT* context, char* buffer, int maxSize);

struct CGI_Mapping g_cgiFiles = {
	"/api/files.cgi",
	CGI_OPT_AUTH_REQUIRED | CGI_OPT_GET_ENABLED,// unsigned long options;

	NULL, //void (*OnCancel)(REQUEST_CONTEXT* context);
	NULL, //void (*OnHeaderReceived)(REQUEST_CONTEXT* context, char* header_line);
	NULL, //void (*OnHeadersReceived)(REQUEST_CONTEXT* context);
	NULL, //int  (*OnContentReceived)(REQUEST_CONTEXT* context, char* buffer, int size);
	Files_OnRequestReceived, //void (*OnRequestReceived)(REQUEST_CONTEXT* context);
	
	Files_SetResponseHeader, //void (*SetResponseHeader)(REQUEST_CONTEXT* context, char* HttpCode);
	Files_ReadOneFileInfo, //int  (*ReadContent)(REQUEST_CONTEXT* context, char* buffer, int maxSize);
	
	NULL, //int  (*OnAllSent)(REQUEST_CONTEXT* context);
	NULL, //void (*OnFinished)(REQUEST_CONTEXT* context);
	
	NULL //struct CGI_Mapping* next;
};

void Files_OnRequestReceived(REQUEST_CONTEXT* context)
{
	char* p = NULL;
	if (context->_posQuestion >= 0)
	{
		LogPrint(LOG_DEBUG_ONLY, "Request file list of %s @%d",
		context->_requestPath + context->_posQuestion + 1, context->_sid);
		p = strstr(context->_requestPath + context->_posQuestion + 1, "path=");
		if (p != NULL)
			p += 5;
	}

	SSI_Context* ctxSSI = (SSI_Context*)context->ctxResponse._appContext;
	if (sizeof(context->ctxResponse._appContext) < sizeof(SSI_Context))
	{
		ctxSSI->_valid = 0;
		context->_result = -500;
		LogPrint(0, "sizeof(_appContext) error=%d, @%d", context->_result, context->_sid);
		return;
	}

	memset(ctxSSI, 0, sizeof(SSI_Context));
	ctxSSI->_valid = 1;

	LWIP_sprintf(context->_responsePath, "%s%s", WEB_ABS_ROOT, WEB_REL_UPLOAD);

	if (p != NULL)
	{ //default path
		int nLen = 0;
		if (p[0] == '/')
			strcat(context->_responsePath, p+1);
		else
			strcat(context->_responsePath, p);
		nLen = strlen(context->_responsePath);
		if (context->_responsePath[nLen-1] != '/')
			strcat(context->_responsePath, "/");
	}
	strcat(context->_responsePath, "*.*");

	context->_options |= CGI_OPT_CHUNK_ENABLED;
	{
		int isFolder;
		char name[64];
		int size;
		time_t date;
		ctxSSI->_fp = LWIP_firstdir(context->_responsePath, &isFolder, name, sizeof(name), &size, &date);
		if (ctxSSI->_fp != NULL)
		{
			LWIP_closedir(ctxSSI->_fp);
			ctxSSI->_fp = NULL;
		}
	}
}

void Files_SetResponseHeader(REQUEST_CONTEXT* context, char* HttpCodeInfo)
{
	if (context->_requestMethod == METHOD_GET)
	{
		LWIP_sprintf(context->ctxResponse._sendBuffer, (char*)header_chunked, HttpCodeInfo, "application/json", "", "close");
	}
}

int Files_ReadOneFileInfo(REQUEST_CONTEXT* context, char* buffer, int maxSize)
{
	int outputCount = -1; //output count

	SSI_Context* ctxSSI = (SSI_Context*)context->ctxResponse._appContext;
	if (context->_requestMethod == METHOD_GET)
	{
		int isFolder;
		char name[128];
		int size;
		int off = 0;
		time_t date;

		if (context->ctxResponse._dwOperStage == STAGE_END)
		{
			if (ctxSSI->_fp != NULL)
			{
				LWIP_closedir(ctxSSI->_fp);
				ctxSSI->_fp = NULL;
			}
			return -1;
		}

		outputCount = 0;
		while (outputCount < maxSize-200)
		{
			name[0] = 0;
			if (context->ctxResponse._dwOperStage == 0)
			{ //first time to open dir
				context->ctxResponse._dwOperStage++;

				ctxSSI->_fp = LWIP_firstdir(context->_responsePath, &isFolder, name, sizeof(name), &size, &date);
				if (ctxSSI->_fp == NULL)
				{ //empty folder
					strcpy(buffer + outputCount, "{\"result\":0, \"data\":[]}");
					outputCount = strlen(buffer);

					context->ctxResponse._dwOperStage = STAGE_END;
					return outputCount;
				}
			}
			else
			{ //next file item
				context->ctxResponse._dwOperStage++;

				if (LWIP_readdir(ctxSSI->_fp, &isFolder, name, sizeof(name), &size, &date) <= 0)
				{ //no more files
					LWIP_closedir(ctxSSI->_fp);
					ctxSSI->_fp = NULL;

					strcpy(buffer + outputCount, "]}");
					outputCount = strlen(buffer);

					context->ctxResponse._dwOperStage = STAGE_END;
					return outputCount;
				}
			}

			if (name[0] == 0)
				continue;

			if ((name[0] == '.') && (name[1] == 0))
			{
				LogPrint(LOG_DEBUG_ONLY, "Ignore first file: folder=%d, name=%s, size=%d, @%d", isFolder, name, size, context->_sid);
				continue;
			}

			if ((name[0] == '.') && (name[1] == '.') && (name[2] == 0))
			{
				if (strlen(context->_responsePath) == strlen(WEB_ABS_ROOT) + strlen(WEB_REL_UPLOAD) + 3)
				{
					LogPrint(LOG_DEBUG_ONLY, "Reached root: folder=%d, name=%s, size=%d, @%d", isFolder, name, size, context->_sid);
					continue;
				}
			}

			LogPrint(LOG_DEBUG_ONLY, "File item: folder=%d, name=%s, size=%d, @%d", isFolder, name, size, context->_sid);

			if (context->ctxResponse._dwTotal == 0)
			{
				strcpy(buffer + outputCount, "{\"result\":1, \"data\":[");
				outputCount = strlen(buffer);
			}
			else
				buffer[outputCount++] = ',';

			LWIP_sprintf(buffer + outputCount, "{\"folder\":%d", isFolder);
			outputCount = strlen(buffer);
			LWIP_sprintf(buffer + outputCount, ",\"name\":\"%s\"", name);
			outputCount = strlen(buffer);
			LWIP_sprintf(buffer + outputCount, ",\"size\":%d", size);
			outputCount = strlen(buffer);
			LWIP_sprintf(buffer + outputCount, ",\"date\":%d}", (long)date);
			outputCount = strlen(buffer);

			context->ctxResponse._dwTotal++;
		}
	}
	return (outputCount > 0) ? outputCount : -1;
}
