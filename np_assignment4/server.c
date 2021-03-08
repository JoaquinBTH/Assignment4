#include <stdio.h>
#include <stdlib.h>
/* You will to add includes here */
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

static int uid = 1;

void printIpAddr(struct sockaddr_in addr)
{
  printf("Client connected from %d.%d.%d.%d:%d\n",
         addr.sin_addr.s_addr & 0xff,
         (addr.sin_addr.s_addr & 0xff00) >> 8,
         (addr.sin_addr.s_addr & 0xff0000) >> 16,
         (addr.sin_addr.s_addr & 0xff000000) >> 24,
         addr.sin_port);
}

typedef struct
{
  struct sockaddr_in address;
  int clientSock;
  int uid;
  char message[255];
} clientDetails;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg)
{
  char protocol[20];
  char okOrError[20];
  memset(protocol, 0, 20);
  strcpy(protocol, "RPS TCP 1\n");
  int leave_flag = 0;

  clientDetails *currentClient = (clientDetails *)arg;

  //Send Protocol
  printf("Server protocol: %s", protocol);
  if ((send(currentClient->clientSock, &protocol, sizeof(protocol), 0)) == -1)
  {
    printf("Send failed!\n");
    leave_flag = 1;
  }

  while (1)
  {
    if (leave_flag)
    {
      break;
    }
  }
  close(currentClient->clientSock);
  //Remove client if 0

  free(currentClient);
  pthread_detach(pthread_self());

  return NULL;
}

int main(int argc, char *argv[])
{

  /* Do more magic */
  if (argc != 2)
  {
    printf("Usage: %s <ip>:<port> \n", argv[0]);
    exit(1);
  }
  /*
    Read first input, assumes <ip>:<port> syntax, convert into one string (Desthost) and one integer (port). 
     Atm, works only on dotted notation, i.e. IPv4 and DNS. IPv6 does not work if its using ':'. 
  */
  char delim[] = ":";
  char *Desthost = strtok(argv[1], delim);
  char *Destport = strtok(NULL, delim);
  if (Desthost == NULL || Destport == NULL)
  {
    printf("Usage: %s <ip>:<port> \n", argv[0]);
    exit(1);
  }
  // *Desthost now points to a sting holding whatever came before the delimiter, ':'.
  // *Dstport points to whatever string came after the delimiter.

  /* Do magic */
  int port = atoi(Destport);

  int backLogSize = 10;
  int yes = 1;

  struct addrinfo hint, *servinfo, *p;
  int rv;
  int serverSock;
  pthread_t tid;

  memset(&hint, 0, sizeof(hint));
  hint.ai_family = AF_UNSPEC;
  hint.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(Desthost, Destport, &hint, &servinfo)) != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }
  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    if ((serverSock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      printf("Socket creation failed.\n");
      continue;
    }

    if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
      perror("setsockopt failed!\n");
      exit(1);
    }

    rv = bind(serverSock, p->ai_addr, p->ai_addrlen);
    if (rv == -1)
    {
      perror("Bind failed!\n");
      close(serverSock);
      continue;
    }
    break;
  }

  freeaddrinfo(servinfo);

  if (p == NULL)
  {
    fprintf(stderr, "Server failed to create an apporpriate socket.\n");
    exit(1);
  }

  printf("[x]Listening on %s:%d \n", Desthost, port);

  rv = listen(serverSock, backLogSize);
  if (rv == -1)
  {
    perror("Listen failed!\n");
    exit(1);
  }

  struct sockaddr_in clientAddr;
  socklen_t client_size = sizeof(clientAddr);

  int clientSock = 0;

  while (1)
  {
    clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &client_size);
    if (clientSock == -1)
    {
      perror("Accept failed!\n");
    }

    printIpAddr(clientAddr);

    clientDetails *currentClient = (clientDetails *)malloc(sizeof(clientDetails));
    memset(currentClient, 0, sizeof(clientDetails));
    currentClient->address = clientAddr;
    currentClient->clientSock = clientSock;
    currentClient->uid = uid++;

    pthread_create(&tid, NULL, &handle_client, (void *)currentClient);
  }

  close(serverSock);
  return 0;
}
