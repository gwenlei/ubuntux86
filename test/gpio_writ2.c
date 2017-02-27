#include <stdio.h>
#include <sys/io.h>
#include <unistd.h>


int main()
{

        ioperm(0x298,0x29A,1); 
        outb(0x04,0x298);
        outb(0xFF,0x299);
        int i=0;
        for(i;i<10;i++){
        printf("%c %X \n",inb(0x29A),inb(0x29A));
        sleep(1);
        }
        ioperm(0x298,0x29A,0);
        return 0;
}
