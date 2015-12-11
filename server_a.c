#include "server_a.h"

int launch_server(int communicationPort, int UEPort){

  Server server = (Server) malloc(sizeof(struct ServerVar));
  //bool StopServer = false;
  int result;
  int infoSize;
  int clientPosition = 0, firstPosition, deltaPosition = 0;
  bool serverFull;
  SOCKET serverSocket, clientSocket;
  SOCKADDR_IN serverInformation, clientInformation;
  pthread_t thread, speakThread;;
  char** delta = (char**) malloc(NB_DELTA_MAX * sizeof(char*));
  pthread_mutex_t mutexDelta = PTHREAD_MUTEX_INITIALIZER;

  server->port = communicationPort;
  server->clients = (Client**) malloc(NB_CLIENT_MAX * sizeof(Client*));

  result = begin_listen(&serverSocket, &serverInformation, communicationPort);
  if(result != NO_ERROR){
    return result;
  }

  infoSize = sizeof(clientInformation);

  #ifdef DEBUG
    printf("Waiting for clients\n");
  #endif
  clientSocket = accept(serverSocket, (SOCKADDR *)&clientInformation, (socklen_t*) &infoSize);

  if(clientSocket == INVALID_SOCKET){
    #ifdef DEBUG
      printf("No connected with the client\n");
    #endif
  }
  else{
    #ifdef DEBUG
      printf("Client connected\n");
    #endif
    firstPosition = clientPosition;
    serverFull = false;
    while(server->clients[clientPosition] != NULL){
        clientPosition++;
        if(clientPosition == NB_CLIENT_MAX){
            clientPosition = 0;
        }
        if(clientPosition == firstPosition){
            printf("The server is full\n");
            serverFull = true;
        }
    }

    if(!serverFull){
        Client client;
        SpeakClient speak;
        pthread_mutex_t mutexClose = PTHREAD_MUTEX_INITIALIZER;
        client.id = clientSocket;
        client.thread = thread;
        client.isClosed = false;
        client.mutexClose = &mutexClose;
        speak.client = &client;
        speak.delta = delta;
        speak.startValue = &deltaPosition;
        speak.mutexDelta = &mutexDelta;
        server->clients[clientPosition] = &client;
        if(pthread_create(&thread, NULL, listenClient, &client) != 0){

            #ifdef DEBUG
              printf("Threads listen not created\n");
            #endif
            server->clients[clientPosition] = NULL;
        }
        if(pthread_create(&speakThread, NULL, speakClient, &speak) != 0){
            #ifdef DEBUG
              printf("Threads speak not created\n");
            #endif
            server->clients[clientPosition] = NULL;
        }
    }
    else{
        pthread_create(&thread, NULL, serverIsFull, &clientSocket);
    }

    #ifdef DEBUG
      printf("Threads lancement\n");
    #endif

    pthread_join(thread, NULL);

    #ifdef DEBUG
      printf("Writing in delta\n");
    #endif

    pthread_mutex_lock(&mutexDelta);
    delta[0] = "Hello";
    pthread_mutex_unlock(&mutexDelta);

    #ifdef DEBUG
      printf("Sleep\n");
    #endif

    sleep(1);
    pthread_mutex_lock(server->clients[0]->mutexClose);
    server->clients[0]->isClosed = true;
    closesocket(server->clients[0]->id);
    pthread_mutex_unlock(server->clients[0]->mutexClose);
    pthread_join(speakThread, NULL);
  }


  closesocket(serverSocket);
  return NO_ERROR;
}

int begin_listen(SOCKET* server, SOCKADDR_IN* info, int port){

  //Socket creation
  *server = socket(AF_INET, SOCK_STREAM, 0);
  if(*server == INVALID_SOCKET)
  {
    #ifdef DEBUG
      printf("Error in socket creation\n");
    #endif
    return OTHER_ERROR;
  }

  #ifdef DEBUG
    printf("Socket creation successfull\n");
  #endif

  info->sin_addr.s_addr = htonl(INADDR_ANY); /* Accept any adress */

  info->sin_family = AF_INET;

  info->sin_port = htons(port);

  if(bind (*server, (SOCKADDR *) info, sizeof(*info)) == SOCKET_ERROR)
  {
    #ifdef DEBUG
      printf("Socket can't be bind\n");
    #endif
    return OTHER_ERROR;
  }

  #ifdef DEBUG
    printf("Socket binded\n");
  #endif

  if(listen(*server, NB_CLIENT_MAX) == SOCKET_ERROR)
  {
    #ifdef DEBUG
      printf("Error while listen\n");
    #endif
    return OTHER_ERROR;
  }

  #ifdef DEBUG
    printf("Server is listening\n");
  #endif

  return NO_ERROR;
}



void* listenClient(void* clientVoid){
    Client* client = (Client*) clientVoid;
    char* buff = (char*) malloc(10*sizeof(char));
    recv(client->id, buff, 10, 0);
    printf("Message reçu : %s\n", buff);

    pthread_exit(NULL);
}

void* speakClient(void* clientVoid){
    SpeakClient* speakClient = (SpeakClient*) clientVoid;
    Client* client = speakClient->client;
    int* value = speakClient->startValue;
    char** delta = speakClient->delta;

    #ifdef DEBUG
      printf("Begin of the loop\n");
    #endif

    for(;;){

        char* result = (char *) malloc(sizeof(char));
        result[0] = '\0';
        bool isClosed;
        bool deltaIsNull;

        for(;;){

            #ifdef DEBUG
              printf("Check if delta\n");
            #endif

            pthread_mutex_lock(speakClient->mutexDelta);
            deltaIsNull = (delta[*value] == NULL);
            pthread_mutex_unlock(speakClient->mutexDelta);

            if(!deltaIsNull){
                #ifdef DEBUG
                  printf("It has delta\n");
                #endif
                result = realloc(result, (strlen(result) + strlen(delta[*value]) * sizeof(char)));
                sprintf(result, "%s%d%s", result, (int) strlen(delta[*value]), delta[*value]);
                *value++;
                break;
            }
            else{
                #ifdef DEBUG
                  printf("It has not delta, end of search\n");
                #endif
                break;
            }
        }

        if(!deltaIsNull){

            #ifdef DEBUG
              printf("Check if client is closed\n");
            #endif

            pthread_mutex_lock(client->mutexClose);
            isClosed = client->isClosed;
            pthread_mutex_unlock(client->mutexClose);

            #ifdef DEBUG
              printf("Begin to sending message\n");
            #endif

            if(!isClosed){
                #ifdef DEBUG
                  printf("Sending : \"%s\" to the client\n", result);
                #endif
                send(client->id, result, strlen(result), 0);
                #ifdef DEBUG
                  printf("Message sent\n");
                #endif

            }
            else{
                #ifdef DEBUG
                  printf("Disconnected client\n");
                #endif
                break;
            }
        }
        else{
            pthread_mutex_lock(client->mutexClose);
            isClosed = client->isClosed;
            pthread_mutex_unlock(client->mutexClose);
            if(isClosed){
                #ifdef DEBUG
                  printf("Disconnected client\n");
                #endif
                break;
            }
        }

    }
    pthread_exit(NULL);
}

void* serverIsFull(void* clientVoid){
    #ifdef DEBUG
      printf("Server is full, client not accepted\n");
    #endif
    SOCKET* client = (SOCKET*) clientVoid;
    closesocket(*client);
    pthread_exit(NULL);
}
