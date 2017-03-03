/*
 * base_test.c
 *
 *  Created on: Feb 22, 2017
 *      Author: weiwen.gan
 */

#include <stdio.h>
#include <sys/io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <signal.h>
//add leisj
#include <sys/mman.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#define SOCKET_RECEIVE_PORT    6666 
#define SOCKET_SEND_PORT    6667
#define LENGTH_OF_LISTEN_QUEUE 20
#define BUFFER_SIZE 1024

#define FLAG_ENABLE 1
#define FLAG_DISABLE 0
#define FLAG_UP 1
#define FLAG_DOWN 0

#define GPIO_MIN_ADDR 0x298
#define GPIO_MAX_ADDR 0x29A
#define GPIO_GROUP_SELECT 0x298
#define GPIO_IO_DIRECTION 0x299
#define GPIO_DATA 0x29A

#define GPIO_BIT_P0 0x01
#define GPIO_BIT_P1 0x02
#define GPIO_BIT_P2 0x04
#define GPIO_BIT_P3 0x08
#define GPIO_BIT_P4 0x10
#define GPIO_BIT_P5 0x20
#define GPIO_BIT_P6 0x40
#define GPIO_BIT_P7 0x80


#define GPIO_FLAG_IO_ALL_OUTPUT 0x00
#define GPIO_FLAG_IO_ALL_INPUT 0xFF

// P0&P1 as input (0000 0011)
#define GPIO_FLAG_IO_CONFIG 0x03

// button press time span 100ms
#define BTN_PRESS_TIME 100
// button press time release 500ms
#define BTN_RELEASE_DELAY_TIME 500

// led flash default timestamp 500ms
#define LED_FLASH_TIME_DEFAULT 500
// flash status retain time
#define LED_FLASH_RETAIN_TIME  500000

// timestamp for main loop, 5ms
#define GPIO_MAIN_TIMESTAMP 10

typedef struct led_status_control {
	int status;    // 0: off, 1:on, 2:flash
	int save_status; // 0:off, 1:on
	int flash_status; // 0:off, 1:on  (using at flash)
	int flash_span_time;
	int flash_retain_time;
	int flash_retain_counter;
	int flash_on_counter;
	int flash_off_counter;
} LED_STATUS;

//add leisj
typedef struct{
  int  flag; // 0:无消息, 1:来自deamon, 2:发往deamon
  char message[BUFFER_SIZE]; // start:启动, success:启动成功, stop:停止, over:停止完成, error:出错
}control_message;

void sig_handler(const int sig)
{
	// release GPIO control
	ioperm(GPIO_MIN_ADDR, GPIO_MAX_ADDR, FLAG_DISABLE);

	printf("SIGINT handled.\n");
	printf("Info: Now stop GPIO demo.\n");

	exit(EXIT_SUCCESS);
}

void init_led_status();
unsigned char led_status_update_process(LED_STATUS* dev_led, unsigned char oldData, unsigned char gpio_bit);
void set_led_flash(LED_STATUS* dev_led);
//add leisj
void control_led_status(char* status);
void socket_receive(control_message* cm_map);
void socket_send(control_message* cm_map);

LED_STATUS dev_led_1;
LED_STATUS dev_led_2;
LED_STATUS dev_led_3;
LED_STATUS dev_led_4;

int main() {

	/* handle SIGINT */
	signal(SIGINT, sig_handler);

	// get GPIO control
	ioperm(GPIO_MIN_ADDR, GPIO_MAX_ADDR, FLAG_ENABLE);

	// GPIO main setting
	outb(0x04, GPIO_GROUP_SELECT);
	outb(GPIO_FLAG_IO_CONFIG, GPIO_IO_DIRECTION);

	// LED initialize
	init_led_status();

	unsigned char ioData = 0x00;
	unsigned char tempData = 0x00;
	unsigned char newIoData = 0x00;
	int p0_low_counter = 0;
	int p0_low_release_delay = 0;
	int p1_low_counter = 0;
	int p1_low_release_delay = 0;
	int btn_press_time = BTN_PRESS_TIME / GPIO_MAIN_TIMESTAMP;
	int btn_release_delay_time = BTN_RELEASE_DELAY_TIME / GPIO_MAIN_TIMESTAMP;

        // add leisj
        //加电初始状态
        control_led_status("start");
        control_message *cm_map;
        cm_map=(control_message*)mmap(NULL,sizeof(control_message)*2,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0); //共享内存，一个接收，一个发送
        memset(cm_map,0x00,sizeof(control_message)*2);
        if(fork() == 0)//fork一个接收deamon消息的子进程
        {
               socket_receive(cm_map);
        }
        if(fork() == 0)//fork一个发送deamon消息的子进程
        {
               socket_send(cm_map);
        }
	// main loop
	while(1) {

		// get status data from GPIO
		ioData = inb(GPIO_DATA);

		// event detect
		tempData = ioData & GPIO_BIT_P0;
		p0_low_counter = (tempData > 0 ? 0 : (p0_low_counter + 1));
		tempData = ioData & GPIO_BIT_P1;
		p1_low_counter = (tempData > 0 ? 0 : (p1_low_counter + 1));


		////////////////////////////////////////////////////////////////////////
		// GPIO business process
		////////////////////////////////////////////////////////////////////////

		// Button 0 (P0)
		if(p0_low_counter > btn_press_time && p0_low_release_delay == 0) {
			p0_low_release_delay = btn_release_delay_time;

			// button 0 (P0) click event process
			printf("Button 0 click.\n");
			// add custom process here
                        // add leisj
                        memset(cm_map+1,0x00,sizeof(control_message));
                        (*(cm_map+1)).flag = 2;
                        memcpy((*(cm_map+1)).message, "start",5);
                        control_led_status("start");
		}
		// Button 2 (P1)
		if(p1_low_counter > btn_press_time && p1_low_release_delay == 0) {
			p1_low_release_delay = btn_release_delay_time;

			// button 1 (P1) click event process
			printf("Button 1 click.\n");
			// add custom process here
                        // add leisj
                        memset(cm_map+1,0x00,sizeof(control_message));
                        (*(cm_map+1)).flag = 2;
                        memcpy((*(cm_map+1)).message, "stop",4);
                        control_led_status("stop");
		}

		newIoData = ioData;

		// LED 1 (P2)
		tempData = ioData & GPIO_BIT_P2;
		tempData = led_status_update_process(&dev_led_1, tempData, GPIO_BIT_P2);
		newIoData = tempData | ((~GPIO_BIT_P2) & newIoData);

		// LED 2 (P3)
		tempData = ioData & GPIO_BIT_P3;
		tempData = led_status_update_process(&dev_led_2, tempData, GPIO_BIT_P3);
		newIoData = tempData | ((~GPIO_BIT_P3) & newIoData);

		// LED 3 (P4)
		tempData = ioData & GPIO_BIT_P4;
		tempData = led_status_update_process(&dev_led_3, tempData, GPIO_BIT_P4);
		newIoData = tempData | ((~GPIO_BIT_P4) & newIoData);

		// LED 4 (P6)
		tempData = ioData & GPIO_BIT_P6;
		tempData = led_status_update_process(&dev_led_4, tempData, GPIO_BIT_P6);
		newIoData = tempData | ((~GPIO_BIT_P6) & newIoData);

		// Update LED status
		outb(newIoData, GPIO_DATA);


		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		// delay counter
		p0_low_release_delay = (p0_low_release_delay > 0 ? (p0_low_release_delay - 1) : 0);
		p1_low_release_delay = (p1_low_release_delay > 0 ? (p1_low_release_delay - 1) : 0);

		//sleep 1msxn,
		usleep(1000 * GPIO_MAIN_TIMESTAMP);
	}

	// never go here
	ioperm(GPIO_MIN_ADDR, GPIO_MAX_ADDR, FLAG_DISABLE);

	return 0;
}

// LED light status initialize.(must invoke after GPIO direction setting)
void init_led_status()
{
	unsigned char ioData = 0x00;
	ioData = inb(GPIO_DATA);

	// clear output bit
	ioData = ioData & (GPIO_BIT_P0 | GPIO_BIT_P1); // ioData & 0x03

	// LED 1
	dev_led_1.status = 1;
	dev_led_1.flash_span_time = LED_FLASH_TIME_DEFAULT;
	dev_led_1.flash_retain_time = LED_FLASH_RETAIN_TIME;
	dev_led_1.flash_retain_counter = 0;
	dev_led_1.flash_off_counter = 0;
	dev_led_1.flash_on_counter = 0;
	dev_led_1.save_status = dev_led_1.status;

	// LED 2
	dev_led_2.status = 1;
	dev_led_2.flash_span_time = LED_FLASH_TIME_DEFAULT;
	dev_led_2.flash_retain_time = LED_FLASH_RETAIN_TIME;
	dev_led_2.flash_retain_counter = 0;
	dev_led_2.flash_off_counter = 0;
	dev_led_2.flash_on_counter = 0;
	dev_led_2.save_status = dev_led_2.status;

	// LED 3
	dev_led_3.status = 1;
	dev_led_3.flash_span_time = LED_FLASH_TIME_DEFAULT;
	dev_led_3.flash_retain_time = LED_FLASH_RETAIN_TIME;
	dev_led_3.flash_retain_counter = 0;
	dev_led_3.flash_off_counter = 0;
	dev_led_3.flash_on_counter = 0;
	dev_led_3.save_status = dev_led_3.status;

	// LED 4
	dev_led_4.status = 1;
	dev_led_4.flash_span_time = LED_FLASH_TIME_DEFAULT;
	dev_led_4.flash_retain_time = LED_FLASH_RETAIN_TIME;
	dev_led_4.flash_retain_counter = 0;
	dev_led_4.flash_off_counter = 0;
	dev_led_4.flash_on_counter = 0;
	dev_led_4.save_status = dev_led_4.status;

	// Now just support on or off
	if(dev_led_1.status == 1)
		ioData = ioData | GPIO_BIT_P2;
	if(dev_led_2.status == 1)
		ioData = ioData | GPIO_BIT_P3;
	if(dev_led_3.status == 1)
		ioData = ioData | GPIO_BIT_P4;
	if(dev_led_4.status == 1)
		ioData = ioData | GPIO_BIT_P6;

	// output data
	outb(ioData, GPIO_DATA);
}

// Update LED GPIO bit data.
unsigned char led_status_update_process(LED_STATUS* dev_led, unsigned char oldData, unsigned char gpio_bit)
{
	unsigned char newData = oldData;
	if(dev_led->status == 0) {
		// off
		newData = 0x00;
	} else if(dev_led->status == 1) {
		// on
		newData = gpio_bit;
	} else if(dev_led->status == 2) {
		// flash
		dev_led->flash_retain_counter --;
		if(dev_led->flash_retain_counter > 0) {
			// continue flash
			if(oldData == gpio_bit) {
				// current is on
				dev_led->flash_on_counter --;
				if(dev_led->flash_on_counter == 0) {
					newData = 0x00; // turn off
					dev_led->flash_off_counter = dev_led->flash_span_time / GPIO_MAIN_TIMESTAMP;
				}
			} else {
				// current is off
				dev_led->flash_off_counter --;
				if(dev_led->flash_off_counter == 0) {
					newData = gpio_bit; // turn on
					dev_led->flash_on_counter = dev_led->flash_span_time / GPIO_MAIN_TIMESTAMP;
				}
			}
		} else {
			// stop flash
			dev_led->status = dev_led->save_status;
			newData = (dev_led->status == 0 ? 0x00 : gpio_bit);
			dev_led->flash_retain_counter = 0;
			dev_led->flash_off_counter = 0;
			dev_led->flash_on_counter = 0;
		}
	}

	return newData;

}

void set_led_flash(LED_STATUS* dev_led)
{
	dev_led->save_status = dev_led->status;
	dev_led->flash_retain_counter = dev_led->flash_retain_time / GPIO_MAIN_TIMESTAMP;
	dev_led->flash_off_counter = dev_led->flash_span_time / GPIO_MAIN_TIMESTAMP;
	dev_led->flash_on_counter = dev_led->flash_span_time / GPIO_MAIN_TIMESTAMP;
	dev_led->status = 2;
}

//add leisj
void control_led_status(char* status)
{
printf("control_led_status %s",status);
        if(memcmp(status,"init",4)==0){
              //加电初始状态
              dev_led_1.status = 0;//绿灯灭
              dev_led_2.status = 1;//红灯亮
              dev_led_3.status = 0;//橙灯灭
        }else if(memcmp(status,"start",5)==0){
              set_led_flash(&dev_led_1);//绿灯闪
              dev_led_2.status = 0;//红灯灭
              dev_led_3.status = 0;//橙灯灭
        }else if(memcmp(status,"success",7)==0){
              dev_led_1.status = 1;//绿灯亮
              dev_led_2.status = 0;//红灯灭
              dev_led_3.status = 0;//橙灯灭
        }else if(memcmp(status,"stop",4)==0){
              dev_led_1.status = 0;//绿灯灭
              set_led_flash(&dev_led_2);//红灯闪
              dev_led_3.status = 0;//橙灯灭
        }else if(memcmp(status,"over",4)==0){
              dev_led_1.status = 0;//绿灯灭
              dev_led_2.status = 1;//红灯亮
              dev_led_3.status = 0;//橙灯灭
        }else if(memcmp(status,"error",5)==0){
              dev_led_1.status = 0;//绿灯灭
              dev_led_2.status = 0;//红灯灭
              dev_led_3.status = 1;//橙灯亮
        }else{
              printf("no meaning %s\n",status);
        }
 
}

void socket_receive(control_message* cm_map){
    //设置一个socket地址结构server_addr,代表服务器internet地址, 端口
    struct sockaddr_in server_addr;
    bzero(&server_addr,sizeof(server_addr)); //把一段内存区的内容全部设置为0
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(SOCKET_RECEIVE_PORT);
 
    //创建用于internet的流协议(TCP)socket,用server_socket代表服务器socket
    int server_socket = socket(PF_INET,SOCK_STREAM,0);
    if( server_socket < 0)
    {
        printf("Create Socket Failed!");
        exit(1);
    }
    int opt =1;
    setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
     
    //把socket和socket地址结构联系起来
    if( bind(server_socket,(struct sockaddr*)&server_addr,sizeof(server_addr)))
    {
        printf("Server Bind Port : %d Failed!", SOCKET_RECEIVE_PORT); 
        exit(1);
    }
 
    //server_socket用于监听
    if ( listen(server_socket, LENGTH_OF_LISTEN_QUEUE) )
    {
        printf("Server Listen Failed!"); 
        exit(1);
    }
    while (1) //服务器端要一直运行
    {
        struct sockaddr_in client_addr;
        socklen_t length = sizeof(client_addr);
 
        int new_server_socket = accept(server_socket,(struct sockaddr*)&client_addr,&length);
        if ( new_server_socket < 0)
        {
            printf("Server Accept Failed!\n");
            break;
        }
         
        char buffer[BUFFER_SIZE];
        bzero(buffer, BUFFER_SIZE);
        length = recv(new_server_socket,buffer,BUFFER_SIZE,0);
        if (length < 0)
        {
            printf("Server Recieve Data Failed!\n");
            control_led_status("error");
            break;
        }
        (*cm_map).flag=1;
        memcpy((*cm_map).message,buffer,sizeof(buffer));
        printf("receive buffer %s\n",buffer);
        printf("receive (*cm_map).message %s\n",(*cm_map).message);
        if(memcmp(buffer,"start",5)==0){
         printf("led start");
            control_led_status("start");
        }else if(memcmp(buffer,"success",7)==0){
            control_led_status("success");
        }else if(memcmp(buffer,"stop",4)==0){
            control_led_status("stop");
        }else if(memcmp(buffer,"over",4)==0){
            control_led_status("over");
        }else if(memcmp(buffer,"error",5)==0){
            control_led_status("error");
        }else {
            printf("no meaning %s\n",buffer);
        }
        close(new_server_socket);
    }
    close(server_socket);
}

void socket_send(control_message* cm_map){
    //设置一个socket地址结构client_addr,代表客户机internet地址, 端口
    struct sockaddr_in client_addr;
    bzero(&client_addr,sizeof(client_addr)); //把一段内存区的内容全部设置为0
    client_addr.sin_family = AF_INET;    //internet协议族
    client_addr.sin_addr.s_addr = htons(INADDR_ANY);//INADDR_ANY表示自动获取本机地址
    client_addr.sin_port = htons(0);    //0表示让系统自动分配一个空闲端口
    //创建用于internet的流协议(TCP)socket,用client_socket代表客户机socket
    int client_socket = socket(AF_INET,SOCK_STREAM,0);
    if( client_socket < 0)
    {
        printf("Create Socket Failed!\n");
        exit(1);
    }
    //把客户机的socket和客户机的socket地址结构联系起来
    if( bind(client_socket,(struct sockaddr*)&client_addr,sizeof(client_addr)))
    {
        printf("Client Bind Port Failed!\n"); 
        exit(1);
    }
 
    //设置一个socket地址结构server_addr,代表服务器的internet地址, 端口
    struct sockaddr_in server_addr;
    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(SOCKET_SEND_PORT);
    socklen_t server_addr_length = sizeof(server_addr);
    //向服务器发起连接,连接成功后client_socket代表了客户机和服务器的一个socket连接
    if(connect(client_socket,(struct sockaddr*)&server_addr, server_addr_length) < 0)
    {
        printf("Can Not Connect To deamon!\n");
        exit(1);
    }
 
    char buffer[BUFFER_SIZE];
    while(1){
      sleep(1);
      if((*(cm_map+1)).flag==2){
        bzero(buffer,BUFFER_SIZE);
        strncpy(buffer, (*(cm_map+1)).message, strlen((*cm_map).message));
        //向服务器发送buffer中的数据
        send(client_socket,buffer,BUFFER_SIZE,0);
        (*(cm_map+1)).flag=0;//发完置0
      }
    }
}
