#include <stdio.h>
#include <stdlib.h>
/* You will to add includes here */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#define MAXDATA 255

int main(int argc, char *argv[])
{
  
  /* Do magic */
  
  if (argc != 3)
  {
    printf("Usage: %s hostname port nickname (%d)\n", argv[0], argc);
    exit(1);
  }

  char delim[]=":";
  char *Desthost=strtok(argv[1],delim);
  char *Destport=strtok(NULL,delim);
  int port=atoi(Destport);

  printf("2 argument: %s \n", argv[2]);
  printf("Connected to %s:%s \n", Desthost, Destport);

  struct addrinfo addrs, *ptr;
  memset(&addrs, 0, sizeof(addrs));
  addrs.ai_family = AF_UNSPEC;
  addrs.ai_socktype = SOCK_STREAM;
  addrs.ai_protocol = IPPROTO_TCP;

  int clientSocket, returnValue, numbrBytes;
  char buf[MAXDATA];

  returnValue = getaddrinfo(argv[1], Destport, &addrs, &ptr);
  if (returnValue != 0)
  {
    perror("Wrong with getaddrinfo \n");
    exit(1);
  }

  clientSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
  if (clientSocket == -1)
  {
    perror("Socket not created \n");
    exit(1);
  }

  returnValue = connect(clientSocket, ptr->ai_addr, ptr->ai_addrlen);
  if (returnValue == -1)
  {
    perror("Client not connected \n");
    exit(1);
  }
  else
  {
    printf("Client connected \n");
  }

  while (1)
  {
    memset(&buf, 0, sizeof(buf));

    numbrBytes = recv(clientSocket, buf, MAXDATA, 0);
    if (numbrBytes == -1)
    {
      perror("Wrong with message \n");
      exit(1);
    }

    if (strcmp(buf, "HELLO 1\n") == 0)
    {
      printf("Protocol supported, sending nickname \n");
    }
    else
    {
      perror("Protocol not supported, closing down \n");
      exit(1);
    }
  }

  close(clientSocket);
}