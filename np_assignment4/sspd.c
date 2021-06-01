#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <string.h>
#include <stdbool.h>

#define MAXDATA 1000

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

int nrPlayers = 0;

struct games
{
  int player1;
  int player2;
  int ready;
};

struct games players[100];

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

  int clientSocket, bytes, clientCount = -1;
  char buf[MAXDATA], command[20], input[10];

  fd_set readfd;
  fd_set tempfd;
  FD_ZERO(&readfd);
  FD_ZERO(&tempfd);
  int newSocket, maxfd;
  
  FD_SET(serverSocket, &tempfd);
  maxfd = serverSocket;

  int selectBytes;

  while (1)
  {
    struct timeval timer;
    timer.tv_sec = 1;
    timer.tv_usec = 0;

    memset(&buf, 0, sizeof(buf));
    readfd = tempfd;

    selectBytes = select(maxfd + 1, &readfd, NULL, NULL, &timer);
    if (selectBytes == -1)
    {
      exit(1);
    }
    
    //Select timed out

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

          bytes = send(clientSocket, "MENU\n", strlen("MENU\n"), 0);
          if (bytes == -1)
          {
            perror("Message not sent \n");
          }
        }
        else
        {
          memset(&buf, 0, sizeof(buf));
          memset(&command, 0, sizeof(command));
          memset(&input, 0, sizeof(input));

          bytes = recv(i, &buf, sizeof(buf), 0);
          if (bytes == -1)
          {
            perror("Message not recieved \n");
            close(i);
            FD_CLR(i, &tempfd);
          }
          if (bytes == 0)
          {
            printf("Client hung up \n");
            close(i);
            FD_CLR(i, &tempfd);
          }
          else
          {
            sscanf(buf, "%s %s", command, input);
          }

          if (strcmp(command, "MENU") == 0)
          {
            //Client chose "Play"
            if (strcmp(input, "1") == 0)
            {
              clientCount++;
              clientCount %= 2;

              if (clientCount != 1)
              {
                bytes = send(i, "MSG Waiting for opponent...\nPress 'Q' to go back\n", strlen("MSG Waiting for opponent...\nPress 'Q' to go back\n"), 0);
                players[nrPlayers].player1 = i;

                if (bytes == -1)
                {
                  perror("Message not sent\n");
                }
              }
              else
              {
                printf("Starting game\n");
                players[nrPlayers].player2 = i;
                players[nrPlayers].ready = 0;

                bytes = send(players[nrPlayers].player1, "START Press 'R' to start!\n", strlen("START Press 'R' to start!\n"), 0);
                bytes = send(players[nrPlayers].player2, "START Press 'R' to start!\n", strlen("START Press 'R' to start!\n"), 0);

                if (bytes == -1)
                {
                  perror("Message not sent\n");
                }

                nrPlayers++;
              }
            }
          }

          //Client wants to leave queue
          if (strcmp(command, "MSG") == 0)
          {
            bytes = send(i, "MENU\n", strlen("MENU\n"), 0);
            if (bytes == -1)
            {
              perror("Message not sent \n");
            }

            clientCount--;
          }

          //Players are ready
          if (strcmp(command, "START") == 0)
          {
            for (int j = 0; j < nrPlayers; j++)
            {
              if (players[j].player1 == i || players[j].player2 == i)
              {
                players[j].ready++;

                if (players[j].ready == 2)
                {
                  send(players[j].player1, "COUNT\n", strlen("COUNT\n"), 0);
                  send(players[j].player2, "COUNT\n", strlen("COUNT\n"), 0);
                }

                break;
              }
            }
          }
        }
      }
    }
  }

  close(serverSocket);
}