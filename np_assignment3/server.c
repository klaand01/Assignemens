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

void* getAddrs(struct sockaddr* addr)
{
  if (addr->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in*)addr)->sin_addr);
  }
  else
  {
    return &((struct sockaddr_in6*)addr)->sin6_addr;
  }
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
  char buf[MAXDATA], msg[MAXDATA];
  char *temp;
  char arrNames[50][100], type[20];

  fd_set readfd;
  fd_set tempfd;
  FD_ZERO(&readfd);
  FD_ZERO(&tempfd);
  int newSocket, maxfd;
  
  FD_SET(serverSocket, &tempfd);
  maxfd = serverSocket;

  while (1)
  {
    readfd = tempfd;
    sentBytes = select(maxfd + 1, &readfd, NULL, NULL, NULL);
    if (sentBytes == -1)
    {
      printf("Wrong with select \n");
      exit(1);
    }

    for (int i = 0; i <= maxfd; i++)
    {
      if (FD_ISSET(i, &readfd))
      {
        if (i == serverSocket)
        {
          clientSocket = accept(serverSocket, (struct sockaddr *)&theirAddrs, &theirSize);
          if (clientSocket == -1)
          {
            perror("Client socket not accepted \n");
            continue;
          }

          FD_SET(clientSocket, &tempfd);
          if (clientSocket > maxfd)
          {
            maxfd = clientSocket;
          }

          char myAddress[20];
	        const char *myAdd;
          getsockname(clientSocket, (struct sockaddr *)&theirAddrs, &theirSize);
          myAdd = inet_ntop(theirAddrs.sin_family, getAddrs((struct sockaddr *)&theirAddrs), myAddress, sizeof(myAddress));
          printf("New connection from %s:%d \n", myAdd, ntohs(theirAddrs.sin_port));

          sentBytes = send(clientSocket, "HELLO 1\n", strlen("HELLO 1\n"), 0);
          if (sentBytes == -1)
          {
            perror("Message not sent \n");
            continue;
          }
        }
        else
        {
          memset(&buf, 0, sizeof(buf));
          recvBytes = recv(i, &buf, sizeof(buf), 0);
          if (recvBytes == -1)
          {
            perror("Message not recieved \n");
            close(i);
            FD_CLR(i, &readfd);
          }
          if (recvBytes == 0)
          {
            printf("Client hung up \n");
            close(i);
            FD_CLR(i, &readfd);
          }
          else
          {
            sscanf(buf, "%s", type);
          }

          if (strcmp(type, "NICK") == 0)
          {
            sscanf(buf, "%s %s", type, arrNames[i]);

            //Checking nickname, to have this in a function didn't work
            char *expression = "^[A-Za-z0-9_]+$";
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

            ret = regexec(&regex, arrNames[i], matches, &items, 0);

            if ((strlen(arrNames[i]) < 12) && (ret == 0))
            {
              printf("Nickname is accepted \n");

              sentBytes = send(i, "OK\n", strlen("OK\n"), 0);
              if (sentBytes == -1)
              {
                perror("Message not sent \n");
                close(serverSocket);
              }
            }
            else
            {
              printf("%s is not accepted \n", arrNames[i]);
              sentBytes = send(i, "ERROR\n", strlen("ERROR\n"), 0);
              if (sentBytes == -1)
              {
                perror("Message not sent \n");
                close(serverSocket);
              }
            }

            regfree(&regex);
          }

          if (strcmp(type, "MSG") == 0)
          {
            temp = strchr(buf, ' ');
            sprintf(msg, "%s: %s", arrNames[i], temp);

            for (int j = 0; j <= maxfd; j++)
            {
              if (FD_ISSET(j, &readfd))
              {
                if (j != serverSocket)
                {
                  sentBytes = send(j, msg, strlen(msg), 0);
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