#ifndef TEST_ASSERT_H
#define TEST_ASSERT_H

#include <assert.h>
#include <stdio.h>

#ifndef NDEBUG
#define TEST_ASSERT(exp) assert(exp)
#else
#define TEST_ASSERT(exp) \
	do {\
		if (!(exp)) {\
			fprintf(stderr, "%s:%d: assertion failed: %s\n",\
				__FILE__, __LINE__, #exp\
			);\
		}\
	} while (0)
#endif // #ifndef NDEBUG

#endif // #ifndef TEST_ASSERT_H
