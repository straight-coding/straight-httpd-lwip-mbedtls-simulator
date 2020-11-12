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

#define FAKE_FS		1

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

#if (FAKE_FS > 0)
char* pFakeFile = "@@FakeFile%03dMB.dat";
http_file_info fakeFiles[] __attribute__((aligned(4))) = {
	{ 0,   1*1024*1024, 1605130877+0*60, 0, 0, 0, 0 },
	{ 1,   2*1024*1024, 1605130877+1*60, 0, 0, 0, 0 },
	{ 2,   8*1024*1024, 1605130877+2*60, 0, 0, 0, 0 },
	{ 3,  16*1024*1024, 1605130877+3*60, 0, 0, 0, 0 },
	{ 4,  64*1024*1024, 1605130877+4*60, 0, 0, 0, 0 },
	{ 5, 128*1024*1024, 1605130877+5*60, 0, 0, 0, 0 },
	{ 6, 256*1024*1024, 1605130877+6*60, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0 }
};
#endif

#endif

typedef struct _file_handle
{
	http_file_info* pFileInfo;
	unsigned long lReadPos;
}file_handle;

file_handle g_FileOpened[MAX_FILES];

int WEB_fs_init(void)
{
	int nCount = 0;
	
#if (LOCAL_FILE_SYSTEM > 0)
#else
	http_file_info* pWebFile = (http_file_info*)g_szWebRoot;
	while (pWebFile->tLastModified != 0)
	{
#ifdef WIN32
		printf("%s, size=%lu, time=%lu\r\n", (char*)(pWebFile+1), pWebFile->nSize, pWebFile->tLastModified);
#endif
		pWebFile = (http_file_info*)(g_szWebRoot + pWebFile->nOffsetNextFile);
		nCount++;
	}
#endif
	
	memset(g_FileOpened, 0, sizeof(g_FileOpened));
	return nCount;
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
		if (strcmp(szFile, (char*)(pWebFile+1)) == 0)
		{
			int i;
			for(i = 0; i < MAX_FILES; i ++)
			{
				if (g_FileOpened[i].pFileInfo == NULL)
				{
					g_FileOpened[i].pFileInfo = pWebFile;
					g_FileOpened[i].lReadPos = 0;
					return (&g_FileOpened[i]);
				}
			}
			break;
		}
		pWebFile = (http_file_info*)(g_szWebRoot + pWebFile->nOffsetNextFile);
	}
	
#if (FAKE_FS > 0)
	{
		char szFakeName[128];
		pWebFile = &fakeFiles[0];
		while(pWebFile->nSize > 0)
		{
			sprintf(szFakeName, pFakeFile, pWebFile->nSize >> 20);
			if ((strstr(szFile, szFakeName) != NULL) || (strchr(mode, 'w') != NULL))
			{
				int i;
				for(i = 0; i < MAX_FILES; i ++)
				{
					if (g_FileOpened[i].pFileInfo == NULL)
					{
						g_FileOpened[i].pFileInfo = pWebFile;
						g_FileOpened[i].lReadPos = 0;
						return (&g_FileOpened[i]);
					}
				}
				break;
			}
			pWebFile ++;
		}
	}
#endif
	return NULL;
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
	
#if (FAKE_FS > 0)
	{
		char szFakeName[128];
		pWebFile = &fakeFiles[0];
		while(pWebFile->nSize > 0)
		{
			sprintf(szFakeName, pFakeFile, pWebFile->nSize >> 20);
			if (strcmp(szFile, szFakeName) == 0)
			{
				return pWebFile->tLastModified;
			}
			pWebFile ++;
		}
	}
#endif
	
	return 0;
#endif
}

int WEB_fseek(void* f, long offset)
{
#if (LOCAL_FILE_SYSTEM > 0)
	return LWIP_fseek(f, offset);
#else
	file_handle* pWebFile = (file_handle*)f;
	if (pWebFile != NULL)
		pWebFile->lReadPos = offset;
	return offset;
#endif
}

void WEB_fclose(void* f)
{
#if (LOCAL_FILE_SYSTEM > 0)
	LWIP_fclose(f);
#else
	file_handle* pWebFile = (file_handle*)f;
	if (pWebFile != NULL)
	{
		pWebFile->pFileInfo = NULL;
		pWebFile->lReadPos = 0;
	}
#endif
}

long WEB_fsize(void* f)
{
#if (LOCAL_FILE_SYSTEM > 0)
	return LWIP_fsize(f);
#else
	file_handle* pWebFile = (file_handle*)f;
	if ((pWebFile != NULL) && (pWebFile->pFileInfo != NULL))
		return pWebFile->pFileInfo->nSize;
	return 0;
#endif
}

int WEB_fread(void* f, char* buf, int count, unsigned int* bytes) //0=success
{
#if (LOCAL_FILE_SYSTEM > 0)
	return LWIP_fread(f, buf, count, bytes);
#else
	int nCount = 0;
	file_handle* pWebFile = (file_handle*)f;
	if ((pWebFile != NULL) && (pWebFile->pFileInfo != NULL))
	{
		nCount = pWebFile->pFileInfo->nSize - pWebFile->lReadPos;
		if (nCount > count)
			nCount = count;
		
		if (pWebFile->pFileInfo->nOffsetFileData == 0)
		{ //fake file
			memset(buf, 'A', nCount);
		}
		else
		{
			memcpy(buf, (char*)(g_szWebRoot + pWebFile->pFileInfo->nOffsetFileData + pWebFile->lReadPos), nCount);
		}
		pWebFile->lReadPos += nCount;

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
#if (FAKE_FS > 0)
	return toWrite;
#else
	return -1;
#endif
#endif
}

void* WEB_firstdir(void* filter, int* isFolder, char* name, int maxLen, int* size, unsigned long long* date)   /* date/time in unix secs past 1-Jan-70 */
{
#if (LOCAL_FILE_SYSTEM > 0)
	return LWIP_firstdir(filter, isFolder, name, maxLen, size, (time_t*)date);
#else
#if (FAKE_FS > 0)
	{	
		int i;
		for(i = 0; i < MAX_FILES; i ++)
		{
			if (g_FileOpened[i].pFileInfo == NULL)
			{
				g_FileOpened[i].lReadPos = 0;
				g_FileOpened[i].pFileInfo = &fakeFiles[0];
				
				*isFolder = 0;
				*size = fakeFiles[0].nSize;
				*date = fakeFiles[0].tLastModified;
				snprintf(name, maxLen, pFakeFile, (fakeFiles[0].nSize >> 20));
				return (&g_FileOpened[i]);
			}
		}
	}
#endif	
	return NULL;
#endif
}

int WEB_readdir(void* hFind, int* isFolder, char* name, int maxLen, int* size, unsigned long long* date)     /* date/time in unix secs past 1-Jan-70 */
{
#if (LOCAL_FILE_SYSTEM > 0)
	return LWIP_readdir(hFind, isFolder, name, maxLen, size, (time_t*)date);
#else
#if (FAKE_FS > 0)
	file_handle* pWebFile = (file_handle*)hFind;
	if (pWebFile != NULL)
	{
		pWebFile->lReadPos ++;
		pWebFile->pFileInfo = &fakeFiles[pWebFile->lReadPos];

		if (pWebFile->pFileInfo->nSize > 0)
		{
			*isFolder = 0;
			*size = pWebFile->pFileInfo->nSize;
			*date = pWebFile->pFileInfo->tLastModified;
			snprintf(name, maxLen, pFakeFile, ((*size) >> 20));
			return 1;
		}
	}
#endif	
	return 0;
#endif
}

void WEB_closedir(void* hFind)
{
#if (LOCAL_FILE_SYSTEM > 0)
	LWIP_closedir(hFind);
#else
#if (FAKE_FS > 0)
	file_handle* pWebFile = (file_handle*)hFind;
	if (pWebFile != NULL)
	{
		pWebFile->pFileInfo = NULL;
		pWebFile->lReadPos = 0;
	}
#endif	
#endif
}
