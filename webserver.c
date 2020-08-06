/* 
webserver.c is written by CS585 group 4 comprised of Thomas Adams, Carolyn Atterbury, Nitin Bhandari, and Catherine Wright.  This code starts a webserver that can deliver stored  webpages to clients when a socket is created.  
*/

//Header file for webserver.c contains variable definitions and function prototypes, as well as all libraries used.  
#include "webserver.h"

//Defines the files that are returned to clients.  Either html_buf and gif_buf (for the requested lab2.html), or an error page (error_buf).  
char html_buf[1024];
FILE *html_f;
int html_num;
char gif_buf[32768]; //2^15
FILE *gif_f;
int gif_num;
char error_buf[256];
FILE *error_f;
int error_num;

//Used for debugging, prints to console update messages
const bool DEBUG = true;

int main(int argc, char *argv[]) {
	char hostbuf[256];
	char *IPbuffer;
	struct hostent *host_entry;
	int hostname;

	struct rlimit rl;
	if(getrlimit(RLIMIT_NOFILE, &rl))
		err(1, "getrlimit failed");
	rl.rlim_cur = rl.rlim_max;
	if(setrlimit(RLIMIT_NOFILE, &rl))
		err(1, "setrlimit failed");


	hostname = gethostname(hostbuf, sizeof(hostbuf));
	host_entry = gethostbyname(hostbuf);
	IPbuffer = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));
	printf("Host IP address: %s\n", IPbuffer);
	
	//Sets all files in memory
	//also puts the respective header in front of each file for easy sending
	char html_buf_init[512]; 
	bzero(&html_buf_init,512);
	html_f = fopen("lab2.html", "r");
	if(html_f == NULL){
		err(1, "lab2.html could not be opened");
	}
	html_num = fread(html_buf_init, 1, 512, html_f);
	char *goodresp = "HTTP/1.1 200 OK\r\nContent-Length: 348\r\n\r\n"; 	
	bzero(&html_buf, 1024);
	memcpy(html_buf, goodresp, strlen(goodresp));
	memcpy(html_buf + strlen(goodresp), html_buf_init, html_num);	
	html_num += strlen(goodresp);
	fclose(html_f);


	char gif_buf_init[32768];
	bzero(&gif_buf_init,32768);
	gif_f = fopen("comic.gif", "r");
	if(gif_f == NULL){
		err(1, "comic.gif could not be opened");
	}
	gif_num = fread(gif_buf_init, 1, 32768, gif_f);
	char goodresp_gif[] = "HTTP/1.1 200 OK\r\nContent-Length: 20189\r\n\r\n";

	bzero(&gif_buf, 32768);
	memcpy(gif_buf, goodresp_gif, strlen(goodresp_gif));
	memcpy(gif_buf + strlen(goodresp_gif), gif_buf_init, gif_num);	
	gif_num += strlen(goodresp_gif);
	fclose(gif_f);
	
	char error_buf_init[256];
	bzero(&error_buf_init, 256);
	error_f = fopen("notfound.html", "r");
	if(error_f == NULL){
		err(1, "notfound.html could not be opened");
	}

	error_num = fread(error_buf_init, 1, 256, error_f);
	char badresp[] = "HTTP/1.1 404 Not Found\r\nContent-Length: 149\r\n\r\n";
	bzero(&error_buf, 256);
	memcpy(error_buf, badresp, strlen(badresp));
	memcpy(error_buf + strlen(badresp), error_buf_init, error_num);	
	error_num += strlen(badresp);
	
	//Parses the command line.  
	switch(argc)
	{
		case 1:	port_number = 8080;
			connection_type = "TCP";
			printf("Default port number %d and connection type %s\n", port_number, connection_type);
			break;
		case 2: if(strncmp(argv[1], "TCP", 5) == 0 || strncmp(argv[1], "UDP", 5) == 0)
			{
				port_number = 8080;
				connection_type = argv[1];
				printf("Default port number %d\n", port_number);
			}
			else
			{
				port_number = atoi(argv[1]);
				if(port_number < 8000 || port_number > 9000) 
				{
					printf("Invalid Arguments! Please use a port in the range (8000,9000) and either a TCP or UDP connection.\n");
					return -1;
				}
				connection_type = "TCP";
				printf("Default connection type %s\n", connection_type);
			}
			break;	
		case 3:	port_number = atoi(argv[1]);
			connection_type = argv[2];
			//Make sure valid port and type parameters supplied
			if(port_number < 8000 || port_number > 9000) 
			{
				printf("Please use a port number in the range (8000, 9000)\n");
				return -1;
			}
			if(strncmp(connection_type, "TCP", 5) != 0 && strncmp(connection_type, "UDP", 5) != 0)
			{
				printf("Please use either TCP or UDP\n");
				return -1;
			}
			break;
		default:
			printf("Too many arguments!\n");
			return -1;
	}
	begin_ss();
	return 0;
}

//Starts the server socket based on the specified connection type (TCP or UDP). 
void begin_ss() 
{
	if(!strncmp(connection_type, "TCP", 5))
		start_TCP_server();
	else
		start_UDP_server();
}

//If TCP connection type, starts TCP server socket that listens for clients continuously.  If a client is heard it forks off a process that handles that clients request. 
void start_TCP_server()
{
	//Begin server socket
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(port_number);
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	
	int server_socket = socket(PF_INET, SOCK_STREAM, 0);
	if(server_socket == -1) err(1, "Server socket failure");
		
	if((bind(server_socket, (struct sockaddr*)&address, sizeof(address))) == -1) err(1, "Bind failure");
	//backlog variable is the max length to which the queue of pending connections for server_socket may grow.  May change this during testing???
	int backlog = 5;
	if((listen(server_socket, backlog)) == -1) err(1, "Listen failure");
	int client_socket;
	struct sockaddr_in client_address;
	int client_length = sizeof(client_address);
	while(1)
	{
		if((client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_length)) == -1) err(1,"Accept failure");
		//else printf("Successfully connected client socket %d\n", client_socket);
		//Fork for client_socket
		int forkstatus = fork();
		if(forkstatus == 0)
			TCP_handle_request(client_socket);
		else if(forkstatus > 0) 
			while((forkstatus = waitpid((int)-1, NULL, WNOHANG)) > 0) printf("REAPED\n");
		else err(1, "Fork error");
	}
}

//Similarily to the above function, this function handles requests that are UDP connections.  Starts a UDP socket and listens for incoming clients.  Given a heard client, forks a process and delivers files to the client accordingly
void start_UDP_server()
{
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(port_number);
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	int serverfd;
	if((serverfd = socket(PF_INET, SOCK_DGRAM, 0))==-1){
		perror("UDP socket creation failed");
		exit(EXIT_FAILURE);
	}
	else{
		if(DEBUG) printf("UDP socket created successfully\n");
	}
	
	if(bind(serverfd, (struct sockaddr*) &address, sizeof(address))){
		close(serverfd);
		perror("UDP binding failed");
		exit(EXIT_FAILURE);
	}

	char buf[1024];
	int msg, len;
	struct sockaddr_in client;
	memset(&client, 0, sizeof(client));
	while(1){
		if((msg = recvfrom(serverfd, buf, 1024, MSG_WAITALL, (struct sockaddr *) &client, &len))==-1){
			perror("UDP receipt failed");
			close(serverfd);
			exit(EXIT_FAILURE);
		}

		switch(fork()){
		case -1:
			perror("UDP forking failed");
			close(serverfd);
			exit(EXIT_FAILURE);
		case 0:
			UDP_handle_request(serverfd, buf,msg, client, len);
		default:
			continue;
		}
	}
}

//If a UDP connection is successfully connected this function handles the GET request from the client.  
void UDP_handle_request(int servfd, char *buf, int msg_len, struct sockaddr_in client, int len){
	if(DEBUG) printf("Entered UDP request\n");
        if(DEBUG) printf("buffer %s\n",buf);
	char *head = strtok(buf, " ");
	if(strncmp(head, "GET", 5)){
		//This shouldn't happen since the client code formats the request.  Produces error
		err(1, "invalid head: %s.  Was expecting GET\n", head);
	}
	head = strtok(NULL, " ");
 	if(DEBUG) printf("Head is currently : %s\n", head);
	int bytes;
	if(!strcmp(head, "/lab2.html"))
	{
		//due to the way UDP handles inputs, the header actually needs to be sent before the 
		//file.  Otherwise UDP will throw out the body of the message.
		char *goodresp = "HTTP/1.1 200 OK\r\nContent-Length: 348\r\n\r\n"; 	
		char goodresp_gif[] = "HTTP/1.1 200 OK\r\nContent-Length: 20189\r\n\r\n";


		//sending headers followed by data
		if((bytes = sendto(servfd, goodresp,strlen(goodresp), 0, (struct sockaddr *) &client, len)) == -1) 
			err(1, "Send failed"); 
		if((bytes = sendto(servfd, html_buf+strlen(goodresp),html_num-strlen(goodresp), 0, (struct sockaddr *) &client, len)) == -1) 
			err(1, "Send failed"); 
		if(DEBUG) printf("First file bytes: %d\n", bytes);
		if((bytes = sendto(servfd, goodresp_gif, strlen(goodresp_gif), 0, (struct sockaddr *) &client, len)) == -1) err(1, "Send failed\n");
		if((bytes = sendto(servfd, gif_buf+strlen(goodresp_gif), gif_num-strlen(goodresp_gif), 0, (struct sockaddr *) &client, len)) == -1) err(1, "Send failed\n");
		if(DEBUG) printf("Second file bytes %d\n", bytes);
	}
	else
	{
		//SEND BAD RQUEST
		char badresp[] = "HTTP/1.1 404 Not Found\r\nContent-Length: 149\r\n\r\n";

		if((bytes = sendto(servfd, badresp,strlen(badresp), 0, (struct sockaddr *) &client, len)) == -1) 
			err(1, "Send failed"); 
	
		if((bytes = sendto(servfd, error_buf+strlen(badresp), error_num-strlen(badresp), 0, (struct sockaddr *) &client, len)) == -1)
			err(1, "Send failed");
		if(DEBUG) printf("Sent bad request\n");
	}
	return;
}

//handles a TCP connection GET request from the client, checking first if the client request was valid and if so sends the appropriate files to the client. 
void TCP_handle_request(int client_socket)
{
	if(DEBUG) printf("Enters TCP request function\n");
	
	int max_msg_size = 1024; //Under the impression GET /lab2.html will be what is sent
	char message[max_msg_size];
	int bytes;

	//NOTE: depending on the way we want to handle this (persistent or non-persistent) we'll either want to immediately
	//close the socket after this, OR we'll want to put this in a while loop and close the socket once we get a -1 returned
	//and errno is set to ECONNRESET.
	//--TEA
	if((bytes = recv(client_socket, message, sizeof(message), 0))==-1) err(1, "Read failure");
	
	//Parsing the message from the client
	int init_msg_size = strlen(message);
	char delim[] = " ";
	char *ptr = strtok(message, delim);
	if(strcmp(ptr, "GET") != 0) err(1, "Invalid client request");
	ptr = strtok(NULL, delim);
	//Here ptr should equal /lab2.html.  If so, TCP_send_files is called which sends both the html file and the gif file to the client. 
	if(strcmp(ptr, "/lab2.html") == 0) TCP_send_files(client_socket, true);
	else TCP_send_files(client_socket, false);
}

//Sends appropriate files to client, whether that is the lab2.html file and gif or the error file. 
void TCP_send_files(int client_socket, bool good_request)
{
	if(good_request)
	{
		int w = write(client_socket, html_buf, html_num);
		if(w == -1) err(1, "html error");
		else
			if(DEBUG) printf("Successfully wrote html file of %d bytes\n", w);	
	
		int wg = write(client_socket, gif_buf, gif_num);
		if(wg == -1) err(1, "gif error");
		else 
			if(DEBUG) printf("Successfully wrote gif file of %d bytes\n", wg);	
		printf("sent\n");
	}
	else
	{
		int we = write(client_socket, error_buf, error_num);
		if(we == -1) err(1, "error error (haha)");
		else
			if(DEBUG) printf("Successfully wrote error file of %d bytes\n", we);
		printf("sent\n");
	}
	//Closes the client socket and exits the forked process
	close(client_socket);
	exit(0);
}
