#include"stdio.h"
int add(int a,int b)
{
  a=a+3;
  b=b-5;
  printf("a=%d,b=%d\n",a,b);
  return a+b;
}
void main()
{
  int a,b=2;
  a=5;
  printf("a=%d,b=%d\n",a,b);
  printf("a+b=%d\n",add(a,b));
  printf("a=%d,b=%d",a,b);
} 
