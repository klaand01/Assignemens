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

int score1 = 0, score2 = 0;

void playGame(fd_set bigReadfd, int bytes, int player1, int player2)
{
  int timer = 3, round = 1;
  int players[2] = {player1, player2};
  int nrAnswers = 0, returnValue, selectBytes;
  int answers[2];
  char gameMsg[MAXDATA], gameAnsw[10];

  struct timeval time;
  time.tv_sec = 10;
  time.tv_usec = 0;

  fd_set readfd;
  fd_set tempfd;
  FD_ZERO(&readfd);
  FD_ZERO(&tempfd);
  int maxfd;
  
  FD_SET(player1, &tempfd);
  FD_SET(player2, &tempfd);
  maxfd = player1;
  if (player2 > player1)
  {
    maxfd = player2;
  }

  while (score1 < 3 && score2 < 3)
  {
    timer = 3;
    nrAnswers = 0;

    while (timer > 0)
    {
      memset(&gameMsg, 0, sizeof(gameMsg));
      sprintf(gameMsg, "Game will start in %d seconds\n", timer--);

      for (int i = 0; i < 2; i++)
      {
        if (FD_ISSET(players[i], &bigReadfd))
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
      if (FD_ISSET(players[i], &bigReadfd))
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
      readfd = tempfd;
      memset(&gameAnsw, 0, sizeof(gameAnsw));
      selectBytes = select(maxfd + 1, &readfd, NULL, NULL, &time);
      if (bytes == -1)
      {
        printf("Wrong with select \n");
        exit(1);
      }

      for (int i = 0; i < 2; i++)
      {
        if (FD_ISSET(players[i], &readfd))
        {
          bytes = recv(players[i], &gameAnsw, sizeof(gameAnsw), 0);
          if (bytes == -1)
          {
            perror("Message not recieved \n");
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

        if (selectBytes == 0)
        {
          if (nrAnswers == 0)
          {
            for (int j = 0; j < 2; j++)
            {
              bytes = send(players[j], "No player selected\n", strlen("No player selected\n"), 0);
            }
          }

          if (nrAnswers == 1)
          {
            if (answers[0] != 1 && answers[0] != 2 && answers[0] != 3)
            {
              bytes = send(players[0], "Too slow, automatic loss\n", strlen("Too slow, automatic loss\n"), 0);
            }
            else
            {
              bytes = send(players[1], "Too slow, automatic loss\n", strlen("Too slow, automatic loss\n"), 0);
            }
          }

          nrAnswers = 2;
        }
      }
    }

    checkWhoWon(answers[0], answers[1], bytes, players, nrAnswers);
  }
  printf("3 rounds done!\n");
  exit(1);
}

void checkWhoWon(int player1Answ, int player2Answ, int bytes, int players[], int nrAnswers)
{
  char scores[MAXDATA];
  memset(&scores, 0, sizeof(scores));

  if (player1Answ == player2Answ)
  {
    for (int i = 0; i < 2; i++)
    {
      bytes = send(players[i], "Draw\n", strlen("Draw\n"), 0);
    }
  }

  if (nrAnswers == 1)
  {
    if (player1Answ == 1 || player1Answ == 2 || player1Answ == 3)
    {
      score1++;
      bytes = send(players[0], "You won!\n", strlen("You won!\n"), 0);
      bytes = send(players[1], "You lost\n", strlen("You lost\n"), 0);
    }
    else
    {
      score2++;
      bytes = send(players[0], "You lost\n", strlen("You lost\n"), 0);
      bytes = send(players[1], "You won!\n", strlen("You won!\n"), 0);
    }
  }

  if (abs(player1Answ - player2Answ) == 2)
  {
    if (player1Answ < player2Answ)
    {
      score1++;
      bytes = send(players[0], "You won!\n", strlen("You won!\n"), 0);
      bytes = send(players[1], "You lost\n", strlen("You lost\n"), 0);
    }
    else
    {
      score2++;
      bytes = send(players[0], "You lost\n", strlen("You lost\n"), 0);
      bytes = send(players[1], "You won!\n", strlen("You won!\n"), 0);
    }
  }

  if (abs(player1Answ - player2Answ) == 1)
  {
    if (player1Answ > player2Answ)
    {
      score1++;
      bytes = send(players[0], "You won!\n", strlen("You won!\n"), 0);
      bytes = send(players[1], "You lost\n", strlen("You lost\n"), 0);
    }
    else
    {
      score2++;
      bytes = send(players[0], "You lost\n", strlen("You lost\n"), 0);
      bytes = send(players[1], "You won!\n", strlen("You won!\n"), 0);
    }
  }

  sprintf(scores, "Scores player 1: %d\nScore player 2: %d\n\n", score1, score2);

  for (int i = 0; i < 2; i++)
  {
    bytes = send(players[i], scores, strlen(scores), 0);
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

  fd_set readfd;
  fd_set tempfd;
  FD_ZERO(&readfd);
  FD_ZERO(&tempfd);
  int newSocket, maxfd;
  
  FD_SET(serverSocket, &readfd);
  maxfd = serverSocket;

  while (1)
  {
    memset(&buf, 0, sizeof(buf));
    tempfd = readfd;
    bytes = select(maxfd + 1, &tempfd, NULL, NULL, NULL);
    if (bytes == -1)
    {
      printf("Wrong with select \n");
      exit(1);
    }

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
            FD_CLR(i, &readfd);
          }
          if (bytes == 0)
          {
            printf("Client hung up \n");
            close(i);
            FD_CLR(i, &readfd);
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
                if (FD_ISSET(players[pairCount][j], &readfd))
                {
                  bytes = send(players[pairCount][j], "Game is starting\n", strlen("Game is starting\n"), 0);
                  if (bytes == -1)
                  {
                    perror("Message not sent \n");
                  }
                }
              }

              //Bekräfta spel
              playGame(readfd, bytes, players[pairCount][0], players[pairCount][1]);

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