/* \brief
 *		Hello World!!
 */

#if 1

 /* the following definitons are used to compile the visual-stdio includes file successfully 
  *
  * BEGIN
  */
 #define __declspec(...)	/* __declspec is un-support */
 #define __inline	inline	/* __inline is un-support */
 #define __forceinline inline
 #define __pragma(...)		 /* __pragma is un-support */
 
 #define __int64 	long long 
 #define _NO_CRT_STDIO_INLINE
 #define _CRT_FUNCTIONS_REQUIRED	1  /* errno.h */
 #define __STDC_WANT_SECURE_LIB__	0
 /* END */
 
 
 #include <stdio.h>

 #include <ctype.h>
 #include <string.h>
 #include <math.h>
 #include <stdlib.h>
 #include <assert.h>
 #include <stdarg.h>
 #include <setjmp.h>
 #include <signal.h>
 #include <time.h>
 #include <limits.h>
 #include <float.h>

 
 extern int printf(const char* fmt, ...);
 
 #define DEBUG(fmt, ...)   printf(fmt, __VA_ARGS__)

 
 typedef struct tagCCInfo {
	int _isdirty:1;
	unsigned int _isvisited:3;
	
	int _age;
	float _score;
 }FCCInfo;
 
 int square(int a) 
 {
	 return a * a;
 }
 
 const char *szhelp = "Hello everyone, this is my c99 compiler.\n"
	"let's try to find the bugs, and fix them.\n"
	
	"Best Regard!    --- JettHuang\n";

 int main(int argc, char *argv[])
 {

	struct tagCCInfo info = { 0, 5, 20 };
	int i;

	int b = square(10);
	printf("b is %d\n", b);
	
	printf(szhelp);
	
	DEBUG("debug: %s, %d\n", szhelp, b);
	
	if (argv[0] != NULL)
	{
		printf("fuck you...\n");
	}

	for (i=0; i<argc; ++i)
	{
		printf("argv[%d] %s\n", i, argv[i]);
	}
	
	printf("is dirty? %d\n", info._isdirty);
	printf("is visited? %d\n", info._isvisited);
	
	for (i=0; i<10; i++)
	{
		printf("hello, world!\n");
	}

	
	return 0;
 }
 
#else
	
  int __stdcall output_string(const char* fmt, ...)
  {
	  return 0;
  }
  
  void __fastcall  kill_process();
  
  int __cdecl sum(int a, int b)
  {
	  return a + b;
  }
  
  int __stdcall mul(int a, int b)
  {
	  return a * b;
  }
  
  double modf(double _X, double* _Y);
  
  inline float modff( float _X, float* _Y)
    {
        double _F, _I;
        _F = modf(_X, &_I);
        *_Y = (float)_I;
        return (float)_F;
    }
 
  int main()
  {
	  float x, y;
	  
	  x = 10.f; y = 20.f;
	  modff(x, &y);
	 
	 kill_process();
	 
	 output_string("hello! %d", 10);
	 sum(100, 200);
	 mul(10, 20);
	 return 0;
  }
  
 #endif
 