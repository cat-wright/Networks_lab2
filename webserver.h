#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
//Variables
int port_number; 	//The server's port number
char* connection_type;  //The socket connection type (TCP or UDP)

//Function prototypes
void begin_ss();  						//Called when command line arguments have been set.  Opens a new server socket
void start_TCP_server();					//Separated from begin_ss() for readability.  Starts the TCP server socket and forks clients
void TCP_handle_request(int client_socket);			//reads from the client socket and returns the files or an error.
void TCP_send_files(int client_socket, bool good_request); 	//If GET request was correct, sends the html file and gif file to the client.  
void start_UDP_server();					//similar to start_TCP_server, separated to keep TCP and UDP segments of code separate
void UDP_handle_request(int servfd, char *buf, int msg_len, struct sockaddr_in client, int len); //handles responses to queries on the server when UDP is used

