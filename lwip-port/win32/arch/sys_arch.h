#ifndef LWIP_ARCH_SYS_ARCH_H
#define LWIP_ARCH_SYS_ARCH_H

#pragma warning(disable:4996) //_CRT_SECURE_NO_WARNINGS

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

s8_t sys_mutex_new(sys_mutex_t *mutex);
void sys_mutex_lock(sys_mutex_t *mutex);
void sys_mutex_unlock(sys_mutex_t *mutex);

#ifndef MAX_QUEUE_ENTRIES
#define MAX_QUEUE_ENTRIES 512
#endif

struct lwip_mbox 
{
	void* sem;
	void* q_mem[MAX_QUEUE_ENTRIES];
	u32_t head, tail;
};
typedef struct lwip_mbox sys_mbox_t;

#define SYS_MBOX_NULL 			(void *)0
#define sys_mbox_valid_val(mbox) (((mbox).sem != NULL)  && ((mbox).sem != (void*)-1))
#define sys_mbox_valid(mbox) ((mbox != NULL) && sys_mbox_valid_val(*(mbox)))
#define sys_mbox_set_invalid(mbox) ((mbox)->sem = NULL)

typedef void* sys_thread_t;
typedef unsigned int sys_prot_t;

typedef struct FILE LWIP_FIL;
void* LWIP_fopen(const char* szTemp, const char* mode);
int  LWIP_fseek(void* f, long offset);
void LWIP_fclose(void* f);
long LWIP_fsize(void* f);
int  LWIP_fread(void* f, char* buf, int count, unsigned int* bytes); //0=success
int  LWIP_fwrite(void* f, char* buf, int count); //>0:success

u32_t sys_jiffies(void);
u32_t sys_now(void);
u32_t msDiff(u32_t now, u32_t last);

unsigned long LWIP_GetTickCount();
void LogPrint(int level, char* format, ...);
void LWIP_sprintf(char* buf, char* format, ...);

void sys_init(void);
void sys_arch_init();
void sys_arch_protect();
void sys_arch_unprotect();

char* gmt4http(time_t* t);
#endif
