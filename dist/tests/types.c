/*
   TESTING:
   1. parsing types
*/


int a, b, c;
const int kId;

int returnfunc(char value)(int error);
/* char (*(*x(int index)))(char name); */
 
int display(const char* str);

int (*ptrfunc)(const char* str);

int returnfunc(char value)(int error);

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


struct Point {
	int a:5;
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

char **argv;
    /* argv:  pointer to char */
int (*daytab)[13];
    /* daytab:  pointer to array[13] of int */
int *daytab_two[13];
    /* daytab:  array[13] of pointer to int */
void *comp();
    /* comp: function returning pointer to void */
void (*comp_two)();
    /* comp: pointer to function returning void */
char (*(*x())[])();
    /* x: function returning pointer to array[] of
    pointer to function returning char */
char (*(*x_two[3])())[5];
    /* x: array[3] of pointer to function returning
    pointer to array[5] of char */
	

	