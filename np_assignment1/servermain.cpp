#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* You will to add includes here */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

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

  serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (serverSocket == -1)
  {
    perror("Socket not created \n");
  }

  struct sockaddr_in myAddr;
  memset(&myAddr, 0, sizeof(myAddr));
  myAddr.sin_family = AF_INET;
  myAddr.sin_port = htons(port);
  inet_aton("0.0.0.0", &myAddr.sin_addr.s_addr);

  returnValue = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &current, sizeof(int));
  if (returnValue == -1)
  {
    perror("Setsocketopt");
    exit(1);
  }
  
  int returnValue = bind(serverSocket, &myAddr, sizeof(myAddr));
    












  close(clientSocket);
}