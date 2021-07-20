/* \brief
 *   main.c
 */
 
 "hello world!!"
 

#include "test0.h"

/*
#define hash_hash # ## #
#define mkstr(a) # a
#define in_between(a) mkstr(a)
#define join(c, d) in_between(c hash_hash d)
*/

char p[] = join(x, y); /* equivalent to char p[] = "x ## y"; */

/*
MIN(a, 
	MIN(b, c))

#define X(b) -b
#define Y - -
X(-)
X(-)Y

#define FUCK  ( __FILE__
char* str = FUCK

JOIN(,)

#define tempfile(dir)    #dir "%s"
tempfile(/usr/tmp)

#define cat(x, y)       x ## y

cat(var, 123)
cat(cat(1,2),3)

-------------------------

#define xcat(x, y)      cat(x,y)
xcat(xcat(1, 2), 3)

---------------------------

mkstr(a b c    d e ++ --)

*/
int main(int argc, char* argv)
{
	
	printf("%s:%d", __FILE__, __LINE__);
	printf("%s,%s", __DATE__, __TIME__);
	return 0;
}
