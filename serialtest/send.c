/*******************************************************
* File Name： send.c
* Description： send data to serial_Port
* Date：
*******************************************************/
/******************头文件定义******************/
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#define max_buffer_size 100 /*定义缓冲区最大宽度*/
/*******************************************/
int fd; /*定义设备文件描述符*/
int flag_close;
int open_serial(int k)
{
if(k==0) /*串口选择*/
{
fd = open("/dev/ttyS0",O_RDWR|O_NOCTTY); /*读写方式打开串口*/
perror("open /dev/ttyS0");
}
else
{
fd = open("/dev/ttyS1",O_RDWR|O_NOCTTY);
perror("open /dev/ttyS1");
}
if(fd == -1) /*打开失败*/
return -1;
else
return 0;
}
/********************************************************************/
int main(int argc, char *argv[ ] )
{
char sbuf[]={"Hello,this is a Serial_Port test!\n"};/*待发送的内容，以\n为结束标志*/
int sfd,retv,i;
struct termios option;
int length=sizeof(sbuf);/*发送缓冲区数据宽度*/
/*******************************************************************/
open_serial(0); /*打开串口1*/
/*******************************************************************/
printf("ready for sending data...\n"); /*准备开始发送数据*/
tcgetattr(fd,&option);
cfmakeraw(&option);
/*****************************************************************/
cfsetispeed(&option,B9600); /*波特率设置为9600bps*/
cfsetospeed(&option,B9600);
/*******************************************************************/
tcsetattr(fd,TCSANOW,&option);
retv=write(fd,sbuf,length); /*接收数据*/
if(retv==-1)
{
perror("write");
}
printf("the number of char sent is %d\n",retv);
flag_close =close(fd);
if(flag_close == -1) /*判断是否成功关闭文件*/
printf("Close the Device failur！\n");
return 0;
}
