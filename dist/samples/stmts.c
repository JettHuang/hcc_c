

void print(const char* str)
{
	;;;
	
	return;
}

int main(int argc, char* argv[])
{
	int a, b;
	
	a = b = 10;
	if (a > 10)
	{
		a = 100;
	} 
	else if (a > 100)
	{
		a = 200;
	}
	else
	{
		a = 300;
	}
	
	if (a && b)
	{
		a = b = 0;
	}
	
	switch (a)
	{
	case 0: 
		print("0");
	case 1:
		print("1");
	default:
		break;
	}
	
	for(;;)
	{
		print("Hello World!\n");
		continue;
	}
	
	for (a = 0; a < 10; a++)
	{
		print("%d", a);
		break;
	}
	
_Label:

	while (a > b)
	{
		
	}
	
	do {
		int a , b;
		
	} while(1);
	
	goto  _Label;
	
	return 0;
}

