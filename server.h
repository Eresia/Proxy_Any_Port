#ifndef DEFINE_SERVER
#define DEFINE_SERVER

	#define NB_CLIENT_MAX 100
	#define NB_DELTA_MAX 1000

	/*Returns and errors :
		0 : No error
		1 : Bad number of arguments
		2 : Incorrect arguments
		3 : Other

	*/
	#define NO_ERROR 0
	#define BAD_NUMBER_OF_ARGUMENTS 1
	#define INCORRECT_ARGUMENT 2
	#define OTHER_ERROR 3

	//General includes
	#include <stdlib.h>
	#include <stdio.h>
	#include <string.h>
	#include <unistd.h>

	//Threads include
	#include <pthread.h>

	//Socket include
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>

	//For Windows or Linux :
	#ifdef WIN32

		#include <winsock2.h>

	#else

		#include <sys/types.h>
		#include <sys/socket.h>
		#include <netinet/in.h>
		#include <arpa/inet.h>
		#include <unistd.h> /* close */
		#include <netdb.h> /* gethostbyname */
		#define INVALID_SOCKET -1
		#define SOCKET_ERROR -1
		#define closesocket(s) close(s)
		typedef int SOCKET;
		typedef struct sockaddr_in SOCKADDR_IN;
		typedef struct sockaddr SOCKADDR;
		typedef struct in_addr IN_ADDR;

	#endif

	//Boolean utilisation
	typedef enum bool bool;
	enum bool{
		false = 0, true = 1
	};

    typedef struct Client Client;
    struct Client{
        SOCKET socket;
        SOCKADDR_IN information;
    };

    int launch_server(int, char*, int);
    void* redirect_client(void* client);
    int begin_listen(SOCKET*, SOCKADDR_IN*, int);

#endif
