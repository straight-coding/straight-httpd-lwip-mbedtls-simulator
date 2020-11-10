/*
  port_file.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: June 20, 2020
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "arch/sys_arch.h"

#ifdef WIN32
#include <windows.h>
#include <direct.h>
#include <sys/stat.h>
#pragma warning(disable:4996) //_CRT_SECURE_NO_WARNINGS
extern char* gmt4http(time_t* t, char* out, int maxSize);
#endif

void* LWIP_fopen(const char* szTemp, const char* mode)
{
#if (LOCAL_FILE_SYSTEM > 0)
	FILE *f;
	//if (0 == _wfopen_s(&f, szTemp, mode))
		//return f;
	errno_t err = fopen_s(&f, szTemp, mode);
	if (0 == err)
		return (void*)f;
#endif
	return 0;
}

int LWIP_fread(void* f, char* buf, int toRead, unsigned int* outBytes) //0=success
{
#if (LOCAL_FILE_SYSTEM > 0)
	unsigned int size = 0;
	*outBytes = 0;
	
	size = fread(buf, 1, toRead, (FILE*)f);
	if (size >= 0)
	{
		*outBytes = size;
		return 0;
	}
#endif
	
	return -1;
}

int LWIP_fseek(void* f, long offset)
{
	if (f == NULL)
		return -1;
#if (LOCAL_FILE_SYSTEM > 0)
	return fseek((FILE*)f, offset, SEEK_SET);
#else
	return -1;
#endif
}

int LWIP_fwrite(void* f, char* buf, int toWrite) //>0: success
{
	if (f == NULL)
		return -1;
#if (LOCAL_FILE_SYSTEM > 0)
	return fwrite(buf, 1, toWrite, (FILE*)f);
#else
	return -1;
#endif
}

time_t LWIP_ftime(char* fname, char* buf, int maxSize)
{
#if (LOCAL_FILE_SYSTEM > 0)
	int result;
	struct _stat stat;

	buf[0] = 0;
	if (fname[0] == 0)
		return 0;

	result = _stat(fname, &stat);
	if (result == 0)
	{
		gmt4http(&stat.st_mtime, buf, maxSize);
		return stat.st_mtime;
	}
#endif
	return 0;
}

long LWIP_fsize(void* f)
{
	long size = 0;
#if (LOCAL_FILE_SYSTEM > 0)
	int pos = 0;

	if (f == NULL)
		return 0;

	pos = ftell((FILE*)f);

	fseek((FILE*)f, 0, SEEK_END);
	size = ftell((FILE*)f);
	fseek((FILE*)f, pos, SEEK_SET);
#endif
	return size;
}

void LWIP_fclose(void* f)
{
#if (LOCAL_FILE_SYSTEM > 0)
	fclose((FILE*)f);
#endif
}

#if (LOCAL_FILE_SYSTEM > 0)
void TimetToFileTime(time_t t, LPFILETIME pft)
{
	LONGLONG ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}

time_t filetime_to_timet(FILETIME* ft)
{
	ULARGE_INTEGER ull;
	ull.LowPart = ft->dwLowDateTime;
	ull.HighPart = ft->dwHighDateTime;
	return ull.QuadPart / 10000000ULL - 11644473600ULL;
}
#endif

void* LWIP_firstdir(void* filter, int* isFolder, char* name, int maxLen, int* size, time_t* date)
{
#if (LOCAL_FILE_SYSTEM > 0)
	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(filter, &fd);

	*isFolder = 0;
	memset(name, 0, maxLen);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			*isFolder = 1;
		else
			*isFolder = 0;

		strncpy(name, fd.cFileName, maxLen - 1);
		*size = fd.nFileSizeLow;
		*date = filetime_to_timet(&fd.ftLastWriteTime);
		return (void*)hFind;
	}
#endif
	return NULL;
}

int LWIP_readdir(void* hFind, int* isFolder, char* name, int maxLen, int* size, time_t* date)
{
#if (LOCAL_FILE_SYSTEM > 0)
	WIN32_FIND_DATA fd;
	memset(name, 0, maxLen);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		if (FindNextFile((HANDLE)hFind, &fd))
		{
			if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				*isFolder = 1;
			else
				*isFolder = 0;

			strncpy(name, fd.cFileName, maxLen - 1);
			*size = fd.nFileSizeLow;
			*date = filetime_to_timet(&fd.ftLastWriteTime);
			return 1;
		}
	}
#endif
	return 0;
}

void LWIP_closedir(void* hFind)
{
#if (LOCAL_FILE_SYSTEM > 0)
	if (hFind != NULL)
		FindClose((HANDLE)hFind);
#endif
}

void MakeDeepPath(char* szPath)
{
#if (LOCAL_FILE_SYSTEM > 0)
	char	*p1, *p2, chTmp;
	char	szTemp[256];

	memset(szTemp, 0, sizeof(szTemp));
	strncpy(szTemp, szPath, sizeof(szTemp)-1);

	p1 = szTemp;
	while (*p1 != 0)
	{
		if (*p1 == '\\')
			*p1 = '/';
		p1++;
	}

	p1 = szTemp;
	while (1)
	{
		p2 = strchr(p1, '/');
		if (p2 == NULL)
			break;
		chTmp = p2[0];
		p2[0] = 0;
		_mkdir(szTemp);
		p2[0] = chTmp;
		p1 = p2 + 1;
	}
#endif
}
