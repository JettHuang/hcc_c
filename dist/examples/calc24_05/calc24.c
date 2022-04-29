

 /* the following definitons are used to compile the visual-stdio includes file successfully 
  *
  * BEGIN
  */
 #define __declspec(...)	/* __declspec is un-support */
 #define __inline	inline	/* __inline is un-support */
 #define __forceinline inline
 #define __pragma(...)		 /* __pragma is un-support */
 
 #define __int64 	long long 
 #define _NO_CRT_STDIO_INLINE
 #define _CRT_FUNCTIONS_REQUIRED	1  /* errno.h */
 #define __STDC_WANT_SECURE_LIB__	0
 /* END */


#include <stdio.h>
#include <malloc.h>
#include <stdlib.h> 
#include <string.h> 
#include <conio.h>
#include <time.h>
/*字符操作函数*/
#include <ctype.h> 

#define BUFFSIZE 32
#define COL 128
#define ROW 64

/* 【自学去】网站收集 http://www.zixue7.com */
/*定义栈1*/
typedef struct node
{
    int data;
    struct node  *next;
}STACK1; 
/*定义栈2*/
typedef struct node2
{
    char data;
    struct node2 *next;
}STACK2;
/*下面定义两个栈基本操作*/
/*入栈函数*/
STACK1 *PushStack(STACK1 *top,int x)
{
    STACK1 *p;  
    p=(STACK1 *)malloc(sizeof(STACK1));
    if(p==NULL)  
    {
        printf("ERROR\n!");
        exit(0);  
    }
    p->data=x;  
    p->next=top;    
    top=p;      		
    return top;     
}
/*出栈函数*/
STACK1 *PopStack(STACK1 *top) 
{
    STACK1 *q; 
    q=top;  
    top=top->next; 
    free(q);  
    return top; 
}
/*读栈顶元素*/
int GetTop(STACK1 *top) 
{
    if(top==NULL)
    {
        printf("Stack is null\n"); 
        return 0;
    }
    /*返回栈顶元素*/
    return top->data; 
}
/*取栈顶元素，并删除栈顶元素*/
STACK1 *GetDelTop(STACK1 *top,int *x) 
{
    *x=GetTop(top);     
    top=PopStack(top); 
    return top; 
}
int EmptyStack(STACK1 *top) /*判栈是否为空*/
{
    if(top==NULL) 
        return 1; 
    return 0; 
}
/*入栈函数*/
STACK2 *PushStack2(STACK2 *top,char x) 
{
    STACK2 *p;
    p=(STACK2 *)malloc(sizeof(STACK2)); 
    if(p==NULL) 
    {
        printf("error\n!"); 
        exit(0); 
    }
    p->data=x; 
    p->next=top; 
    top=p; 
    return top; 
}
STACK2 *PopStack2(STACK2 *top) /*出栈*/
{
    STACK2 *q; 
    q=top; 
    top=top->next; 
    free(q); 
    return top; 
}
/*读栈顶元素*/
char GetTop2(STACK2 *top) 
{
    if(top==NULL) 
    {
        printf("Stack is null\n"); 
        return 0; 
    }
    return top->data; 
}
/*取栈顶元素，并删除栈顶元素*/
STACK2 *GetDelTop2(STACK2 *top,char *x) 
{
    *x=GetTop2(top); 
    top=PopStack2(top);
    return top; 
}
/*判栈是否为空*/
int EmptyStack2(STACK2 *top) 
{
    if(top==NULL)
        return 1; 
    else
        return 0; 
}
/*随机发牌函数*/
void GenCard()
{
    int num,i;
	
    srand(time(NULL));
    for(i=0;i<4;i++)
    {
        num=rand() % 13; /*大小随机数*/
        printf("%d ",num);
    } 
}
/*中缀字符串e转后缀字符串a函数*/
void ExpressTransform(char *expMiddle,char *expBack) 
{
    STACK2 *top=NULL; /* 定义栈顶指针*/
    int i=0,j=0;
    char ch;
    while(expMiddle[i]!='\0') 
    {
        /*判断字符是数字*/
        if(isdigit(expMiddle[i])) 
        {
            do{
                expBack[j]=expMiddle[i];
                i++;j++; 
            }while(expMiddle[i]!='.');
            expBack[j]='.';
            j++;
        }
        /*处理“(”*/
        if(expMiddle[i]=='(')  
            top=PushStack2(top,expMiddle[i]);
        /*处理“)”*/
        if(expMiddle[i]==')')  
        {
            top=GetDelTop2(top,&ch); 
            while(ch!='(')  
        	{
                expBack[j]=ch;  
                j++; 
                top=GetDelTop2(top,&ch);
        	}
        }
        /*处理加或减号*/
        if(expMiddle[i]=='+'||expMiddle[i]=='-') 
        {
            if(!EmptyStack2(top))
        	{
                ch=GetTop2(top);
                while(ch!='(') 
            	{
                    expBack[j]=ch;
                    j++; 
                    top=PopStack2(top);
                    if(EmptyStack2(top))
                        break; 
                    else
                        ch=GetTop2(top); 
            	}
        	}
            top=PushStack2(top,expMiddle[i]);
        }
        /*处理乘或除号*/
        if(expMiddle[i]=='*'||expMiddle[i]=='/') 
        {
            if(!EmptyStack2(top)) 
        	{
                ch=GetTop2(top);
                while(ch=='*'||ch=='/')
            	{
                    expBack[j]=ch;
                    j++; 
                    top=PopStack2(top);
                    if(EmptyStack2(top)) 
                        break; 
                    else
                        ch=GetTop2(top); 
            	}
        	}
            top=PushStack2(top,expMiddle[i]); 
        }
        i++; 
    }
    while(!EmptyStack2(top)) 
        top=GetDelTop2(top,&expBack[j++]);
    expBack[j]='\0';  
}
/*后缀表达式求值函数*/
int ExpressComputer(char *s)
{
    STACK1 *top=NULL;
    int i,k,num1,num2,result;
    i=0;
    while(s[i]!='\0')  /*当字符串没有结束时作以下处理*/
    {
        if(isdigit(s[i])) /*判字符是否为数字*/
        {
            k=0;  /*k初值为0*/
            do{
                k=10*k+s[i]-'0';  /*将字符连接为十进制数字*/
                i++;   /*i加1*/
            }while(s[i]!='.'); /*当字符不为‘.’时重复循环*/
            top=PushStack(top,k); /*将生成的数字压入堆栈*/
        }
        if(s[i]=='+')  /*如果为'+'号*/
        {
            top=GetDelTop(top,&num2); /*将栈顶元素取出存入num2中*/
            top=GetDelTop(top,&num1);  /*将栈顶元素取出存入num1中*/
            result=num2+num1;  /*将num1和num2相加存入result中*/
            top=PushStack(top,result);  /*将result压入堆栈*/
        }
        if(s[i]=='-')  /*如果为'-'号*/
        {
            top=GetDelTop(top,&num2); /*将栈顶元素取出存入num2中*/
            top=GetDelTop(top,&num1); /*将栈顶元素取出存入num1中*/
            result=num1-num2; /*将num1减去num2结果存入result中*/
            top=PushStack(top,result); /*将result压入堆栈*/
        }
        if(s[i]=='*')  /*如果为'*'号*/
        {
            top=GetDelTop(top,&num2); /*将栈顶元素取出存入num2中*/
            top=GetDelTop(top,&num1); /*将栈顶元素取出存入num1中*/
            result=num1*num2; /*将num1与num2相乘结果存入result中*/
            top=PushStack(top,result); /*将result压入堆栈*/
        }
        if(s[i]=='/') /*如果为'/'号*/
        {
            top=GetDelTop(top,&num2); /*将栈顶元素取出存入num2中*/
            top=GetDelTop(top,&num1); /*将栈顶元素取出存入num1中*/
            result=num1/num2;               /*将num1除num2结果存入result中*/
            top=PushStack(top,result); /*将result压入堆栈*/
        }
        i++;  /*i加1*/
    }
    top=GetDelTop(top,&result); /*最后栈顶元素的值为计算的结果*/
    return result;  /*返回结果*/
}
/*检查输入的表达式是否正确*/
int CheckExpression(char *e)
{
    char ch;
    int i=0;
    while(e[i]!='\0')
    {
        if(isdigit(e[i])) 
        {
            if(isdigit(e[i+1]))
        	{
                i++;
                continue;
        	}
            if(e[i+1]!='.')
        	{
                printf("\n The wrong express format!!\n");
                return 0;
        	}
            i++;
        }
        i++;
    }
    return 1;
}
/*主函数*/
int main()
{
    char expMiddle[BUFFSIZE],expBack[BUFFSIZE],ch;
    int i,result;
    
	system("cls");
    /*提示输入字符串格式*/
    printf("*******************************************\n");
    printf("|  Welcome to play our game : 24 points!  |\n");
    printf("|      The input format as follows:       |\n");
    printf("|              10.*(4.-3.)                |\n");
    printf("*******************************************\n");
    while(1)
    {
        printf("\n The four digits are: ");
        GenCard();
        printf("\n");
        do{
            printf(" Please input the express:\n");
            /*输入字符串压回车键*/
            scanf("%s%c",expMiddle,&ch); 
            /*检查输入的表达式是否正确*/
        }while(!CheckExpression(expMiddle));
        
        printf("%s\n",expMiddle);
        /*调用ExpressTransform函数将中缀表达式expMiddle转换为后缀表达式expBack*/
        ExpressTransform(expMiddle,expBack); 
        /*计算后缀表达式的值*/
        result=ExpressComputer(expBack); 
        printf("The value of %s is:%d.\n",expMiddle,result);
        if(result==24) 
            printf("You are right!");
        else printf("You are wrong!");
        printf(" Do you want to play again(y/n)?\n");
        scanf("%c",&ch); 
        if(ch=='n'||ch=='N') 
            break;   
    } 
    return 0;
}
