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
		printf ("Unknown host %s.\n", hostname);
		pthread_exit(NULL);
	}

	sin.sin_addr = *(IN_ADDR *) hostinfo->h_addr; /* l'adresse se trouve dans le champ h_addr de la structure hostinfo */
	sin.sin_port = htons(client.portTarget); /* on utilise htons pour le port */
	sin.sin_family = AF_INET;

	if(connect(client.target,(SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		printf("connect()");
		pthread_exit(NULL);
	}
	else{
		pthread_t thread;
		char* buffer = malloc(100*sizeof(char));
		pthread_create(&thread, NULL, redirect_to_client, client_void);
		while(1){
			recv(client.socket, buffer, 100, 0);
			send(client.target, buffer, strlen(buffer), 0);
		}
	}
	pthread_exit(NULL);
}

void* redirect_to_client(void* client_void){
	Client client = *((Client*) client_void);
	char* buffer = malloc(100*sizeof(char));
	while(1){
		recv(client.target, buffer, 100, 0);
		send(client.socket, buffer, strlen(buffer), 0);
	}
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
