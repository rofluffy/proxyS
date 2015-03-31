#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>

#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#define BUFLEN 256
#define DEFAULT_PORT 80
#define NUM_THREADS 4


char* str_to_lower(char* str){
	// to lower
	size_t i, len= strlen(str);
	for (i = 0; i<len; i++){
		str[i] = tolower((unsigned char) str[i]);
	}
	return str;
}

int checkBlacklist(char* hostname, FILE* blacklist){
	char* blbuff = NULL;
	size_t k = 0;
	int contains_word = 0;

	while (getline(&blbuff, &k, blacklist) != -1){
		char* compare;
		if (strstr(blbuff, "\n") != NULL){
		  	int clen = strstr(blbuff, "\n") - blbuff - 1;
		  	compare = malloc(clen * sizeof(char));
		  	strncpy(compare, blbuff, clen);
		} else {
		  	compare = (char*) malloc(strlen(blbuff));
		  	strcpy(compare, blbuff);
		}
		  	
		// printf("check compare: %s\n", compare);
		// printf("check lenght: %d\n", strlen(compare), compare);
		// printf("check null: %s and %s is %s\n", hostname, compare, strstr(hostname, compare));

		str_to_lower(compare);
		if (strstr(hostname, compare) != NULL){
		  	//printf("comparing hostname. %d\n", contains_word);
			contains_word = 1;
			//printf("comparing hostname. (set)%d\n", contains_word);
		}

		compare = NULL;
		free(compare);
	}

		  // can be deleted ↓

		  // while (fgets(blbuff, BUFLEN, blacklist) != NULL){
		  // 	//printf("check blbuff: %s", blbuff);
		  // 	char* compare;
		  // 	if (strstr(blbuff, "\n") != NULL){
		  // 		int clen = strstr(blbuff, "\n") - blbuff - 1;
		  // 		compare = malloc(clen * sizeof(char));
		  // 		strncpy(compare, blbuff, clen);
		  // 	} else {
		  // 		compare = (char*) malloc(strlen(blbuff));
		  // 		strcpy(compare, blbuff);
		  // 	}
		  	
		  // 	printf("check compare: %s\n", compare);
		  // 	printf("check lenght: %d\n", strlen(compare), compare);
		  // 	printf("check null: %s and %s is %s\n", hostname, compare, strstr(hostname, compare));

		  // 	str_to_lower(compare);
		  // 	if (strstr(hostname, compare) != NULL){
		  // 		printf("comparing hostname. %d\n", contains_word);
				// contains_word = 1;
				// printf("comparing hostname. (set)%d\n", contains_word);
		  // 	}

		  // 	compare = NULL;
		  // 	free(compare);
		  // }



		  // set blbuff back to the start of the file
	blbuff = NULL;
	free(blbuff);
	rewind(blacklist);

	return contains_word;
}

void handler(char *hostname, int portNum, char* absPath, char* httpVer, int new_socket){
	
	struct sockaddr_in host_addr;
	int hostsd, new_hostsd, i; 
	
	char request[BUFLEN];
	char response[BUFLEN];
	struct hostent* host;
	
	// get the host from host name
	host = gethostbyname(hostname);
	// ADD AN EXCEPTION THAT RETURNS HOST INVALID !!!!!!!!!!!!!!!!!!!!!!!
	
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
				send(new_socket, response, strlen(response), 0);
			}
			// print response in server
			//printf("%s", response);
		  
		} while(i > 0);
		char* ending = "\nConnection closing by server.\n";
		send(new_socket, ending, strlen(ending), 0);
			  
	}
		  
		  
	// close connection
	close(hostsd);
	close(new_hostsd);
	
}

struct client_data{
	int thread_id;
	FILE* blacklist;
	int s_sd;
	int n_sd;
	int client_len;
	struct sockaddr_in * client;
};

struct client_data cd_array[NUM_THREADS];

void* connectClient(void* client_args){
	// threading......
	//printf("Hey there the thread is created yoooo\n");

	// extract data from client_args
	struct client_data *curr_data;

	curr_data = (struct client_data *) client_args;
	int c_id = curr_data->thread_id;
	FILE* blacklist = curr_data->blacklist;
	int server_sock = curr_data->s_sd;

	// some initialize variables
	struct sockaddr_in client;
	int client_len, client_sock;

	client = *(curr_data->client);
	client_len = curr_data->client_len;
	client_sock = curr_data->n_sd;

	char* err_msg;
  	char bp[BUFLEN], buf[BUFLEN];

  	FILE* readClient;
    
	
	printf("\n-------Reserve connection-------\n\n");

  	printf("so thread %d has been created ----------------------\n", c_id);

  	// need to lock here?
  	// read with fdopen? using file descriptor
  	readClient = fdopen(client_sock, "r");
  	printf("check after readClient\n");
  	if (readClient == NULL){
  		fprintf(stderr, "cannot read...\n");
  	}

  	printf("check before fgets\n");

  	while (fgets(bp, BUFLEN, readClient) != NULL){

  		if (*bp == '\n'){
  			// close(client_sock);
  			break;
  		}

  		printf("start reading from client (%d)\n", c_id);
	  
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
		  	printf("(1)check path: %s\n", path);

		 	 //read the next line
		  	char temp[BUFLEN];
		  	fgets(temp, BUFLEN, readClient);
		  
		  	if (*temp == '\n'){
				  break;
		  	}
		  	
		  	// check if next line is valid
		  	http = strstr(temp, "HOST: ");
		  	if (http == NULL){
				  // send err_msg to client
				  send(client_sock, "Require HOST request.\n", 23, 0);
		  	}else {
				  http += 6;
		 	}
		  
		 	// get the hostname
		 	int hlen = strrchr(temp, '\0') - http;
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
		  	printf("(2)check path: %s\n", path);
		  	int plen = (ver - path);
		  	absPath = malloc(plen * sizeof(char));
		  	//bzero((char*) absPath, sizeof(absPath));
		  	strncpy(absPath, path, plen);
		 	absPath[plen] = 0;
		  	if (absPath[0] == ' '){
				  absPath[0] = '/';
		  	}
		  	
		  	printf("GET %s %s\nHOST: %s\n", absPath, httpVer, hostname);
		  	
		  	// check the hostname and return 403 if it should be block
		  	int contains_word = checkBlacklist(hostname, blacklist);
	
		  	
		  	// call handler to connect to the host
		  	if (contains_word == 0){
				  printf("call handler.");
				  handler(hostname, portNum, absPath, httpVer, client_sock);
		  	} else {
				  err_msg = "HTTP/1.1 403 Forbidden.\n\n";
				  send(client_sock, err_msg, strlen(err_msg), 0);
				  printf(err_msg);
		  	}
		  	//handler(hostname, portNum, absPath, httpVer, client_sock);
		  	
		  	// clean up (check later!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!)
		 	hostname = NULL;
		  	absPath = NULL;
		  	httpVer = NULL;
		  	
		  	free(hostname);
		  	free(absPath);
		  	free(httpVer);
		  	free(hostport);
		  	
		  	// close connection
		  	close(client_sock);
		  	printf("\n-------Closing client(%d)-------\n\n\n", c_id);
		  
		  
		  
	  	} else {
		 
			  err_msg = "HTTP/1.1 405 Method not allowed.\n\n";
		 	 send(client_sock, err_msg, strlen(err_msg), 0);
		 	 printf("%s, (%d)", err_msg, c_id);
		 	 printf("\n-------Closing client-------\n\n\n");
		 	 close(client_sock);
	  	}


  	}

  	close(readClient);

    printf("Received: %s\n", buf);
    pthread_exit(NULL);

}




int main(int argc, char **argv) {

    // the main function

  int sd, new_sd, client_len, port, *client_sd, *server_sd;
  struct sockaddr_in server, client;

  // avoid using global var so these stufffffff......
  // int n, bytes_to_read;
  // char *bp, buf[BUFLEN];

  // not used
  //char outbuf[BUFLEN];
  
  // char* err_msg;
  
  FILE *blacklist;

  // usage is port, blacklist
  if (argc != 3){
	  fprintf(stderr, "Usage: %s [port], %s, [blacklist]\n", argv[0]);
      exit(1);
  }
  port = atoi(argv[1]);
  blacklist = fopen(argv[2], "r");
  if (blacklist == NULL){
	  fprintf(stderr,"Failed to open: %s\n", argv[2]);
	  exit(1);
  }
  

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
  pthread_attr_t attr;
  int curr_id = 0;
  while(1){

  	if ((new_sd = accept(sd, (struct sockaddr *)&client, &client_len)) == -1) {
      fprintf(stderr, "Can't accept client.\n");
      exit(1);
    }
    pthread_t new_thread;
    struct client_data c_data;
    int rc;
    printf("creating thread......\n");
		c_data.thread_id = ++curr_id;
		c_data.blacklist = blacklist;
		c_data.s_sd = sd;
		c_data.n_sd = new_sd;
		c_data.client = &client;
		c_data.client_len = client_len;
		rc = pthread_create(&new_thread, NULL, connectClient, (void*) &c_data);
		if (rc) {
			printf("Fail to create thread (%d)\n", rc);
		}
  }
  //while (1) {
  /*
  	pthread_t clientTheard[NUM_THREADS];
  	pthread_attr_t attr;

	int id;
	server_sd = malloc(1);
	*server_sd = sd;
	int rc; //return code
	void *status;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	for (id = 0; id < NUM_THREADS; id++){
		printf("creating thread......\n");
		cd_array[id].thread_id = id;
		cd_array[id].blacklist = blacklist;
		cd_array[id].s_sd = *server_sd;
		rc = pthread_create(&clientTheard[id], &attr, connectClient, (void*) &cd_array[id]);
		if (rc) {
			printf("Fail to create thread (%d)\n", rc);
		}
	}

	// join thread
	pthread_attr_destroy(&attr);
	for (id = 0; id<NUM_THREADS; id++){
		rc = pthread_join(clientTheard[id], &status);
		if (rc){
			printf("Fail to join thread (%d)\n", rc);
		}
	}

	// char* err_msg;
	// int n, bytes_to_read;
 //  	char *bp, buf[BUFLEN];

 //  	memset(buf, 0, sizeof(buf));
    
 //    if ((new_sd = accept(sd, (struct sockaddr *)&client, &client_len)) == -1) {
 //      fprintf(stderr, "Can't accept client.\n");
 //      exit(1);
 //    }
	
	// printf("\n-------Reserve connection-------\n\n");

 //    bp = buf;
 //    bytes_to_read = BUFLEN;
	

 //    while((n = read(new_sd, bp, bytes_to_read)) > 0) {
 //      if (*bp == '\n') {
 //        break;
 //      }
	  
	//   char *checker = NULL;
	//   char *http = NULL;
	//   char *path = NULL;
	//   char *ver = NULL;
	//   char *flag = NULL;
	  
	//   char *hostname = NULL;
	//   char *absPath = NULL;
	//   char *httpVer = NULL;
	//   char *hostport = NULL;
	
	  
	//   int portNum = DEFAULT_PORT;
	  
	//   // get pointer to HTTPver
	//   ver = strstr(bp, "HTTP/1.1");
	//   checker = strstr(bp, "GET");
	//   http = strstr(bp, "http://");
	  
	//   if ((checker == bp) && ver != NULL){
		  
	// 	  // get the HTTPver
	// 	  int vlen = strrchr(bp, '\0') - ver - 1;
	// 	  httpVer = malloc(vlen * sizeof(char));
	// 	  //bzero((char*) httpVer, sizeof(httpVer));
	// 	  strcpy(httpVer, ver);
	// 	  httpVer[vlen] = 0;
		  
	// 	  if (http == NULL){
			  
	// 		  // case that requires 2 lines entry
	// 		  // get path pointer
	// 		  path = strstr(bp, "GET ") + 4;
	// 		  bp += n;
	// 		  bytes_to_read -= n;
			  
	// 		  // read the next line
	// 		  n = read(new_sd, bp, bytes_to_read);
			  
	// 		  if (*bp == '\n'){
	// 			  break;
	// 		  }
			  
	// 		  // check if next line is valid
	// 		  http = strstr(bp, "HOST: ");
	// 		  if (http == NULL){
	// 			  // send err_msg to client
	// 			  send(new_sd, "Require HOST request.\n", 23, 0);
	// 		  }else {
	// 			  http += 6;
	// 		  }
			  
	// 		  // get the hostname
	// 		  int hlen = strrchr(bp, '\0') - http;
	// 		  hostname = malloc(hlen * sizeof(char));
	// 		  strncpy(hostname, http, hlen-1);
			  
			  
	// 	  }else {
			  
	// 		  // case with full URL including host
			  
	// 		  // get the pointer to absPath
	// 		  http += 7;
	// 		  if (strstr(http, "/") > ver) {
	// 			  path = strstr(http, " ");
	// 		  }else {
	// 			  path = strstr(http, "/");
	// 		  }
			  
	// 		  // get the hostname
	// 		  int hlen = (path - http)+1;
	// 		  hostname = malloc(hlen * sizeof(char));
	// 		  strncpy(hostname, http, hlen-1);
			  
	// 	  }
		  
	// 	  // check for port
	// 	  flag = strstr(hostname, ":");
	// 	  if (flag != NULL){
	// 		  int portlen = hostname + strlen(hostname) - flag;
	// 		  hostport = malloc(portlen * sizeof(char));
	// 		  strncpy(hostport, flag+1, portlen);
	// 		  if (portlen > 2){
	// 			  portNum = atoi(hostport);
	// 		  }
	// 		  hostname = strtok(hostname, ":");
	// 	  }
	// 	  hostname[strlen(hostname)] = 0;
		  
	// 	  // get the absPath
	// 	  // if it's empty, then should be "/"
	// 	  int plen = (ver - path);
	// 	  absPath = malloc(plen * sizeof(char));
	// 	  //bzero((char*) absPath, sizeof(absPath));
	// 	  strncpy(absPath, path, plen);
	// 	  absPath[plen] = 0;
	// 	  if (absPath[0] == ' '){
	// 		  absPath[0] = '/';
	// 	  }
		  
	// 	  printf("GET %s %s\nHOST: %s\n", absPath, httpVer, hostname);
		  
	// 	  // check the hostname and return 403 if it should be block
	// 	  int contains_word = checkBlacklist(hostname, blacklist);

		  
	// 	  // call handler to connect to the host
	// 	  if (contains_word == 0){
	// 		  printf("call handler.");
	// 		  handler(hostname, portNum, absPath, httpVer, new_sd);
	// 	  } else {
	// 		  err_msg = "HTTP/1.1 403 Forbidden.\n\n";
	// 		  send(new_sd, err_msg, strlen(err_msg), 0);
	// 		  printf(err_msg);
	// 	  }
	// 	  //handler(hostname, portNum, absPath, httpVer, new_sd);
		  
	// 	  // clean up (check later!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!)
	// 	  hostname = NULL;
	// 	  absPath = NULL;
	// 	  httpVer = NULL;
		  
	// 	  free(hostname);
	// 	  free(absPath);
	// 	  free(httpVer);
	// 	  free(hostport);
		  
	// 	  // close connection
	// 	  close(new_sd);
	// 	  printf("\n-------Closing client-------\n\n\n");
		  
		  
		  
	//   } else {
		 
	// 	  err_msg = "HTTP/1.1 405 Method not allowed.\n\n";
	// 	  send(new_sd, err_msg, strlen(err_msg), 0);
	// 	  printf(err_msg);
	// 	  printf("\n-------Closing client-------\n\n\n");
	// 	  close(new_sd);
	//   }
	  
 //      bp += n;
 //      bytes_to_read -= n;
	  
	  
 //    }

    //printf("Received: %s\n", buf);
    /* Write to socket and send to the client. */
    //snprintf(outbuf, BUFLEN, "%d\n", (int) strlen(buf));
    //write(new_sd, outbuf, strlen(outbuf));
    //printf("Sent: %s\n", outbuf);

    /* Clean up. */
	//close(new_sd);
	//printf("\n-------Closing client-------\n\n\n");
  //}

  // close file blacklist
  close(blacklist);
  close(sd);
  // exit the thread
  pthread_exit(NULL);

  return 0;

}
