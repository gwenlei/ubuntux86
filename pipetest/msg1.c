#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/io.h>
struct my_msg_st {
  long int my_msg_type;
  char some_text[BUFSIZ];
};
void blinkblink(){
  ioperm(0x297,0x29A,1);
  outb(0x04,0x298);
  outb(0x00,0x299);
  int i=0;
  for(i;i<60;i++){
  outb(0x00,0x29A);
  sleep(1);
  outb(0xFF,0x29A);
  sleep(1);
  }
  ioperm(0x298,0x29A,0);
}
int main()
{
  int running = 1;
  int msgid;
  struct my_msg_st some_data;
  long int msg_to_receive = 0;
  msgid = msgget((key_t)1234, 0666 | IPC_CREAT);
  if (msgid == -1) 
  {
    fprintf(stderr, "msgget failed with error: %d\n", errno);
    exit(EXIT_FAILURE);
  }
  while(running) 
  {
    if (msgrcv(msgid, (void *)&some_data, BUFSIZ, msg_to_receive, 0) == -1) 
    {
      fprintf(stderr, "msgrcv failed with error: %d\n", errno);
      exit(EXIT_FAILURE);
    }
    printf("You wrote: %s", some_data.some_text);
    if (strncmp(some_data.some_text, "end", 3) == 0) 
    {
      running = 0;
    }
    else if (strncmp(some_data.some_text, "blink", 5) == 0){
      pid_t fork_result=fork();
      if (fork_result == (pid_t)-1) {  
        fprintf(stderr, "Fork failure");  
        exit(EXIT_FAILURE);  
      }
      if (fork_result == (pid_t)0) { 
        blinkblink();
      }
      else{
        printf("fork blink\n");
      }
    }
  }
  if (msgctl(msgid, IPC_RMID, 0) == -1) 
  {
    fprintf(stderr, "msgctl(IPC_RMID) failed\n");
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}
