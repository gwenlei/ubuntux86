#include <stdio.h>
#include <linux/gpio.h> 
int main(){
printf("begin gpio test!!\n");
unsigned gpio;
const char *label;
int a=gpio_request(gpio,label);
}
