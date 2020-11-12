/*
  http_fs.h
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: June 28, 2020
*/

#ifndef _HTTP_FS_H_
#define _HTTP_FS_H_

#define MAX_FILES	32

#define ATTRI_GZIP	0x00000001

#ifdef WIN32
#define __attribute__(x)
#endif

typedef struct _http_file_info
{
	unsigned long nIndex; //index for read pointer
	unsigned long nSize; //file size
	unsigned long long tLastModified; //file date
	unsigned long nAttributes; //ATTRI_GZIP
	unsigned long nReserved;   //padding for 4-byte alignment
	unsigned long nOffsetFileData; //file content offset
	unsigned long nOffsetNextFile; // ==> _http_file_info*
}http_file_info __attribute__((aligned(4)));

int WEB_fs_init(void);

void* WEB_fopen(const char* szTemp, const char* mode);
unsigned long WEB_ftime(char* fname, char* buf, int maxSize);
int  WEB_fseek(void* f, long offset);
void WEB_fclose(void* f);
long WEB_fsize(void* f);
int  WEB_fread(void* f, char* buf, int count, unsigned int* bytes); //0=success
int  WEB_fwrite(void* f, char* buf, int count); //>0:success

void* WEB_firstdir(void* filter, int* isFolder, char* name, int maxLen, int* size, unsigned long long* date);   /* date/time in unix secs past 1-Jan-70 */
int   WEB_readdir(void* hFind, int* isFolder, char* name, int maxLen, int* size, unsigned long long* date);     /* date/time in unix secs past 1-Jan-70 */
void  WEB_closedir(void* hFind);

#endif
