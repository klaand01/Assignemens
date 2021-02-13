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

//#include "protocol.h"

int main(int argc, char *argv[])
{
  /* Do magic */
  if (argc != 3)
  {
    printf("Usage: %s hostname:port (%d)\n", argv[0], argc);
  }

  struct addrinfo addrs, *ptr;
  int clientSocket;
  int returnValue;

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
    printf("Socket created! \n");
  }

  //Creating calcMessage
  struct message
  {
    int32_t type;
    int32_t message;
    int32_t protocol;

    int32_t majorVersion;
    int32_t minorVersion;
  };

  struct message calcMessage;
  calcMessage.type = htons(22);
  calcMessage.message = htons(0);
  calcMessage.protocol = 17;
  calcMessage.majorVersion = 1;
  calcMessage.minorVersion = 0;

  returnValue = sendto(clientSocket, &calcMessage, sizeof(calcMessage), 0, ptr->ai_addr, ptr->ai_addrlen);
  if (returnValue == -1)
  {
    printf("Message not sent");
  }
  else
  {
    printf("Message sent\n");
  }

  close(clientSocket);
}