#ifndef LWIP_ARCH_SYS_ARCH_H
#define LWIP_ARCH_SYS_ARCH_H

#include "lwip/err.h"

#ifdef WIN32
#pragma warning(disable:4996) //_CRT_SECURE_NO_WARNINGS
#endif

///////////////////////////////////////////////////////////////////////////////////////////
// OS
///////////////////////////////////////////////////////////////////////////////////////////

#define SYS_ARCH_DECL_PROTECT(lev)	
#define SYS_ARCH_PROTECT(lev)		sys_arch_protect()
#define SYS_ARCH_UNPROTECT(lev)		sys_arch_unprotect()

struct _sys_sem 
{
	void *sem;
};
typedef struct _sys_sem sys_sem_t;

#define sys_sem_valid_val(sema) (((sema).sem != NULL)  && ((sema).sem != (void*)-1))
#define sys_sem_valid(sema) (((sema) != NULL) && sys_sem_valid_val(*(sema)))
#define sys_sem_set_invalid(sema) ((sema)->sem = NULL)

struct _sys_mut 
{
	void *mut;
};
typedef struct _sys_mut sys_mutex_t;

#define SYS_SEM_NULL  			(void *)0
#define sys_mutex_valid_val(mutex) (((mutex).mut != NULL)  && ((mutex).mut != (void*)-1))
#define sys_mutex_valid(mutex) (((mutex) != NULL) && sys_mutex_valid_val(*(mutex)))
#define sys_mutex_set_invalid(mutex) ((mutex)->mut = NULL)

err_t sys_mutex_new(sys_mutex_t *mutex);
void sys_mutex_lock(sys_mutex_t *mutex);
void sys_mutex_unlock(sys_mutex_t *mutex);

#ifndef MAX_QUEUE_ENTRIES
#define MAX_QUEUE_ENTRIES 512
#endif

struct lwip_mbox 
{
	void* sem;
	void* q_mem[MAX_QUEUE_ENTRIES];
	unsigned long head, tail;
};
typedef struct lwip_mbox sys_mbox_t;

#define SYS_MBOX_NULL 			(void *)0
#define sys_mbox_valid_val(mbox) (((mbox).sem != NULL)  && ((mbox).sem != (void*)-1))
#define sys_mbox_valid(mbox) ((mbox != NULL) && sys_mbox_valid_val(*(mbox)))
#define sys_mbox_set_invalid(mbox) ((mbox)->sem = NULL)

typedef void* sys_thread_t;
typedef unsigned int sys_prot_t;

unsigned long sys_jiffies(void);
unsigned long sys_now(void);
unsigned long msDiff(long long now, long long last);

unsigned long LWIP_GetTickCount();
void LogPrint(int level, char* format, ...);
void LWIP_sprintf(char* buf, char* format, ...);

void sys_init(void);
void sys_arch_init();
void sys_arch_protect();
void sys_arch_unprotect();

char* getClock(char* buf, int maxSize);
time_t parseHttpDate(char* s);

extern const char wday_name[][4];
extern const char mon_name[][4];

///////////////////////////////////////////////////////////////////////////////////////////
// File
///////////////////////////////////////////////////////////////////////////////////////////
#define LOCAL_FILE_SYSTEM	1

typedef struct FILE LWIP_FIL;
void* LWIP_fopen(const char* szTemp, const char* mode);
time_t LWIP_ftime(char* fname, char* buf, int maxSize);
int  LWIP_fseek(void* f, long offset);
void LWIP_fclose(void* f);
long LWIP_fsize(void* f);
int  LWIP_fread(void* f, char* buf, int count, unsigned int* bytes); //0=success
int  LWIP_fwrite(void* f, char* buf, int count); //>0:success

void  MakeDeepPath(char* szPath);
void* LWIP_firstdir(void* filter, int* isFolder, char* name, int maxLen, int* size, time_t* date);
int   LWIP_readdir(void* hFind, int* isFolder, char* name, int maxLen, int* size, time_t* date);
void  LWIP_closedir(void* hFind);

#endif
