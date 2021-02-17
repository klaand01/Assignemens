#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* You will to add includes here */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Included to get the support library
#include <calcLib.h>

// Enable if you want debugging to be printed, see examble below.
// Alternative, pass argument during compilation '-DDEBUG'
#define DEBUG


using namespace std;


int main(int argc, char *argv[]){
  
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

  int serverSocket,returnValue;
  int current = 1;
  int queue = 5;

  struct sockaddr servAddrs;
  memset(&servAddrs, 0, sizeof(servAddrs));

  serverSocket = socket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP);
  if (serverSocket == -1)
  {
    perror("Socket failed to create \n");
    exit(1);
  }

  returnValue = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &current, sizeof(int));
  if (returnValue == -1)
  {
    perror("Wrong with SO_REUSEADDR \n");
    exit(1);
  }

  returnValue = bind(serverSocket, &servAddrs, sizeof(servAddrs));
  if (returnValue == -1)
  {
    perror("Could not bind \n");
    exit(1);
  }

  returnValue = listen(serverSocket, queue);
  if (returnValue == -1)
  {
    perror("Did not listen \n");
    exit(1);
  }

  struct sockaddr_in clienAddrs;
  int clientSocket;
  char buf[10000];

  while (1)
  {
    
  }
}