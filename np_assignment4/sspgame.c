#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXDATA 1000
#define STDIN 0

int main(int argc, char *argv[])
{
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

  struct addrinfo addrs, *ptr;
  memset(&addrs, 0, sizeof(addrs));
  addrs.ai_family = AF_UNSPEC;
  addrs.ai_socktype = SOCK_STREAM;
  addrs.ai_protocol = IPPROTO_TCP;
  addrs.ai_flags = AI_PASSIVE;

  int clientSocket, returnValue, numbrBytes;
  char buf[MAXDATA], msg[MAXDATA], command[20];

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
    close(clientSocket);
  }
  printf("Client connected \n");

  fd_set readfd;
  fd_set tempfd;
  FD_ZERO(&readfd);
  FD_ZERO(&tempfd);
  FD_SET(STDIN, &tempfd);
  FD_SET(clientSocket, &tempfd);

  while(1)
  {
    readfd = tempfd;

    numbrBytes = select(clientSocket + 1, &readfd, NULL, NULL, NULL);
    if (numbrBytes == -1)
    {
      exit(1);
    }

    if (FD_ISSET(STDIN, &readfd))
    {
      memset(&buf, 0, sizeof(buf));
      memset(&msg, 0, sizeof(msg));
      scanf("%s", buf);
      printf("Command %s buf %s\n", command, buf);

      if (strcmp(buf, "0\n") == 0)
      {
        close(clientSocket);
        FD_CLR(clientSocket, &readfd);
      }

      if (strcmp(command, "MENU") == 0)
      {
        sprintf(msg, "MENU %s\n", buf);
      }

      if (strcmp(command, "GAME") == 0)
      {
        sprintf(msg, "GAME %s\n", buf);
      }

      numbrBytes = send(clientSocket, msg, strlen(msg), 0);
      if (numbrBytes == -1)
      {
        perror("Wrong with menu send\n");
      }
    }

    if (FD_ISSET(clientSocket, &readfd))
    {
      memset(&buf, 0, sizeof(buf));
      numbrBytes = recv(clientSocket, &buf, sizeof(buf), 0);
      if (numbrBytes == -1)
      {
        perror("Wrong with message \n");
        exit(1);
      }
      else if (numbrBytes == 0)
      {
        printf("Server closed down \n");
        close(clientSocket);
        FD_CLR(clientSocket, &readfd);
      }
      else
      {
        sscanf(buf, "%s", command);
        printf("REAL command %s\n", command);
      }

      if (strcmp(command, "MSG") == 0)
      {
        printf("%s\n", buf);
      }

      if (strcmp(command, "MENU") == 0)
      {
        printf("1. Play\n2. Watch\n0. Exit\n");
      }

      if (strcmp(command, "GAME") == 0)
      {
        printf("%s\n", buf);
      }
    }

    //fflush(stdin);
  }

  close(clientSocket);
}