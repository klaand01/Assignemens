#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

/* You will add includes here */
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>

//Included to get the support library
#include <calcLib.h>
#include "protocol.h"

int nrOfClients = 0;

struct clientAddr
{
  struct sockaddr_storage *clientInfo;
  socklen_t ai_addrlen;
  struct calcProtocol *clientProtocol;
  struct timeval time;
  bool sentMsg;
};

clientAddr clients[100];

void removeClient(int index)
{
  for (int i = index; i < nrOfClients - 1; i++)
  {
    clients[i] = clients[i + 1];
  }
  nrOfClients--;
}

void checkJobList(int signum)
{
  printf("Checking clients\n");

  if (signum == SIGALRM)
  {
    struct timeval compTime;
    gettimeofday(&compTime, NULL);

    for (int i = 0; i < nrOfClients; i++)
    {
      if ((compTime.tv_sec - clients[i].time.tv_sec) >= 10)
      {
        removeClient(i);
        printf("Client removed\n");
      }
    }
  }
}

void *getAddr(struct sockaddr *addr)
{
  if (addr->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in*)addr)->sin_addr);
  }

  return &(((struct sockaddr_in6*)addr)->sin6_addr);
}

int getPort(struct sockaddr *addr)
{
  if (addr->sa_family == AF_INET)
  {
    return ((struct sockaddr_in*)addr)->sin_port;
  }

  return ((struct sockaddr_in6*)addr)->sin6_port;
}

void cMessageNtoH(struct calcMessage* clientMsg)
{
  clientMsg->type = ntohs(clientMsg->type);
  clientMsg->message = ntohl(clientMsg->message);
  clientMsg->protocol = ntohs(clientMsg->protocol);
  clientMsg->major_version = ntohs(clientMsg->major_version);
  clientMsg->minor_version = ntohs(clientMsg->minor_version);
}

void cMessageHtoN(struct calcMessage *serverMsg)
{
  serverMsg->type = htons(serverMsg->type);
  serverMsg->message = htonl(serverMsg->message);
  serverMsg->protocol = htons(serverMsg->protocol);
  serverMsg->major_version = htons(serverMsg->major_version);
  serverMsg->minor_version = htons(serverMsg->minor_version);
}

void cProtocolNtoH(struct calcProtocol* clientMsg)
{
  clientMsg->type = ntohs(clientMsg->type);
  clientMsg->major_version = ntohs(clientMsg->major_version);
  clientMsg->minor_version = ntohs(clientMsg->minor_version);
  clientMsg->id = ntohl(clientMsg->id);
  clientMsg->arith = ntohl(clientMsg->arith);
  clientMsg->inValue1 = ntohl(clientMsg->inValue1);
  clientMsg->inValue2 = ntohl(clientMsg->inValue2);
  clientMsg->inResult = ntohl(clientMsg->inResult);
}

void cProtocolHtoN(calcProtocol *serverMsg)
{
  serverMsg->type = htons(serverMsg->type);
  serverMsg->major_version = htons(serverMsg->major_version);
  serverMsg->minor_version = htons(serverMsg->minor_version);
  serverMsg->id = htonl(serverMsg->id);
  serverMsg->arith = htonl(serverMsg->arith);
  serverMsg->inValue1 = htonl(serverMsg->inValue1);
  serverMsg->inValue2 = htonl(serverMsg->inValue2);
  serverMsg->inResult = htonl(serverMsg->inResult);
}

int main(int argc, char *argv[])
{
  /* Do more magic */

  if (argc != 2)
  {
    printf("Usage: %s hostname:port \n", argv[0]);
    exit(1);
  }

  initCalcLib();
  char delim[] = ":";
  char *Desthost = strtok(argv[1], delim);
  char *Destport = strtok(NULL, delim);

  int port = atoi(Destport);
  printf("Host %s, and port %d. \n", Desthost, port);

  /*
    Prepare to set up a reocurring event every 10s. If it_interva, or it_value is omitted, it will be a sigle alarm 10s after it has been set
  */

  struct itimerval alarmTime;
  alarmTime.it_interval.tv_sec = 10;
  alarmTime.it_interval.tv_usec = 10;
  alarmTime.it_value.tv_sec = 10;
  alarmTime.it_value.tv_usec = 10;

  /* Register a callback function, associated with the SIGALRM signal, which will be raised when the alarm goes off */

  signal(SIGALRM, checkJobList);
  setitimer(ITIMER_REAL, &alarmTime, NULL);

  socklen_t addrlen;
  struct addrinfo addrs, *servinfo, *ptr;
  int returnValue, serverSocket, numbytes;
  int current = 1;
  int idCounter = 0;
  char buf[256];
  int iNumb1, iNumb2, iRes, iDiff;
  double dNumb1, dNumb2, dRes, dDiff;

  char ipAddr1[INET6_ADDRSTRLEN];
  char ipAddr2[INET6_ADDRSTRLEN];
  int port1, port2;

  memset(&addrs, 0, sizeof(addrs));
  addrs.ai_family = AF_UNSPEC;
  addrs.ai_socktype = SOCK_DGRAM;
  addrs.ai_flags = AI_PASSIVE;

  calcMessage okMsg, notOkMsg;
  okMsg.major_version = 1;
  okMsg.minor_version = 0;
  okMsg.message = 1;
  okMsg.protocol = 17;
  okMsg.type = 2;
  cMessageHtoN(&okMsg);
  
  notOkMsg.major_version = 1;
  notOkMsg.minor_version = 0;
  notOkMsg.message = 2;
  notOkMsg.protocol = 17;
  notOkMsg.type = 2;
  cMessageHtoN(&notOkMsg);

  returnValue = getaddrinfo(Desthost, Destport, &addrs, &servinfo);
  if (returnValue != 0)
  {
    perror("Wrong with gettaddrinfo\n");
    exit(1);
  }

  for (ptr = servinfo; ptr != NULL; ptr = ptr->ai_next)
  {
    serverSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (serverSocket == -1)
    {
      perror("Socket not created\n");
      exit(1);
    }

    returnValue = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &current, sizeof(current));
    if (returnValue == -1)
    {
      perror("Wrong with setsocketopt\n");
      exit(1);
    }

    returnValue = bind(serverSocket, ptr->ai_addr, ptr->ai_addrlen);
    if (returnValue == -1)
    {
      perror("Could not bind\n");
      exit(1);
    }

    break;
  }

  freeaddrinfo(servinfo);
  if (ptr == NULL)  
  {
		perror("Server: failed to bind \n");
		exit(1);
	}

  addrlen = sizeof(addrlen);

  calcProtocol *cProtocol = new calcProtocol;
  calcProtocol *tempProtocol = new calcProtocol;
  calcMessage *cMessage = new calcMessage;

  sockaddr_storage clientIn;
  socklen_t clientinSize = sizeof(clientIn);

  while(1)
  {
    memset(cProtocol, 0, sizeof(*cProtocol));
    
    numbytes = recvfrom(serverSocket, cProtocol, sizeof(*cProtocol), 0, (struct sockaddr *)&clientIn, &clientinSize);
    if (numbytes == -1)
    {
      perror("Message not received\n");
      close(serverSocket);
    }
    if (numbytes == 0)
    {
      printf("Nothing received\n");
    }
    else
    {
      printf("Message received\n");
    }

    if (numbytes == sizeof(calcMessage))
    {
      printf("Calc Message received\n");
      cMessage = (calcMessage *)cProtocol;
      cMessageNtoH(cMessage);

      if (cMessage->major_version != 1 || cMessage->message != 0 || cMessage->minor_version != 0 || 
      cMessage->protocol != 17 || cMessage->type != 22)
      {
        numbytes = sendto(serverSocket, &notOkMsg, sizeof(calcMessage), 0, (struct sockaddr *)&clientIn, clientinSize);
        if (numbytes == -1)
        {
          perror("'Not ok' message not sent\n");
          break;
        }
      }
      else
      {
        printf("Calc message OK\n");
        clients[nrOfClients].clientInfo = &clientIn;
        clients[nrOfClients].ai_addrlen = sizeof(clientinSize);
        clients[nrOfClients].sentMsg = true;

        //Calculations
        char *oper = randomType();

        tempProtocol->type = 1;
        tempProtocol->major_version = 1;
        tempProtocol->minor_version = 0;
        tempProtocol->id = idCounter++;

        memset(buf, 0, sizeof(buf));

        if (oper[0] == 'f')
        {
          dNumb1 = randomFloat();
          dNumb2 = randomFloat();

          tempProtocol->flValue1 = dNumb1;
          tempProtocol->flValue2 = dNumb2;
          tempProtocol->flResult = 0.0f;

          tempProtocol->inValue1 = 0;
          tempProtocol->inValue2 = 0;
          tempProtocol->inResult = 0;

          if (strcmp(oper, "fadd") == 0)
          {
            tempProtocol->arith = 5;
            dRes = dNumb1 + dNumb2;
          }

          if (strcmp(oper, "fsub") == 0)
          {
            tempProtocol->arith = 6;
            dRes = dNumb1 - dNumb2;
          }

          if (strcmp(oper, "fmul") == 0)
          {
            tempProtocol->arith = 7;
            dRes = dNumb1 * dNumb2;
          }

          if (strcmp(oper, "fdiv") == 0)
          {
            tempProtocol->arith = 8;
            dRes = dNumb1 / dNumb2;
          }
        }
        else
        {
          iNumb1 = randomInt();
          iNumb2 = randomInt();

          tempProtocol->inValue1 = iNumb1;
          tempProtocol->inValue2 = iNumb2;
          tempProtocol->inResult = 0;

          tempProtocol->flValue1 = (double)0;
          tempProtocol->flValue2 = (double)0;
          tempProtocol->flResult = (double)0;

          if (strcmp(oper, "add") == 0)
          {
            tempProtocol->arith = 1;
            iRes = iNumb1 + iNumb2;
          }

          if (strcmp(oper, "sub") == 0)
          {
            tempProtocol->arith = 2;
            iRes = iNumb1 - iNumb2;
          }

          if (strcmp(oper, "mul") == 0)
          {
            tempProtocol->arith = 3;
            iRes = iNumb1 * iNumb2;
          }

          if (strcmp(oper, "div") == 0)
          {
            tempProtocol->arith = 4;
            iRes = iNumb1 / iNumb2;
          }
        }

        cProtocolHtoN(tempProtocol);
        
        printf("Sending assignment\n");
        numbytes = sendto(serverSocket, tempProtocol, sizeof(*tempProtocol), 0, (struct sockaddr *)&clientIn, clientinSize);
        if (numbytes == -1)
        {
          perror("Could not send calcProtocol\n");
        }
        else
        {
          clients[nrOfClients].clientProtocol = tempProtocol;
          gettimeofday(&clients[nrOfClients].time, NULL);
          nrOfClients++;
        }
      }
    }
    else if (numbytes == sizeof(calcProtocol))
    {
      printf("Calc Protocol received\n");
      cProtocolNtoH(cProtocol);

      for (int i = 0; i < nrOfClients; i++)
      {
        if (clients[i].sentMsg)
        {
          inet_ntop(clientIn.ss_family, getAddr((struct sockaddr *)&clientIn), ipAddr2, sizeof(ipAddr2));  
          inet_ntop(clients[i].clientInfo->ss_family, getAddr((struct sockaddr *)clients[i].clientInfo), ipAddr1, sizeof(ipAddr1));

          port1 = getPort((struct sockaddr *)&clientIn);
          port2 = getPort((struct sockaddr *)clients[i].clientInfo);

          cProtocolNtoH(clients[i].clientProtocol);

          if (clients[i].clientProtocol->id == cProtocol->id && strcmp(ipAddr1, ipAddr2) == 0 && port1 == port2)
          {
            dDiff = dRes - cProtocol->flResult;
            iDiff = iRes - cProtocol->inResult;

            if (dDiff < 0.0001 || iDiff < 0.0001)
            {
              printf("Correct answer\n");
              numbytes = sendto(serverSocket, &okMsg, sizeof(calcMessage), 0, (struct sockaddr *)&clientIn, clientinSize);
              if (numbytes == -1)
              {
                perror("Answer not sent\n");
              }
            }
            else
            {
              printf("Not correct answer\n");
              numbytes = sendto(serverSocket, &notOkMsg, sizeof(calcMessage), 0, (struct sockaddr*)&clientIn, clientinSize);
              if (numbytes == -1)
              {
                perror("Correction not sent\n");
                break;
              }
            }
          }
          else
          {
            printf("Wrong client\n");
            numbytes = sendto(serverSocket, &notOkMsg, sizeof(calcMessage), 0, (struct sockaddr*)&clientIn, clientinSize);
            if (numbytes == -1)
            {
              perror("Not ok message not sent\n");
            }
          }

          clients[i].sentMsg = false;
          printf("Removing finished client\n");
          removeClient(i);
        }
        else
        {
          printf("Client didn't send a calcMessage first\n");
          numbytes = sendto(serverSocket, &notOkMsg, sizeof(calcMessage), 0, (struct sockaddr *)&clientIn, clientinSize);
          if (numbytes == -1)
          {
            perror("Not ok message not sent\n");
          }
        }
      }
    }
    else
    {
      printf("Wrong message received\n");
      numbytes = sendto(serverSocket, &notOkMsg, sizeof(calcMessage), 0, (struct sockaddr *)&clientIn, clientinSize);
      if (numbytes == -1)
      {
        perror("Not ok message not sent\n");
      }
    }
  }

  delete cProtocol;
  delete cMessage;
  delete tempProtocol;
  close(serverSocket);
}