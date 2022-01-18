/* testing statements
 *
 */

/* ie: "ABC" will be ignore for str[4]. */

struct FBlock {
	int id;
	int data[32];
};

struct Struct {
	int a;
	int b;
	int c:4;
	int d:5;
	char str[5];
	
	struct FBlock blk;
};

char str[4] = { 'a', 'b', "ABC" };
struct Struct s = { 1, 2, 4, 3, 'a', 'b' };
					

int test_init()
{
	char a[10] = { 0 };
	struct Struct tmp = { 1, 2, 4, 3, 'a', 'b', 'c', 'd', 0, { 1000, 0 }	};
	
	int flag = 5;
	tmp.d = flag;
	
	return 0;
}


extern int account;

void decl()
{
   int Count;
   int Array[10];

   Array[Count-1] = Count;
}

struct S {
	int id;
	int age;
};

struct S getStudentInfo()
{
	struct S s;
	return s;
}

void booltest()
{
	struct S s0, s1, s2;
	int a, b;
	
	s0.id = 100;
	s0.age = 19;
	
	s1 = a > b ? s0 : getStudentInfo(); 
	
	s2 = getStudentInfo();
	
	getStudentInfo();
}


struct fred
{
   int boris;
   int natasha;
};

void printf(const char* format, ...)
{
	
}

void assignment()
{
   int a;
   int b = 64;
    int c = 12, d = 34;
   
   a = 42;
   printf("%d\n", a);
   printf("%d\n", b);
   printf("%d, %d\n", c, d);

   return;
}


void structure()
{
   struct fred bloggs;
   struct fred jones[2];

   bloggs.boris = 12;
   bloggs.natasha = 34;

   printf("%d\n", bloggs.boris);
   printf("%d\n", bloggs.natasha);
   
   jones[0].boris = 12;
   jones[0].natasha = 34;
   jones[1].boris = 56;
   jones[1].natasha = 78;
}


int display_array() 
{
   int Count;
   int Array[10];

   for (Count = 1; Count <= 10; Count++)
   {
      Array[Count-1] = Count * Count;
   }

   for (Count = 0; Count < 10; Count++)
   {
      printf("%d\n", Array[Count]);
   }

   return 0;
}

int statement_for() 
{
   int Count;

   for (Count = 1; Count <= 10; Count++)
   {
      printf("%d\n", Count);
   }

   return 0;
}

int statement_switch()
{
   int Count;

   for (Count = 0; Count < 4; Count++)
   {
      printf("%d\n", Count);
      switch (Count)
      {
         case 1:
            printf("%d\n", 1);
            break;

         case 2:
            printf("%d\n", 2);
            break;

         default:
            printf("%d\n", 0);
            break;
      }
   }

   return 0;
}

void vfunc(int a)
{
   printf("a=%d\n", a);
}

void statement_call()
{
	vfunc(1234);
}

int statement_while()
{
   int a;
   int p;
   int t;

   a = 1;
   p = 0;
   t = 0;

   while (a < 100)
   {
      printf("%d\n", a);
      t = a;
      a = t + p;
      p = t;
   }

   return 0;
}

int statement_dowhile()
{
   int a;
   int p;
   int t;

   a = 1;
   p = 0;
   t = 0;

   do
   {
      printf("%d\n", a);
      t = a;
      a = t + p;
      p = t;
   } while (a < 100);

   return 0;
}

struct ziggy
{
   int a;
   int b;
   int c;
} bolshevic;

int test_pointer()
{
   int a;
   int *b;
   int c;

   a = 42;
   b = &a;
   printf("a = %d\n", *b);

   bolshevic.a = 12;
   bolshevic.b = 34;
   bolshevic.c = 56;

   printf("bolshevic.a = %d\n", bolshevic.a);
   printf("bolshevic.b = %d\n", bolshevic.b);
   printf("bolshevic.c = %d\n", bolshevic.c);

   struct ziggy *tsar = &bolshevic;

   printf("tsar->a = %d\n", tsar->a);
   printf("tsar->b = %d\n", tsar->b);
   printf("tsar->c = %d\n", tsar->c);

   b = &(bolshevic.b);
   printf("bolshevic.b = %d\n", *b);

   return 0;
}

int test_precedence()
{
   int a;
   int b;
   int c;
   int d;
   int e;
   int f;
   int x;
   int y;

   a = 12;
   b = 34;
   c = 56;
   d = 78;
   e = 0;
   f = 1;

   printf("%d\n", c + d);
   printf("%d\n", (y = c + d));
   printf("%d\n", e || e && f);
   printf("%d\n", e || f && f);
   printf("%d\n", e && e || f);
   printf("%d\n", e && f || f);
   printf("%d\n", a && f | f);
   printf("%d\n", a | b ^ c & d);
   printf("%d, %d\n", a == a, a == b);
   printf("%d, %d\n", a != a, a != b);
   printf("%d\n", a != b && c != d);
   printf("%d\n", a + b * c / f);
   printf("%d\n", a + b * c / f);
   printf("%d\n", (4 << 4));
   printf("%d\n", (64 >> 4));

   return 0;
}

int statement_ifelse()
{
   int a = 1;

   if (a)
      printf("a is true\n");
   else
      printf("a is false\n");

   int b = 0;
   if (b)
      printf("b is true\n");
   else
      printf("b is false\n");

   return 0;
}

int statement_nestloop()
{
   int x, y, z;

   for (x = 0; x < 2; x++)
   {
      for (y = 0; y < 3; y++)
      {
         for (z = 0; z < 3; z++)
         {
            printf("%d %d %d\n", x, y, z);
         }
      }
   }

   return 0;
}

void fred(int x)
{
   switch (x)
   {
      case 1: printf("1\n"); return;
      case 2: printf("2\n"); break;
      case 3: printf("3\n"); return;
   }

   printf("out\n");
}

int nested_break()
{
   int a;
   char b;

   a = 0;
   while (a < 2)
   {
      printf("%d", a++);
      break;

      b = 'A';
      while (b < 'C')
      {
         printf("%c", b++);
      }
      printf("e");
   }
   printf("\n");

   return 0;
}

void henry()
{
   static int fred = 4567;

   printf("%d\n", fred);
   fred++;
}

void statement_goto()
{
   int b = 5678;

   printf("In joe()\n");

   {
      int c = 1234;
      printf("c = %d\n", c);
      goto outer;
      printf("uh-oh\n");
   }

outer:    

   printf("done\n");
}



