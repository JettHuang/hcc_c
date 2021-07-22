/* \brief
 *   main.c
 */
#if 1

#define X   100

#if X > 0 ? X > 50 ? 0 : 1 : 100

testing conditon expression xxxx

#endif

#if X > 0 ? (X-'a') : 100

testing conditon expression yyyy

#endif
 
#if !defined(_VA_LIST) && !defined(__VA_LIST_DEFINED)

#endif

#include "test1.h"

#if 0
	int a0 = 0;
#else
	int b0 = 0;
#endif

#if 1
	int a1 = 0;
#else
	int b1 = 0;
#endif

#define VERSION	  100
#if VERSION >= 100
	int version = VERSION;
#endif


#ifndef __ASSERT
#define __ASSERT

void assert(int);

#endif /* __ASSERT */

#undef assert
#ifdef NDEBUG
#define assert(ignore) ((void)0)
#else
extern int _assert(char *, char *, unsigned);
#define assert(e) ((void)((e)||_assert(#e, __FILE__, __LINE__)))
#endif /* NDEBUG */


#ifndef __STDARG
#define __STDARG

#if !defined(_VA_LIST) && !defined(__VA_LIST_DEFINED)
#define _VA_LIST
#define _VA_LIST_DEFINED
typedef char *__va_list;
#endif
static float __va_arg_tmp;
typedef __va_list va_list;

#define va_start(list, start) ((void)((list) = (sizeof(start)<4 ? \
	(char *)((int *)&(start)+1) : (char *)(&(start)+1))))
#define __va_arg(list, mode, n) (\
	__typecode(mode)==1 && sizeof(mode)==4 ? \
	  (__va_arg_tmp = *(double *)(&(list += ((sizeof(double)+n)&~n))[-(int)((sizeof(double)+n)&~n)]), \
		*(mode *)&__va_arg_tmp) : \
	  *(mode *)(&(list += ((sizeof(mode)+n)&~n))[-(int)((sizeof(mode)+n)&~n)]))
#define _bigendian_va_arg(list, mode, n) (\
	sizeof(mode)==1 ? *(mode *)(&(list += 4)[-1]) : \
	sizeof(mode)==2 ? *(mode *)(&(list += 4)[-2]) : __va_arg(list, mode, n))
#define _littleendian_va_arg(list, mode, n) __va_arg(list, mode, n)
#define va_end(list) ((void)0)
#define va_arg(list, mode) _littleendian_va_arg(list, mode, 3U)
typedef void *__gnuc_va_list;
#endif

#endif /* endif 0 */

int main(int argc, char* argv)
{
	
	printf("%s:%d", __FILE__, __LINE__);
	printf("%s,%s", __DATE__, __TIME__);
	return 0;
}
