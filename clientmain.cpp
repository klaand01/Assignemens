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

  sentBytes = recvfrom(clientSocket, &cMessage, sizeof(cMessage), 0, ptr->ai_addr, &ptr->ai_addrlen);
  if (sentBytes == -1)
  {
    perror("Message not recieved \n");
  }
  else
  {
    printf("Message received \n");
  }

  //Kontroll
  if (ntohs(cMessage.message) == 2)
  {
    printf("NOT OKAY FOR SERVER \n");
    close(clientSocket);
  }
  printf("Number on message: %d \n", ntohs(cMessage.message));


  



  close(clientSocket);
}