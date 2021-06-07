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

int nrPlayers = 0, nrHighscores = 0;
float highscores[10];
char command[10], watchers[50];
char msgP1[80], msgP2[80], msgWatch[80];
struct timeval serverTimer;

struct games
{
  int player1, player2;
  int answerP1, answerP2;
  int scoreP1, scoreP2;
  int rounds, timeCounter;

  float timeP1, timeP2;
  struct timeval timerForP1, timerForP2;

  bool readyP1, readyP2;
  bool countReady;
};

struct games players[80];

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

void checkWhoWon(int player1, int player2, int index)
{
  int bytes;
  memset(&msgP1, 0, sizeof(msgP1));
  memset(&msgP2, 0, sizeof(msgP2));

  
  if (abs(players[index].answerP1 - players[index].answerP2) == 2)
  {
    if (players[index].answerP1 < players[index].answerP2)
    {
      players[index].scoreP1++;
      sprintf(msgP1, "COUNT You won!\nScore: %d -- %d\n", players[index].scoreP1, players[index].scoreP2);
      sprintf(msgP2, "COUNT You lost\nScore: %d -- %d\n", players[index].scoreP1, players[index].scoreP2);

      sprintf(msgWatch, "WATCH Player 1 won\nScore: %d -- %d\n", players[index].scoreP1, players[index].scoreP2);
    }
    else
    {
      players[index].scoreP2++;
      sprintf(msgP2, "COUNT You won!\nScore: %d -- %d\n", players[index].scoreP1, players[index].scoreP2);
      sprintf(msgP1, "COUNT You lost\nScore: %d -- %d\n", players[index].scoreP1, players[index].scoreP2);

      sprintf(msgWatch, "WATCH Player 2 won\nScore: %d -- %d\n", players[index].scoreP1, players[index].scoreP2);
    }
  }

  if (abs(players[index].answerP1 - players[index].answerP2) == 1)
  {
    if (players[index].answerP1 < players[index].answerP2)
    {
      players[index].scoreP1++;
      sprintf(msgP1, "COUNT You won!\nScore: %d -- %d\n", players[index].scoreP1, players[index].scoreP2);
      sprintf(msgP2, "COUNT You lost\nScore: %d -- %d\n", players[index].scoreP1, players[index].scoreP2);

      sprintf(msgWatch, "WATCH Player 1 won\nScore: %d -- %d\n", players[index].scoreP1, players[index].scoreP2);
    }
    else
    {
      players[index].scoreP2++;
      sprintf(msgP2, "COUNT You won!\nScore: %d -- %d\n", players[index].scoreP1, players[index].scoreP2);
      sprintf(msgP1, "COUNT You lost\nScore: %d -- %d\n", players[index].scoreP1, players[index].scoreP2);

      sprintf(msgWatch, "WATCH Player 2 won\nScore: %d -- %d\n", players[index].scoreP1, players[index].scoreP2);
    }
  }

  bytes = send(players[index].player1, msgP1, strlen(msgP1), 0);
  bytes = send(players[index].player2, msgP2, strlen(msgP2), 0);
  bytes = send(watchers[index], msgWatch, strlen(msgWatch), 0);
}

int counter;

void sortHighscores()
{
  if (nrHighscores <= 1)
  {
    return;
  }

  counter = nrHighscores - 1;

  if (highscores[counter - 1] > highscores[counter])
  {
    float temp = highscores[counter - 1];
    highscores[counter - 1] = highscores[counter];
    highscores[counter] = temp;

    counter--;
    sortHighscores();
  }
  else
  {
    return;
  }
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printf("Usage: %s hostname:port (%d)\n", argv[0], argc);
    exit(1);
  }

  char delim[] = ":";
  char *Desthost = strtok(argv[1],delim);
  char *Destport = strtok(NULL,delim);

  int port = atoi(Destport);
  printf("Host %s, and port %d. \n", Desthost, port);

  /* My magic */

  int serverSocket, returnValue;
  int current = 1;
  int queue = 5;
  serverTimer.tv_sec = 0;
  serverTimer.tv_usec = 0;

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
  char buf[50], input[10];

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
    memset(&buf, 0, sizeof(buf));
    readfd = tempfd;

    struct timeval timer;
    timer.tv_sec = 1;
    timer.tv_usec = 0;

    selectBytes = select(maxfd + 1, &readfd, NULL, NULL, &timer);
    if (selectBytes == -1)
    {
      exit(1);
    }

    //Select timed out
    if (selectBytes == 0)
    {
      for (int i = 0; i < nrPlayers; i++)
      {
        if (players[i].countReady)
        {
          memset(&msgP1, 0, sizeof(msgP1));
          memset(&msgP2, 0, sizeof(msgP2));
          memset(&msgWatch, 0, sizeof(msgWatch));

          //If the game is done
          if (players[i].rounds == 4)
          {
            if (players[i].scoreP1 == 3)
            {
              sprintf(msgP1, "MENU Congratulations you won the game!\nScore: %d -- %d\n", players[i].scoreP1, players[i].scoreP2);
              sprintf(msgP2, "MENU Unfortunately you lost the game\nScore: %d -- %d\n", players[i].scoreP1, players[i].scoreP2);
              sprintf(msgWatch, "MENU Player 1 won the whole game!\nScore: %d -- %d\n", players[i].scoreP1, players[i].scoreP2);

              if (players[i].timeP1 > 0.0000)
              {
                if (nrHighscores != 10)
                {
                  highscores[nrHighscores++] = players[i].timeP1 / 3;
                  sortHighscores();
                }
                else if (highscores[9] > (players[i].timeP1 / 3))
                {
                  highscores[9] = players[i].timeP1 / 3;
                  sortHighscores();
                }
              }

              if (players[i].timeP2 > 0.0000)
              {
                if (nrHighscores != 10)
                {
                  highscores[nrHighscores++] = players[i].timeP2 / 3;
                  sortHighscores();
                }
                else if (highscores[9] > (players[i].timeP2 / 3))
                {
                  highscores[9] = players[i].timeP2 / 3;
                  sortHighscores();
                }
              }
                  
              for (int j = i; j < nrPlayers - 1; j++)
              {
                players[j] = players[j + 1];
                watchers[j] = watchers[j + 1];
              }
              nrPlayers--;
            }
            else if (players[i].scoreP2 == 3)
            {
              sprintf(msgP2, "MENU Congratulations you won the game!\nScore: %d -- %d\n", players[i].scoreP1, players[i].scoreP2);
              sprintf(msgP1, "MENU Unfortunately you lost the game\nScore: %d -- %d\n", players[i].scoreP1, players[i].scoreP2);
              sprintf(msgWatch, "MENU Player 2 won the whole game!\nScore: %d -- %d\n", players[i].scoreP1, players[i].scoreP2);

              if (players[i].timeP1 > 0.0000)
              {
                if (nrHighscores != 10)
                {
                  highscores[nrHighscores++] = players[i].timeP1 / 3;
                  sortHighscores();
                }
                else if (highscores[9] > (players[i].timeP1 / 3))
                {
                  highscores[9] = players[i].timeP1 / 3;
                  sortHighscores();
                }
              }

              if (players[i].timeP2 > 0.0000)
              {
                if (nrHighscores != 10)
                {
                  highscores[nrHighscores++] = players[i].timeP2 / 3;
                  sortHighscores();
                }
                else if (highscores[9] > (players[i].timeP2 / 3))
                {
                  highscores[9] = players[i].timeP2 / 3;
                  sortHighscores();
                }
              }

              for (int j = i; j < nrPlayers - 1; j++)
              {
                players[j] = players[j + 1];
                watchers[j] = watchers[j + 1];
              }
              nrPlayers--;
            }

            if (players[i].scoreP1 != 3 && players[i].scoreP2 != 3)
            {
              players[i].scoreP1 = 0;
              players[i].scoreP2 = 0;

              sprintf(msgP1, "COUNT Match draw, starting over\nScore: %d -- %d\n\n", players[i].scoreP1, players[i].scoreP2);
              sprintf(msgP2, "COUNT Match draw, starting over\nScore: %d -- %d\n\n", players[i].scoreP1, players[i].scoreP2);
              sprintf(msgWatch, "WATCH Match didn't end, starting over\nScore: %d -- %d\n\n", players[i].scoreP1, players[i].scoreP2);

              players[i].timeP1 = 0;
              players[i].timeP2 = 0;
              players[i].rounds = 1;
            }

            send(players[i].player1, msgP1, strlen(msgP1), 0);
            send(players[i].player2, msgP2, strlen(msgP2), 0);
            send(watchers[i], msgWatch, strlen(msgWatch), 0);
          }
          else
          {
            if (players[i].timeCounter == 0)
            {
              sprintf(msgWatch, "WATCH \nRound %d\n", players[i].rounds);
              sprintf(msgP1, "GAME \nRound %d\n", players[i].rounds++);
              bytes = send(players[i].player1, msgP1, strlen(msgP1), 0);
              bytes = send(players[i].player2, msgP1, strlen(msgP1), 0);
              bytes = send(watchers[i], msgWatch, strlen(msgWatch), 0);

              sprintf(msgP2, "GAME \n1. Rock\n2. Paper\n3. Scissor\n");
              bytes = send(players[i].player1, msgP2, strlen(msgP2), 0);
              bytes = send(players[i].player2, msgP2, strlen(msgP2), 0);
              bytes = send(watchers[i], "\nPlayers choose\n1. Rock\n2. Paper\n3. Scissor\n", strlen("\nPlayers choose\n1. Rock\n2. Paper\n3. Scissor\n"), 0);

              players[i].timeCounter = 3;
              players[i].countReady = false;
            }
            else
            {
              sprintf(msgWatch, "Players have %d seconds to game\n", players[i].timeCounter);
              sprintf(msgP1, "Seconds to game: %d\n", players[i].timeCounter--);
              bytes = send(players[i].player1, msgP1, strlen(msgP1), 0);
              bytes = send(players[i].player2, msgP1, strlen(msgP1), 0);
              bytes = send(watchers[i], msgWatch, strlen(msgWatch), 0);
            }
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

            if (FD_ISSET(players[nrPlayers].player1, &readfd))
            {
              clientCount--;
            }

            for (int j = 0; j < nrPlayers; j++)
            {
              if (watchers[j] == i)
              {
                watchers[j] = 0;
              }

              if (players[j].player1 == i)
              {
                memset(&msgP2, 0, sizeof(msgP2));
                memset(&msgWatch, 0, sizeof(msgWatch));
                sprintf(msgP2, "MENU Player 1 disconnected, ending game\n");
                sprintf(msgWatch, "WATCH Player 1 disconnected, ending watch\n");

                bytes = send(players[j].player2, msgP2, strlen(msgP2), 0);
                bytes = send(watchers[j], msgWatch, strlen(msgWatch), 0);

                for (int k = j; k < nrPlayers - 1; k++)
                {
                  players[k] = players[k + 1];
                  watchers[k] = watchers[k + 1];
                }
                nrPlayers--;
              }

              if (players[j].player2 == i)
              {
                memset(&msgP1, 0, sizeof(msgP1));
                memset(&msgWatch, 0, sizeof(msgWatch));
                sprintf(msgP1, "MENU Player 2 disconnected, ending game\n");
                sprintf(msgWatch, "WATCH Player 2 disconnected, ending watch\n");

                bytes = send(players[j].player1, msgP1, strlen(msgP1), 0);
                bytes = send(watchers[j], msgWatch, strlen(msgWatch), 0);

                for (int k = j; k < nrPlayers - 1; k++)
                {
                  players[k] = players[k + 1];
                  watchers[k] = watchers[k + 1];
                }
                nrPlayers--;
              }
            }

            close(i);
            FD_CLR(i, &tempfd);
          }
          else
          {
            sscanf(buf, "%s %s", command, input);

            if (strcmp(command, "COUNT") == 0)
            {
              for (int j = 0; j < nrPlayers; j++)
              {
                if (players[j].player1 == i || players[j].player2 == i)
                {
                  players[j].countReady = true;
                }
              }
            }
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
                players[nrPlayers].readyP1 = false;
                players[nrPlayers].readyP2 = false;
                players[nrPlayers].countReady = false;
                
                players[nrPlayers].answerP1 = 0;
                players[nrPlayers].answerP2 = 0;
                
                players[nrPlayers].scoreP1 = 0;
                players[nrPlayers].scoreP2 = 0;

                players[nrPlayers].timerForP1.tv_sec = 0;
                players[nrPlayers].timerForP1.tv_usec = 0;
                players[nrPlayers].timeP1 = 0.0f;

                players[nrPlayers].timerForP2.tv_sec = 0;
                players[nrPlayers].timerForP2.tv_usec = 0;
                players[nrPlayers].timeP2 = 0.0f;

                players[nrPlayers].rounds = 1;
                players[nrPlayers].timeCounter = 3;

                bytes = send(players[nrPlayers].player1, "START Press 'R' to start!\n", strlen("START Press 'R' to start!\n"), 0);
                bytes = send(players[nrPlayers].player2, "START Press 'R' to start!\n", strlen("START Press 'R' to start!\n"), 0);

                if (bytes == -1)
                {
                  perror("Message not sent\n");
                }

                nrPlayers++;
              }
            }

            //Client chose "Watch"
            if (strcmp(input, "2") == 0)
            {
              memset(&msgWatch, 0, sizeof(msgWatch));

              if (nrPlayers == 0)
              {
                bytes = send(i, "MENU No games active\n", strlen("MENU No games active\n"), 0);
              }
              else
              {
                for (int j = 0; j < nrPlayers; j++)
                {
                  sprintf(msgWatch, "Game %d: Score %d -- %d\n", j + 1, players[j].scoreP1, players[j].scoreP2);
                  bytes = send(i, msgWatch, strlen(msgWatch), 0);
                }

                bytes = send(i, "\n...or 'q' to go back to menu\n", strlen("\n...or 'q' to go back to menu\n"), 0);
              }
            }

            //Client chose "Highscore"
            if (strcmp(input, "3") == 0)
            {
              if (nrHighscores != 0)
              {
                bytes = send(i, "MENU Highscore\n", strlen("MENU Highscore\n"), 0);
                
                for (int j = 0; j < nrHighscores; j++)
                {
                  sprintf(msgP1, "Highscore %d: %f\n", j + 1, highscores[j]);
                  bytes = send(i, msgP1, strlen(msgP1), 0);
                }
              }
              else
              {
                bytes = send(i, "MENU No highscores yet\n", strlen("MENU No highscores yet\n"), 0);
              }
            }
          }

          //Client chose a game to watch
          if (strcmp(command, "CHOICE") == 0)
          {
            if (strcmp(input, "q") == 0)
            {
              bytes = send(i, "MENU", strlen("MENU"), 0);
            }
            else
            {
              for (int j = 0; j < nrPlayers; j++)
              {
                char answer[2];
                sprintf(answer, "%d", j + 1);

                if (strcmp(input, answer) == 0)
                {
                  watchers[j] = i;
                }
              }
            }
          }

          //Watchers want out
          if (strcmp(command, "WATCH") == 0 || strcmp(command, "Players") == 0)
          {
            memset(&msgWatch, 0, sizeof(msgWatch));

            if (nrPlayers == 0)
            {
              sprintf(msgWatch, "MENU No game active\n");
              bytes = send(i, msgWatch, strlen(msgWatch), 0);
            }
            else
            {
              for (int j = 0; j < nrPlayers; j++)
              {
                if (watchers[j] == i)
                {
                  watchers[j] = 0;
                  sprintf(msgWatch, "Game %d: Score %d -- %d\n", j + 1, players[j].scoreP1, players[j].scoreP2);
                  bytes = send(i, msgWatch, strlen(msgWatch), 0);
                }
              }

              bytes = send(i, "\n...or 'q' to go back to menu\n", strlen("\n...or 'q' to go back to menu\n"), 0);
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
              if (players[j].player1 == i)
              {
                players[j].readyP1 = true;
              }

              if (players[j].player2 == i)
              {
                players[j].readyP2 = true;
              }

              if (players[j].readyP1 && players[j].readyP2)
              {
                players[j].readyP1 = false;
                players[j].readyP2 = false;
                players[j].countReady = true;
              }
            }
          }

          //Players answer
          if (strcmp(command, "GAME") == 0)
          {
            gettimeofday(&serverTimer, NULL);

            if (strcmp(input, "1") == 0 || strcmp(input, "2") == 0 || strcmp(input, "3") == 0)
            {
              for (int j = 0; j < nrPlayers; j++)
              {
                if (players[j].player1 == i)
                {
                  players[j].answerP1 = atoi(input);
                  players[j].readyP1 = true;
                  
                  gettimeofday(&players[j].timerForP1, NULL);
                  players[j].timeP1 += players[j].timerForP1.tv_usec - serverTimer.tv_usec;
                }

                if (players[j].player2 == i)
                {
                  players[j].answerP2 = atoi(input);
                  players[j].readyP2 = true;

                  gettimeofday(&players[j].timerForP2, NULL);
                  players[j].timeP2 += players[j].timerForP2.tv_usec - serverTimer.tv_usec;
                }

                if (players[j].readyP1 && players[j].readyP2)
                {
                  checkWhoWon(players[j].player1, players[j].player2, j);
                  players[j].readyP1 = false;
                  players[j].readyP2 = false;

                  players[j].answerP1 = 0;
                  players[j].answerP2 = 0;
                }
              }
            }
          }

          //Players took too long
          if (strcmp(command, "OVER") == 0)
          {
            for (int j = 0; j < nrPlayers; j++)
            {
              memset(&msgP1, 0, sizeof(msgP1));
              memset(&msgP2, 0, sizeof(msgP2));
              memset(&msgWatch, 0, sizeof(msgWatch));

              if (players[j].readyP1 && !players[j].readyP2)
              {
                players[j].scoreP1++;

                sprintf(msgP1, "COUNT You won!\nScore: %d -- %d\n\n", players[j].scoreP1, players[j].scoreP2);
                sprintf(msgP2, "COUNT Too long to answer, you lose\nScore: %d -- %d\n\n", players[j].scoreP1, players[j].scoreP2);
                sprintf(msgWatch, "WATCH Player 1 won, player 2 took too long\nScore: %d -- %d\n\n", players[j].scoreP1, players[j].scoreP2);
              }
              else if (players[j].readyP2 && !players[j].readyP1)
              {
                players[j].scoreP2++;

                sprintf(msgP2, "COUNT You won!\nScore: %d -- %d\n\n", players[j].scoreP1, players[j].scoreP2);
                sprintf(msgP1, "COUNT Too long to answer, you lose\nScore: %d -- %d\n\n", players[j].scoreP1, players[j].scoreP2);
                sprintf(msgWatch, "WATCH Player 2 won, player 1 took too long\nScore: %d -- %d\n\n", players[j].scoreP1, players[j].scoreP2);
              }
              else if (!players[j].readyP1 && !players[j].readyP2)
              {
                if (players[j].player1 == i)
                {
                  sprintf(msgP1, "COUNT No one answered, draw\nScore: %d -- %d\n\n", players[j].scoreP1, players[j].scoreP2);
                  sprintf(msgWatch, "WATCH No one answered, draw\nScore: %d -- %d\n\n", players[j].scoreP1, players[j].scoreP2);
                }
                else if (players[j].player2 == i)
                {
                  sprintf(msgP2, "COUNT No one answered, draw\nScore: %d -- %d\n\n", players[j].scoreP1, players[j].scoreP2);
                }
              }

              bytes = send(players[j].player1, msgP1, strlen(msgP1), 0);
              bytes = send(players[j].player2, msgP2, strlen(msgP2), 0);
              bytes = send(watchers[j], msgWatch, strlen(msgWatch), 0);

              players[j].answerP1 = 0;
              players[j].answerP2 = 0;

              players[j].readyP1 = false;
              players[j].readyP2 = false;
            }
          }
        }
      }
    }
  }

  close(serverSocket);
}