#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* You will to add includes here */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>

#define MAXDATA 10000

// Included to get the support library
#include <calcLib.h>

// Enable if you want debugging to be printed, see examble below.
// Alternative, pass 
#define DEBUG
using namespace std;

int main(int argc, char *argv[])
{
   /*
    Read first input, assumes <ip>:<port> syntax, convert into one string (Desthost) and one integer (port). 
     Atm, works only on dotted notation, i.e. IPv4 and DNS. IPv6 does not work if its using ':'. 
  */

  if (argc != 2)
  {
    printf("Usage: %s hostname:port (%d)\n", argv[0], argc);
  }

  char delim[]=":";
  char *Desthost=strtok(argv[1],delim);
  char *Destport=strtok(NULL,delim);
  // *Desthost now points to a string holding whatever came before the delimiter, ':'
  // *Destport points to whatever string cam after the delimiter.

  /* Do magic */
  int port=atoi(Destport);
  #ifdef DEBUG
  printf("Host %s, and port %d. \n", Desthost, port);
  #endif

  int serverSocket, returnValue;
  int current = 1;
  int queue = 5;

  int iNumb1, iNumb2, iRes, iAnswer, iDiff;
  double dNumb1, dNumb2, dRes, dAnswer, dDiff;

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

    struct timeval time;
    time.tv_sec = 5;
    time.tv_usec = 0;

    returnValue = setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(time));
    if (returnValue == -1)
    {
      perror("Wrong with SO_RCVTIMEO \n");
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

  while (1)
  {
    clientSocket = accept(serverSocket, &theirAddrs, &theirSize);
    if (clientSocket == -1)
    {
      perror("Client socket not accepted \n");
      exit(1);
    }

    printf("Client connected \n");

    initCalcLib();
    char *oper = randomType();
    dNumb1 = randomFloat();
    dNumb2 = randomFloat();
    iNumb1 = randomInt();
    iNumb2 = randomInt();

    if (oper[0] == 'f')
    {
      if (strcmp(oper, "fadd") == 0)
      {
        dRes = dNumb1 + dNumb2;
      }

      if (strcmp(oper, "fdiv") == 0)
      {
        dRes = dNumb1 / dNumb2;
      }

      if (strcmp(oper, "fmul") == 0)
      {
        dRes = dNumb1 * dNumb2;
      }

      if (strcmp(oper, "fsub") == 0)
      {
        dRes = dNumb1 - dNumb2;
      }
    }
    else
    {
      if (strcmp(oper, "add") == 0)
      {
        iRes = iNumb1 + iNumb2;
      }

      if (strcmp(oper, "div") == 0)
      {
        iRes = iNumb1 / iNumb2;
      }

      if (strcmp(oper, "mul") == 0)
      {
        iRes = iNumb1 * iNumb2;
      }

      if (strcmp(oper, "sub") == 0)
      {
        iRes = iNumb1 - iNumb2;
      }
    }

    memset(&buf, 0, MAXDATA);

    bytes = send(clientSocket, "TEXT TCP 1.0\n\n", sizeof("TEXT TCP 1.0\n\n"), 0);
    printf("Sent TEXT TCP 1.0 \n");
    if (bytes == -1)
    {
      perror("Message not sent \n");
    }

    bytes = recv(clientSocket, &buf, sizeof(buf), 0);
    if (bytes == -1)
    {
      if (errno == EAGAIN)
      {
        printf("Message not recevied on time \n");
        bytes = send(clientSocket, "ERROR TO\n", sizeof("ERROR TO\n"), 0);
        exit(1);
      }
    }
    else
    {
      printf("Recieved from client: '%s' \n", buf);
    }

    if (strcmp(buf, "OK\n") != 0)
    {
      perror("Closing down \n");
      exit(1);
    }

    //Räkneoperationer
    memset(&buf, 0, MAXDATA);

    if (oper[0] == 'f')
    {
      sprintf(buf, "%s %8.8g %8.8g\n", oper, dNumb1, dNumb2);
    }
    else
    {
      sprintf(buf, "%s %d %d\n", oper, iNumb1, iNumb2);
    }

    bytes = send(clientSocket, buf, sizeof(buf), 0);
    if (bytes == -1)
    {
      perror("Assignment not sent \n");
    }
    else
    {
      printf("Assignment sent \n");
    }

    bytes = recv(clientSocket, &buf, sizeof(buf), 0);
    if (bytes == -1)
    {
      if (errno == EAGAIN)
      {
        printf("Message not recevied on time \n");
        bytes = send(clientSocket, "ERROR TO\n", sizeof("ERROR TO\n"), 0);
        exit(1);
      }
    }
    else
    {
      printf("Recieved from client: '%s' \n", buf);
    }

    if (oper[0] == 'f')
    {
      dAnswer = atoi(buf);
      dDiff = dRes - dAnswer;

      if (dDiff < 0.0001)
      {
        bytes = send(clientSocket, "OK\n", sizeof("OK\n"), 0);
        if (bytes == -1)
        {
          perror("Correction not sent \n");
        }
        else
        {
          printf("Correction sent \n");
          exit(1);
        }
      }
    }
    else
    {
      iAnswer = atoi(buf);
      iDiff = iRes - iAnswer;

      if (iDiff < 0.0001)
      {
        bytes = send(clientSocket, "OK\n", sizeof("OK\n"), 0);
        if (bytes == -1)
        {
          perror("Correction not sent \n");
        }
        else
        {
          printf("Correction sent \n");
          exit(1);
        }
      }
    }  
  }

  close(serverSocket);
}