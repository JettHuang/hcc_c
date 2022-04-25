
 /* the following definitons are used to compile the visual-stdio includes file successfully 
  *
  * BEGIN
  */
 #define __declspec(...)	/* __declspec is un-support */
 #define __inline	inline	/* __inline is un-support */
 #define __forceinline inline
 #define __cdecl			 /* default is cdecl */
 #define __fastcall          /* __fastcall is un-support */
 #define __pragma(...)		 /* __pragma is un-support */
 
 #define __int64 	long long 
 #define _NO_CRT_STDIO_INLINE
 #define _CRT_FUNCTIONS_REQUIRED	1  /* errno.h */
 #define __STDC_WANT_SECURE_LIB__	0
 /* END */
 

#include <stdio.h>
#include <stdlib.h>


#define EXPECT(expected, expr)                                  \
  do {                                                          \
    int e1 = (expected);                                        \
    int e2 = (expr);                                            \
    if (e1 == e2) {                                             \
      printf("%s => %d\n", #expr, e2);                 \
    } else {                                                    \
      printf("line %d: %s: %d expected, but got %d\n", \
              __LINE__, #expr, e1, e2);                         \
      exit(1);                                                  \
    }                                                           \
  } while (0)

int one() { return 1; }
int two() { return 2; }
int plus(int x, int y) { return x + y; }
int mul(int x, int y) { return x * y; }
int add(int a, int b, int c, int d, int e, int f) { return a+b+c+d+e+f; }
int add2(int (*a)[2]) { return a[0][0] + a[1][0]; }
int add3(int a[][2]) { return a[0][0] + a[1][0]; }
int add4(int a[2][2]) { return a[0][0] + a[1][0]; }
void nop() {}

int var1;
int var2[5];
typedef int myint;

// Single-line comment test

/***************************
 * Multi-line comment test *
 ***************************/

int main() {
  EXPECT(0, 0);
  EXPECT(1, 1);
  EXPECT(493, 0755);
  EXPECT(48879, 0xBEEF);
  EXPECT(255, 0Xff);
  EXPECT(2, 1+1);
  EXPECT(10, 2*3+4);
  EXPECT(26, 2*3+4*5);
  EXPECT(5, 50/10);
  EXPECT(9, 6*3/2);
  EXPECT(45, (2+3)*(4+5));
  EXPECT(153, 1+2+3+4+5+6+7+8+9+10+11+12+13+14+15+16+17);

  EXPECT(5, plus(2, 3));
  EXPECT(1, one());
  EXPECT(3, one()+two());
  EXPECT(6, mul(2, 3));
  EXPECT(21, add(1,2,3,4,5,6));

  EXPECT(0, 0 || 0);
  EXPECT(1, 1 || 0);
  EXPECT(1, 0 || 1);
  EXPECT(1, 1 || 1);

  EXPECT(0, 0 && 0);
  EXPECT(0, 1 && 0);
  EXPECT(0, 0 && 1);
  EXPECT(1, 1 && 1);

  EXPECT(0, 0 < 0);
  EXPECT(0, 1 < 0);
  EXPECT(1, 0 < 1);
  EXPECT(0, 0 > 0);
  EXPECT(0, 0 > 1);
  EXPECT(1, 1 > 0);

  EXPECT(0, 4 == 5);
  EXPECT(1, 5 == 5);
  EXPECT(1, 4 != 5);
  EXPECT(0, 5 != 5);

  EXPECT(1, 4 <= 5);
  EXPECT(1, 5 <= 5);
  EXPECT(0, 6 <= 5);

  EXPECT(0, 4 >= 5);
  EXPECT(1, 5 >= 5);
  EXPECT(1, 6 >= 5);

  EXPECT(8, 1 << 3);
  EXPECT(4, 16 >> 2);

  EXPECT(4, 19 % 5);
  EXPECT(0, 9 % 3);

  EXPECT(0-3, -3);

  EXPECT(0, !1);
  EXPECT(1, !0);

  EXPECT(-1, ~0);
  EXPECT(-4, ~3);


  EXPECT(5, 0 ? 3 : 5);
  EXPECT(3, 1 ? 3 : 5);

  EXPECT(3, (1, 2, 3));

  EXPECT(11, 9 | 2);
  EXPECT(11, 9 | 3);
  EXPECT(5, 6 ^ 3);
  EXPECT(2, 6 & 3);
  EXPECT(0, 6 & 0);

  EXPECT(4, sizeof("abc"));

  EXPECT(0, '\0');
  EXPECT(0, '\00');
  EXPECT(0, '\000');
  EXPECT(1, '\1');
  EXPECT(7, '\7');
  EXPECT(64, '\100');

  EXPECT(64, "\10000"[0]);
  EXPECT('0', "\10000"[1]);
  EXPECT('0', "\10000"[2]);
  EXPECT(0, "\10000"[3]);
  EXPECT(255, "\xffxyz"[0]);
  EXPECT('x', "\xffxyz"[1]);

  EXPECT(0, var1);
  EXPECT(20, sizeof(var2));

  EXPECT(128, ((((((1+1)+(1+1))+(1+1)+(1+1))+(((1+1)+(1+1))+(1+1)+(1+1)))+((((1+1)+(1+1))+(1+1)+(1+1))+(((1+1)+(1+1))+(1+1)+(1+1))))+(((((1+1)+(1+1))+(1+1)+(1+1))+(((1+1)+(1+1))+(1+1)+(1+1)))+((((1+1)+(1+1))+(1+1)+(1+1))+(((1+1)+(1+1))+(1+1)+(1+1)))))+((((((1+1)+(1+1))+(1+1)+(1+1))+(((1+1)+(1+1))+(1+1)+(1+1)))+((((1+1)+(1+1))+(1+1)+(1+1))+(((1+1)+(1+1))+(1+1)+(1+1))))+(((((1+1)+(1+1))+(1+1)+(1+1))+(((1+1)+(1+1))+(1+1)+(1+1)))+((((1+1)+(1+1))+(1+1)+(1+1))+(((1+1)+(1+1))+(1+1)+(1+1))))));

  printf("======= PASSED OK ==========\n");
  return 0;
}
