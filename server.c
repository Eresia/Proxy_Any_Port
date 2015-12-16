#include "server.h"

int launch_server(int port, char* redirection, int portRedirection){
	int result;
	int infoSize;
	SOCKET serverSocket;
	SOCKADDR_IN serverInformation = { 0 }, clientInformation = { 0 };
	Client client;
	client.targetName = redirection;
	client.portTarget = portRedirection;

	infoSize = sizeof(clientInformation);

	result = begin_listen(&serverSocket, &serverInformation, port);
	if(result == NO_ERROR){
		while(true){
			client.socket = accept(serverSocket, (SOCKADDR *)&(clientInformation), (socklen_t*) &infoSize);
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
				//pthread_mutex_t mutexSocket = PTHREAD_MUTEX_INITIALIZER;
				//pthread_mutex_t mutexTarget = PTHREAD_MUTEX_INITIALIZER;
				//newClient.mutexSocket = mutexSocket;
				//newClient.mutexTarget = mutexTarget;
				pthread_mutex_t mutexConnexion = PTHREAD_MUTEX_INITIALIZER;
				newClient.mutexConnexion = mutexConnexion;
				newClient.connexionLost = malloc(sizeof(bool));
				*newClient.connexionLost = false;
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

void* redirect_client(void* client_void){
	Client client = *((Client*) client_void);
	client.target = socket(AF_INET, SOCK_STREAM, 0);
	struct hostent *hostinfo = NULL;
	SOCKADDR_IN sin = { 0 }; /* initialise la structure avec des 0 */
	const char *hostname = client.targetName;

	hostinfo = gethostbyname(hostname); /* on récupère les informations de l'hôte auquel on veut se connecter */
	if (hostinfo == NULL) /* l'hôte n'existe pas */
	{
		#ifdef DEBUG
		printf ("Unknown host %s.\n", hostname);
		#endif
		closesocket(client.socket);
		pthread_exit(NULL);
	}

	sin.sin_addr = *(IN_ADDR *) hostinfo->h_addr; /* l'adresse se trouve dans le champ h_addr de la structure hostinfo */
	sin.sin_port = htons(client.portTarget); /* on utilise htons pour le port */
	sin.sin_family = AF_INET;

	if(connect(client.target,(SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		#ifdef DEBUG
		printf("Connexion failed\n");
		#endif
		pthread_exit(NULL);
	}
	else{
		int n = 0;
		#ifdef DEBUG
		printf("Connexion to %s succes\n", client.targetName);
		#endif
		pthread_t thread;
		char* buffer = malloc(100*sizeof(char));
		pthread_create(&thread, NULL, redirect_to_client, &client);
		bool connexionLost;
		pthread_mutex_lock(&client.mutexConnexion);
		connexionLost = *client.connexionLost;
		pthread_mutex_unlock(&client.mutexConnexion);
		while(connexionLost == false){
			if((n = recv(client.socket, buffer, 100, 0)) < 0){
				#ifdef DEBUG
				printf("Fail to receved message from client\n");
				#endif
				pthread_mutex_lock(&client.mutexConnexion);
				*client.connexionLost = true;
				pthread_mutex_unlock(&client.mutexConnexion);
				connexionLost = true;
			}
			else if(n == 0){
				#ifdef DEBUG
				printf("Closing connexion\n");
				#endif
				pthread_mutex_lock(&client.mutexConnexion);
				*client.connexionLost = true;
				pthread_mutex_unlock(&client.mutexConnexion);
				connexionLost = true;
			}
			else{
				#ifdef DEBUG
				printf("Client send a message of size : %d\n", n);
				#endif
				pthread_mutex_lock(&client.mutexConnexion);
				connexionLost = *client.connexionLost;
				pthread_mutex_unlock(&client.mutexConnexion);
				if(connexionLost == false){
					if(send(client.target, buffer, n, 0) < 0){
						#ifdef DEBUG
						printf("Fail to send message to server\n");
						#endif
					}
					else{
						#ifdef DEBUG
						printf("Message send to server\n");
						#endif
					}
				}
			}
		}
		#ifdef DEBUG
		printf("close client\n");
		#endif
		closesocket(client.socket);
		pthread_exit(NULL);
	}
	pthread_exit(NULL);
}

void* redirect_to_client(void* client_void){
	Client client = *((Client*) client_void);
	unsigned char* buffer = malloc(100*sizeof(unsigned char));
	int n = 0;
	bool connexionLost;
	pthread_mutex_lock(&client.mutexConnexion);
	connexionLost = *client.connexionLost;
	pthread_mutex_unlock(&client.mutexConnexion);
	while(connexionLost == false){
		if((n = recv(client.target, buffer, 100, 0)) < 0){
			#ifdef DEBUG
			printf("Fail to receved message from server\n");
			#endif
			pthread_mutex_lock(&client.mutexConnexion);
			*client.connexionLost = true;
			pthread_mutex_unlock(&client.mutexConnexion);
			connexionLost = true;
		}
		else if(n == 0){
			#ifdef DEBUG
			printf("Closing connexion\n");
			#endif
			pthread_mutex_lock(&client.mutexConnexion);
			*client.connexionLost = true;
			pthread_mutex_unlock(&client.mutexConnexion);
			connexionLost = true;
		}
		else{
			#ifdef DEBUG
			printf("Server send a message of size : %d\n", n);
			#endif
			pthread_mutex_lock(&client.mutexConnexion);
			connexionLost = *client.connexionLost;
			pthread_mutex_unlock(&client.mutexConnexion);
			if(connexionLost == false){
				if(send(client.socket, buffer, n, 0) < 0){
					#ifdef DEBUG
					printf("Fail to send message to client\n");
					#endif
				}
				else{
					#ifdef DEBUG
					printf("Message send to client\n");
					#endif
				}
			}
		}
	}
	#ifdef DEBUG
	printf("close server\n");
	#endif
	closesocket(client.target);
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
