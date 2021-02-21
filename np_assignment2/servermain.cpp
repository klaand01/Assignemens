#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

/* You will to add includes here */
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>


// Included to get the support library
#include <calcLib.h>

#include "protocol.h"


using namespace std;
/* Needs to be global, to be rechable by callback and main */
int loopCount=0;
int terminate=0;


/* Call back function, will be called when the SIGALRM is raised when the timer expires. */
void checkJobbList(int signum)
{
  // As anybody can call the handler, its good coding to check the signal number that called it.

  printf("Let me be, I want to sleep.\n");

  if(loopCount>20)
  {
    printf("I had enough.\n");
    terminate=1;
  }
  
  return;
}


int main(int argc, char *argv[])
{
  /* Do more magic */
  if (argc != 2)
  {
    printf("Usage: %s hostname:port (%d)\n", argv[0], argc);
  }

  char delim[]=":";
  char *Desthost=strtok(argv[1],delim);
  char *Destport=strtok(NULL,delim);
  int port=atoi(Destport);
  printf("Host %s, and port %d. \n", Desthost, port);

  int serverSocket, returnValue;
  struct sockaddr_in myAddr;
  memset(&myAddr, 0, sizeof(myAddr));
  myAddr.sin_family = AF_UNSPEC;
  myAddr.sin_port = htons(port);
  inet_aton("0.0.0.0", (struct in_addr*)&myAddr.sin_addr.s_addr);
  
  serverSocket = socket(AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP);
  if (serverSocket == -1)
  {
    perror("Socket failed to create \n");
    exit(1);
  }

  returnValue = bind(serverSocket, (struct sockaddr*)&myAddr, sizeof(myAddr));
  if (returnValue == -1)
  {
    perror("Could not bind \n");
    exit(1);
  }

  /* 
     Prepare to setup a reoccurring event every 10s. If it_interval, or it_value is omitted, it will be a single alarm 10s after it has been set. 
  */
  struct itimerval alarmTime;
  alarmTime.it_interval.tv_sec=10;
  alarmTime.it_interval.tv_usec=10;
  alarmTime.it_value.tv_sec=10;
  alarmTime.it_value.tv_usec=10;

  /* Regiter a callback function, associated with the SIGALRM signal, which will be raised when the alarm goes of */
  signal(SIGALRM, checkJobbList);
  setitimer(ITIMER_REAL,&alarmTime,NULL); // Start/register the alarm. 

  
  while(terminate==0)
  {
    printf("This is the main loop, %d time.\n",loopCount);

    

    sleep(1);
    loopCount++;
  }

  printf("done.\n");
  return(0);
  close(serverSocket);
}