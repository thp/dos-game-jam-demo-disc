#ifndef SIZEINT_H_
#define SIZEINT_H_

/* for C99 or selected toolchain versions we can use stdint.h */
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199900) || \
	(defined(_MSC_VER) && _MSC_VER >= 1600) || \
	(defined(__WATCOMC__) && __WATCOMC__ >= 1200)
#include <stdint.h>

#elif defined(__DOS__) && defined(__WATCOMC__) && __WATCOMC__ < 1200
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef long int32_t;
typedef unsigned long uint32_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;

#elif defined(_MSC_VER) && (_MSC_VER < 1600)
typedef signed __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;

#else

#ifdef __sgi
#include <inttypes.h>
#else
#include <sys/types.h>
#endif
#endif


#endif	/* SIZEINT_H_ */
