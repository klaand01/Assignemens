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
#include <errno.h>

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

void playGame(fd_set readfd, int bytes, int player1, int player2)
{
  int result1 = 0, result2 = 0, timer = 3, round = 1;
  int players[2] = {player1, player2};
  int nrAnswers = 0, returnValue;
  int answers[2];
  char gameMsg[MAXDATA], gameAnsw[10];

  struct timeval time;
  time.tv_sec = 2;
  time.tv_usec = 0;

  for (int i = 0; i < 2; i++)
  {
    returnValue = setsockopt(players[i], SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(time));
    if (returnValue == -1)
    {
      perror("Wrong with SO_RCVTIMEO \n");
      exit(1);
    }  
  }

  while (result1 < 3 || result2 < 3)
  {
    while (timer > 0)
    {
      memset(&gameMsg, 0, sizeof(gameMsg));
      sprintf(gameMsg, "Game will start in %d seconds\n", timer--);

      for (int i = 0; i < 2; i++)
      {
        if (FD_ISSET(players[i], &readfd))
        {
          bytes = send(players[i], gameMsg, strlen(gameMsg), 0);
          if (bytes == -1)
          {
            perror("Message not sent \n");
            exit(1);
          }
        }
      }

      sleep(1);
    }

    memset(&gameMsg, 0, sizeof(gameMsg));
    sprintf(gameMsg, "Round %d\n1. Rock\n2. Paper\n3. Scissor\n", round++);

    for (int i = 0; i < 2; i++)
    {
      if (FD_ISSET(players[i], &readfd))
      {
        bytes = send(players[i], gameMsg, strlen(gameMsg), 0);
        if (bytes == -1)
        {
          perror("Message not sent \n");
          exit(1);
        }
      }
    }

    while (nrAnswers != 2)
    {
      memset(&gameAnsw, 0, sizeof(gameAnsw));
      
      for (int i = 0; i < 2; i++)
      {
        if (FD_ISSET(players[i], &readfd))
        {
          bytes = recv(players[i], &gameAnsw, sizeof(gameAnsw), 0);
          if (bytes == -1)
          {
            if (errno == EAGAIN)
            {
              if (nrAnswers == 0)
              {
                printf("No client answered on time\n");
                for (int j = 0; j < 2; j++)
                {
                  bytes = send(players[j], "You were too slow\n", strlen("You were too slow\n"), 0);
                }
              }
              else if (nrAnswers == 1)
              {
                printf("Only 1 client answered\n");
                bytes = send(players[i], "You were too slow\n", strlen("You were too slow\n"), 0);
              }
              
              nrAnswers++;
            }
            else
            {
              perror("Message not recieved \n");
              close(players[i]);
              FD_CLR(players[i], &readfd);
            }
          }
          if (bytes == 0)
          {
            printf("Client hung up \n");
            close(players[i]);
            FD_CLR(players[i], &readfd);
          }
          else
          {
            answers[i] = atoi(gameAnsw);
            printf("Clint: %d\n", answers[i]);
            nrAnswers++;
          }
        }
      }
    }
  }
}

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

  int clientSocket, bytes, players[50][2], pairCount = 0, clientCount = -1;
  char buf[MAXDATA];

  fd_set readFd;
  fd_set tempFd;
  FD_ZERO(&readFd);
  FD_ZERO(&tempFd);
  int newSocket, maxFd;
  
  FD_SET(serverSocket, &readFd);
  maxFd = serverSocket;

  while (1)
  {
    memset(&buf, 0, sizeof(buf));
    tempFd = readFd;
    bytes = select(maxFd + 1, &tempFd, NULL, NULL, NULL);
    if (bytes == -1)
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
          myAdd = inet_ntop(theirAddrs.sin_family, getAddrs((struct sockaddr *)&theirAddrs), myAddress, sizeof(myAddress));
          printf("New connection from %s:%d \n", myAdd, ntohs(theirAddrs.sin_port));

          bytes = send(clientSocket, "Please select:\n1. Play\n2. Watch\n0. Exit\n", strlen("Please select:\n1. Play\n2. Watch\n0. Exit\n"), 0);
          if (bytes == -1)
          {
            perror("Message not sent \n");
            continue;
          }
        }
        else
        {
          memset(&buf, 0, sizeof(buf));
          bytes = recv(i, &buf, sizeof(buf), 0);
          if (bytes == -1)
          {
            perror("Message not recieved \n");
            close(i);
            FD_CLR(i, &readFd);
          }
          if (bytes == 0)
          {
            printf("Client hung up \n");
            close(i);
            FD_CLR(i, &readFd);
          }
          else
          {
            printf("Client sent: %s", buf);
          }

          //If the players choose "Play"
          if (strcmp(buf, "1\n") == 0)
          {
            clientCount++;
            players[pairCount][clientCount] = i;

            if (clientCount != 1)
            {
              bytes = send(i, "Waiting for opponent...\n", strlen("Waiting for opponent...\n"), 0);
              if (bytes == -1)
              {
                perror("Message not sent \n");
              }
            }
            else
            {
              printf("Creating game\n");
              
              for (int j = 0; j <= clientCount; j++)
              {
                if (FD_ISSET(players[pairCount][j], &readFd))
                {
                  bytes = send(players[pairCount][j], "Game is starting\n", strlen("Game is starting\n"), 0);
                  if (bytes == -1)
                  {
                    perror("Message not sent \n");
                  }
                }
              }

              //Bekräfta spel
              playGame(readFd, bytes, players[pairCount][0], players[pairCount][1]);

              clientCount = -1;
              pairCount++;
            }
          }

          //If the players choose "Watch"
          if (strcmp(buf, "2\n") == 0)
          {
            printf("Watch game\n");
          }

          //If the players choose "Exit"
          if (strcmp(buf, "0\n") == 0)
          {
            bytes = send(clientSocket, "Exit\n", strlen("Exit\n"), 0);
            if (bytes == -1)
            {
              perror("Message not sent \n");
              continue;
            }
          }
        }
      }
    }
  }

  close(serverSocket);
}