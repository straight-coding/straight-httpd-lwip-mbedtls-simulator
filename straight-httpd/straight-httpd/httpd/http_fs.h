/*
  http_fs.h
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: June 28, 2020
*/

#ifndef _HTTP_FS_H_
#define _HTTP_FS_H_

#define MAX_FILES	32

typedef struct _http_file_info
{
	unsigned long nIndex; //index for read pointer
	unsigned long tLastModified;
	unsigned long nSize;
	unsigned long nOffsetFileData; //file content offset
	unsigned long nOffsetNextFile; // ==> _http_file_info*
	char  szfilepath[0];
}http_file_info;

void WEB_fs_init(void);

void* WEB_fopen(const char* szTemp, const char* mode);
unsigned long WEB_ftime(char* fname, char* buf, int maxSize);
int  WEB_fseek(void* f, long offset);
void WEB_fclose(void* f);
long WEB_fsize(void* f);
int  WEB_fread(void* f, char* buf, int count, unsigned int* bytes); //0=success

#endif
