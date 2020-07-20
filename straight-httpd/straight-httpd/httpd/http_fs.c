/*
  http_fs.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: June 28, 2020
*/

#include "http_fs.h"

#include "fs_data.c" //this file can be created using straight-buildfs.exe

#define USE_FILE_IO		1
#if (USE_FILE_IO > 0)
#define LOCAL_WEBROOT	"D:/straight/straight-httpd/straight-httpd/straight-httpd/httpd/cncweb/"
#endif

static http_file_info* g_WebPages = (http_file_info*)g_szWebRoot; //web pages in ROM
static unsigned long g_FilePosition[MAX_FILES]; //read pointers in RAM

void WEB_fs_init(void)
{
	int nCount = 0;
	http_file_info* pWebFile = (http_file_info*)g_szWebRoot;
	while (pWebFile->tLastModified != 0)
	{
		printf("%s, size=%lu, time=%lu\r\n", pWebFile->szfilepath, pWebFile->nSize, pWebFile->tLastModified);
		pWebFile = (http_file_info*)(g_szWebRoot + pWebFile->nOffsetNextFile);
		nCount++;
	}
	memset(g_FilePosition, 0, sizeof(g_FilePosition));

	if (nCount > MAX_FILES)
		printf("MAX_FILES should be greater than %d\r\n", nCount);
}

void* WEB_fopen(const char* szFile, const char* mode)
{
#if (USE_FILE_IO > 0)
	char szTemp[128];
	strcpy(szTemp, LOCAL_WEBROOT);
	if (szTemp[strlen(szTemp) - 1] != '/')
		strcat(szTemp, "/");
	if (szFile[0] != '/')
		strcat(szTemp, szFile);
	else
		strcat(szTemp, szFile+1);
	return LWIP_fopen(szTemp, mode);
#else
	http_file_info* pWebFile = (http_file_info*)g_szWebRoot;
	while (pWebFile->tLastModified != 0)
	{
		if ((strcmp(szFile, pWebFile->szfilepath) == 0) && (pWebFile->nIndex < MAX_FILES))
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
#if (USE_FILE_IO > 0)
	char szTemp[128];
	strcpy(szTemp, LOCAL_WEBROOT);
	if (szTemp[strlen(szTemp) - 1] != '/')
		strcat(szTemp, "/");
	if (szFile[0] != '/')
		strcat(szTemp, szFile);
	else
		strcat(szTemp, szFile + 1);
	return LWIP_ftime(szTemp, buf, maxSize);
#else
	http_file_info* pWebFile = (http_file_info*)g_szWebRoot;
	while (pWebFile->tLastModified != 0)
	{
		if (strcmp(szFile, pWebFile->szfilepath) == 0)
			return pWebFile->tLastModified;
		pWebFile = (http_file_info*)(g_szWebRoot + pWebFile->nOffsetNextFile);
	}
	return 0;
#endif
}

int WEB_fseek(void* f, long offset)
{
#if (USE_FILE_IO > 0)
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
#if (USE_FILE_IO > 0)
	LWIP_fclose(f);
#else
	http_file_info* pWebFile = (http_file_info*)f;
	if (pWebFile->nIndex < MAX_FILES)
		g_FilePosition[pWebFile->nIndex] = 0; //read position
#endif
}

long WEB_fsize(void* f)
{
#if (USE_FILE_IO > 0)
	return LWIP_fsize(f);
#else
	http_file_info* pWebFile = (http_file_info*)f;
	return pWebFile->nSize;
#endif
}

int WEB_fread(void* f, char* buf, int count, unsigned int* bytes) //0=success
{
#if (USE_FILE_IO > 0)
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
