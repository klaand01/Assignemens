#include <stdio.h>
#include <stdlib.h>
/* You will to add includes here */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

// Included to get the support library
#include <calcLib.h>

#define SERVERPORT "5000"

#include "protocol.h"

int main(int argc, char *argv[])
{
  /* Do magic */
  if (argc != 3)
  {
    printf("Usage: %s hostname:port (%d)\n", argv[0], argc);
  }

  struct addrinfo addrs, *ptr;
  int clientSocket;
  int returnValue, sentBytes;
  int operNumbr, iNumb1, iNumb2, iRes;
  double dNumb1, dNumb2, dRes;

  memset(&addrs, 0, sizeof(addrs));
  addrs.ai_family = AF_UNSPEC;
  addrs.ai_socktype = SOCK_DGRAM;
  addrs.ai_protocol = IPPROTO_UDP;

  returnValue = getaddrinfo(argv[1], SERVERPORT, &addrs, &ptr);
  if (returnValue == -1)
  {
    perror("Wrong with getaddrinfo \n");
  }

  clientSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
  if (clientSocket == -1)
  {
    perror("Failed to create socket \n");
  }
  else
  {
    printf("Socket created \n");
  }

  struct calcProtocol cProtocol;
  struct calcMessage cMessage;
  cMessage.type = htons(22);
  cMessage.message = htonl(0);
  cMessage.protocol = htons(17);
  cMessage.major_version = htons(1);
  cMessage.minor_version = htons(0);

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
    perror("calcProtocol not recieved \n");
  }
  else
  {
    printf("calcProtocol received \n");
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
      break;

      case 2:
      iRes = iNumb1 - iNumb2;
      break;

      case 3:
      iRes = iNumb1 * iNumb2;
      break;

      case 4:
      iRes = iNumb1 / iNumb2;
      break;
    }

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
      break;

      case 6:
      dRes = dNumb1 - dNumb2;
      break;

      case 7:
      dRes = dNumb1 * dNumb2;
      break;

      case 8:
      dRes = dNumb1 / dNumb2;
      break;
    }

    cProtocol.flResult = dRes;
  }

  sentBytes = sendto(clientSocket, &cProtocol, sizeof(cProtocol), 0, ptr->ai_addr, ptr->ai_addrlen);
  if (sentBytes == -1)
  {
    perror("Answer not sent \n");
  }

  sentBytes = recvfrom(clientSocket, &cMessage, sizeof(cMessage), 0, ptr->ai_addr, &ptr->ai_addrlen);
  if (sentBytes == -1)
  {
    perror("Correction not recieved \n");
  }
  

  //Final control
  if (ntohl(cMessage.message) == 1)
  {
    printf("OK \n");
  }
  else if (ntohl(cMessage.message) == 2)
  {
    printf("NOT OK \n");
    close(clientSocket);
  }

  close(clientSocket);
}