#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>

#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#define BUFLEN 256
#define DEFAULT_PORT 80

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
  
  printf("Before listen. \n");

  /* Receive from the client. */
  listen(sd, 5);
  while (1) {
    memset(buf, 0, sizeof(buf));
    if ((new_sd = accept(sd, (struct sockaddr *)&client, &client_len)) == -1) {
      fprintf(stderr, "Can't accept client.\n");
      exit(1);
    }
	printf("after new_sd if statement\n");

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
	  char *port = NULL;
	
	  struct sockaddr_in host_addr;
	  int hostsd, new_hostsd, i; 
	
	  char request[BUFLEN];
	  char response[BUFLEN];
	  char *token;
	  struct hostent* host;
	  int portNum = DEFAULT_PORT;
	  
	  // get pointer to HTTPver
	  ver = strstr(bp, "HTTP/1.1");
 
	  checker = strstr(bp, "GET");

	  http = strstr(bp, "http://");
	  
	  if ((checker == bp) && ver != NULL){
		   // get the pointer to http://
		  //http = strstr(bp, "http://");
		  
		  // get the pointer to absPath
		  http += 7;
		  if (strstr(http, "/") > ver) {
			  path = strstr(http, " ");
		  }else {
			  path = strstr(http, "/");
		  }
		  
		  // get the host
		  // if port exist, then extract it
		  int hlen = (path - http)+1;
		  char *hostname = malloc(hlen * sizeof(char));
		  //bzero((char*) hostname, sizeof(hostname));
		  printf("check hlen: %d\n", hlen);
		  strncpy(hostname, http, hlen-1);
		  flag = strstr(hostname, ":");
		  if (flag != NULL){
			  int portlen = hostname + hlen - flag;
			  port = malloc(portlen * sizeof(char));
			  strncpy(port, flag+1, portlen);
			  if (portlen > 2){
				  portNum = atoi(port);
			  }
			  hostname = strtok(hostname, ":");
		  }
		  hostname[strlen(hostname)] = 0;
		  host = gethostbyname(hostname);
		  
		  // get the absPath
		  // if it's empty, then should be "/"
		  int plen = (ver - path);
		  char *absPath = malloc(plen * sizeof(char));
		  //bzero((char*) absPath, sizeof(absPath));
		  strncpy(absPath, path, plen);
		  absPath[plen] = 0;
		  if (absPath[0] == ' '){
			  absPath[0] = '/';
		  }
		  
		  // get the HTTPver
		  int vlen = strrchr(bp, '\0') - ver - 1;
		  char *httpVer = malloc(vlen * sizeof(char));
		  //bzero((char*) httpVer, sizeof(httpVer));
		  strcpy(httpVer, ver);
		  httpVer[vlen] = 0;

		  printf("GET %s %s\nHOST: %s\n", absPath, httpVer, hostname);
		  
		  // Check statements
		  /*printf("check host: %s\n", host);
		  printf("check absPath: %s\n", absPath);
		  printf("check HTTPver: %s\n", httpVer);*/
		  //printf("check (0)");
		  // create new socket on HOST, [port]
		  /* Bind host address to the socket */
		  memset((char *) &host_addr, 0, sizeof(struct sockaddr_in));
		  //printf("check (1)");
		  bzero((char*) &host_addr, sizeof(host_addr));
		  //printf("check (2)");
		  host_addr.sin_family = AF_INET;
		  //printf("check (3)");
		  // if port exist, use that instead of default
		  host_addr.sin_port = htons(portNum);  
		  //printf("check (4)");
		  bcopy((char*)host->h_addr,(char*)&host_addr.sin_addr.s_addr,host->h_length);
		  //printf("check (5)");
		  //host_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		  
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
		  //sprintf(request, "GET %s %s\r\nHOST: %s\r\nConnection: close\r\n\r\n", absPath, httpVer, hostname);
		  bzero((char*)request,sizeof(request));
		  if (port != NULL){
			  sprintf(request, "GET %s %s\r\nHOST: %s:%s\r\n\r\n", absPath, httpVer, hostname, port);
		  }else{
			  sprintf(request, "GET %s %s\r\nHOST: %s\r\n\r\n", absPath, httpVer, hostname);
		  }
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
					  // print response
					  send(new_hostsd, response, strlen(response), 0);
				  }
				  response[strlen(response)] = 0;
				  printf("%s", response);
				  
			  } while(i > 0);
		  }
		  // get response
		  //printf("\nResponse: %s\n", response);
		  //fprintf(stdout, "\nResponse: %s\n", response);
		  
		  // clean up
		  bzero((char*) hostname, sizeof(hostname));
		  bzero((char*) absPath, sizeof(absPath));
		  bzero((char*) httpVer, sizeof(httpVer));
		  free(hostname);
		  free(absPath);
		  free(httpVer);
		  free(port);
		  printf("clean up freeing memory.");
		  
		  // close connection
		  close(hostsd);
		  close(new_hostsd);
		  close(new_sd);
		  printf("-------Client closing-------\n\n");
		  
	  } else {
		 
		  printf("HTTP/1.1 405 Method not allowed.\n");
	  }
	  
	  // http:// host restofURL [port] HTTPver
	  // HTTPver 405 Method not allowed
	  
      bp += n;
      bytes_to_read -= n;
	  
	  
    }

    printf("Received: %s\n", buf);
    /* Write to socket and send to the client. */
    snprintf(outbuf, BUFLEN, "%d\n", (int) strlen(buf));
    write(new_sd, outbuf, strlen(outbuf));
    printf("Sent: %s\n", outbuf);

    /* Clean up. */
    //close(new_sd);
	//printf("after close new_sd\n");
  }

  close(sd);
  return 0;

}
