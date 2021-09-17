

struct Bits {
	int a:5;
	char b:3;
	char c:5;
	int d: 28;
};

struct Point {
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


struct Bits bits = { 1, 2, 3, 4};
struct Point pt = { 100.f, 200.f };
union Info info = { 10, 20, 30.f, 40.f};
struct Rect rect = { { 10, 10}, {20, 20}};

