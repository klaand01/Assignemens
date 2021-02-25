#include <stdio.h>
#include <stdlib.h>
/* You will to add includes here */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <arpa/inet.h>

// Included to get the support library
#include <calcLib.h>
#include "protocol.h"

int main(int argc, char *argv[])
{
  /* Do magic */
  if (argc != 2)
  {
    printf("Usage: %s hostname:port (%d)\n", argv[0], argc);
  }

  char delim[]=":";
  char *Desthost=strtok(argv[1],delim);
  char *Destport=strtok(NULL,delim);
  // *Desthost now points to a sting holding whatever came before the delimiter, ':'.
  // *Dstport points to whatever string came after the delimiter. 

  /* Do magic */
  int port=atoi(Destport);
  printf("Host %s, and port %d.\n", Desthost,port);

  int clientSocket, returnValue, sentBytes;
  int timeCounter = 0;
  int operNumbr, iNumb1, iNumb2, iRes;
  double dNumb1, dNumb2, dRes;

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

  clientSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
  if (clientSocket == -1)
  {
    perror("Failed to create socket \n");
    exit(1);
  }
  else
  {
    printf("Socket created \n");
  }

  struct timeval time;
  time.tv_sec = 2;

  returnValue = setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(time));
  if (returnValue == -1)
  {
    perror("Wrong with SO_RCVTIMEO \n");
    exit(1);
  }

  struct calcProtocol cProtocol;
  struct calcMessage cMessage;
  cMessage.type = htons(22);
  cMessage.message = htonl(0);
  cMessage.protocol = htons(17);
  cMessage.major_version = htons(1);
  cMessage.minor_version = htons(0);

  while (timeCounter < 3)
  {
    sentBytes = sendto(clientSocket, &cMessage, sizeof(cMessage), 0, ptr->ai_addr, ptr->ai_addrlen);
    if (sentBytes == -1)
    {
      perror("Message not sent \n");
    }
    else
    {
      printf("Message sent \n");
    }

    //Caclprotocol
    sentBytes = recvfrom(clientSocket, &cProtocol, sizeof(cProtocol), 0, ptr->ai_addr, &ptr->ai_addrlen);
    if (sentBytes == -1)
    {
      if (errno == EAGAIN)
      {
        timeCounter++;
        printf("Message not recevied on time \n");
        printf("Timecounter: %d \n", timeCounter);
      }
    }
    else
    {
      printf("Message received \n");
      timeCounter = 5;
    }
  }
  if (timeCounter == 3)
  {
    printf("Closing down \n");
    exit(1);
  }

  if (ntohl(cMessage.message) == 2)
  {
    printf("NOT OK \n");
    exit(1);
  }
  
  //Calcprotocol calculations
  operNumbr = ntohl(cProtocol.arith);

  if ((operNumbr >= 1) && (operNumbr <= 4))
  {
    iNumb1 = ntohl(cProtocol.inValue1);
    iNumb2 = ntohl(cProtocol.inValue2);

    switch (operNumbr)
    {
      case 1:
      iRes = iNumb1 + iNumb2;
      printf("ASSIGNMENT: add %d %d \n", iNumb1, iNumb2);
      break;

      case 2:
      iRes = iNumb1 - iNumb2;
      printf("ASSIGNMENT: sub %d %d \n", iNumb1, iNumb2);
      break;

      case 3:
      iRes = iNumb1 * iNumb2;
      printf("ASSIGNMENT: mul %d %d \n", iNumb1, iNumb2);
      break;

      case 4:
      iRes = iNumb1 / iNumb2;
      printf("ASSIGNMENT: div %d %d \n", iNumb1, iNumb2);
      break;
    }

    printf("Result: %d \n", iRes);
    cProtocol.inResult = htonl(iRes);
  }
  else if ((operNumbr >= 5) && (operNumbr <= 8))
  {
    dNumb1 = cProtocol.flValue1;
    dNumb2 = cProtocol.flValue2;

    switch (operNumbr)
    {
      case 5:
      dRes = dNumb1 + dNumb2;
      printf("ASSIGNMENT: fadd %f %f \n", dNumb1, dNumb2);
      break;

      case 6:
      dRes = dNumb1 - dNumb2;
      printf("ASSIGNMENT: fsub %f %f \n", dNumb1, dNumb2);
      break;

      case 7:
      dRes = dNumb1 * dNumb2;
      printf("ASSIGNMENT: fmul %f %f \n", dNumb1, dNumb2);
      break;

      case 8:
      dRes = dNumb1 / dNumb2;
      printf("ASSIGNMENT: fdiv %f %f \n", dNumb1, dNumb2);
      break;
    }

    printf("Result: %f \n", dRes);
    cProtocol.flResult = dRes;
  }

  timeCounter = 0;

  while (timeCounter < 3)
  {
    sentBytes = sendto(clientSocket, &cProtocol, sizeof(cProtocol), 0, ptr->ai_addr, ptr->ai_addrlen);
    if (sentBytes == -1)
    {
      perror("Answer not sent \n");
    }
    else
    {
      printf("Answer sent \n");
    }

    sentBytes = recvfrom(clientSocket, &cMessage, sizeof(cMessage), 0, ptr->ai_addr, &ptr->ai_addrlen);
    if (sentBytes == -1)
    {
      if (errno == EAGAIN)
      {
        timeCounter++;
        perror("Message not received on time \n");
      }
    }
    else
    {
      timeCounter = 5;
      printf("Message received \n");
    }
  }
  if (timeCounter == 3)
  {
    printf("Closing down \n");
    exit(1);
  }

  char myAddress[20];
	const char *myAdd;
  struct sockaddr_in sockAddrss;
  socklen_t sockAddrLen = sizeof(sockAddrss);
  getsockname(clientSocket, (struct sockaddr*)&sockAddrss, &sockAddrLen);
  
  myAdd = inet_ntop(sockAddrss.sin_family, &sockAddrss.sin_addr, myAddress, sizeof(myAddress));

  printf("Connected to %s:%d local %s:%d \n", Desthost, port,
  myAdd, ntohs(sockAddrss.sin_port));

  if (ntohl(cMessage.message) == 1)
  {
    printf("OK \n");
    exit(1);
  }

  if (ntohl(cMessage.message) == 2)
  {
    printf("NOT OK \n");
    exit(1);
  }
  
  close(clientSocket);
}