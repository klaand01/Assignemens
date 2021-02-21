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
#include <netdb.h>


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

  int serverSocket, returnValue, sentBytes;
  int current = 1;

  struct addrinfo addrs, *ptr;
  memset(&addrs, 0, sizeof(addrs));
  addrs.ai_family = AF_UNSPEC;
  addrs.ai_socktype = SOCK_DGRAM;
  addrs.ai_protocol = IPPROTO_UDP;

  returnValue = getaddrinfo(argv[1], Destport, &addrs, &ptr);
  if (returnValue != 0)
  {
    perror("Wrong with getaddrinfo \n");
    exit(1);
  }
  
  serverSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
  if (serverSocket == -1)
  {
    perror("Socket failed to create \n");
    exit(1);
  }

  returnValue = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &current, sizeof(current));
  if (returnValue == -1)
  {
    perror("Setsocketopt");
    exit(1);
  }

  returnValue = bind(serverSocket, ptr->ai_addr, ptr->ai_addrlen);
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

  struct calcMessage cMessage;
  struct calcProtocol cProtocol;

  while(terminate==0)
  {
    printf("This is the main loop, %d time.\n",loopCount);

    sentBytes = recvfrom(serverSocket, &cMessage, sizeof(cMessage), 0, ptr->ai_addr, &ptr->ai_addrlen);
    if (sentBytes == -1)
    {
      perror("Message not received \n");
    }
    else
    {
      printf("CalcMessage received \n");
    }

    cMessage.type = ntohs(cMessage.type);
    cMessage.message = ntohl(cMessage.message);
    cMessage.protocol = ntohs(cMessage.protocol);
    cMessage.major_version = ntohs(cMessage.major_version);

    if (cMessage.type != 22 || cMessage.message != 0 || 
    cMessage.protocol != 17 || cMessage.major_version != 1)
    {
      cMessage.message = htons(2);
      sentBytes = sendto(serverSocket, &cMessage, sizeof(cMessage), 0, ptr->ai_addr, ptr->ai_addrlen);
      printf("Wrong message received \n");
      exit(1);
    }

    


    sleep(1);
    loopCount++;
  }

  printf("done.\n");
  return(0);
  close(serverSocket);
}