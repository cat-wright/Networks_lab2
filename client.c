#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <err.h>
#include <stdbool.h>
#include <time.h>

// Structure containing key parts of the URL, including IP (addr), Port (port), and Path (path)
struct url
{
  char* addr;
  in_port_t port;
  char* path;
};

// HTTP GET request that will be formatted and sent to the server
char getrequest[1024];

const bool TESTING = false;
const bool DEBUG = false;


/* Calculate Latency
 * Function to calculate the difference between two time events
 */
double calculate_latency(struct timespec time1, struct timespec time2)
{
	return (double)(time1.tv_sec - time2.tv_sec) + (double)(time1.tv_nsec-time2.tv_nsec) / 1000000000.0;
}


/* Format Get Request 
 * This function accepts a request structure containing an IP, Port, and Path
 *      and creates a GET request based off of that information. 
 * If no Path has been specified, the user is prompted to enter a path.
 */
char* format_get_request(struct url request) {

  char path[256];
  if (!request.path) {
    printf("What is the file or path you are requesting?  (e.g. filename.html)\n");
    scanf("%s",path);
    printf("PATH: %s\n", path);
    request.path = path;
  }

  // format message
  sprintf(getrequest, "GET /%s HTTP/1.1\r\nHOST: %s\r\n\r\n", request.path, request.addr);
  if(DEBUG) printf("Get request: %s\n", getrequest);
  return getrequest;
}


/* Create TCP Socket
 * This function takes a request structure containing an IP, Port, and Path, and then connects to
 *      webserver with the corresponding IP and Port. 
 * A get request is sent through the TCP socket with the correct path.
 * The response of the GET request will be saved to a file and displayed in the Firefox Browser
 */
int create_tcp_socket(struct url request) {


	struct hostent *server;
	struct sockaddr_in server_address;

        // add info to structure
	server_address.sin_family = AF_INET; 
        server_address.sin_addr.s_addr = inet_addr(request.addr); 
        server_address.sin_port = htons(request.port);
        
        // connect to socket
        if(DEBUG) printf("about to create socket\n");
        int c_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (c_socket == -1) {
		perror("could not create socket");
		return -1;
	} else {
		if(DEBUG) printf("successfull created socket\n");
	}
	int address_length = sizeof(server_address);

        // connect to server
	if (connect(c_socket, (struct sockaddr *)&server_address, address_length) < 0) {
		 perror("Could not connect to server");
		 exit(1);
	} else {
          	if(DEBUG) printf("connection to server successful\n");
        }

        char* getrequest = format_get_request(request);
	
	int buffer_size = 1024;
	char buffer[buffer_size];
  
	//If testing, before writing we store the current time
	struct timespec sent;
	if(TESTING) clock_gettime(CLOCK_MONOTONIC, &sent);

	int w = write(c_socket, getrequest, strlen(getrequest));
        if (w < 0) {
          	perror("Could not write to socket");
          	exit(1);
        } else {
          	if(DEBUG) printf("Successful write to socket size %d\n", w);
        }
         
        // sets all the values in the buffer to 0
        bzero(buffer,buffer_size);

        //read from socket
	int bad_flag = 0; //0 means not bad
	char head_buffer[128];
        char html_buffer[1024];
        char gif_buffer[32768];
	int head = read(c_socket,head_buffer, 128);
	if (DEBUG)
	{ 
		for (int j = 0; j < head; j++) 
			printf("%c", head_buffer[j]);
	}
	char *buf;

	//parse the header and read in the .html file
	buf = strtok(head_buffer, "\n");
	while(!strstr(buf, "Content-Length"))
		buf = strtok(NULL, "\n");
	while(*buf != ' ') buf++;
	int read_len = atoi(buf);
	while(strncmp(buf, "\r", 3))
		buf = strtok(NULL, "\n");
	memcpy(html_buffer, buf + 2, head - (buf + 2 - head_buffer));
	int html = head - (buf + 2 - head_buffer);
	if(DEBUG) printf("%s\n", html_buffer);
	while(html < read_len)
		html += read(c_socket, html_buffer + html, read_len - html);

	//This checks to ensure the file wasn't delivered broken
	//with more time, this would be handled using the header response: 404 opposed to 200
	if(html > 256)
	{
		//parse in header and read in .gif file
		head = read(c_socket,head_buffer, 128);
		if(DEBUG) printf("%s\n", head_buffer);

		//Parses header to find Content length and ensure entire file has been read
		buf = strtok(head_buffer, "\n");
		while(!strstr(buf, "Content-Length"))
			buf = strtok(NULL, "\n");
		while(*buf != ' ') buf++;
		read_len = atoi(buf);
		while(strncmp(buf, "\r", 3))
			buf = strtok(NULL, "\n");
		memcpy(gif_buffer, buf + 2, head - (buf + 2 - head_buffer));
		int gif = head - (buf + 2 - head_buffer);
		while(gif < read_len)
			gif += read(c_socket, gif_buffer + gif, read_len - gif);
		FILE *htmlf = fopen("lab2.html", "w");

		if(!TESTING && DEBUG){
			printf("html:\n%s\n", html_buffer);
			printf("gif:\n");
			printf("raw data of %d bytes\n", gif);
			/*
			for(int i=0; i < gif; i++){
			printf("%X", gif_buffer[i]);
			}
			*/

		}
		fwrite(html_buffer, 1, html, htmlf);
		fclose(htmlf);

		FILE *giff = fopen("comic.gif", "w");
		fwrite(gif_buffer, 1, gif, giff);
		fclose(giff);
	}
	else
	{
		//Bad request was returned
		bad_flag = 1;
		printf("Bad request returned\n");
		FILE *badreq = fopen("client_error.html", "w");
		fwrite(html_buffer, 1, html, badreq);
		fclose(badreq);	
	}
	
	//For testing, need to store the time after we have received both files
	struct timespec rec;	
	if(TESTING)
	{
		clock_gettime(CLOCK_MONOTONIC, &rec);
		double elapsed = calculate_latency(rec, sent);
		FILE *latency = fopen("latency.txt", "a+");
		char elapsedstr[64];
		sprintf(elapsedstr,"%e\n", elapsed);
		fwrite(elapsedstr, strlen(elapsedstr), 1, latency);
		fclose(latency);
	}
	//If not testing, open the browser to show the page
	else
	{
		switch(fork()){
		case -1:
			err(1, "fork failed");
		case 0:
			close(2);
			if(bad_flag) execl("/usr/bin/firefox", "-new-window", "./client_error.html", NULL);
			else execl("/usr/bin/firefox", "-new-window", "./lab2.html", NULL);
		default:
			break;
		}
	}

	
        // close socket 
	close(c_socket);
        return 0;
}

/* Parse Url
 * This function takes a url string and parses it into the IP, Port, and Path
 *      which are saved in a structure and returned at the end.
 * This function will exit if an invalid URL is detected.
 */
struct url parse_url(char* request_url)
{
	struct url ret_url;
	
	char delim[] = ":/";
	//Will be http
	char* ptr;
	ptr = strtok(request_url, delim);
	
	if((ptr = strtok(NULL, delim)) == NULL) 
	{
		printf("Please use a valid URI\n");
		exit(1);
	}
        
	//Will be ip address
	ret_url.addr = ptr;
        printf("IP: %s\n", ptr);
        
	if((ptr = strtok(NULL, delim)) == NULL)
	{
		printf("Please use a valid URI\n");
		exit(1);
	}	 
	//Will be port
	ret_url.port = atoi(ptr);
        printf("Port: %s\n",ptr);
        
	if((ptr = strtok(NULL, delim)) == NULL)
	{

          ret_url.path = NULL;
          //printf("Please use a valid URI\n");
          //	exit(1);
	} else {
          
          ret_url.path = ptr;
          
        }

	return ret_url;
}


/* Create UDP Socket
 * This function takes a request structure containing an IP, Port, and Path, and then connects to
 *      webserver with the corresponding IP and Port. 
 * A get request is sent through the UDP socket with the correct path.
 * The response of the GET request will be saved to a file and displayed in the Firefox Browser
 */
int create_udp_socket(struct url request)
{ 
	struct hostent *server;
  	struct sockaddr_in serv_addr;

  	//filling all the characters of serveraddress to 0
  	memset(&serv_addr, 0, sizeof(serv_addr)); 

  	//structure containing an internet address
  	serv_addr.sin_family = AF_INET;  //code for address family
  	serv_addr.sin_port = htons(request.port);  //contain the port number, converted to network byte order
  	serv_addr.sin_addr.s_addr = inet_addr(request.addr); //storing host address

 	//creating socket 
 	int c_socket;

 	printf("creating socket \n");
 	c_socket = socket(AF_INET, SOCK_DGRAM, 0);
 	if(c_socket < 0) 
 	{
		 printf("ERROR opening socket");
 	}
 	else
 	{
   		 printf("connection successful \n");
 	}

	//buffers 
	char head_buffer[128];
  	char html_buffer[1024]; //for storing html data
	char gif_buffer[329999]; //for storing gif data

        char* getrequest = format_get_request(request);

	struct timespec sent;
	if(TESTING) clock_gettime(CLOCK_MONOTONIC, &sent);


	//requesting
	int sending_message = sendto(c_socket, getrequest, 1024, MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof(serv_addr)); 
	if(DEBUG) printf("sending: %d\n", sending_message);

	//recieving
	int bad_flag = 0;
	int head = recv(c_socket,head_buffer, 128, MSG_WAITALL);
//	for (int j = 0; j < head; j++){
//		printf(head_buffer[j]);
//	}
	char *buf;

	//parse the header and read in the .html file
	buf = strtok(head_buffer, "\n");
	while(!strstr(buf, "Content-Length"))
		buf = strtok(NULL, "\n");
	while(*buf != ' ') buf++;
	int read_len = atoi(buf);
	while(strncmp(buf, "\r", 3))
		buf = strtok(NULL, "\n");
	memcpy(html_buffer, buf + 2, head - (buf + 2 - head_buffer));
	int html = head - (buf + 2 - head_buffer);
	printf("%s\n", html_buffer);
	while(html < read_len)
		html += recv(c_socket, html_buffer + html, read_len - html, MSG_WAITALL);

	//Here we ensure we weren't given a broken file
	//with more time, this would be if we were given 404 rather than 200
	if(html > 256)
	{
		//parse in header and read in .gif file
		head = recv(c_socket,head_buffer, 128, MSG_WAITALL);
		if(DEBUG) printf("%s\n", head_buffer);

		//Parses header to find the length of content in the message
		buf = strtok(head_buffer, "\n");
		while(!strstr(buf, "Content-Length"))
			buf = strtok(NULL, "\n");
		while(*buf != ' ') buf++;
		read_len = atoi(buf);
		while(strncmp(buf, "\r", 3))
			buf = strtok(NULL, "\n");
		
		memcpy(gif_buffer, buf + 2, head - (buf + 2 - head_buffer));
		int gif = head - (buf + 2 - head_buffer);
		while(gif < read_len)
			gif += recv(c_socket, gif_buffer + gif, read_len - gif, MSG_WAITALL);
		//Reads in the html file and gif file and writes them into local client files
		FILE *htmlf = fopen("lab2.html", "w");
		fwrite(html_buffer, 1, html, htmlf);
		fclose(htmlf);

		FILE *giff = fopen("comic.gif", "w");
		fwrite(gif_buffer, 1, gif, giff);
		fclose(giff);
        }
	else
	{
		//Bad request was returned
		bad_flag = 1;
		printf("Bad request returned\n");
		FILE *badreq = fopen("client_error.html", "w");
		fwrite(html_buffer, 1, html, badreq);
		fclose(badreq);	
	}

	//For testing, need to store the time after we have received both files
	struct timespec rec;	
	if(TESTING)
	{
		clock_gettime(CLOCK_MONOTONIC, &rec);
		double elapsed = calculate_latency(rec, sent);
		FILE *latency = fopen("latency.txt", "a+");
		char elapsedstr[64];
		sprintf(elapsedstr,"%e\n", elapsed);
		fwrite(elapsedstr, strlen(elapsedstr), 1, latency);
		fclose(latency);
	}
	else{
		switch(fork()){
		case -1:
			err(1, "fork failed");
		case 0:
			close(2);
			if(bad_flag) execl("/usr/bin/firefox", "-new-window", "./client_error.html", NULL);
			else execl("/usr/bin/firefox", "-new-window", "./lab2.html", NULL);
		default:
			break;
		}
	}

  	close(c_socket);
	return 0;
}

/* Main 
 * This function takes the URL and TCP/UDP as command line arguments, and then calls
 *      create_tcp_socket, or create_udp_socket with the parsed URL.
 */
int main(int argc, char *argv[]) {

//This parses the client command line but for now it's easiest to do it the way we have been. 
	char* request_url;
	char* connection_type;
	switch(argc)
	{
        case 3:	request_url = argv[1];
          connection_type = argv[2];
          break;

        case 2:
          printf("Not enough arguments. Please format your request as follows: ./client http://[ip]:[port]/lab2.html [TCP/UDP]\n");
          return -1;
        case 1:
          printf("Not enough arguments. Please format your request as follows: ./client http://[ip]:[port]/lab2.html [TCP/UDP]\n");
          return -1;
        default:
          printf("Please format your request as follows: ./client http://[ip]:[port]/lab2.html [TCP/UDP]\n");
          return -1;
	}
	struct url request;
	request = parse_url(request_url);

	if(strcmp(connection_type, "TCP") == 0) create_tcp_socket(request);	
	else create_udp_socket(request);
	return 0;
}
