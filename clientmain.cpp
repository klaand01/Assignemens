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
  int server_socket;
  int returnValue;

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
    server_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (server_socket == -1)
    {
      perror("Socket not created \n");
    }

    if (connect(server_socket, ptr->ai_addr, ptr->ai_addrlen) == -1)
    {
      close(server_socket);
      perror("Server not connected \n");
    }

    break;
  }

  if (ptr == NULL)
  {
    perror("Client: Failed to connect \n");
  }
}
