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
#include <signal.h>

#define DEBUG

volatile sig_atomic_t flag = 0;

void catch_ctrl_c()
{
  flag = 1;
}

void str_overwrite_stdout()
{
  printf("\r");
  fflush(stdout);
}

void recv_msg_handler(void *arg)
{
  char message[255];

  int *socket = (int *)arg;

  while (1)
  {
    memset(message, 0, 255);
    int recieve = recv(*socket, message, sizeof(message), 0);
    if (recieve == -1)
    {
      printf("Recieve failed!\n");
      break;
    }
    else
    {
      char messageWithoutName[255];
      memset(messageWithoutName, 0, 255);
      int spacesFound = 0;
      printf("strlen of message: %d\n", (int)strlen(message));
      for (int i = 0; i < ((int)strlen(message)); i++)
      {
        if (message[i] == ' ')
        {
          spacesFound++;
        }

        if (spacesFound > 1)
        {
          messageWithoutName[(int)strlen(messageWithoutName)] = message[i];
        }
      }
      memcpy(messageWithoutName, &messageWithoutName[1], sizeof(messageWithoutName));
      printf("%s", messageWithoutName);
      str_overwrite_stdout();
    }
  }
}

void send_msg_handler(void *arg)
{
  char message[1000];

  int *socket = (int *)arg;

  while (1)
  {
    char tempMessage[255];
    memset(tempMessage, 0, 255);
    memset(message, 0, 1000);
    fgets(tempMessage, 255, stdin);
    flushinp();
    sprintf(message, "MSG %s: %s", tempMessage);
    if (strcmp(message, "\n") != 0 && strlen(message) <= 238)
    {
      int response = send(*socket, &message, 255, 0);
      if (response == -1)
      {
        printf("Send failed!\n");
        break;
      }
    }
  }
  catch_ctrl_c(2);
}

int main(int argc, char *argv[])
{

  /* Do magic */
  if (argc != 2)
  {
    printf("Usage: %s <ip>:<port>\n", argv[0]);
    exit(1);
  }
  /*
    Read first input, assumes <ip>:<port> syntax, convert into one string (Desthost) and one integer (port). 
     Atm, works only on dotted notation, i.e. IPv4 and DNS. IPv6 does not work if its using ':'. 
  */
  char *hoststring, *portstring, *rest, *org;
  org = strdup(argv[1]);
  rest = argv[1];
  hoststring = strtok_r(rest, ":", &rest);
  portstring = strtok_r(rest, ":", &rest);
  free(org);

  /* Do magic */
  int port = atoi(portstring);
#ifdef DEBUG
  printf("Connected to %s:%d\n", hoststring, port);
#endif

  struct addrinfo hint, *servinfo, *p;
  int rv;
  int clientSock;

  memset(&hint, 0, sizeof(hint));
  hint.ai_family = AF_UNSPEC;
  hint.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(hoststring, portstring, &hint, &servinfo)) != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }
  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    if ((clientSock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      printf("Socket creation failed.\n");
      continue;
    }
    int con = connect(clientSock, p->ai_addr, p->ai_addrlen);
    if (con == -1)
    {
      printf("Connection failed!\n");
      exit(1);
    }
    break;
  }

  if (p == NULL)
  {
    fprintf(stderr, "Client failed to create an apporpriate socket.\n");
    freeaddrinfo(servinfo);
    exit(1);
  }

  signal(SIGINT, catch_ctrl_c);

  char buf[255];
  int recieve, response;
  memset(buf, 0, 255);

  recieve = recv(clientSock, &buf, sizeof(buf), 0);
  if (recieve == 0)
  {
    printf("Recieve failed!\n");
  }

  printf("Server protocol: %s", buf);

  if (strcmp(buf, "RPS TCP 1\n") == 0)
  {
    printf("Protocol supported\n");
  }
  else
  {
    printf("Protocol failed!\n");
    exit(1);
  }

  pthread_t send_msg_thread;
  if (pthread_create(&send_msg_thread, NULL, (void *)send_msg_handler, (void *)&clientSock) != 0)
  {
    printf("Error!\n");
    exit(1);
  }

  pthread_t recv_msg_thread;
  if (pthread_create(&recv_msg_thread, NULL, (void *)recv_msg_handler, (void *)&clientSock) != 0)
  {
    printf("Error!\n");
    exit(1);
  }

  while (1)
  {
    if (flag)
    {
      memset(buf, 0, 255);
      strcpy(buf, "exit\n");
      if (send(clientSock, &buf, sizeof(buf), 0) == -1)
      {
        printf("Last send with exit failed!\n");
      }
      printf("\nBye\n");
      break;
    }
  }

  close(clientSock);
  return 0;
}
