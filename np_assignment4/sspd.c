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

int score1 = 0, score2 = 0, round = 0, totalRounds = 0;

void checkWhoWon(int player1Answ, int player2Answ, int bytes, int players[], int nrAnswers)
{
  bool finished = false;
  char scores[MAXDATA];
  memset(&scores, 0, sizeof(scores));

  if (nrAnswers == 1)
  {
    if (player1Answ == 1 || player1Answ == 2 || player1Answ == 3)
    {
      score1++;
    }
    else
    {
      score2++;
    }
  }

  if (abs(player1Answ - player2Answ) == 2)
  {
    if (player1Answ < player2Answ)
    {
      score1++;
    }
    else
    {
      score2++;
    }
  }

  if (abs(player1Answ - player2Answ) == 1)
  {
    if (player1Answ > player2Answ)
    {
      score1++;
    }
    else
    {
      score2++;
    }
  }

  if (player1Answ == player2Answ)
  {
    sprintf(scores, "RESULT \nDraw, round starts over\n");
    round--;
  }

  if (round == 3 && (score1 == 3 || score2 == 3))
  {
    finished = true;
  }
  
  else if (round == 3 && score1 < 3 && score2 < 3)
  {
    sprintf(scores, "RESULT \nNo one won, game starting over\n");
    round = 0;
    score1 = 0;
    score2 = 0;
  }
  else
  {
    sprintf(scores, "RESULT \nScores player 1: %d\nScore player 2: %d\n\n", score1, score2);
  }

  if (finished)
  {
    if (score1 == 3)
    {
      sprintf(scores, "MENU \nCongratulatios you won!\n");
      bytes = send(players[0], scores, strlen(scores), 0);

      memset(&scores, 0, sizeof(scores));

      sprintf(scores, "MENU \nToo bad you lost!\n");
      bytes = send(players[1], scores, strlen(scores), 0);
    }
    else
    {
      sprintf(scores, "MENU \nCongratulatios you won!\n");
      bytes = send(players[1], scores, strlen(scores), 0);

      memset(&scores, 0, sizeof(scores));
      
      sprintf(scores, "MENU \nToo bad you lost!\n");
      bytes = send(players[0], scores, strlen(scores), 0);
    }

    round = 0;
    score1 = 0;
    score2 = 0;
  }
  else
  {
    for (int i = 0; i < 2; i++)
    {
      bytes = send(players[i], scores, strlen(scores), 0);
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

  int clientSocket, bytes, players[50][2], clientCount = -1, pairCount = -1, gamePlayers[2];
  char buf[MAXDATA], command[20], input[10];

  fd_set readfd;
  fd_set tempfd;
  FD_ZERO(&readfd);
  FD_ZERO(&tempfd);
  int newSocket, maxfd;
  
  FD_SET(serverSocket, &tempfd);
  maxfd = serverSocket;

  int secondsToGame = 3, games = 0;
  int nrAnswers = 0;
  int answers[2];
  char gameMsg[MAXDATA];

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
            //printf("Client sent: %s", buf);
            sscanf(buf, "%s %s", command, input);
          }

          //Menu
          if (strcmp(command, "MENU") == 0)
          {
            //Client chose "Play"
            if (strcmp(input, "1") == 0)
            {
              clientCount++;
              clientCount %= 2;

              if (clientCount == 0)
              {
                pairCount++;
              }
              printf("PairCOUNt %d\n", pairCount);

              players[pairCount][clientCount] = i;

              if (clientCount != 1)
              {
                bytes = send(i, "MSG \nWaiting for opponent...\n", strlen("MSG \nWaiting for opponent...\n"), 0);
                if (bytes == -1)
                {
                  perror("Message not sent \n");
                }
              }
              else
              {
                printf("Starting game\n");
                gamePlayers[0] = players[pairCount][0];
                gamePlayers[1] = players[pairCount][1];
                games++;

                for (int j = 0; j < 2; j++)
                {
                  bytes = send(gamePlayers[j], "START \nPress 'R' to start!\n", strlen("START \nPress 'R' to start!\n"), 0);
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
              bytes = send(i, "MSG Choose the game you want to see:\n", strlen("MSG Choose the game you want to see:\n"), 0);
              if (bytes == -1)
              {
                perror("Message not sent \n");
              }
            }

            if (strcmp(input, "3") == 0)
            {
              sprintf(gameMsg, "SCORES %d\n", totalRounds);
              bytes = send(i, gameMsg, strlen(gameMsg), 0);
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
              secondsToGame = 3;
              struct timeval startTime, compTime;

              while (secondsToGame > -1)
              {
                gettimeofday(&compTime, NULL);

                if ((compTime.tv_sec - startTime.tv_sec) >= 1)
                {
                  memset(&gameMsg, 0, sizeof(gameMsg));
                  sprintf(gameMsg, "GAME \nGame will start in %d seconds\n", secondsToGame);

                  for (int j = 0; j < 2; j++)
                  {
                    bytes = send(gamePlayers[j], gameMsg, strlen(gameMsg), 0);
                    if (bytes == -1)
                    {
                      perror("Message not sent \n");
                      exit(1);
                    }
                  }

                  gettimeofday(&startTime, NULL);
                  secondsToGame--;
                }
              }
              
              memset(&gameMsg, 0, sizeof(gameMsg));
              sprintf(gameMsg, "GAME \n\nRound %d\n1. Rock\n2. Paper\n3. Scissor\n", ++round);
              totalRounds++;

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
            if (((strcmp(input, "1") == 0) || (strcmp(input, "2") == 0) || (strcmp(input, "3") == 0)) )
            {
              nrAnswers++;
              if (nrAnswers == 1)
              {
                for (int j = 0; j < 2; j++)
                {
                  if (gamePlayers[j] == i)
                  {
                    answers[j] = atoi(input);
                  }
                }
              }

              if (nrAnswers == 2)
              {
                for (int j = 0; j < 2; j++)
                {
                  if (gamePlayers[j] == i)
                  {
                    answers[j] = atoi(input);
                  }
                }

                checkWhoWon(answers[0], answers[1], bytes, gamePlayers, nrAnswers);
                nrAnswers = 0;
                answers[0] = 0;
                answers[1] = 0;
              }
            }
          }

          if (strcmp(command, "LOST") == 0)
          {
            for (int j = 0; j < 2; j++)
            {
              if (answers[j] != 1 && answers[j] != 2 && answers[j] != 3)
              {
                bytes = send(gamePlayers[j], "RESULT \nToo late, automatic loss\n", strlen("RESULT \nToo late, automatic loss\n"), 0);
                if (j == 0 && score1 > 0)
                {
                  score1--;
                }
                if (j == 1 && score2 > 0)
                {
                  score2--;
                }
              }
            }

            answers[0] = 0;
            answers[1] = 0;
          }
        }
      }
    }
  }

  close(serverSocket);
}