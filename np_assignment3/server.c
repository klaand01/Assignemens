#include <stdio.h>
#include <stdlib.h>
/* You will to add includes here */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <regex.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAXDATA 255

void checkNickName(char buf[], int i, int sentBytes, char *name[], char type[])
{
  char *expression = "^[A-Za-z_]+$";
  regex_t regex;
  int ret;

  ret = regcomp(&regex, expression, REG_EXTENDED);
  if (ret != 0)
  {
    perror("Could not compile regex.\n");
    exit(1);
  }

  int matches = 0;
  regmatch_t items;

  ret = regexec(&regex, *name, matches, &items, 0);

  if ((strlen(*name) < 12) && (ret == 0))
  {
    printf("Nickname is accepted \n");

    sentBytes = send(i, "OK\n", sizeof("OK\n"), 0);
    if (sentBytes == -1)
    {
      perror("Message not sent \n");
      exit(1);
    }
  }
  else
  {
    printf("%s is not accepted \n", *name);
    sentBytes = send(i, "ERROR\n", sizeof("ERROR\n"), 0);
    if (sentBytes == -1)
    {
      perror("Message not sent \n");
      exit(1);
    }
  }

  regfree(&regex);
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

  int serverSocket, returnValue;
  int current = 1;
  int queue = 5;

  struct addrinfo addrs, *ptr, *servinfo;
  memset(&addrs, 0, sizeof(addrs));
  addrs.ai_family = AF_UNSPEC;
  addrs.ai_socktype = SOCK_STREAM;
  addrs.ai_protocol = IPPROTO_TCP;
  addrs.ai_flags = AI_PASSIVE;

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

  freeaddrinfo(servinfo);
  if (ptr == NULL)  
  {
		perror("Server: failed to bind \n");
		exit(1);
	}

  returnValue = listen(serverSocket, queue);
  if (returnValue == -1)
  {
    perror("Nothing to connect to \n");
    exit(1);
  }

  struct sockaddr_in theirAddrs;
  socklen_t theirSize = sizeof(theirAddrs);

  int clientSocket, sentBytes, recvBytes;
  char buf[MAXDATA];
  char *name[20];
  char *arrNames;
  char type[20];

  fd_set readFd;
  fd_set tempFd;
  FD_ZERO(&readFd);
  FD_ZERO(&tempFd);
  int newSocket, maxFd;
  
  FD_SET(serverSocket, &readFd);
  maxFd = serverSocket;

  while (1)
  {
    tempFd = readFd;
    sentBytes = select(maxFd + 1, &tempFd, NULL, NULL, NULL);
    if (sentBytes == -1)
    {
      printf("Wrong with select \n");
      exit(1);
    }

    for (int i = 0; i <= maxFd; i++)
    {
      if (FD_ISSET(i, &tempFd))
      {
        if (i == serverSocket)
        {
          clientSocket = accept(serverSocket, (struct sockaddr *)&theirAddrs, &theirSize);
          if (clientSocket == -1)
          {
            perror("Client socket not accepted \n");
            continue;
          }

          FD_SET(clientSocket, &readFd);
          if (clientSocket > maxFd)
          {
            maxFd = clientSocket;
          }

          char myAddress[20];
	        const char *myAdd;
          getsockname(clientSocket, (struct sockaddr *)&theirAddrs, &theirSize);
          myAdd = inet_ntop(theirAddrs.sin_family, &theirAddrs.sin_addr, myAddress, sizeof(myAddress));
          printf("New connection from %s:%d \n", myAdd, ntohs(theirAddrs.sin_port));

          sentBytes = send(clientSocket, "HELLO 1\n", sizeof("HELLO 1\n"), 0);
          if (sentBytes == -1)
          {
            perror("Message not sent \n");
            continue;
          }
        }
        else
        {
          recvBytes = recv(i, &buf, sizeof(buf), 0);
          if (recvBytes == -1)
          {
            perror("Message not recieved \n");
            close(i);
            FD_CLR(i, &readFd);
          }
          if (recvBytes == 0)
          {
            printf("Client hung up \n");
            close(i);
            FD_CLR(i, &readFd);
          }
          else
          {
            sscanf(buf, "%s", type);
          }

          if (strcmp(type, "NICK") == 0)
          {
            sscanf(buf, "%s %s", type, *name);
            //arrNames = strdup(*name);
            //printf("Name recv: %s\n", arrNames);

            checkNickName(buf, i, sentBytes, name, type);
          }

          if (strcmp(type, "MSG") == 0)
          {
            for (int j = 0; j <= maxFd; j++)
            {
              if (FD_ISSET(j, &readFd))
              {
                if (j != serverSocket && j != i)
                {
                  sentBytes = send(j, buf, sizeof(buf), 0);
                  if (sentBytes == -1)
                  {
                    perror("Message not sent \n");
                    continue;
                  }
                }
              }
            }
          }

          memset(&type, 0, sizeof(type));
        }
      }
    }
  }

  close(serverSocket);
}