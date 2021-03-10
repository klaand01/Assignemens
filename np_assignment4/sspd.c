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
#include <time.h>

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

void menu(int bytes, int clientsocket)
{
  bytes = send(clientsocket, "MENU\n", strlen("MENU\n"), 0);
  if (bytes == -1)
  {
    perror("Message not sent \n");
  }
}

int score1 = 0, score2 = 0, round = 0;

void checkWhoWon(int player1Answ, int player2Answ, int bytes, int players[], int nrAnswers)
{
  char scores[MAXDATA];
  memset(&scores, 0, sizeof(scores));

  if (player1Answ == player2Answ)
  {
    for (int i = 0; i < 2; i++)
    {
      bytes = send(players[i], "RESULT \nDraw, round starts over\n", strlen("RESULT \nDraw, round starts over\n"), 0);
    }
    round--;
  }

  if (nrAnswers == 1)
  {
    if (player1Answ == 1 || player1Answ == 2 || player1Answ == 3)
    {
      score1++;
      bytes = send(players[0], "RESULT \nYou won!\n", strlen("RESULT \nYou won!\n"), 0);
      bytes = send(players[1], "RESULT \nYou lost\n", strlen("RESULT \nYou lost\n"), 0);
    }
    else
    {
      score2++;
      bytes = send(players[0], "RESULT \nYou lost\n", strlen("RESULT \nYou lost\n"), 0);
      bytes = send(players[1], "RESULT \nYou won!\n", strlen("RESULT \nYou won!\n"), 0);
    }
  }

  if (abs(player1Answ - player2Answ) == 2)
  {
    if (player1Answ < player2Answ)
    {
      score1++;
      bytes = send(players[0], "RESULT \nYou won!\n", strlen("RESULT \nYou won!\n"), 0);
      bytes = send(players[1], "RESULT \nYou lost\n", strlen("RESULT \nYou lost\n"), 0);
    }
    else
    {
      score2++;
      bytes = send(players[0], "RESULT \nYou lost\n", strlen("RESULT \nYou lost\n"), 0);
      bytes = send(players[1], "RESULT \nYou won!\n", strlen("RESULT \nYou won!\n"), 0);
    }
  }

  if (abs(player1Answ - player2Answ) == 1)
  {
    if (player1Answ > player2Answ)
    {
      score1++;
      bytes = send(players[0], "RESULT \nYou won!\n", strlen("RESULT \nYou won!\n"), 0);
      bytes = send(players[1], "RESULT \nYou lost\n", strlen("RESULT \nYou lost\n"), 0);
    }
    else
    {
      score2++;
      bytes = send(players[0], "RESULT \nYou lost\n", strlen("RESULT \nYou lost\n"), 0);
      bytes = send(players[1], "RESULT \nYou won!\n", strlen("RESULT \nYou won!\n"), 0);
    }
  }

  sprintf(scores, "RESULT Scores player 1: %d\nScore player 2: %d\n\n", score1, score2);

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

  int clientSocket, bytes, players[50][2], clientCount = -1, pairCount = -1, gamePlayers[2];
  char buf[MAXDATA], command[20], input[10];

  fd_set readfd;
  fd_set tempfd;
  FD_ZERO(&readfd);
  FD_ZERO(&tempfd);
  int newSocket, maxfd;
  
  FD_SET(serverSocket, &tempfd);
  maxfd = serverSocket;

  int timer = 3;
  int nrAnswers = 0;
  int answers[2];
  char gameMsg[MAXDATA];

  clock_t startTime, endTime;

  while (1)
  {
    memset(&buf, 0, sizeof(buf));
    readfd = tempfd;
    bytes = select(maxfd + 1, &readfd, NULL, NULL, NULL);
    if (bytes == -1)
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

          menu(bytes, clientSocket);
        }
        else
        {
          memset(&buf, 0, sizeof(buf));
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
            printf("Client sent: %s", buf);
            sscanf(buf, "%s %s", command, input);
          }

          //Menu
          if (strcmp(command, "MENU") == 0)
          {
            //Client chose "Play"
            if (strcmp(input, "1") == 0)
            {
              clientCount++;
              if (clientCount == 0)
              {
                pairCount++;
              }

              players[pairCount][clientCount] = i;

              if (clientCount != 1)
              {
                bytes = send(i, "MSG Waiting for opponent...\n", strlen("MSG Waiting for opponent...\n"), 0);
                if (bytes == -1)
                {
                  perror("Message not sent \n");
                }
              }
              else
              {
                printf("Starting game\n");

                for (int j = 0; j <= clientCount; j++)
                {
                  gamePlayers[j] = players[pairCount][j];
                  bytes = send(gamePlayers[j], "START Press 'R' to start!\n", strlen("START Press 'R' to start!\n"), 0);
                  if (bytes == -1)
                  {
                    perror("Message not sent \n");
                  }
                }
                
                printf("Game starting!\n");
              }
            }

            //Client chose "Watch"
            if (strcmp(input, "2") == 0)
            {
              printf("Watch game\n");
            }
          }

          //Start of game
          if (strcmp(command, "START") == 0)
          {
            

            if (nrAnswers != 2)
            {
              nrAnswers++;
            }
            
            //Fixa att båda måste trycka Enter, inte bara 1 två gånger
            if (nrAnswers == 2)
            {
              timer = 3;

              while (timer > 0)
              {
                time_t compTime = time(NULL);
                if ((time(NULL) - 1) == compTime)
                {
                  memset(&gameMsg, 0, sizeof(gameMsg));
                  sprintf(gameMsg, "GAME Game will start in %d seconds\n", timer);

                  for (int j = 0; j < 2; j++)
                  {
                    bytes = send(gamePlayers[j], gameMsg, strlen(gameMsg), 0);
                    if (bytes == -1)
                    {
                      perror("Message not sent \n");
                      exit(1);
                    }
                  }

                  timer--;
                }
              }

              memset(&gameMsg, 0, sizeof(gameMsg));
              sprintf(gameMsg, "GAME \nRound %d\n1. Rock\n2. Paper\n3. Scissor\n", ++round);

              for (int j = 0; j < 2; j++)
              {
                bytes = send(gamePlayers[j], gameMsg, strlen(gameMsg), 0);
                if (bytes == -1)
                {
                  perror("Message not sent \n");
                  exit(1);
                }
              }

              nrAnswers = 0;
            }
            
          }

          //Game
          if (strcmp(command, "GAME") == 0)
          {
            if ((strcmp(input, "1") == 0) || (strcmp(input, "2") == 0) || (strcmp(input, "3") == 0))
            {
              nrAnswers++;
              if (nrAnswers == 1)
              {
                answers[0] = atoi(input);
                
              }

              if (nrAnswers == 2)
              {
                answers[1] = atoi(input);
                checkWhoWon(answers[0], answers[1], bytes, gamePlayers, nrAnswers);
                nrAnswers = 0;
                answers[0] = 0;
                answers[1] = 0;

                if (round == 3 && (score1 == 3 || score2 == 3))
                {
                  for (int j = 0; j < 2; j++)
                  {
                    menu(bytes, gamePlayers[j]);
                  }
                }
                else if (round == 3 && score1 != 3 && score2 != 3)
                {
                  for (int j = 0; j < 2; j++)
                  {
                    bytes = send(gamePlayers[j], "START Match draw, game starting over\n", strlen("START Match draw, Game starting over\n"), 0);
                  }

                  round = 0;
                }
              }
            }
          }
        }
      }
    }
  }

  close(serverSocket);
}