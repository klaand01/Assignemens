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

int totalRounds = 0, nrGames = 0, nrWatches = 0;
int playerCounter = 0, counter = -1;
int answers[2], players[2], playersWatch[50];;
bool watches = false;

struct games
{
  struct timeval startTime;
  int player1;
  int player2;
  int score1;
  int score2;
  int secondsToGame;
  int roundNr;
  int index;
  int answer1;
  int answer2;
  int ready;
  int timeReady;
};

struct games activePlayers[100];


void checkWhoWon(int player1Answ, int player2Answ, int bytes, int player1, int player2, int indexPlayer, bool both)
{
  bool finished = false;
  int players[2] = {player1, player2};
  char scores[MAXDATA];
  int index = indexPlayer;
  memset(&scores, 0, sizeof(scores));

  if (both == true)
  {
    sprintf(scores, "RESULT No player selected\n");
  }

  else if (player1Answ == 0)
  {
    activePlayers[index].score2++;
  }
  
  else if (player2Answ == 0)
  {
    activePlayers[index].score1++;
  }

  else if (abs(player1Answ - player2Answ) == 2)
  {
    if (player1Answ < player2Answ)
    {
      activePlayers[index].score1++;
    }
    else
    {
      activePlayers[index].score2++;
    }
  }

  else if (abs(player1Answ - player2Answ) == 1)
  {
    if (player1Answ < player2Answ)
    {
      activePlayers[index].score1++;
    }
    else
    {
      activePlayers[index].score2++;
    }
  }

  else if (player1Answ == player2Answ)
  {
    if (activePlayers[index].roundNr > 0)
    {
      activePlayers[index].roundNr--;
    }
  }

  if (activePlayers[index].roundNr >= 3 && (activePlayers[index].score1 == 3 || activePlayers[index].score2 == 3))
  {
    finished = true;
  }

  if (activePlayers[index].roundNr >= 3 && activePlayers[index].score1 < 3 && activePlayers[index].score2 < 3)
  {
    sprintf(scores, "RESULT \nNo one won, game starting over\n");
    activePlayers[index].roundNr = 0;
    activePlayers[index].score1 = 0;
    activePlayers[index].score2 = 0;
  }
  else
  {
    sprintf(scores, "RESULT \nScores player 1: %d\nScore player 2: %d\n\n", activePlayers[index].score1, activePlayers[index].score2);
  }
  
  if (finished)
  {
    if (activePlayers[index].score1 == 3)
    {
      sprintf(scores, "MENU \nCongratulations you won!\n");
      bytes = send(players[0], scores, strlen(scores), 0);

      memset(&scores, 0, sizeof(scores));

      sprintf(scores, "MENU \nToo bad you lost!\n");
      bytes = send(players[1], scores, strlen(scores), 0);
    }
    else
    {
      sprintf(scores, "MENU \nCongratulations you won!\n");
      bytes = send(players[1], scores, strlen(scores), 0);

      memset(&scores, 0, sizeof(scores));
    
      sprintf(scores, "MENU \nToo bad you lost!\n");
      bytes = send(players[0], scores, strlen(scores), 0);
    }

    if (watches = true)
    {
      bytes = send(playersWatch[counter], scores, strlen(scores), 0);
    }

    activePlayers[index].score1 = 0;
    activePlayers[index].score2 = 0;
    activePlayers[index].roundNr = 0;
    nrGames--;
    watches = false;
  }
  else if (both == true)
  {
    bytes = send(activePlayers[index].player1, scores, strlen(scores), 0);
    bytes = send(activePlayers[index].player2, scores, strlen(scores), 0);

    if (watches = true)
    {
      bytes = send(playersWatch[counter], scores, strlen(scores), 0);
    }

    activePlayers[index].timeReady = 2;
  }
  else
  {
    bytes = send(activePlayers[index].player1, scores, strlen(scores), 0);
    bytes = send(activePlayers[index].player2, scores, strlen(scores), 0);

    if (watches = true)
    {
      bytes = send(playersWatch[counter], scores, strlen(scores), 0);
    }
  
    activePlayers[index].timeReady = 2;
  }

  activePlayers[index].answer1 = 0;
  activePlayers[index].answer1 = 0;
}

int countDown(int index, int bytes)
{
  char gameMsg[MAXDATA];
  memset(&gameMsg, 0, sizeof(gameMsg));

  if (activePlayers[index].secondsToGame > 0)
  {
    sprintf(gameMsg, "GAME \nGame will start in %d seconds", activePlayers[index].secondsToGame);
    bytes = send(activePlayers[index].player1, gameMsg, sizeof(gameMsg), 0);
    bytes = send(activePlayers[index].player2, gameMsg, sizeof(gameMsg), 0);

    if (watches = true)
    {
      bytes = send(playersWatch[counter], gameMsg, strlen(gameMsg), 0);
    }
    
    activePlayers[index].secondsToGame--;
    gettimeofday(&activePlayers[index].startTime, NULL);
  }

  if (activePlayers[index].secondsToGame == 0)
  {
    memset(&gameMsg, 0, sizeof(gameMsg));
    sprintf(gameMsg, "GAME \n\nRound %d\n1. Rock\n2. Paper\n3. Scissor\n", activePlayers[index].roundNr);
    totalRounds++;

    bytes = send(activePlayers[index].player1, gameMsg, sizeof(gameMsg), 0);
    bytes = send(activePlayers[index].player2, gameMsg, sizeof(gameMsg), 0);
    if (bytes == -1)
    {
      perror("Message not sent \n");
      exit(1);
    }

    if (watches = true)
    {
      bytes = send(playersWatch[counter], gameMsg, strlen(gameMsg), 0);
    }
    
    activePlayers[index].roundNr++;
    activePlayers[index].timeReady = 0;
    activePlayers[index].secondsToGame = 3;
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
  char gameMsg[MAXDATA], watchMsg[MAXDATA];
  bool both = false;

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
      printf("Wrong with select \n");
      exit(1);
    }
    if (selectBytes == 0)
    {
      for (int i = 0; i < playerCounter; i++)
      {
        if (activePlayers[i].timeReady == 2)
        {
          struct timeval compTime;
          gettimeofday(&compTime, NULL);

          if ((compTime.tv_sec - activePlayers[i].startTime.tv_sec) >= 1)
          {
            countDown(i, bytes);
          }
        }
      }
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

              if (clientCount != 1)
              {
                bytes = send(i, "MSG \nWaiting for opponent...\n", strlen("MSG \nWaiting for opponent...\n"), 0);
                activePlayers[playerCounter].player1 = i;

                if (bytes == -1)
                {
                  perror("Message not sent \n");
                }
              }
              else
              {
                printf("Starting game\n");
                activePlayers[playerCounter].player2 = i;
                nrGames++;

                bytes = send(activePlayers[playerCounter].player1, "START \nPress 'R' to start!\n", strlen("START \nPress 'R' to start!\n"), 0);
                bytes = send(activePlayers[playerCounter].player2, "START \nPress 'R' to start!\n", strlen("START \nPress 'R' to start!\n"), 0);
                if (bytes == -1)
                {
                  perror("Message not sent \n");
                }
                
                activePlayers[playerCounter].index = playerCounter;
                activePlayers[playerCounter].score1 = 0;
                activePlayers[playerCounter].score2 = 0;
                activePlayers[playerCounter].roundNr = 1;
                activePlayers[playerCounter].ready = 0;
                activePlayers[playerCounter].timeReady = 0;
                activePlayers[playerCounter].secondsToGame = 3;
                memset(&playersWatch, 0, sizeof(playersWatch));
                playerCounter++;
                printf("Game starting!\n");
              }
            }

            //Client chose "Watch"
            if (strcmp(input, "2") == 0)
            {
              sprintf(watchMsg, "WATCH Game %d\nPress 'Q' to go back\n", nrGames);
              bytes = send(i, watchMsg, strlen(watchMsg), 0);
              counter++;

              if (bytes == -1)
              {
                perror("Message not sent \n");
              }
            }

            //Client chose "Highscore"
            if (strcmp(input, "3") == 0)
            {
              sprintf(gameMsg, "SCORES %d\n", totalRounds);
              bytes = send(i, gameMsg, strlen(gameMsg), 0);
            }
          }

          //Watching
          if (strcmp(command, "WATCH"))
          {
            if ((strcmp(input, "1") == 0))
            {
              watches = true;
              playersWatch[counter] = i;
              nrWatches++;
            }
          }

          //Start of game
          if (strcmp(command, "START") == 0)
          {
            int indexPlayer;
            for (int j = 0; j < playerCounter; j++)
            {
              if (activePlayers[j].player1 == i)
              {
                indexPlayer = j;
              }

              if (activePlayers[j].player2 == i)
              {
                indexPlayer = j;
              }
            }

            if (activePlayers[indexPlayer].timeReady < 2)
            {
              activePlayers[indexPlayer].timeReady++;
            }

            if (activePlayers[indexPlayer].timeReady == 2)
            {
              struct timeval compTime;
              gettimeofday(&compTime, NULL);

              for (int j = 0; j < playerCounter; j++)
              {
                if ((compTime.tv_sec - activePlayers[j].startTime.tv_sec) >= 1)
                {
                  countDown(j, bytes);
                }
              }
            }
          }

          //Game
          if (strcmp(command, "GAME") == 0)
          {
            int indexPlayer;
            if (((strcmp(input, "1") == 0) || (strcmp(input, "2") == 0) || (strcmp(input, "3") == 0)) )
            {
              for (int j = 0; j < playerCounter; j++)
              {
                if (activePlayers[j].player1 == i)
                {
                  indexPlayer = j;
                  players[0] = activePlayers[j].player1;
                  activePlayers[j].answer1 = atoi(input);
                  answers[0] = activePlayers[j].answer1;
                }

                if (activePlayers[j].player2 == i)
                {
                  indexPlayer = j;
                  players[1] = activePlayers[j].player2;
                  activePlayers[j].answer2 = atoi(input);
                  answers[1] = activePlayers[j].answer2;
                }
              }

              if (activePlayers[indexPlayer].ready < 2)
              {
                activePlayers[indexPlayer].ready++;
              }

              if (activePlayers[indexPlayer].ready == 2)
              {
                checkWhoWon(answers[0], answers[1], bytes, players[0], players[1], indexPlayer, both);
                activePlayers[indexPlayer].ready = 0;
              }
            }
          }

          //Client timed out
          if (strcmp(command, "LOST") == 0)
          {
            int indexPlayer;
            for (int j = 0; j < playerCounter; j++)
            {
              if (activePlayers[j].player1 == i)
              {
                indexPlayer = j;
                answers[0] = 0;
                players[0] = activePlayers[j].player1;
              }

              if (activePlayers[j].player2 == i)
              {
                indexPlayer = j;
                answers[1] = 0;
                players[1] = activePlayers[j].player2;
              }
            }

            if (answers[0] == 0 && answers[1] == 0)
            {
              both = true;
            }

            checkWhoWon(answers[0], answers[1], bytes, players[0], players[1], indexPlayer, both);
            both = false;
          }
        }
      }
    }
  }

  close(serverSocket);
}