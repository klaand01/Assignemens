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
  memset(&addrs, 0, sizeof(addrs));
  addrs.ai_family = AF_UNSPEC;
  addrs.ai_socktype = SOCK_STREAM;
  addrs.ai_protocol = IPPROTO_TCP;

  int clientSocket;
  int returnValue;
  int numbrBytes;
  char buf[MAXDATA];

  char oper[5], result[20];
  int iNumb1, iNumb2, iRes;
  float fNumb1, fNumb2, fRes;

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
    perror("Server not connected \n");
    exit(1);
  }

  char myAddress[20];
	const char *myAdd;
  struct sockaddr_in sockAddrss;
  socklen_t sockAddrLen = sizeof(sockAddrss);
  getsockname(clientSocket, (struct sockaddr*)&sockAddrss, &sockAddrLen);
  
  myAdd = inet_ntop(sockAddrss.sin_family, &sockAddrss.sin_addr, myAddress, sizeof(myAddress));

  printf("Connected to %s:%d local %s:%d \n", Desthost, port,
  myAdd, ntohs(sockAddrss.sin_port));

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
      numbrBytes = send(clientSocket, "OK\n", strlen("OK\n"), 0);
      printf("Sent OK \n");
      if (numbrBytes == -1)
      {
        perror("'OK' not gone through \n");
      }
    }

    if (buf[0] == 'f')
    {
      sscanf(buf, "%s %f %f", oper, &fNumb1, &fNumb2);

      if (strcmp(oper, "fadd") == 0)
      {
        fRes = fNumb1 + fNumb2;
      }

      else if (strcmp(oper, "fdiv") == 0)
      {
        fRes = fNumb1 / fNumb2;
      }

      else if (strcmp(oper, "fmul") == 0)
      {
        fRes = fNumb1 * fNumb2;
      }

      else if (strcmp(oper, "fsub") == 0)
      {
        fRes = fNumb1 - fNumb2;
      }

      sprintf(result, "%f\n", fRes);
      numbrBytes = send(clientSocket, result, strlen(result), 0);
      printf("Sent answer %s", result);
      
      if (numbrBytes == -1)
      {
        perror("Answer not gone through \n");
      }
    }
    else if (buf[0] == 'a' || buf[0] == 'd' ||
    buf[0] == 'm' || buf[0] == 's')
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

      sprintf(result, "%d\n", iRes);
      numbrBytes = send(clientSocket, result, strlen(result), 0);
      printf("Sent answer %s", result);

      if (numbrBytes == -1)
      {
        perror("Answer not gone through \n");
      }
    }

    if (buf[0] == 'O' || buf[0] == 'E')
    {
      exit(1);
    }
  }
  close(clientSocket);
}