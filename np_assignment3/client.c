#include <stdio.h>
#include <stdlib.h>
/* You will to add includes here */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>

#define MAXDATA 255
#define STDIN 0

int main(int argc, char *argv[])
{
  
  /* Do magic */
  
  if (argc != 3)
  {
    printf("Usage: %s hostname:port nickname (%d)\n", argv[0], argc);
    exit(1);
  }

  char delim[]=":";
  char *Desthost=strtok(argv[1],delim);
  char *Destport=strtok(NULL,delim);
  int port=atoi(Destport);
  printf("Connected to %s:%s \n", Desthost, Destport);
  char *name = argv[2];

  //Checking nickname
  char *expression="^[A-Za-z_]+$";
  regex_t regex;
  int ret;
  
  ret = regcomp(&regex, expression, REG_EXTENDED);
  if(ret)
  {
    perror("Could not compile regex.\n");
    exit(1);
  }
  
  int matches = 0;
  regmatch_t items;
    
  
  if(strlen(name) < 12)
  {
    ret = regexec(&regex, name, matches, &items, 0);

    if(ret == 0)
    {
	    printf("Nick %s is accepted.\n", name);
    }
    else
    {
	    printf("%s is not accepted.\n", name);
      exit(1);
    }
  }
  else
  {
    printf("%s is too long.\n", name);
  }
  
  regfree(&regex);

  struct addrinfo addrs, *ptr;
  memset(&addrs, 0, sizeof(addrs));
  addrs.ai_family = AF_UNSPEC;
  addrs.ai_socktype = SOCK_STREAM;
  addrs.ai_protocol = IPPROTO_TCP;
  addrs.ai_flags = AI_PASSIVE;

  int clientSocket, returnValue, numbrBytes;
  char buf[MAXDATA];
  char userInput[MAXDATA];

  returnValue = getaddrinfo(argv[1], Destport, &addrs, &ptr);
  if (returnValue != 0)
  {
    perror("Wrong with getaddrinfo \n");
    exit(1);
  }

  clientSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
  if (clientSocket == -1)
  {
    perror("Socket not created \n");
    exit(1);
  }

  returnValue = connect(clientSocket, ptr->ai_addr, ptr->ai_addrlen);
  if (returnValue == -1)
  {
    perror("Client not connected \n");
    close(clientSocket);
  }
  printf("Client connected \n");
  

  memset(&buf, 0, sizeof(buf));
  numbrBytes = recv(clientSocket, &buf, sizeof(buf), 0);
  if (numbrBytes == -1)
  {
    perror("Wrong with message \n");
    close(clientSocket);
  }
  printf("Server: '%s' \n", buf);

  if (strcmp(buf, "HELLO 1\n") == 0)
  {
    printf("Protocol supported, sending nickname \n");
  }
  else
  {
    perror("Protocol not supported, closing down \n");
    close(clientSocket);
  }  

  sprintf(buf, "NICK %s\n", name);
  numbrBytes = send(clientSocket, buf, strlen(buf), 0);
  if (numbrBytes == -1)
  {
    perror("Send not gone through \n");
    close(clientSocket);
  }
  printf("Sent name \n");

  memset(&buf, 0, sizeof(buf));
  numbrBytes = recv(clientSocket, &buf, sizeof(buf), 0);
  if (numbrBytes == -1)
  {
    perror("Wrong with message \n");
    close(clientSocket);
  }
  printf("Server: '%s' \n", buf);

  if (strcmp(buf, "OK\n") != 0)
  {
    perror("Name not valid \n");
    close(clientSocket);
  }

  fd_set readfd;
  fd_set tempfd;
  FD_ZERO(&readfd);
  FD_ZERO(&tempfd);
  FD_SET(STDIN, &tempfd);
  FD_SET(clientSocket, &tempfd);

  while (1)
  {
    readfd = tempfd;
    memset(&buf, 0, sizeof(buf));

    numbrBytes = select(clientSocket + 1, &readfd, NULL, NULL, NULL);
    if (numbrBytes == -1)
    {
      printf("Wrong with select \n");
      exit(1);
    }

    if (FD_ISSET(STDIN, &readfd))
    {
      fgets(userInput, MAXDATA, stdin);
      sprintf(buf, "MSG %s", userInput);

      numbrBytes = send(clientSocket, buf, strlen(buf), 0);
      if (numbrBytes == -1)
      {
        perror("Send not gone through \n");
        exit(1);
      }
    }

    if (FD_ISSET(clientSocket, &readfd))
    {
      numbrBytes = recv(clientSocket, &buf, sizeof(buf), 0);
      if (numbrBytes == -1)
      {
        perror("Wrong with message \n");
        exit(1);
      }
      else if (numbrBytes == 0)
      {
        printf("Server closed down \n");
        close(clientSocket);
        FD_CLR(clientSocket, &readfd);
      }
      else
      {
        printf("%s", buf);
      }
    }
  }

  close(clientSocket);
}