#include "server.h"

//Lancement du serveur/Thread principal
int launch_server(int port, char* redirection, int portRedirection){
	int result;
	int infoSize;
	SOCKET serverSocket;
	SOCKADDR_IN serverInformation = { 0 }, clientInformation = { 0 };
	Client client;
	client.targetName = redirection;
	client.portTarget = portRedirection;

	infoSize = sizeof(clientInformation);

	//On commence à écouter sur le port
	result = begin_listen(&serverSocket, &serverInformation, port);

	if(result == NO_ERROR){
		while(true){
			//On attend qu'un client se connecte
			client.socket = accept(serverSocket, (SOCKADDR *)&(clientInformation), (socklen_t*) &infoSize);

			if(client.socket == INVALID_SOCKET){ //S'il y a une erreur pendant la connexion, on la refuse
				#ifdef DEBUG
				printf("No connected with the client\n");
				#endif
			}
			else{ //Sinon on se connecte
				#ifdef DEBUG
				printf("Client connected\n");
				#endif

				Client newClient = client;
				pthread_mutex_t mutexConnexion = PTHREAD_MUTEX_INITIALIZER;
				pthread_t thread;

				newClient.mutexConnexion = mutexConnexion;
				newClient.connexionLost = malloc(sizeof(bool));
				*newClient.connexionLost = false;

				//On lance le thread de communication avec le client puis on recommence
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

//Thread de communication et d'écoute d'un client vers le serveur
void* redirect_client(void* client_void){
	Client client = *((Client*) client_void);
	client.target = socket(AF_INET, SOCK_STREAM, 0);
	struct hostent *hostinfo = NULL;
	SOCKADDR_IN sin = { 0 };
	const char *hostname = client.targetName;

	hostinfo = gethostbyname(hostname); //on récupère les informations de l'hôte auquel on veut se connecter

	if (hostinfo == NULL) //L'hôte n'existe pas, on annule tout
	{
		#ifdef DEBUG
		printf ("Unknown host %s.\n", hostname);
		#endif
		closesocket(client.socket);
		pthread_exit(NULL);
	}

	//On rentre les informations de connexions
	sin.sin_addr = *(IN_ADDR *) hostinfo->h_addr;
	sin.sin_port = htons(client.portTarget);
	sin.sin_family = AF_INET;

	//On essaye de se connecter au serveur distant
	if(connect(client.target,(SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		#ifdef DEBUG
		printf("Connexion failed\n");
		#endif

		pthread_exit(NULL); //Si on rate, on rentre à la maison
	}
	else{
		#ifdef DEBUG
		printf("Connexion to %s succes\n", client.targetName);
		#endif

		pthread_t thread;

		//On lance le thread d'écoute du serveur pour ce client
		pthread_create(&thread, NULL, redirect_to_client, &client);

		//On commence l'échange de données
		listening_socket(client, client.socket, client.target);

		#ifdef DEBUG
		printf("close client\n");
		#endif

		//La connexion a été fermée, on ferme les sockets
		closesocket(client.socket);
		pthread_exit(NULL);
	}
	pthread_exit(NULL);
}

//Thread d'écoute du serveur vers un client
void* redirect_to_client(void* client_void){
	Client client = *((Client*) client_void);

	//On commence l'échange de données
	listening_socket(client, client.target, client.socket);

	#ifdef DEBUG
	printf("close server\n");
	#endif

	//La connexion a été fermée, on ferme les socket
	closesocket(client.target);
	pthread_exit(NULL);
}

//Fonction de gestion des échanges de données
void listening_socket(Client client, SOCKET src, SOCKET dest){
	bool connexionLost;
	int n = 0;
	unsigned char* buffer = malloc(100*sizeof(unsigned char));

	//Tant que la connexion n'est pas perdu, on boucle la reception des données du serveur
	do{
		//On tente de receptionner les données
		if((n = recv(src, buffer, 100, 0)) < 0){ //Si on reçoit une erreur
			#ifdef DEBUG
			printf("Fail to receved message from client\n");
			#endif

			//On ferme la connexion
			pthread_mutex_lock(&client.mutexConnexion);
			*client.connexionLost = true;
			pthread_mutex_unlock(&client.mutexConnexion);
			connexionLost = true;
		}
		else if(n == 0){ //Si on reçoit plus rien -> Le client s'est déconnecté
			#ifdef DEBUG
			printf("Closing connexion\n");
			#endif

			//On ferme la connexion
			pthread_mutex_lock(&client.mutexConnexion);
			*client.connexionLost = true;
			pthread_mutex_unlock(&client.mutexConnexion);
			connexionLost = true;
		}
		else{ //Sinon, tout s'est bien passé
			#ifdef DEBUG
			printf("Client send a message of size : %d\n", n);
			#endif

			pthread_mutex_lock(&client.mutexConnexion);
			connexionLost = *client.connexionLost;
			pthread_mutex_unlock(&client.mutexConnexion);

			//On vérifie que la connexion n'a pas été fermer par le serveur
			if(connexionLost == false){
				if(send(dest, buffer, n, 0) < 0){ //On envoie le message
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
	}while(connexionLost == false);
}

//Fonction qui lance l'écoute du serveur
int begin_listen(SOCKET* server, SOCKADDR_IN* info, int port){

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

	//On assigne les informations de connexions au serveur
	info->sin_addr.s_addr = htonl(INADDR_ANY); /* Accept any adress */
	info->sin_family = AF_INET;
	info->sin_port = htons(port);

	//On bind le serveur sur le port s'il est disponible
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

	//On commence à écouter
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
