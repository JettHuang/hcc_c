

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
/*�ַ���������*/
#include <ctype.h> 

#define BUFFSIZE 32
#define COL 128
#define ROW 64

/* ����ѧȥ����վ�ռ� http://www.zixue7.com */
/*����ջ1*/
typedef struct node
{
    int data;
    struct node  *next;
}STACK1; 
/*����ջ2*/
typedef struct node2
{
    char data;
    struct node2 *next;
}STACK2;
/*���涨������ջ��������*/
/*��ջ����*/
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
/*��ջ����*/
STACK1 *PopStack(STACK1 *top) 
{
    STACK1 *q; 
    q=top;  
    top=top->next; 
    free(q);  
    return top; 
}
/*��ջ��Ԫ��*/
int GetTop(STACK1 *top) 
{
    if(top==NULL)
    {
        printf("Stack is null\n"); 
        return 0;
    }
    /*����ջ��Ԫ��*/
    return top->data; 
}
/*ȡջ��Ԫ�أ���ɾ��ջ��Ԫ��*/
STACK1 *GetDelTop(STACK1 *top,int *x) 
{
    *x=GetTop(top);     
    top=PopStack(top); 
    return top; 
}
int EmptyStack(STACK1 *top) /*��ջ�Ƿ�Ϊ��*/
{
    if(top==NULL) 
        return 1; 
    return 0; 
}
/*��ջ����*/
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
STACK2 *PopStack2(STACK2 *top) /*��ջ*/
{
    STACK2 *q; 
    q=top; 
    top=top->next; 
    free(q); 
    return top; 
}
/*��ջ��Ԫ��*/
char GetTop2(STACK2 *top) 
{
    if(top==NULL) 
    {
        printf("Stack is null\n"); 
        return 0; 
    }
    return top->data; 
}
/*ȡջ��Ԫ�أ���ɾ��ջ��Ԫ��*/
STACK2 *GetDelTop2(STACK2 *top,char *x) 
{
    *x=GetTop2(top); 
    top=PopStack2(top);
    return top; 
}
/*��ջ�Ƿ�Ϊ��*/
int EmptyStack2(STACK2 *top) 
{
    if(top==NULL)
        return 1; 
    else
        return 0; 
}
/*������ƺ���*/
void GenCard()
{
    int num,i;
	
    srand(time(NULL));
    for(i=0;i<4;i++)
    {
        num=rand() % 13; /*��С�����*/
        printf("%d ",num);
    } 
}
/*��׺�ַ���eת��׺�ַ���a����*/
void ExpressTransform(char *expMiddle,char *expBack) 
{
    STACK2 *top=NULL; /* ����ջ��ָ��*/
    int i=0,j=0;
    char ch;
    while(expMiddle[i]!='\0') 
    {
        /*�ж��ַ�������*/
        if(isdigit(expMiddle[i])) 
        {
            do{
                expBack[j]=expMiddle[i];
                i++;j++; 
            }while(expMiddle[i]!='.');
            expBack[j]='.';
            j++;
        }
        /*����(��*/
        if(expMiddle[i]=='(')  
            top=PushStack2(top,expMiddle[i]);
        /*����)��*/
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
        /*����ӻ����*/
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
        /*����˻����*/
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
/*��׺���ʽ��ֵ����*/
int ExpressComputer(char *s)
{
    STACK1 *top=NULL;
    int i,k,num1,num2,result;
    i=0;
    while(s[i]!='\0')  /*���ַ���û�н���ʱ�����´���*/
    {
        if(isdigit(s[i])) /*���ַ��Ƿ�Ϊ����*/
        {
            k=0;  /*k��ֵΪ0*/
            do{
                k=10*k+s[i]-'0';  /*���ַ�����Ϊʮ��������*/
                i++;   /*i��1*/
            }while(s[i]!='.'); /*���ַ���Ϊ��.��ʱ�ظ�ѭ��*/
            top=PushStack(top,k); /*�����ɵ�����ѹ���ջ*/
        }
        if(s[i]=='+')  /*���Ϊ'+'��*/
        {
            top=GetDelTop(top,&num2); /*��ջ��Ԫ��ȡ������num2��*/
            top=GetDelTop(top,&num1);  /*��ջ��Ԫ��ȡ������num1��*/
            result=num2+num1;  /*��num1��num2��Ӵ���result��*/
            top=PushStack(top,result);  /*��resultѹ���ջ*/
        }
        if(s[i]=='-')  /*���Ϊ'-'��*/
        {
            top=GetDelTop(top,&num2); /*��ջ��Ԫ��ȡ������num2��*/
            top=GetDelTop(top,&num1); /*��ջ��Ԫ��ȡ������num1��*/
            result=num1-num2; /*��num1��ȥnum2�������result��*/
            top=PushStack(top,result); /*��resultѹ���ջ*/
        }
        if(s[i]=='*')  /*���Ϊ'*'��*/
        {
            top=GetDelTop(top,&num2); /*��ջ��Ԫ��ȡ������num2��*/
            top=GetDelTop(top,&num1); /*��ջ��Ԫ��ȡ������num1��*/
            result=num1*num2; /*��num1��num2��˽������result��*/
            top=PushStack(top,result); /*��resultѹ���ջ*/
        }
        if(s[i]=='/') /*���Ϊ'/'��*/
        {
            top=GetDelTop(top,&num2); /*��ջ��Ԫ��ȡ������num2��*/
            top=GetDelTop(top,&num1); /*��ջ��Ԫ��ȡ������num1��*/
            result=num1/num2;               /*��num1��num2�������result��*/
            top=PushStack(top,result); /*��resultѹ���ջ*/
        }
        i++;  /*i��1*/
    }
    top=GetDelTop(top,&result); /*���ջ��Ԫ�ص�ֵΪ����Ľ��*/
    return result;  /*���ؽ��*/
}
/*�������ı��ʽ�Ƿ���ȷ*/
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
/*������*/
int main()
{
    char expMiddle[BUFFSIZE],expBack[BUFFSIZE],ch;
    int i,result;
    
	system("cls");
    /*��ʾ�����ַ�����ʽ*/
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
            /*�����ַ���ѹ�س���*/
            scanf("%s%c",expMiddle,&ch); 
            /*�������ı��ʽ�Ƿ���ȷ*/
        }while(!CheckExpression(expMiddle));
        
        printf("%s\n",expMiddle);
        /*����ExpressTransform��������׺���ʽexpMiddleת��Ϊ��׺���ʽexpBack*/
        ExpressTransform(expMiddle,expBack); 
        /*�����׺���ʽ��ֵ*/
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
