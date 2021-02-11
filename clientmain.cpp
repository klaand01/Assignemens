#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* You will to add includes here */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVERPORT "5000"
#define MAXDATA 1000

// Enable if you want debugging to be printed, see examble below.
// Alternative, pass 
#define DEBUG


// Included to get the support library
#include <calcLib.h>

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
  // *Desthost now points to a sting holding whatever came before the delimiter, ':'.
  // *Dstport points to whatever string came after the delimiter. 

  /* Do magic */
  int port=atoi(Destport);
  #ifdef DEBUG 
  printf("Host %s, and port %d.\n",Desthost,port);
  #endif

  struct addrinfo addrs, *ptr;
  int clientSocket;
  int returnValue;
  int numbrBytes;
  char buf[MAXDATA];

  char oper[5];
  int iNumb1, iNumb2, iRes;
  double dNumb1, dNumb2, dRes;

  memset(&addrs, 0, sizeof(addrs));
  addrs.ai_family = AF_INET;
  addrs.ai_socktype = SOCK_STREAM;
  addrs.ai_protocol = IPPROTO_TCP;

  returnValue = getaddrinfo(argv[1], SERVERPORT, &addrs, &ptr);
  if (returnValue != 0)
  {
    perror("Wrong getaddrinfo \n");
  }

  for (; ptr != NULL;)
  {
    clientSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (clientSocket == -1)
    {
      perror("Socket not created \n");
    }

    if (connect(clientSocket, ptr->ai_addr, ptr->ai_addrlen) == -1)
    {
      close(clientSocket);
      perror("Server not connected \n");
    }

    break;
  }

  if (ptr == NULL)
  {
    perror("Client: Failed to connect \n");
  }

  while (1)
  {
    numbrBytes = recv(clientSocket, buf, MAXDATA, 0);
    if (numbrBytes == -1)
    {
      perror("Wrong with message \n");
    }

    printf("Client: Recieved '%s' \n", buf);

    if (strcmp(buf, "TEXT TCP 1.0\n\n") == 0)
    {
      numbrBytes = send(clientSocket, "OK\n", 4, 0);
      printf("Sent OK \n");
      if (numbrBytes == -1)
      {
        perror("Send 'OK' not got through \n");
      }
    }
    else if (buf[0] == 'f')
    {
      sscanf(buf, "%s %lg %lg", oper, &dNumb1, &dNumb2);

      if (strcmp(oper, "fadd") == 0)
      {
        dRes = dNumb1 + dNumb2;
      }

      else if (strcmp(oper, "fdiv") == 0)
      {
        dRes = dNumb1 / dNumb2;
      }

      else if (strcmp(oper, "mul") == 0)
      {
        dRes = dNumb1 * dNumb2;
      }

      else if (strcmp(oper, "fsub") == 0)
      {
        dRes = dNumb1 - dNumb2;
      }

      numbrBytes = send(clientSocket, &dRes, sizeof(dRes), 0);
      printf("Sent answer %8.8g \n", dRes);
      if (numbrBytes == -1)
      {
        perror("Answer not gone through \n");
      }
    }
    else
    {
      sscanf(buf, "%s %d %d", oper, &iNumb1, &iNumb2);

      if (strcmp(oper, "add") == 0)
      {
        iRes = iNumb1 + iNumb2;
      }

      else if (strcmp(oper, "div") == 0)
      {
        iRes = iNumb1 / iNumb2;
      }

      else if (strcmp(oper, "mul") == 0)
      {
        iRes = iNumb1 * iNumb2;
      }

      else if (strcmp(oper, "sub") == 0)
      {
        iRes = iNumb1 - iNumb2;
      }

      numbrBytes = send(clientSocket, &iRes, sizeof(iRes), 0);
      printf("Sent answer %d \n", iRes);
      if (numbrBytes == -1)
      {
        perror("Answer not gone through \n");
      }
    }

    if (strcmp(buf, "OK") == 0)
    {
      close(clientSocket);
    }
  }
}