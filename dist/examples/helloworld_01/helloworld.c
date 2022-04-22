/* \brief 
 *		Hello world!
 */
 
 #include <stdio.h>
 
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
 
 int main(int argc, char *argv[])
 {

	struct tagCCInfo info = { 0, 5, 20 };
	int i;

	int b = square(10);
	
	printf("b is %d\n", b);
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
 