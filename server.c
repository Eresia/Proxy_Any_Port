#include "server.h"

int launch_server(int port, char* redirection, int portRedirection){
    int result;
    int infoSize;
    SOCKET serverSocket;
    SOCKADDR_IN serverInformation;
    Client client;

    infoSize = sizeof(client.information);

    result = begin_listen(&serverSocket, &serverInformation, port);
    if(result == NO_ERROR){
        while(true){
            client.socket = accept(serverSocket, (SOCKADDR *)&(client.information), (socklen_t*) &infoSize);
            if(client.socket == INVALID_SOCKET){
              #ifdef DEBUG
                printf("No connected with the client\n");
              #endif
            }
            else{
              #ifdef DEBUG
                printf("Client connected\n");
              #endif
              Client newClient = client;
              pthread_t thread;
              if(pthread_create(&thread, NULL, redirect_client, &newClient) != 0){
                  #ifdef DEBUG
                    printf("Threads speak not created\n");
                  #endif
              }
            }
        }
        return NO_ERROR;
    }
    else{
        return result;
    }
}

void* redirect_client(void* client){

    pthread_exit(NULL);
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
