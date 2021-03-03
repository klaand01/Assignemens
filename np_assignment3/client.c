#include <stdio.h>
#include <stdlib.h>
/* You will to add includes here */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>

#define MAXDATA 255
#define STDIN 0

int main(int argc, char *argv[])
{
  
  /* Do magic */
  
  if (argc != 3)
  {
    printf("Usage: %s hostname:port nickname (%d)\n", argv[0], argc);
    exit(1);
  }

  char delim[]=":";
  char *Desthost=strtok(argv[1],delim);
  char *Destport=strtok(NULL,delim);
  int port=atoi(Destport);
  printf("Connected to %s:%s \n", Desthost, Destport);
  char *name = argv[2];
  fd_set readFd;

  struct addrinfo addrs, *ptr;
  memset(&addrs, 0, sizeof(addrs));
  addrs.ai_family = AF_UNSPEC;
  addrs.ai_socktype = SOCK_STREAM;
  addrs.ai_protocol = IPPROTO_TCP;
  addrs.ai_flags = AI_PASSIVE;

  int clientSocket, returnValue, numbrBytes;
  char buf[MAXDATA];
  char userInput[MAXDATA];

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
  printf("Client connected \n");
  

  memset(&buf, 0, sizeof(buf));
  numbrBytes = recv(clientSocket, &buf, MAXDATA, 0);
  if (numbrBytes == -1)
  {
    perror("Wrong with message \n");
    exit(1);
  }
  printf("Server: '%s' \n", buf);

  if (strcmp(buf, "HELLO 1\n") == 0)
  {
    printf("Protocol supported, sending nickname \n");
  }
  else
  {
    perror("Protocol not supported, closing down \n");
    exit(1);
  }  

  sprintf(buf, "NICK %s\n", name);
  numbrBytes = send(clientSocket, buf, sizeof(buf), 0);
  if (numbrBytes == -1)
  {
    perror("Send not gone through \n");
    exit(1);
  }
  printf("Sent name \n");

  memset(&buf, 0, sizeof(buf));
  numbrBytes = recv(clientSocket, &buf, MAXDATA, 0);
  if (numbrBytes == -1)
  {
    perror("Wrong with message \n");
    exit(1);
  }
  printf("Server: '%s' \n", buf);

  if (strcmp(buf, "OK\n") != 0)
  {
    perror("Name not valid \n");
    exit(1);
  }

  FD_ZERO(&readFd);

  while (1)
  {
    FD_SET(STDIN, &readFd);
    FD_SET(clientSocket, &readFd);
    memset(&buf, 0, sizeof(buf));

    numbrBytes = select(clientSocket + 1, &readFd, NULL, NULL, NULL);
    if (numbrBytes == -1)
    {
      printf("Wrong with select \n");
      exit(1);
    }

    if (FD_ISSET(STDIN, &readFd))
    {
      fgets(userInput, MAXDATA, stdin);
      sprintf(buf, "MSG %s", userInput);

      numbrBytes = send(clientSocket, buf, sizeof(buf), 0);
      if (numbrBytes == -1)
      {
        perror("Send not gone through \n");
        exit(1);
      }
    }

    if (FD_ISSET(clientSocket, &readFd))
    {
      numbrBytes = recv(clientSocket, &buf, MAXDATA, 0);
      if (numbrBytes == -1)
      {
        perror("Wrong with message \n");
        exit(1);
      }
      printf("Server: %s \n", buf);
    }
  }

  close(clientSocket);
}