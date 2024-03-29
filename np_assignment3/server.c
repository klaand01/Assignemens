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
  char *temp, tempArr[256], temperArr[256];
  char arrNames[50][100], type[20];

  fd_set readfd;
  fd_set tempfd;
  FD_ZERO(&readfd);
  FD_ZERO(&tempfd);
  int maxfd;
  
  FD_SET(serverSocket, &readfd);
  maxfd = serverSocket;

  while (1)
  {
    tempfd = readfd;
    sentBytes = select(maxfd + 1, &tempfd, NULL, NULL, NULL);

    for (int i = 0; i <= maxfd; i++)
    {
      if (FD_ISSET(i, &tempfd))
      {
        if (i == serverSocket)
        {
          clientSocket = accept(serverSocket, (struct sockaddr *)&theirAddrs, &theirSize);
          if (clientSocket == -1)
          {
            perror("Client socket not accepted \n");
            continue;
          }

          FD_SET(clientSocket, &readfd);
          if (clientSocket > maxfd)
          {
            maxfd = clientSocket;
          }

          char myAddress[20];
	        const char *myAdd;
          getsockname(clientSocket, (struct sockaddr *)&theirAddrs, &theirSize);
          myAdd = inet_ntop(theirAddrs.sin_family, getAddrs((struct sockaddr *)&theirAddrs), myAddress, sizeof(myAddress));
          printf("New connection from %s:%d \n", myAdd, ntohs(theirAddrs.sin_port));

          sentBytes = send(clientSocket, "Hello 1\n", strlen("Hello 1\n"), 0);
          if (sentBytes == -1)
          {
            perror("Message not sent \n");
            continue;
          }
        }
        else
        {
          memset(&buf, 0, sizeof(buf));
          memset(&type, 0, sizeof(type));

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

            //Checking nickname
            char *expression = "^[A-Za-z0-9_]+$";
            regex_t regex;
            int ret = 0;

            ret = regcomp(&regex, expression, REG_EXTENDED);
            if (ret != 0)
            {
              perror("Could not compile regex.\n");
              close(serverSocket);
              exit(1);
            }

            int matches = 0;
            regmatch_t items;

            if (strlen(arrNames[i]) <= 12)
            {
              ret = regexec(&regex, arrNames[i], matches, &items, 0);

              if (ret == 0)
              {
                printf("Nickname is accepted \n");

                sentBytes = send(i, "OK\n", strlen("OK\n"), 0);
                if (sentBytes == -1)
                {
                  perror("Message not sent \n");
                  close(serverSocket);
                  exit(1);
                }
              }
              else
              {
	              printf("Name not accepted\n");
                sentBytes = send(i, "ERROR Name not accepted\n", strlen("ERROR Name not accepted\n"), 0);
              }
            }
            else
            {
              printf("Name too long\n");
              sentBytes = send(i, "ERROR Name is too long\n", strlen("ERROR Name is too long\n"), 0);
              if (sentBytes == -1)
              {
                perror("Message not sent \n");
                close(serverSocket);
                exit(1);
              }
            }

            regfree(&regex);
          }

          if (strcmp(type, "MSG") == 0)
          {
            temp = strstr(buf, " ");
            strcpy(tempArr, temp);
            strcpy(temperArr, temp);
            temp = strtok(tempArr, "\n");

            while (temp != NULL)
            {
              for (int j = 0; j <= maxfd; j++)
              {
                if (FD_ISSET(j, &readfd))
                {
                  if (j == i)
                  {
                    sprintf(msg, "MSG %s %s\n", arrNames[i], temp);
                    sentBytes = send(i, msg, strlen(msg), 0);
                    if (sentBytes == -1)
                    {
                      perror("Message not sent \n");
                    }
                  }
                  else if (j != serverSocket)
                  {
                    sprintf(msg, "%s:%s\n", arrNames[i], temp);

                    sentBytes = send(j, msg, strlen(msg), 0);
                    if (sentBytes == -1)
                    {
                      perror("Message not sent \n");
                    }
                  }
                }
              }

              temp = strstr(temperArr, "MSG");

              if (temp == NULL)
              {
                break;
              }
              strcpy(tempArr, temp);
              temp = strstr(tempArr, " ");

              if (temp == NULL)
              {
                break;
              }
              strcpy(tempArr, temp);
              strcpy(temperArr, temp);
              temp = strtok(tempArr, "\n");
            }
          }
        }
      }
    }
  }

  close(serverSocket);
}