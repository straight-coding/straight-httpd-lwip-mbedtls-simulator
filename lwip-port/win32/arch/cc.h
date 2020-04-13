#ifndef __CC_H__
#define __CC_H__

#include "cpu.h"
#include "stdio.h"

/*-------------data type------------------------------------------------------*/

typedef unsigned   char		u8_t;    /* Unsigned 8 bit quantity         */
typedef signed     char		s8_t;    /* Signed    8 bit quantity        */
typedef unsigned   short	u16_t;   /* Unsigned 16 bit quantity        */
typedef signed     short	s16_t;   /* Signed   16 bit quantity        */
typedef unsigned   long		u32_t;   /* Unsigned 32 bit quantity        */
typedef signed     long		s32_t;   /* Signed   32 bit quantity        */

typedef signed     long		mem_ptr_t; /* Unsigned 32 bit quantity        */

/*----------------------------------------------------------------------------*/

/* define compiler specific symbols */
#if defined (WIN32)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#elif defined (__ICCARM__)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT 
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_USE_INCLUDES

#elif defined (__CC_ARM)

#define PACK_STRUCT_BEGIN __packed
#define PACK_STRUCT_STRUCT 
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#elif defined (__GNUC__)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT __attribute__ ((__packed__))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#elif defined (__TASKING__)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#endif

/*---define (sn)printf formatters for these lwip types, for lwip DEBUG/STATS--*/

#define U16_F "4d"
#define S16_F "4d"
#define X16_F "4x"
#define U32_F "8ld"
#define S32_F "8ld"
#define X32_F "8lx"

/*--------------macros--------------------------------------------------------*/
#ifndef LWIP_PLATFORM_ASSERT
//#define LWIP_PLATFORM_ASSERT(x) //do {LwipLogPrint x;} while(0)
#define LWIP_PLATFORM_ASSERT(x) \
    do \
    {   printf("Assertion \"%s\" failed at line %d in %s\n", x, __LINE__, __FILE__); \
    } while(0)
#endif

#ifndef LWIP_PLATFORM_DIAG
#define LWIP_PLATFORM_DIAG(x) do {LwipLogPrint x;} while(0)
//#define LWIP_PLATFORM_DIAG(x) \
//    do \
//    {   printf("Assertion \"%s\" failed at line %d in %s\n", x, __LINE__, __FILE__); \
//    } while(0)
#endif

#endif /* __CC_H__ */
