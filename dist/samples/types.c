/*
   TESTING:
   1. parsing types
*/

/*
int a, b, c;
const int kId;

int display(const char* str);

int (*ptrfunc)(const char* str);
*/

const int* array[100];
int** grid[10][5];

enum COLOR
{
	BLACK = 1,
	WHITE,
	YELLOW,
	GRAY,
	GREEN,
	BLUE
};

/*
struct Point {
	int a:5;
	char b:3;
	char c:5;
	int d: 28;
	
	float x;
	float y;
};

union Info
{
	int a;
	int b;
	float c;
	double d;
};

struct Rect {
	struct Point min;
	struct Point max;
};

*/
