///////////////////////////////////////////////////////////////////////////////////////
// in order to overwrite definitions, include this file at the end of config.h

///////////////////////////////////////////////////////////////////////////////////////
// disable MICRO
///////////////////////////////////////////////////////////////////////////////////////

#undef  MBEDTLS_HAVE_TIME
#undef  MBEDTLS_HAVE_TIME_DATE

#undef  MBEDTLS_SSL_PROTO_DTLS
#undef  MBEDTLS_SSL_DTLS_ANTI_REPLAY
#undef  MBEDTLS_SSL_DTLS_HELLO_VERIFY
#undef  MBEDTLS_SSL_DTLS_CLIENT_PORT_REUSE
#undef  MBEDTLS_SSL_DTLS_BADMAC_LIMIT

///////////////////////////////////////////////////////////////////////////////////////
// enable MICRO
///////////////////////////////////////////////////////////////////////////////////////

#define MBEDTLS_SSL_PROTO_TLS1
#define MBEDTLS_SSL_PROTO_TLS1_1
#define MBEDTLS_SSL_PROTO_TLS1_2

#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_NV_SEED
#define MBEDTLS_ENTROPY_C
#define ALTCP_MBEDTLS_RNG_FN					mbedtls_entropy_func 
#define MBEDTLS_PLATFORM_NV_SEED_READ_MACRO		mbedtls_platform_std_nv_seed_read
#define MBEDTLS_PLATFORM_NV_SEED_WRITE_MACRO	mbedtls_platform_std_nv_seed_write

#define MBEDTLS_HAVE_ASM

#define MBEDTLS_NO_64BIT_MULTIPLICATION

#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_C

#define MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH
#define MBEDTLS_SSL_MAX_CONTENT_LEN             16384
#define MBEDTLS_SSL_IN_CONTENT_LEN              16384

#ifdef _WIN32
#define MBEDTLS_SSL_OUT_CONTENT_LEN             16384
#undef  MBEDTLS_PLATFORM_NO_STD_FUNCTIONS
#else
#define MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH
#define MBEDTLS_SSL_OUT_CONTENT_LEN             1024    //16384

#define MBEDTLS_PLATFORM_NO_STD_FUNCTIONS
#define MBEDTLS_PLATFORM_SNPRINTF_MACRO			sprintf_s
#define MBEDTLS_PLATFORM_VSNPRINTF_MACRO		vsnprintf
#endif

//0 No debug,1 Error,2 State change,3 Informational,4 Verbose
#define MBEDTLS_DEBUG_LEVEL 					1
