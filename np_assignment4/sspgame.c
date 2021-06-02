#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>

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

  int clientSocket, returnValue, numbrBytes, selectBytes;
  char buf[MAXDATA], msg[MAXDATA], command[20], *temp;

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

  int timeCounter = 3, round = 1;

  while (1)
  {
    readfd = tempfd;
    struct timeval timer;
    timer.tv_sec = 1;
    timer.tv_usec = 0;

    selectBytes = select(clientSocket + 1, &readfd, NULL, NULL, &timer);
    if (selectBytes == -1)
    {
      exit(1);
    }

    //Select timed out
    if (selectBytes == 0)
    {
      if (strcmp(command, "COUNT") == 0)
      {
        if (round == 4)
        {
          numbrBytes = send(clientSocket, "OVER", strlen("OVER"), 0);
          strcpy(command, "GAME");
          round = 1;
        }
        else
        {
          printf("%d seconds to game\n", timeCounter--);

          if (timeCounter == 0)
          {
            printf("\nRound %d\n", round++);
            printf("\n1. Rock\n2. Paper\n3. Scissor\n");
            strcpy(command, "GAME");
            numbrBytes = send(clientSocket, "GAME", strlen("GAME"), 0);
          }
        }
      }
    }

    //From the server
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
        temp = strchr(buf, ' ');
      }

      //Commands
      if (strcmp(command, "MENU") == 0)
      {
        if (temp != NULL)
        {
          printf("%s\n\n1. Play\n2. Watch\n0. Exit\n", temp);
        }
        else
        {
          printf("\n1. Play\n2. Watch\n0. Exit\n");
        }
      }

      if (strcmp(command, "MSG") == 0 || strcmp(command, "START") == 0)
      {
        printf("%s\n", temp);
      }

      if (strcmp(command, "RESULT") == 0)
      {
        printf("%s\n", temp);
        timeCounter = 3;
        strcpy(command, "COUNT");
      }
    }

    //From input
    if (FD_ISSET(STDIN, &readfd))
    {
      memset(&msg, 0, sizeof(msg));
      memset(&buf, 0, sizeof(buf));
      scanf("%s", buf);

      sprintf(msg, "%s %s\n", command, buf);

      //Exit
      if (strcmp(buf, "0") == 0)
      {
        printf("Exiting\n");
        close(clientSocket);
        FD_CLR(clientSocket, &readfd);
        exit(1);
      }

      numbrBytes = send(clientSocket, msg, strlen(msg), 0);
      if (numbrBytes == -1)
      {
        perror("Wrong with menu send\n");
        exit(1);
      }
    }
  }

  close(clientSocket);
}