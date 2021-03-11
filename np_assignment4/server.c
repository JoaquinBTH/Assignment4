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
#include <sys/time.h>

static int uid = 1;
int counter = 0;
int gameStarted = 0;
int scoreFirstClient = 0;
int scoreSecondClient = 0;
int currentRound = 1;
int clientOne = -1;
int clientTwo = -1;
struct timeval timerL;

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
  int inQueue;
  int isPlaying;
  int hasRecievedReadyMessage;
  int isSpectating;
  int option;
  int leave_flag;
  int sendMenu;
  char message[10];
} clientDetails;

clientDetails *clients[10];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_client(clientDetails *client)
{
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < 10; i++)
  {
    if (!clients[i])
    {
      clients[i] = client;
      counter++;
      break;
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int uid)
{
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < 10; i++)
  {
    if (clients[i])
    {
      if (clients[i]->uid == uid)
      {
        printf("Removing client!\n");
        clients[i] = NULL;
        counter--;
        break;
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

void send_message(char *message, int index)
{
  pthread_mutex_lock(&clients_mutex);
  if (index != -1)
  {
    if (index == 100)
    {
      for (int i = 0; i < counter; i++)
      {
        if (clients[i]->isPlaying >= 1 || clients[i]->isSpectating == 1)
        {
          if (write(clients[i]->clientSock, message, strlen(message)) == -1)
          {
            printf("Error while sending message to all clients playing and Spectating.\n");
            break;
          }
        }
      }
    }
    else
    {
      if (clients[index]->isPlaying >= 1)
      {
        if (write(clients[index]->clientSock, message, strlen(message)) == -1)
        {
          printf("Error while sending message to index.\n");
        }
      }
    }
  }
  else
  {
    for (int i = 0; i < counter; i++)
    {
      if (clients[i]->isPlaying == 1 || clients[i]->isPlaying == 2 || clients[i]->isSpectating == 1)
      {
        if (write(clients[i]->clientSock, message, strlen(message)) == -1)
        {
          printf("Error while sending message to all clients that aren't currently in game.\n");
          break;
        }
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

int check_if_game_can_start()
{
  clientOne = -1;
  clientTwo = -1;
  for (int i = 0; i < counter; i++)
  {
    if (clients[i]->inQueue >= 1 && clientOne == -1)
    {
      clientOne = i;
    }
    else if (clients[i]->inQueue >= 1 && clientOne != i && clientTwo == -1)
    {
      clientTwo = i;
    }
  }

  if (clientOne != -1 && clientTwo != -1)
  {
    clients[clientOne]->isPlaying = 1;
    clients[clientOne]->inQueue = 0;
    clients[clientTwo]->isPlaying = 1;
    clients[clientTwo]->inQueue = 0;
    printf("ClientOne %d\nClientTwo %d\n", clientOne, clientTwo);
    gameStarted = 1;
    return 1;
  }
  else
  {
    gameStarted = 0;
    return 0;
  }
}

void play_game(int sectionOfGame)
{
  char message[100];
  memset(message, 0, 100);

  if (sectionOfGame == 1)
  {
    strcpy(message, "A game is ready, press \"Enter\" to accept\n");
    send_message(message, clientOne);
    send_message(message, clientTwo);
    printf("Sent messages!\n");
    clients[clientOne]->hasRecievedReadyMessage = 1;
    clients[clientTwo]->hasRecievedReadyMessage = 1;
  }

  if (sectionOfGame == 2)
  {
    clients[clientOne]->hasRecievedReadyMessage = 2;
    clients[clientTwo]->hasRecievedReadyMessage = 2;
    struct timeval timer, compare;
    int roundMessage = 0;

    while (roundMessage < 4)
    {
      gettimeofday(&compare, NULL);
      if (roundMessage == 0)
      {
        gettimeofday(&timer, NULL);
        memset(message, 0, 100);
        strcpy(message, "Game will start in 3 seconds\n");
        send_message(message, 100);
        roundMessage++;
      }
      else if (roundMessage == 1 && (compare.tv_sec - timer.tv_sec) >= 1)
      {
        memset(message, 0, 100);
        strcpy(message, "Game will start in 2 seconds\n");
        send_message(message, 100);
        roundMessage++;
      }
      else if (roundMessage == 2 && (compare.tv_sec - timer.tv_sec) >= 2)
      {
        memset(message, 0, 100);
        strcpy(message, "Game will start in 1 seconds\n");
        send_message(message, 100);
        roundMessage++;
      }
      else if (roundMessage == 3 && (compare.tv_sec - timer.tv_sec) >= 3)
      {
        memset(message, 0, 100);
        sprintf(message, "Round %d\n", currentRound);
        send_message(message, 100);
        memset(message, 0, 100);
        strcpy(message, "Timer started\nPlease select an option\n1.Rock\n2.Paper\n3.Scissor\n");
        send_message(message, clientOne);
        send_message(message, clientTwo);
        roundMessage++;
        currentRound++;
        gettimeofday(&timerL, NULL);
        clients[clientOne]->isPlaying = 3;
        clients[clientTwo]->isPlaying = 3;
        clients[clientOne]->option = 0;
        clients[clientTwo]->option = 0;
      }
    }
  }

  if (sectionOfGame == 3 && clients[clientOne]->hasRecievedReadyMessage == 2 && clients[clientTwo]->hasRecievedReadyMessage == 2)
  {
    int optionClientOne = clients[clientOne]->option;
    int optionClientTwo = clients[clientTwo]->option;

    if (optionClientOne == 0 && optionClientTwo == 0)
    {
      memset(message, 0, 100);
      strcpy(message, "No player selected, restarting round\n");
      send_message(message, 100);
    }
    else if (optionClientOne == 0 && optionClientTwo != 0)
    {
      memset(message, 0, 100);
      strcpy(message, "You did not make a selection, automatic loss of round\n");
      send_message(message, clientOne);
      memset(message, 0, 100);
      strcpy(message, "Other player didn't select, you won the round\n");
      send_message(message, clientTwo);
      scoreSecondClient++;
    }
    else if (optionClientOne != 0 && optionClientTwo == 0)
    {
      memset(message, 0, 100);
      strcpy(message, "You did not make a selection, automatic loss of round\n");
      send_message(message, clientTwo);
      memset(message, 0, 100);
      strcpy(message, "Other player didn't select, you won the round\n");
      send_message(message, clientOne);
      scoreFirstClient++;
    }

    if (optionClientOne != 0 && optionClientTwo != 0)
    {
      if ((optionClientOne == 1 && optionClientTwo == 1) || (optionClientOne == 2 && optionClientTwo == 2) || (optionClientOne == 3 && optionClientTwo == 3))
      {
        memset(message, 0, 100);
        strcpy(message, "Draw\n");
        send_message(message, -1);
      }
      else if ((optionClientOne == 1 && optionClientTwo == 3) || (optionClientOne == 2 && optionClientTwo == 1) || (optionClientOne == 3 && optionClientTwo == 2))
      {
        scoreFirstClient++;
        memset(message, 0, 100);
        strcpy(message, "Player 1 has won the round\n");
        send_message(message, -1);
        memset(message, 0, 100);
        strcpy(message, "You've won the round: ");
        send_message(message, clientOne);
        memset(message, 0, 100);
        strcpy(message, "You've lost the round: ");
        send_message(message, clientTwo);
      }
      else if ((optionClientTwo == 1 && optionClientOne == 3) || (optionClientTwo == 2 && optionClientOne == 1) || (optionClientTwo == 3 && optionClientOne == 2))
      {
        scoreSecondClient++;
        memset(message, 0, 100);
        strcpy(message, "Player 2 has won the round\n");
        send_message(message, -1);
        memset(message, 0, 100);
        strcpy(message, "You've lost the round: ");
        send_message(message, clientOne);
        memset(message, 0, 100);
        strcpy(message, "You've won the round: ");
        send_message(message, clientTwo);
      }
    }
    memset(message, 0, 100);
    sprintf(message, "Score %d - %d\n", scoreFirstClient, scoreSecondClient);
    send_message(message, -1);
    memset(message, 0, 100);
    sprintf(message, "Score %d - %d\n", scoreFirstClient, scoreSecondClient);
    send_message(message, clientOne);
    memset(message, 0, 100);
    sprintf(message, "Score %d - %d\n", scoreSecondClient, scoreFirstClient);
    send_message(message, clientTwo);
    clients[clientOne]->isPlaying = 2;
    clients[clientOne]->isPlaying = 2;
    clients[clientOne]->hasRecievedReadyMessage = 1;
    clients[clientOne]->hasRecievedReadyMessage = 1;
  }
}

void *handle_client(void *arg)
{
  char protocol[20];
  char menuMessage[100];
  char anyMessage[100];
  memset(protocol, 0, 20);
  strcpy(protocol, "RPS TCP 1\n");

  clientDetails *currentClient = (clientDetails *)arg;

  //Send Protocol
  printf("Server protocol: %s", protocol);
  if ((send(currentClient->clientSock, protocol, strlen(protocol), 0)) == -1)
  {
    printf("Send failed!\n");
    currentClient->leave_flag = 1;
  }

  if (currentClient->leave_flag != 1)
  {
    printf("Sending to menu\n");
    memset(menuMessage, 0, 100);
    strcpy(menuMessage, "Please select:\n1. Play\n2. Watch\n0. Exit\n");
  }

  while (1)
  {
    if (currentClient->leave_flag)
    {
      break;
    }
    else if (currentClient->sendMenu == 0)
    {
      if ((send(currentClient->clientSock, menuMessage, strlen(menuMessage), 0)) == -1)
      {
        printf("Send failed!\n");
        currentClient->leave_flag = 1;
        break;
      }
      else
      {
        currentClient->sendMenu = 1;
      }
    }

    if (currentClient->leave_flag == 0 && currentClient->sendMenu == 1 && currentClient->inQueue >= 1 && currentClient->isPlaying == 0 && currentClient->isSpectating == 0 && gameStarted == 0)
    {
      if (check_if_game_can_start() == 1)
      {
        printf("Game started\n");
      }
      else if (currentClient->inQueue == 1)
      {
        memset(anyMessage, 0, 100);
        strcpy(anyMessage, "One more player required to start game\nPress \"Enter\" to leave the queue\n");
        if ((send(currentClient->clientSock, anyMessage, strlen(anyMessage), 0)) == -1)
        {
          printf("Send failed!\n");
          currentClient->leave_flag = 1;
          break;
        }
        else
        {
          currentClient->inQueue = 2;
        }
      }
    }
    else if (currentClient->leave_flag == 0 && currentClient->sendMenu == 1 && currentClient->inQueue == 0 && currentClient->isPlaying == 1 && currentClient->isSpectating == 0 && gameStarted == 1)
    {
      while (scoreFirstClient < 3 && scoreSecondClient < 3)
      {
        if (currentClient->clientSock == clients[clientOne]->clientSock)
        {
          while (clients[clientOne]->isPlaying != 2 && clients[clientTwo]->isPlaying != 2)
          {
            if (currentClient->isPlaying == 1 && currentClient->hasRecievedReadyMessage == 0)
            {
              play_game(currentClient->isPlaying);
            }
          }
          while (clients[clientOne]->isPlaying != 3 && clients[clientTwo]->isPlaying != 3)
          {
            if (currentClient->isPlaying == 2 && currentClient->hasRecievedReadyMessage == 1)
            {
              play_game(currentClient->isPlaying);
            }
          }
        }
      }
      //struct timeval compareL;
      /*while (scoreFirstClient < 3 && scoreSecondClient < 3)
      {
        if (currentClient->clientSock == clients[clientOne]->clientSock)
        {
          if (currentClient->isPlaying == 2 && currentClient->hasRecievedReadyMessage == 1)
          {
            play_game(currentClient->isPlaying);
          }
        }
        /*else if (clients[clientOne]->isPlaying == 3 && clients[clientTwo]->isPlaying == 3 && currentClient->hasRecievedReadyMessage == 2)
        {
          gettimeofday(&compareL, NULL);
          if ((compareL.tv_sec - timerL.tv_sec) + ((compareL.tv_usec - timerL.tv_usec) / 1000000) < 2)
          {
            if (clients[clientOne]->option != 0 && clients[clientTwo]->option != 0)
            {
              play_game(currentClient->isPlaying);
            }
          }
          else
          {
            play_game(currentClient->isPlaying);
          }
        }*/
      //}
      printf("Continuing the loop\n");
    }
  }
  remove_client(currentClient->uid);
  close(currentClient->clientSock);
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

  //Trying to make select work
  fd_set current_sockets, ready_sockets;

  //Initialize current set
  FD_ZERO(&current_sockets);
  FD_SET(serverSock, &current_sockets);
  int fdmax = serverSock;

  struct sockaddr_in clientAddr;
  socklen_t client_size = sizeof(clientAddr);

  int clientSock = 0;

  while (1)
  {
    //Select stuff
    ready_sockets = current_sockets;

    if (select(fdmax + 1, &ready_sockets, NULL, NULL, NULL) < 0)
    {
      perror("select failed!\n");
      exit(1);
    }

    for (int i = serverSock; i < (fdmax + 1); i++)
    {
      if (FD_ISSET(i, &ready_sockets))
      {
        if (i == serverSock)
        {
          //This is a new connection
          clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &client_size);
          if (clientSock == -1)
          {
            perror("Accept failed!\n");
          }
          else
          {
            if (clientSock > fdmax)
            {
              fdmax = clientSock;
            }
          }

          if ((counter + 1) == 10)
          {
            printf("Maximum clients already connected\n");
            close(clientSock);
            continue;
          }
          FD_SET(clientSock, &current_sockets);

          //Adding client details to currentClient
          printIpAddr(clientAddr);
          clientDetails *currentClient = (clientDetails *)malloc(sizeof(clientDetails));
          memset(currentClient, 0, sizeof(clientDetails));
          currentClient->address = clientAddr;
          currentClient->clientSock = clientSock;
          currentClient->inQueue = 0;
          currentClient->isPlaying = 0;
          currentClient->hasRecievedReadyMessage = 0;
          currentClient->isSpectating = 0;
          currentClient->option = 0;
          currentClient->sendMenu = 0;
          currentClient->leave_flag = 0;
          currentClient->uid = uid++;

          //Adding client to the array
          add_client(currentClient);
          pthread_create(&tid, NULL, &handle_client, (void *)currentClient);
        }
        else
        {
          int index;
          for (int j = 0; j < counter; j++)
          {
            if (clients[j]->clientSock == i)
            {
              index = j;
            }
          }
          memset(clients[index]->message, 0, 10);
          if ((read(clients[index]->clientSock, &clients[index]->message, sizeof(clients[index]->message))) == -1)
          {
            printf("Recieve failed!\n");
          }
          if (clients[index]->leave_flag == 0 && clients[index]->sendMenu == 1 && clients[index]->inQueue == 0 && clients[index]->isPlaying == 0 && clients[index]->isSpectating == 0)
          {
            if (strcmp(clients[index]->message, "1\n") == 0)
            {
              clients[index]->inQueue = 1;
              printf("Recieved: 1\n");
            }
            else if (strcmp(clients[index]->message, "2\n") == 0)
            {
              clients[index]->isSpectating = 1;
              printf("Recieved: 2\n");
            }
            else if (strcmp(clients[index]->message, "") == 0)
            {
              //Do nothing
            }
            else if (strcmp(clients[index]->message, "0\n") == 0)
            {
              printf("Recieved: 0\n");
              clients[index]->leave_flag = 1;
            }
            else
            {
              clients[index]->sendMenu = 0;
            }
          }
          else if (clients[index]->leave_flag == 0 && clients[index]->sendMenu == 1 && clients[index]->inQueue >= 1 && clients[index]->isPlaying == 0 && clients[index]->isSpectating == 0 && gameStarted == 0)
          {
            if (strcmp(clients[index]->message, "\n") == 0 && clients[index]->inQueue >= 1 && clients[index]->isPlaying == 0)
            {
              clients[index]->inQueue = 0;
              clients[index]->sendMenu = 0;
            }
            else if (strcmp(clients[index]->message, "") == 0)
            {
              //Do nothing
            }
          }
          else if (clients[index]->leave_flag == 0 && clients[index]->sendMenu == 1 && clients[index]->inQueue == 0 && clients[index]->isPlaying == 1 && clients[index]->isSpectating == 0 && gameStarted == 1)
          {
            if (strcmp(clients[index]->message, "\n") == 0)
            {
              char ready[20];
              memset(ready, 0, 20);
              strcpy(ready, "You are now ready!\n");
              clients[index]->isPlaying = 2;
              memset(clients[index]->message, 0, 10);
              if ((send(clients[index]->clientSock, ready, strlen(ready), 0)) == -1)
              {
                printf("Error\n");
              }
            }
            else if (strcmp(clients[index]->message, "") == 0)
            {
              //Do nothing
            }
          }
          else if (clients[index]->leave_flag == 0 && clients[index]->sendMenu == 1 && clients[index]->inQueue == 0 && clients[index]->isPlaying == 3 && clients[index]->isSpectating == 0 && gameStarted == 1)
          {
            if (strcmp(clients[index]->message, "1\n") == 0)
            {
              clients[index]->option = 1;
              printf("Changes option to: 1\n");
            }
            else if (strcmp(clients[index]->message, "2\n") == 0)
            {
              clients[index]->option = 2;
              printf("Changes option to: 2\n");
            }
            else if (strcmp(clients[index]->message, "3\n") == 0)
            {
              clients[index]->option = 3;
              printf("Changes option to: 3\n");
            }
          }
        }
      }
    }
  }
  close(serverSock);
  return 0;
}
