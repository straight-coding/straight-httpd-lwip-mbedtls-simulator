/*
  http_fs.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: June 28, 2020
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arch/sys_arch.h"
#include "http_fs.h"

#ifdef _WIN32
#pragma warning(disable:4996) //_CRT_SECURE_NO_WARNINGS
#endif

#if (LOCAL_FILE_SYSTEM > 0)
#define LOCAL_WEBROOT	"D:/straight/straight-httpd/straight-httpd/straight-httpd/httpd/cncweb/"

extern void* LWIP_fopen(const char* szTemp, const char* mode);
extern time_t LWIP_ftime(char* fname, char* buf, int maxSize);
extern int  LWIP_fseek(void* f, long offset);
extern void LWIP_fclose(void* f);
extern long LWIP_fsize(void* f);
extern int  LWIP_fread(void* f, char* buf, int count, unsigned int* bytes); //0=success
extern int  LWIP_fwrite(void* f, char* buf, int count); //>0:success

#else
#include "fs_data.c" //this file can be created using straight-buildfs.exe
//static http_file_info* g_WebPages = (http_file_info*)g_szWebRoot; //web pages in ROM
#endif

static unsigned long g_FilePosition[MAX_FILES]; //read pointers in RAM

void WEB_fs_init(void)
{
#if (LOCAL_FILE_SYSTEM > 0)
#else
	int nCount = 0;
	http_file_info* pWebFile = (http_file_info*)g_szWebRoot;
	while (pWebFile->tLastModified != 0)
	{
#ifdef WIN32
		printf("%s, size=%lu, time=%lu\r\n", (char*)(pWebFile+1), pWebFile->nSize, pWebFile->tLastModified);
#endif
		pWebFile = (http_file_info*)(g_szWebRoot + pWebFile->nOffsetNextFile);
		nCount++;
	}
	memset(g_FilePosition, 0, sizeof(g_FilePosition));

	if (nCount > MAX_FILES)
	{
#ifdef WIN32
		printf("MAX_FILES should be greater than %d\r\n", nCount);
#endif
	}
#endif
}

void* WEB_fopen(const char* szFile, const char* mode)
{
#if (LOCAL_FILE_SYSTEM > 0)
	if (strchr(szFile, ':') == NULL)
	{
		char szTemp[128];
		strcpy(szTemp, LOCAL_WEBROOT);
		if (szTemp[strlen(szTemp) - 1] != '/')
			strcat(szTemp, "/");
		if (szFile[0] != '/')
			strcat(szTemp, szFile);
		else
			strcat(szTemp, szFile+1);
		return LWIP_fopen(szTemp, mode);
	}
	return LWIP_fopen(szFile, mode);
#else
	http_file_info* pWebFile = (http_file_info*)g_szWebRoot;
	while (pWebFile->tLastModified != 0)
	{
		if ((strcmp(szFile, (char*)(pWebFile+1)) == 0) && (pWebFile->nIndex < MAX_FILES))
		{
			g_FilePosition[pWebFile->nIndex] = 0; //read position
			return pWebFile;
		}
		pWebFile = (http_file_info*)(g_szWebRoot + pWebFile->nOffsetNextFile);
	}
	return 0;
#endif
}

unsigned long WEB_ftime(char* szFile, char* buf, int maxSize)
{
#if (LOCAL_FILE_SYSTEM > 0)
	char szTemp[128];
	strcpy(szTemp, LOCAL_WEBROOT);
	if (szTemp[strlen(szTemp) - 1] != '/')
		strcat(szTemp, "/");
	if (szFile[0] != '/')
		strcat(szTemp, szFile);
	else
		strcat(szTemp, szFile + 1);
	return (unsigned long)LWIP_ftime(szTemp, buf, maxSize);
#else
	http_file_info* pWebFile = (http_file_info*)g_szWebRoot;
	while (pWebFile->tLastModified != 0)
	{
		if (strcmp(szFile, (char*)(pWebFile+1)) == 0)
			return pWebFile->tLastModified;
		pWebFile = (http_file_info*)(g_szWebRoot + pWebFile->nOffsetNextFile);
	}
	return 0;
#endif
}

int WEB_fseek(void* f, long offset)
{
#if (LOCAL_FILE_SYSTEM > 0)
	return LWIP_fseek(f, offset);
#else
	http_file_info* pWebFile = (http_file_info*)f;
	if (pWebFile->nIndex < MAX_FILES)
		g_FilePosition[pWebFile->nIndex] = offset; //read position
	return offset;
#endif
}

void WEB_fclose(void* f)
{
#if (LOCAL_FILE_SYSTEM > 0)
	LWIP_fclose(f);
#else
	http_file_info* pWebFile = (http_file_info*)f;
	if (pWebFile->nIndex < MAX_FILES)
		g_FilePosition[pWebFile->nIndex] = 0; //read position
#endif
}

long WEB_fsize(void* f)
{
#if (LOCAL_FILE_SYSTEM > 0)
	return LWIP_fsize(f);
#else
	http_file_info* pWebFile = (http_file_info*)f;
	return pWebFile->nSize;
#endif
}

int WEB_fread(void* f, char* buf, int count, unsigned int* bytes) //0=success
{
#if (LOCAL_FILE_SYSTEM > 0)
	return LWIP_fread(f, buf, count, bytes);
#else
	int nCount = 0;
	http_file_info* pWebFile = (http_file_info*)f;
	if (pWebFile->nIndex < MAX_FILES)
	{
		nCount = pWebFile->nSize - g_FilePosition[pWebFile->nIndex];
		if (nCount > count)
			nCount = count;
		memcpy(buf, (char*)(g_szWebRoot + pWebFile->nOffsetFileData + g_FilePosition[pWebFile->nIndex]), nCount);

		g_FilePosition[pWebFile->nIndex] += nCount;

		*bytes = nCount;
		return 0;
	}
	return -1;
#endif
}

int WEB_fwrite(void* f, char* buf, int toWrite) //>0: success
{
#if (LOCAL_FILE_SYSTEM > 0)
	return LWIP_fwrite((FILE*)f, buf, toWrite);
#else
	return -1;
#endif
}

void* WEB_firstdir(void* filter, int* isFolder, char* name, int maxLen, int* size, unsigned long long* date)   /* date/time in unix secs past 1-Jan-70 */
{
#if (LOCAL_FILE_SYSTEM > 0)
	return LWIP_firstdir(filter, isFolder, name, maxLen, size, (time_t*)date);
#else
	return NULL;
#endif
}

int WEB_readdir(void* hFind, int* isFolder, char* name, int maxLen, int* size, unsigned long long* date)     /* date/time in unix secs past 1-Jan-70 */
{
#if (LOCAL_FILE_SYSTEM > 0)
	return LWIP_readdir(hFind, isFolder, name, maxLen, size, (time_t*)date);
#else
	return 0;
#endif
}

void WEB_closedir(void* hFind)
{
#if (LOCAL_FILE_SYSTEM > 0)
	LWIP_closedir(hFind);
#else
#endif
}
