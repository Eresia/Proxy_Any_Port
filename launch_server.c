#include "server.h"

/*Server arguments :
	1 : Port of communication
	2 : Port of UE

*/
int main(int argc, char** argv){

	int port, portRedirection;
	char* redirection;
	int resultServ;
	char* msg;

	//On vérifie si on a le bon nombre d'arguments
	if(argc != (NB_ARGUMENT + 1)){
		msg = "Bad number of arguments\n";
		write(1, msg, strlen(msg));
		#ifdef DEBUG
			printf("%s", msg);
		#endif
		return BAD_NUMBER_OF_ARGUMENTS;
	}

	//On récupère les informations
	port = atoi(argv[1]);
	redirection = argv[2];
	portRedirection = atoi(argv[3]);

	//On vérifie si les informations sont correctes
	if((port == 0) || (portRedirection == 0)){
		msg = "Incorrect arguments\n";
		write(1, msg, strlen(msg));
		#ifdef DEBUG
			printf("%s", msg);
		#endif
		return INCORRECT_ARGUMENT;
	}

	#ifdef DEBUG
		printf("Launch server for redirection to %s in port %d\n", redirection, portRedirection);
	#endif

	//On lance le serveur
	resultServ = launch_server(port, redirection, portRedirection);

	if(resultServ != NO_ERROR){
		msg = "Server stopped with errors\n";
		write(1, msg, strlen(msg));
		#ifdef DEBUG
			printf("%s", msg);
		#endif
		return resultServ;
	}

	msg = "Serveur stopped without error\n";
	printf("%s", msg);

	return NO_ERROR;



}
