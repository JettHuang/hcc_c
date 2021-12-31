
/*
int b[4];
int *ptr03 = b;


char str[] = "abcdefg";
char str1[] = {"123456"};
char str2[] = { 'a', 'b', '123' };
char str3[] = { "abc" };
char str4[3] = "jet";
char str5[3] = {"jet"};
char str6[3] = {'a', 'b', 'c' };

*/

struct S {
	int a;
	int b;
};

struct S gs;

int* pval = &gs.b;

char name[] = "123";
char *pstr = "123456";

void display(int display)
{
}

/*
struct Struct {
	char ch;
	int a;
	int b;
	char str[4];
};

struct Struct s = { 1, 2 };

struct Struct gPairs[] = {
	{1, 2, 'a', 'b'},
	3, 4,
	{ 5 },
	6, 7
};

struct BitFileds {
	int a : 4;
	int b : 4;
	unsigned int c: 4;
	int d : 28;
	
	int x;
	int y;
};

struct BitFileds bitfield = { 2, 4, 15,  32,  10, 20 };

int a = 100;
int *ptr01 = 0;
int *ptr02 = &a + 4;

*/

