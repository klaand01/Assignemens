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
#include <stdlib.h>

// Included to get the support library
#include <calcLib.h>
#include "protocol.h"

using namespace std;
/* Needs to be global, to be rechable by callback and main */
int loopCount=0;

int nrOfClients = 0;
int maxClient = 100;
int currentClient = 0;

struct clientAddr
{
  struct sockaddr_storage *clientInfo;
  socklen_t ai_addrlen;
  struct calcProtocol *clientProtocol;
  struct timeval time;
};

clientAddr clients[1000];

bool removeClient(int index)
{
  bool removed = false;

  for (int i = index; i < nrOfClients -1; i++)
  {
    clients[i] = clients[i + 1];
    removed = true;
  }
  
  nrOfClients--;
  return removed;
}

/* Call back function, will be called when the SIGALRM is raised when the timer expires. */
void checkJobbList(int signum)
{
  // As anybody can call the handler, its good coding to check the signal number that called it.
  if (signum == SIGALRM)
  {
    struct timeval compTime;
    gettimeofday(&compTime, NULL);

    for (int i = 0; i < nrOfClients; i++)
    {
      if ((compTime.tv_sec - clients[i].time.tv_sec) >= 10)
      {
        removeClient(i);
      }
    }
  }

  printf("Let me be, I want to sleep.\n");
  
  return;
}

void convertMsgPrint(struct calcMessage* cMessage)
{
  cMessage->type = ntohs(cMessage->type);
  cMessage->message = ntohl(cMessage->message);
  cMessage->protocol = ntohs(cMessage->protocol);
  cMessage->major_version = ntohs(cMessage->major_version);
  cMessage->minor_version = ntohs(cMessage->minor_version);
}

void convertProtocolPrint(struct calcProtocol* cProtocol)
{
  cProtocol->type = ntohs(cProtocol->type);
  cProtocol->major_version = ntohs(cProtocol->major_version);
  cProtocol->minor_version = ntohs(cProtocol->minor_version);
  cProtocol->id = ntohl(cProtocol->id);
  cProtocol->arith = ntohl(cProtocol->arith);
  cProtocol->inValue1 = ntohl(cProtocol->inValue1);
  cProtocol->inValue2 = ntohl(cProtocol->inValue2);
  cProtocol->inResult = ntohl(cProtocol->inResult);
}

void convertProtocolSend(struct calcProtocol* cProtocol)
{
  cProtocol->type = htons(cProtocol->type);
  cProtocol->major_version = htons(cProtocol->major_version);
  cProtocol->minor_version = htons(cProtocol->minor_version);
  cProtocol->id = htonl(cProtocol->id);
  cProtocol->arith = htonl(cProtocol->arith);
  cProtocol->inValue1 = htonl(cProtocol->inValue1);
  cProtocol->inValue2 = htonl(cProtocol->inValue2);
  cProtocol->inResult = htonl(cProtocol->inResult);
}

void *getAddr(struct sockaddr* addr)
{
  if (addr->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in*)addr)->sin_addr);
  }
  else
  {
    return &(((struct sockaddr_in6*)addr)->sin6_addr);
  }
}

int getPort(struct sockaddr* addr)
{
  if (addr->sa_family == AF_INET)
  {
    return ((struct sockaddr_in*)addr)->sin_port;
  }
  else
  {
    return ((struct sockaddr_in6*)addr)->sin6_port;
  }
}


int main(int argc, char *argv[])
{
  /* Do more magic */
  if (argc != 2)
  {
    printf("Usage: %s hostname:port (%d)\n", argv[0], argc);
    exit(1);
  }

  char delim[]=":";
  char *Desthost=strtok(argv[1],delim);
  char *Destport=strtok(NULL,delim);
  int port=atoi(Destport);
  printf("Host %s, and port %d. \n", Desthost, port);

  int serverSocket, numBytes, returnValue;
  int idCounter = 0;
  int current = 1;

  char ipAddr1[INET6_ADDRSTRLEN];
  char ipAddr2[INET6_ADDRSTRLEN];
  int port1, port2;

  int iNumb1, iNumb2, iRes, iDiff;
  double dNumb1, dNumb2, dRes, dDiff;

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

  struct addrinfo addrs, *ptr, *servinfo;
  socklen_t addrlen;
  memset(&addrs, 0, sizeof(addrs));
  addrs.ai_family = AF_UNSPEC;
  addrs.ai_socktype = SOCK_DGRAM;
  addrs.ai_flags = AI_PASSIVE;
  addrs.ai_protocol = IPPROTO_UDP;

  returnValue = getaddrinfo(Desthost, Destport, &addrs, &servinfo);
  if (returnValue != 0)
  {
    perror("Wrong with getaddrinfo \n");
    exit(1);
  }
  
  for (ptr = servinfo; ptr != NULL; ptr = ptr->ai_next)
  {
    serverSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (serverSocket == -1)
    {
      perror("Socket not created \n");
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
      perror("Bind not gone through \n");
      exit(1);
    }

    break;
  }

  if (ptr == NULL)
  {
    printf("Server failed to bind \n");
    exit(1);
  }
  freeaddrinfo(servinfo);


  initCalcLib();
  calcMessage *cMessage = new calcMessage;
  calcProtocol *cProtocol = new calcProtocol;
  calcProtocol *tempProtocol = new calcProtocol;
  bool found = false;

  sockaddr_storage clientIn;
  socklen_t clientinSize = sizeof(clientIn);
  addrlen = sizeof(addrlen);

  while(1)
  {
    printf("This is the main loop, %d time.\n",loopCount);
    memset(cProtocol, 0, sizeof(*cProtocol));

    numBytes = recvfrom(serverSocket, cProtocol, sizeof(*cProtocol), 0, 
    (struct sockaddr*)&clientIn, &clientinSize);
    if (numBytes == -1)
    {
      perror("Message not received \n");
      close(serverSocket);
    }
    if (numBytes == 0)
    {
      perror("Nothing recieved \n");
      close(serverSocket);
    }
    else
    {
      printf("Message received \n");
    }

    if (numBytes == sizeof(calcMessage))
    {
      printf("Calcmessage recieved \n");
      cMessage = (calcMessage *)cProtocol;
      convertMsgPrint(cMessage);

      if (cMessage->type != 22 || cMessage->message != 0 || 
      cMessage->protocol != 17 || cMessage->major_version != 1)
      {
        cMessage->message = htons(2);
        numBytes = sendto(serverSocket, &cMessage, sizeof(calcMessage), 0, (struct sockaddr*)&clientIn, clientinSize);
        printf("Wrong message received \n");
      }

      clients[nrOfClients].clientInfo = &clientIn;
      clients[nrOfClients].ai_addrlen = sizeof(clientinSize);

      //Räkneoperationer
      char *oper = randomType();
      iNumb1 = randomInt();
      iNumb2 = randomInt();
      dNumb1 = randomFloat();
      dNumb2 = randomFloat();

      cProtocol->inValue1 = iNumb1;
      cProtocol->inValue2 = iNumb2;
      cProtocol->flValue1 = dNumb1;
      cProtocol->flValue2 = dNumb2;

      cProtocol->id = idCounter++;
      cProtocol->type = 1;
      cProtocol->major_version = 1;
      cProtocol->minor_version = 0;


      if (strcmp(oper, "add") == 0)
      {
        cProtocol->arith = 1;
        iRes = iNumb1 + iNumb2;
      }

      if (strcmp(oper, "sub") == 0)
      {
        cProtocol->arith = 2;
        iRes = iNumb1 - iNumb2;
      }

      if (strcmp(oper, "mul") == 0)
      {
        cProtocol->arith = 3;
        iRes = iNumb1 * iNumb2;
      }

      if (strcmp(oper, "div") == 0)
      {
        cProtocol->arith = 4;
        iRes = iNumb1 / iNumb2;
      }

      if (strcmp(oper, "fadd") == 0)
      {
        cProtocol->arith = 5;
        dRes = dNumb1 + dNumb2;
      }

      if (strcmp(oper, "fsub") == 0)
      {
        cProtocol->arith = 6;
        dRes = dNumb1 - dNumb2;
      }

      if (strcmp(oper, "fmul") == 0)
      {
        cProtocol->arith = 7;
        dRes = dNumb1 * dNumb2;
      }

      if (strcmp(oper, "fdiv") == 0)
      {
        cProtocol->arith = 8;
        dRes = dNumb1 / dNumb2;
      }

      convertProtocolSend(cProtocol);

      numBytes = sendto(serverSocket, tempProtocol, sizeof(*tempProtocol), 0, (struct sockaddr*)&clientIn, clientinSize);
      if (numBytes == -1)
      {
        perror("Calcprotocol not sent \n");
      }
      else
      {
        printf("Calcprotocol sent \n");
        clients[nrOfClients].clientProtocol = cProtocol;
        gettimeofday(&clients[nrOfClients++].time, NULL);
      }
    }
    else if (numBytes == sizeof(calcProtocol))
    {
      printf("Calcprotocol received \n");
      convertProtocolPrint(cProtocol);

      inet_ntop(clientIn.ss_family, getAddr((struct sockaddr *)&clientIn), ipAddr2, sizeof(ipAddr2));

      for (int i = 0; i < nrOfClients; i++)
      {
        inet_ntop(clients[i].clientInfo->ss_family, getAddr((struct sockaddr *)clients[i].clientInfo),
        ipAddr1, sizeof(ipAddr1));
        port1 = getPort((struct sockaddr *)&clientIn);
        port2 = getPort((struct sockaddr *)clients[i].clientInfo);

        convertProtocolPrint(clients[i].clientProtocol);

        if (clients[i].clientProtocol->id == cProtocol->id && strcmp(ipAddr1, ipAddr2) == 0 &&
        port1 == port2 && found != true)
        {
          currentClient = i;
          found = true;
        }
      }

      if (found == true)
      {
        found = false;

        iDiff = iRes - cProtocol->inResult;
        dDiff = dRes - cProtocol->flResult;

        if (iDiff < 0.0001 || dDiff < 0.0001)
        {
          cMessage->message = htonl(1);
          numBytes = sendto(serverSocket, &cMessage, sizeof(calcMessage), 0, (struct sockaddr*)&clientIn, clientinSize);
          if (numBytes == -1)
          {
            perror("Answer not sent \n");
          }
          else

          
          {
            printf("Answer sent everything correct!\n");
          }

          for (int i = 0; i < nrOfClients; i++)
          {
            removeClient(currentClient);
          }
        }
        else
        {
          cMessage->message = htonl(2);
          numBytes = sendto(serverSocket, &cMessage, sizeof(calcMessage), 0, (struct sockaddr*)&clientIn, clientinSize);
          if (numBytes == -1)
          {
            perror("Answer not sent \n");
          }
          else
          {
            printf("Answer sent \n");
          }
        }
      }      
    }

    loopCount++;
  }

  delete cMessage;
  delete cProtocol;
  delete tempProtocol;

  printf("done.\n");
  return(0);
  close(serverSocket);
}