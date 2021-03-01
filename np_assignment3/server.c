#include <stdio.h>
#include <stdlib.h>
/* You will to add includes here */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <regex.h>
#include <string.h>
#include <unistd.h>



#define MAXDATA 255

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

  struct sockaddr theirAddrs;
  socklen_t theirSize = sizeof(theirAddrs);

  int clientSocket, bytes;
  char buf[MAXDATA];
  char nick[20], name[20];

  char *expression = "^[A-Za-z_]+$";
  regex_t regex;
  int ret;

  while (1)
  {
    clientSocket = accept(serverSocket, &theirAddrs, &theirSize);
    if (clientSocket == -1)
    {
      perror("Client socket not accepted \n");
      continue;
    }
    
    bytes = send(clientSocket, "HELLO 1\n", sizeof("HELLO 1\n"), 0);
    if (bytes == -1)
    {
      perror("Message not sent \n");
      continue;
    }

    bytes = recv(clientSocket, &buf, sizeof(buf), 0);
    if (bytes == -1)
    {
      perror("Message not recieved \n");
      exit(1);
    }
    printf("Receieved from client: '%s'\n", buf);


    //Check nickname
    sscanf(buf, "%s %s", nick, name);
    printf("Nick: %s Name: %s \n", nick, name);

    if (strcmp(nick, "NICK") == 0)
    {
      ret = regcomp(&regex, expression, REG_EXTENDED);
      if (ret != 0)
      {
        perror("Could not compile regex.\n");
        continue;
      }

      int matches;
      regmatch_t items;
      printf("Testing nickname \n");

      ret = regexec(&regex, name, matches, &items, 0);
      if ((strlen(name) < 12) && (ret == 0))
      {
        printf("Nick %s is accepted \n", name);
        bytes = send(clientSocket, "OK\n", sizeof("OK\n"), 0);
        if (bytes == -1)
        {
          perror("Message not sent \n");
          continue;
        }
      }
      else
      {
        printf("%s is not accepted \n", name);
        bytes = send(clientSocket, "ERROR\n", sizeof("ERROR\n"), 0);
        if (bytes == -1)
        {
          perror("Message not sent \n");
          continue;
        }
      }
      regfree(&regex);
    }








    while (1)
    {

    }
  }

  //close(serverSocket);
}