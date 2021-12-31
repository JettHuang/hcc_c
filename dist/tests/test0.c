/* \brief
 *   main.c
 */
 
#define cat(x, y)       x ## y
#define xcat(x, y) cat(x,y)

xcat(xcat(1,2), 3) /* 123 */


cat(var, 123)   /* var123 */
cat(cat(1,2),3) /* cat(1,2)3 */



#include "test0.h"

#define hash_hash # ## #
#define mkstr(a) # a
#define in_between(a) mkstr(a)
#define join(c, d) in_between(c hash_hash d)


#define mkstr(a) # a
char p[] = join(x, y); /* equivalent to char p[] = "x ## y"; */

#define tempfile(dir)    #dir "%s"
tempfile(/usr/tmp)




int main(int argc, char* argv)
{
	
	printf("%s:%d", __FILE__, __LINE__);
	printf("%s,%s", __DATE__, __TIME__);
	return 0;
}

