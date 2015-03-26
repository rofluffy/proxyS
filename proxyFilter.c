#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>

#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#define BUFLEN 256
#define DEFAULT_PORT 80

char* str_to_lower(char* str){
	// to lower
	size_t i, len= strlen(str);
	for (i = 0; i<len; i++){
		str[i] = tolower((unsigned char) str[i]);
	}
	return str;
}

void handler(char *hostname, int portNum, char* absPath, char* httpVer, int new_sd){
	
	struct sockaddr_in host_addr;
	int hostsd, new_hostsd, i; 
	
	char request[BUFLEN];
	char response[BUFLEN];
	struct hostent* host;
	
	// get the host from host name
	host = gethostbyname(hostname);
	
	// create new socket on HOST, [port]
	/* Bind host address to the socket */
	memset((char *) &host_addr, 0, sizeof(struct sockaddr_in));
	bzero((char*) &host_addr, sizeof(host_addr));
	host_addr.sin_family = AF_INET;
	// if port exist, use that instead of default
	host_addr.sin_port = htons(portNum);  
	bcopy((char*)host->h_addr,(char*)&host_addr.sin_addr.s_addr,host->h_length);
		  
	/* Create a stream socket. */
	if ((hostsd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't create host socket.\n");
		//exit(1);
	}
		  
	if ((new_hostsd = connect(hostsd, (struct sockaddr*)&host_addr, sizeof(struct sockaddr))) == -1) {
		fprintf(stderr, "Can't connect to remote server.\n");
		//exit(1);
	}  
		  
	// send "GET absPath httpver" request to the socket
	bzero((char*)request,sizeof(request));
	sprintf(request, "GET %s %s\r\nHOST: %s:%d\r\n\r\n", absPath, httpVer, hostname, portNum);
		  
	i = send(hostsd, request, strlen(request), 0);
	printf("\n%s\n", request);
		  
	if (i < 0){
		fprintf(stderr, "Can't write to socket.\n");
	} else {
		do {
			bzero((char*)response,strlen(response));
			i = recv(hostsd, response, BUFLEN, 0);
			//response[strlen(response)] = 0;
			/*if (response == strstr(response, httpVer)){
					  char* temp = (char*) malloc(sizeof(response));
					  strcpy(temp, response);
					  token = strtok(temp, " ");
					  while( token != NULL ) {
						  printf("try: %s\n", token);
						  token = strtok(NULL, " ");
					  }
				  }*/
			if (!(i <= 0)){
				// print response in client
				response[strlen(response)] = 0;
				//send(new_hostsd, response, strlen(response), 0);
				send(new_sd, response, strlen(response), 0);
			}
			// print response in server
			//printf("%s", response);
		  
		} while(i > 0);
		char* ending = "\nConnection closing by server.\n";
		send(new_sd, ending, strlen(ending), 0);
			  
	}
		  
		  
	// close connection
	close(hostsd);
	close(new_hostsd);
	
}


int main(int argc, char **argv) {

    // This is some sample code feel free to delete it

    /*int i;

    for (i = 0; i < argc; i++) {
        printf("Arg %d is: %s\n", i, argv[i]);
    }

    return 0;*/
	
  int n, bytes_to_read;
  int sd, new_sd, client_len, port;
  struct sockaddr_in server, client;
  char *bp, buf[BUFLEN], outbuf[BUFLEN];
  
  char* err_msg;

  // running start up (have 2 argument, so one more to implement)
  /*if (argc != 3){
	  fprintf(stderr, "Usage: %s [port] %s [blacklist]\n", argv[0]);
      exit(1);
  }*/
  if (argc != 2){
	  fprintf(stderr, "Usage: %s [port]\n", argv[0]);
      exit(1);
  }
  port = atoi(argv[1]);

  /* Create a stream socket. */
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    fprintf(stderr, "Can't create a socket.\n");
    exit(1);
  }

  /* Bind an address to the socket */
  memset((char *) &server, 0, sizeof(struct sockaddr_in));
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(sd, (struct sockaddr *)&server, sizeof(server)) ==-1) {
    fprintf(stderr, "Can't bind name to socket.\n");
    exit(1);
  }

  /* Receive from the client. */
  listen(sd, 5);
  while (1) {
    memset(buf, 0, sizeof(buf));
    if ((new_sd = accept(sd, (struct sockaddr *)&client, &client_len)) == -1) {
      fprintf(stderr, "Can't accept client.\n");
      exit(1);
    }
	
	printf("\n-------Reserve connection-------\n\n");

    bp = buf;
    bytes_to_read = BUFLEN;
	

    while((n = read(new_sd, bp, bytes_to_read)) > 0) {
      if (*bp == '\n') {
        break;
      }
	  
	  char *checker = NULL;
	  char *http = NULL;
	  char *path = NULL;
	  char *ver = NULL;
	  char *flag = NULL;
	  
	  char *hostname = NULL;
	  char *absPath = NULL;
	  char *httpVer = NULL;
	  char *hostport = NULL;
	
	  
	  int portNum = DEFAULT_PORT;
	  
	  // get pointer to HTTPver
	  ver = strstr(bp, "HTTP/1.1");
	  checker = strstr(bp, "GET");
	  http = strstr(bp, "http://");
	  
	  if ((checker == bp) && ver != NULL){
		  
		  // get the HTTPver
		  int vlen = strrchr(bp, '\0') - ver - 1;
		  httpVer = malloc(vlen * sizeof(char));
		  //bzero((char*) httpVer, sizeof(httpVer));
		  strcpy(httpVer, ver);
		  httpVer[vlen] = 0;
		  
		  if (http == NULL){
			  
			  // case that requires 2 lines entry
			  // get path pointer
			  path = strstr(bp, "GET ") + 4;
			  bp += n;
			  bytes_to_read -= n;
			  
			  // read the next line
			  n = read(new_sd, bp, bytes_to_read);
			  
			  if (*bp == '\n'){
				  break;
			  }
			  
			  // check if next line is valid
			  http = strstr(bp, "HOST: ");
			  if (http == NULL){
				  // send err_msg to client
				  send(new_sd, "Require HOST request.\n", 23, 0);
			  }else {
				  http += 6;
			  }
			  
			  // get the hostname
			  int hlen = strrchr(bp, '\0') - http;
			  hostname = malloc(hlen * sizeof(char));
			  strncpy(hostname, http, hlen-1);
			  
			  
		  }else {
			  
			  // case with full URL including host
			  
			  // get the pointer to absPath
			  http += 7;
			  if (strstr(http, "/") > ver) {
				  path = strstr(http, " ");
			  }else {
				  path = strstr(http, "/");
			  }
			  
			  // get the hostname
			  int hlen = (path - http)+1;
			  hostname = malloc(hlen * sizeof(char));
			  strncpy(hostname, http, hlen-1);
			  
		  }
		  
		  // check for port
		  flag = strstr(hostname, ":");
		  if (flag != NULL){
			  int portlen = hostname + strlen(hostname) - flag;
			  hostport = malloc(portlen * sizeof(char));
			  strncpy(hostport, flag+1, portlen);
			  if (portlen > 2){
				  portNum = atoi(hostport);
			  }
			  hostname = strtok(hostname, ":");
		  }
		  hostname[strlen(hostname)] = 0;
		  
		  // get the absPath
		  // if it's empty, then should be "/"
		  int plen = (ver - path);
		  absPath = malloc(plen * sizeof(char));
		  //bzero((char*) absPath, sizeof(absPath));
		  strncpy(absPath, path, plen);
		  absPath[plen] = 0;
		  if (absPath[0] == ' '){
			  absPath[0] = '/';
		  }
		  
		  printf("GET %s %s\nHOST: %s\n", absPath, httpVer, hostname);
		  handler(hostname, portNum, absPath, httpVer, new_sd);
		  
		  // clean up
		  hostname = NULL;
		  absPath = NULL;
		  httpVer = NULL;
		  
		  free(hostname);
		  free(absPath);
		  free(httpVer);
		  free(hostport);
		  
		  // close connection
		  close(new_sd);
		  printf("\n-------Closing client-------\n\n\n");
		  
		  
		  
	  } else {
		 
		  err_msg = "HTTP/1.1 405 Method not allowed.\n\n";
		  send(new_sd, err_msg, strlen(err_msg), 0);
		  printf(err_msg);
		  printf("\n-------Closing client-------\n\n\n");
		  close(new_sd);
	  }
	  
      bp += n;
      bytes_to_read -= n;
	  
	  
    }

    printf("Received: %s\n", buf);
    /* Write to socket and send to the client. */
    //snprintf(outbuf, BUFLEN, "%d\n", (int) strlen(buf));
    //write(new_sd, outbuf, strlen(outbuf));
    //printf("Sent: %s\n", outbuf);

    /* Clean up. */
	//close(new_sd);
	//printf("\n-------Closing client-------\n\n\n");
  }

  close(sd);
  return 0;

}

