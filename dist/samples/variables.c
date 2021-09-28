
/*
int a = 100;
int b[4] = {1, 2, 3, 4};
char str[] = "abcdefg";
*/

/*
char str1[] = {"123456"};
char str2[] = { 'a', 'b', '123' };
char str3[] = { "abc" };
char str4[3] = "jet";
char str5[3] = {"jet"};
char str6[3] = {'a', 'b', 'c' };

char *pstr = "123456";
*/

struct Struct {
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

