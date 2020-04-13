#include <stdlib.h>
#include <stdio.h>

#include <windows.h>
#include <time.h>
#include <stdarg.h>

#include <lwip/arch.h>
#include <lwip/stats.h>
#include <lwip/sys.h>

#include "../../../straight-httpd/straight-httpd/utils.h"

unsigned long LWIP_GetTickCount()
{
	return (u32_t)GetTickCount();
}

u32_t sys_jiffies(void)
{
	return (u32_t)GetTickCount();
}

u32_t sys_now(void)
{
	return (u32_t)GetTickCount();
}

u32_t msDiff(u32_t now, u32_t last)
{
	if (now >= last)
		return (now - last);
	return ((u32_t)0xFFFFFFFF - last + now);
}

CRITICAL_SECTION g_critSec;
void sys_arch_init()
{
	InitializeCriticalSection(&g_critSec);
}

void sys_arch_protect()
{
	EnterCriticalSection(&g_critSec);
}

void sys_arch_unprotect()
{
	LeaveCriticalSection(&g_critSec);
}

void sys_init(void)
{
	sys_arch_init();
}

err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
	HANDLE new_sem = CreateSemaphore(0, count, 100000, 0);
	if (new_sem != NULL) 
	{
		sem->sem = new_sem;
		return ERR_OK;
	}
	sem->sem = NULL;
	return ERR_MEM;
}

void sys_sem_free(sys_sem_t *sem)
{
	if (sem->sem != NULL)
		CloseHandle(sem->sem);
	sem->sem = NULL;
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
	DWORD ret;
	LONGLONG starttime, endtime;

	if (timeout == 0) 
	{	/* wait infinite */
		starttime = (u32_t)GetTickCount();
		ret = WaitForSingleObject(sem->sem, INFINITE);
		endtime = (u32_t)GetTickCount();

		return msDiff(endtime, starttime);
	}

	starttime = (u32_t)GetTickCount();
	ret = WaitForSingleObject(sem->sem, timeout);
	if (ret == WAIT_OBJECT_0) 
	{
		endtime = (u32_t)GetTickCount();

		return msDiff(endtime, starttime);
	}
	return SYS_ARCH_TIMEOUT;
}

void sys_sem_signal(sys_sem_t *sem)
{
	if (sem->sem != NULL)
		ReleaseSemaphore(sem->sem, 1, NULL);
}

err_t sys_mutex_new(sys_mutex_t *mutex)
{
	HANDLE new_mut = CreateMutex(NULL, FALSE, NULL);
	if (new_mut != NULL) 
	{
		mutex->mut = new_mut;
		return ERR_OK;
	}
	mutex->mut = NULL;
	return ERR_MEM;
}

void sys_mutex_free(sys_mutex_t *mutex)
{
	if (mutex->mut != NULL)
		CloseHandle(mutex->mut);
	mutex->mut = NULL;
}

void sys_mutex_lock(sys_mutex_t *mutex)
{
	WaitForSingleObject(mutex->mut, INFINITE);
}

void sys_mutex_unlock(sys_mutex_t *mutex)
{
	ReleaseMutex(mutex->mut);
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn function, void *arg, int stacksize, int prio)
{
	return 0;
}

err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
	mbox->sem = CreateSemaphore(0, 0, MAX_QUEUE_ENTRIES, 0);
	if (mbox->sem == NULL) 
		return ERR_MEM;

	memset(&mbox->q_mem, 0, sizeof(u32_t)*MAX_QUEUE_ENTRIES);
	mbox->head = 0;
	mbox->tail = 0;

	return ERR_OK;
}

void sys_mbox_free(sys_mbox_t *mbox)
{
	if (mbox->sem != NULL)
		CloseHandle(mbox->sem);
	mbox->sem = NULL;
}

void sys_mbox_post(sys_mbox_t *q, void *msg)
{
	BOOL ret;
	SYS_ARCH_DECL_PROTECT(lev);
	SYS_ARCH_PROTECT(lev);

	q->q_mem[q->head] = msg;
	q->head++;
	if (q->head >= MAX_QUEUE_ENTRIES)
		q->head = 0;

	ret = ReleaseSemaphore(q->sem, 1, 0);

	SYS_ARCH_UNPROTECT(lev);
}

err_t sys_mbox_trypost(sys_mbox_t *q, void *msg)
{
	u32_t new_head;
	BOOL ret;
	SYS_ARCH_DECL_PROTECT(lev);

	SYS_ARCH_PROTECT(lev);

	new_head = q->head + 1;
	if (new_head >= MAX_QUEUE_ENTRIES) 
		new_head = 0;

	if (new_head == q->tail) 
	{
		SYS_ARCH_UNPROTECT(lev);
		return ERR_MEM;
	}

	q->q_mem[q->head] = msg;
	q->head = new_head;

	ret = ReleaseSemaphore(q->sem, 1, 0);

	SYS_ARCH_UNPROTECT(lev);
	return ERR_OK;
}

err_t sys_mbox_trypost_fromisr(sys_mbox_t *q, void *msg)
{
	return sys_mbox_trypost(q, msg);
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *q, void **msg, u32_t timeout)
{
	DWORD ret;
	LONGLONG starttime, endtime;
	SYS_ARCH_DECL_PROTECT(lev);

	if (timeout == 0) 
		timeout = INFINITE;

	starttime = (u32_t)GetTickCount();
	ret = WaitForSingleObject(q->sem, timeout);
	if (ret == WAIT_OBJECT_0) 
	{
		SYS_ARCH_PROTECT(lev);
		if (msg != NULL) 
			*msg = q->q_mem[q->tail];

		q->tail++;
		if (q->tail >= MAX_QUEUE_ENTRIES) 
			q->tail = 0;

		SYS_ARCH_UNPROTECT(lev);

		endtime = (u32_t)GetTickCount();
		return msDiff(endtime, starttime);
	}

	if (msg != NULL) 
		*msg = NULL;

	return SYS_ARCH_TIMEOUT;
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *q, void **msg)
{
	DWORD ret;
	SYS_ARCH_DECL_PROTECT(lev);

	ret = WaitForSingleObject(q->sem, 0);
	if (ret == WAIT_OBJECT_0) 
	{
		SYS_ARCH_PROTECT(lev);
		if (msg != NULL) 
			*msg = q->q_mem[q->tail];

		q->tail++;
		if (q->tail >= MAX_QUEUE_ENTRIES) 
			q->tail = 0;

		SYS_ARCH_UNPROTECT(lev);
		return 0;
	}

	if (msg != NULL) 
		*msg = NULL;

	return SYS_MBOX_EMPTY;
}

void LogPrint(int level, char* format, ...)
{
	u32_t len;
	char sMsg[2048];

	va_list ap;

	SYSTEMTIME st, lt;
	GetSystemTime(&st);
	GetLocalTime(&lt);

	va_start(ap, format);
	memset(sMsg, 0, sizeof(sMsg));
	if ((format[0] == '%') && (format[1] == 'T'))
	{
		snprintf(sMsg, 14, "%02d:%02d:%02d.%03d ", lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds);
		vsnprintf(sMsg + 13, sizeof(sMsg) - 16, format + 2, ap);
	}
	else if ((format[0] == '\r') && (format[1] == '\n'))
	{
		vsnprintf(sMsg, sizeof(sMsg) - 1, format, ap);
	}
	else if ((format[0] == '%') && (format[1] == 'C'))
	{ //command
		vsnprintf(sMsg, sizeof(sMsg) - 1, format + 2, ap);
	}
	else
	{
		snprintf(sMsg, 14, "%02d:%02d:%02d.%03d ", lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds);
		vsnprintf(sMsg + 13, sizeof(sMsg) - 16, format, ap);
	}
	va_end(ap);

	printf(sMsg);

	len = strlen(sMsg);
	if ((sMsg[len - 1] != '\r') && (sMsg[len - 1] != '\n'))
		printf("\r\n");
}

void LwipLogPrint(char* format, ...)
{
	u32_t len;
	char szMsg[2048];

	SYSTEMTIME st, lt;
	GetSystemTime(&st);
	GetLocalTime(&lt);

	va_list ap;
	va_start(ap, format);
	{
		snprintf(szMsg, 14, "%02d:%02d:%02d.%03d ", lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds);
		vsnprintf(szMsg + 13, sizeof(szMsg) - 16, format, ap);
	}
	va_end(ap);

	printf(szMsg);

	len = strlen(szMsg);
	if ((szMsg[len - 1] != '\r') && (szMsg[len - 1] != '\n'))
		printf("\r\n");
}

void LWIP_sprintf(char* buf, char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	vsnprintf(buf, 2048, format, ap);
	va_end(ap);
}

LWIP_FIL* LWIP_fopen(char* szTemp, char* mode)
{
	return 0;
}

int LWIP_fread(LWIP_FIL* f, char* buf, int count, unsigned int* bytes) //0=success
{
	return 0;
}

long LWIP_fsize(LWIP_FIL* f)
{
	return 0;
}

void LWIP_fclose(LWIP_FIL* f)
{
}
