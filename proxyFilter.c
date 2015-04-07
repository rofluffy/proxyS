#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>

#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#define _GNU_SOURCE
#include <string.h>
#include <pthread.h>


#define BUFLEN 4096
#define LINEBUFLEN 64
#define DEFAULT_HOST_PORT "80"


char* str_to_lower(char* str){
	// to lower
	size_t i, len= strlen(str);
	for (i = 0; i<len; i++){
		str[i] = tolower((unsigned char) str[i]);
	}
	return str;
}


int parse_client_input(char ** line_buf, char ** hostname, char ** host_port, char ** absPath, int new_sd){
	char * http = NULL;
	char * ver = NULL;
	char * path = NULL;
	int hlen;

	ver = strstr(line_buf[0], "HTTP/"); //Pointer to http version.
	http = strstr(line_buf[0], "http://"); //Pointer to beggining of absolute uri

	if (strncmp(line_buf[0], "GET", 3) != 0){
	 	char *send_msg = "<html><head>HTTP/1.1 405 Method not allowed.\n\n</head></html>\n\n";
	  	send(new_sd, send_msg, strlen(send_msg), 0);
	} else if(ver == NULL){
	  	printf("No HTTP version specified.\n");
	} else {
		if(http != NULL){ //Absolute URI is used.
		  	http += + 7;
		  	// Get the pointer to path.
		  	if (strstr(http, "/") > ver) {
				path = strstr(http, " "); // Path is empty.
			} else {
				path = strstr(http, "/"); // Path is not empty.
			}
			// get the host
			hlen = (path - http);
			*hostname = malloc((hlen+1) * sizeof(char));
			strncpy(*hostname, http, hlen);
			(*hostname)[hlen] = 0; 
		} else { //Attempt to extract host from second line.
			path = strstr(line_buf[0], "GET ") + 4;
			*hostname = strcasestr(line_buf[1], "Host: ");
			if(*hostname == NULL){
				printf("No host.\n");
			} else {
				*hostname += 6;
				*hostname = strtok(*hostname, "\n");
			}
		}
		char * port_start = strstr(*hostname, ":");
		if (port_start != NULL){ //Port number is given, extract it.
			int portlen = *hostname + hlen - port_start;			
			if (portlen > 2){ //Port field is not empty.
				*host_port = malloc(portlen * sizeof(char));
				strncpy(*host_port, port_start+1, portlen);
			}
			*hostname = strtok(*hostname, ":");
		}
		// get the absPath
		// if it's empty, then should be "/"
		printf("Before plen.\n");
		int plen = ver - path - 1;
		printf("path: %s\n", path);
		*absPath = malloc((plen+1) * sizeof(char));
		strncpy(*absPath, path, plen);
		printf("absPath: %s\n", *absPath);
		(*absPath)[plen] = 0;
		if ((*absPath)[0] == ' '){
			(*absPath)[0] = '/';
		}
		return 0;
	} 
	return -1;
}
void send_and_save(int new_sd, char * message, size_t len, FILE * write_to){
	send(new_sd, message, len, 0);
	if (write_to != NULL){
		fprintf(write_to,message);
	}
}
void recv_from_host(int host_sd, int new_sd, FILE * cache_file){
    FILE * read_host = fdopen(host_sd, "r");
    printf("After open file.\n");
	char latest_line_read[BUFLEN];
	int lines_read = 0;
	size_t content_length = 0;
	char * response;
	//Read response and headers
	//host_line_buf[0] = (char *)malloc(BUFLEN);
	printf("Getting first line. . .");
	fgets(latest_line_read, BUFLEN, read_host);
	printf("done.\n");
	if(strstr(latest_line_read, "HTTP/1.1 ") == NULL){
		//Gateway error
		return;
	}
	printf("Line %d: %s", lines_read, latest_line_read);

	lines_read ++;
	char * response_code = strtok(latest_line_read+9, " ");
	FILE * write_to = NULL;
	if (response_code[0] == '2' || response_code[0] == '3'){
		//write_to = cache_file;
	}
	send_and_save(new_sd, latest_line_read, strlen(latest_line_read), write_to);
	int chunked = 0;
	while(lines_read < LINEBUFLEN){
		fgets(latest_line_read, BUFLEN, read_host);
		printf("Line %d: %s", lines_read, latest_line_read);
		send_and_save(new_sd, latest_line_read, strlen(latest_line_read), write_to);
		if (strcasestr(latest_line_read, "Transfer-Encoding: chunked") == latest_line_read){
			chunked ++;
		} else if (strcasestr(latest_line_read, "Content-Length: ") == latest_line_read){
			content_length = atoi(latest_line_read+16);
			printf("Content len: %d\n", content_length);

		}
		if(latest_line_read[0] == '\r' && latest_line_read[1] == '\n'){
			printf("Headers ended with host sd: %d\n", host_sd);
			break;
		}
		lines_read ++;
		memset(latest_line_read, 0, BUFLEN);
	}
	if (chunked == 1){
		while(1){
			char chunk_len_str[10];
			printf("Getting chunk size. . .");
			fgets(chunk_len_str, 10, read_host);
			size_t chunk_len = strtoul(chunk_len_str, NULL, 16);
			printf("Chunk size: %d, %s",chunk_len, chunk_len_str);
			send_and_save(new_sd, chunk_len_str, strlen(chunk_len_str), write_to);
			printf("Sent chunk size.\n");
			char chunk[chunk_len];
			size_t chunk_read = fread(chunk, 1, chunk_len+2, read_host);
			printf("Chunk is sent length: %d, %d\n", chunk_read, strlen(chunk));
			//printf("Chunk:\n%s", chunk);
			send_and_save(new_sd, chunk, chunk_read, write_to);
			printf("Sent chunk\n");
			if (chunk_len == 0){	
				break;
			}
		}
		//Get trailing headers.
		
	} else if (content_length > 0){
		response = (char *)malloc(content_length);
		printf("About to read body, host_sd: %d\n", host_sd);
		size_t recv_len = fread(response, 1, content_length, read_host);
		send_and_save(new_sd, response, content_length, write_to);
		printf("content len: %d, recv len: %d, strlen: %d, host sd: %d\n",content_length, recv_len, strlen(response), host_sd);
	}

	printf("Finished recieving from host.\n");
	close(host_sd);
}

int connect_to_host(char *hostname, char* host_port, char* absPath, int new_sd){
	//Connect to host.
	int host_sd, new_host_sd;
	struct sockaddr_storage host;
	struct addrinfo hints, *res, *rp;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;

	int status;
	if ((status = getaddrinfo(hostname, host_port, &hints, &res)) != 0) {
		printf("BAD REQUEST\n");
		char *send_msg = "<html><head>HTTP/1.1 400 BAD REQUEST.\n</head></html>\n";
		send(new_sd, send_msg, strlen(send_msg), 0);
		fprintf(stderr, "HTTP/1.1 400 BAD REQUEST.\n");
		return -1;
	}
	host_sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (connect(host_sd, res->ai_addr, res->ai_addrlen) != 0) {
		close(host_sd);
		perror("Can't connect to remote server..");
		return -1;
	}
	freeaddrinfo(res);
	printf("Returning host_sd\n");
	return host_sd;
	
}

/*
Computes the Pearson hash of a key.
*/
char * get_hash(char * key, int num_bytes){
	unsigned char *h = (char *)malloc(sizeof(char)*num_bytes);
	memset(h, 255, num_bytes);

	char * hash = malloc(2*sizeof(char)*num_bytes+1);
	memset(hash, 0, num_bytes);

	static const unsigned char permutation_table[256] = {99, 135, 111, 62, 240, 130, 49, 81, 254, 170, 175, 116, 157, 219, 88, 190, 178, 108, 69, 138, 60, 165, 21, 70, 87, 86, 73, 198, 90, 50, 162, 233, 171, 214, 9, 218, 177, 132, 220, 180, 174, 119, 252, 125, 97, 58, 206, 234, 100, 197, 231, 200, 106, 17, 105, 112, 148, 154, 57, 236, 131, 96, 121, 210, 243, 37, 38, 30, 89, 182, 245, 22, 43, 56, 66, 51, 79, 137, 28, 217, 36, 158, 128, 230, 41, 143, 2, 150, 225, 149, 133, 110, 136, 83, 151, 205, 166, 98, 55, 102, 244, 24, 126, 95, 32, 163, 160, 141, 45, 14, 124, 29, 247, 46, 179, 33, 241, 72, 221, 48, 123, 52, 232, 191, 27, 187, 77, 127, 184, 10, 104, 117, 39, 183, 113, 202, 213, 53, 199, 207, 167, 11, 169, 249, 251, 80, 226, 186, 228, 23, 85, 61, 209, 216, 203, 7, 64, 255, 129, 65, 173, 235, 84, 147, 68, 15, 122, 212, 71, 242, 42, 44, 101, 31, 246, 134, 75, 195, 3, 155, 176, 215, 208, 237, 67, 152, 250, 91, 181, 26, 227, 54, 172, 40, 142, 8, 168, 76, 13, 94, 223, 25, 107, 63, 114, 78, 16, 103, 47, 196, 156, 109, 115, 0, 5, 189, 120, 222, 92, 74, 146, 229, 204, 59, 35, 159, 19, 145, 12, 224, 194, 253, 161, 188, 185, 239, 144, 1, 93, 164, 20, 82, 140, 153, 211, 4, 248, 193, 192, 18, 139, 34, 201, 6, 238, 118};
	int i,j,k;
	for (i = 0; i < num_bytes; ++i)
	{
		h[i] = permutation_table[(key[0]+i)%256];
		for (j = 0; j < strlen(key); ++j)
		{
			h[i] = permutation_table[h[i]^key[j]]; 
		}
		sprintf(hash+2*i, "%x", h[i]);

	}

	return hash;
}

void get_data(char * hostname, char * host_port, char * absPath, int host_sd, int new_sd){
	char * key = malloc(strlen(hostname)+strlen(absPath)+1);
	sprintf(key, "%s:%s%s", hostname, host_port, absPath);
	printf("Key : %s\n", key);
	char * hashed = get_hash(key,8);
	printf("Hash: %s\n", hashed);
	char * cached_fname = (char *)malloc(strlen(hashed)+11);
	cached_fname[strlen(hashed)+10] = 0;
	sprintf(cached_fname, "cache/%s.txt",hashed);
	printf("Cache name: %s\n", cached_fname);
	FILE * cached = fopen(cached_fname, "r");
	if(cached != NULL){
		printf("Cache not NULL.\n");
		char * buff = NULL;
		size_t k = 0;
		while (getline(&buff, &k, cached) != -1){
			send(new_sd, buff, strlen(buff), 0);
		}
		free(buff);
		fclose(cached);
	} else {
		char * cached_temp_fname = (char *)malloc(strlen(hashed)+16);
		cached_temp_fname[strlen(hashed)+15] = 0;
		sprintf(cached_temp_fname, "cache/temp_%s.txt",hashed);
		FILE * cached_temp_read = fopen(cached_temp_fname, "r");
		if (cached_temp_read == NULL){
			FILE * cached_temp_write = fopen(cached_temp_fname, "w");
			printf("Cached temp NULL\n");
			printf("%s\n", cached_temp_fname);

			recv_from_host(host_sd, new_sd, cached_temp_write);
			fclose(cached_temp_write);
			rename(cached_temp_fname, cached_fname);
			free(cached_temp_fname);
		} else {
			recv_from_host(host_sd, new_sd, NULL);
			free(cached_temp_fname);
		}
	}

	free(key);
	free(hashed);
	free(cached_fname);
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
			printf("comparing hostname. (set)%d\n", contains_word);
		}

		compare = NULL;
		free(compare);
	}
	// set blbuff back to the start of the file
	blbuff = NULL;
	free(blbuff);
	rewind(blacklist);

	return contains_word;
}

struct thread_args{
	int new_sd;
	FILE * blacklist;
};
void * connect_to_client(void * args){
	int new_sd = ((struct thread_args *)args)->new_sd;
	FILE * blacklist = ((struct thread_args *)args)->blacklist;
	char * line_buf[LINEBUFLEN];
	int lines_read = 0;
	FILE *read_client = fdopen(new_sd, "r");
	size_t k = 0;
	printf("Before getline.\n");
	while(lines_read < LINEBUFLEN){
		line_buf[lines_read] = (char *)malloc(BUFLEN);
		fgets(line_buf[lines_read], BUFLEN, read_client);
		printf("Read Client Line %d: %s", lines_read, line_buf[lines_read]);
		if (line_buf[lines_read][0] == 0xd || line_buf[lines_read][0] == 0xa){
			break;
		}
		lines_read ++;
	}

	char *absPath = NULL;
	char *httpVer = NULL;
	char *hostname = NULL;
	char *host_port = DEFAULT_HOST_PORT;

 	if(parse_client_input(line_buf, &hostname, &host_port, &absPath, new_sd) == 0){
 		if (checkBlacklist(hostname, blacklist)){
 			//Forbidden.
 		} else {
	    	printf("hostname: %s, host_port: %s, absPath: %s\n", hostname, host_port, absPath); //Malloc error here.
	    	int host_sd = connect_to_host(hostname, host_port, absPath, new_sd);
	    	printf("Host sd: %d\n", host_sd);
	    	if (host_sd > 0){
		    	//Send the get request and host header tohost.
		    	char request[BUFLEN];
				sprintf(request, "GET %s HTTP/1.1\r\nHOST: %s:%s\r\n", absPath, hostname, host_port);
				printf("Request: \n%s\n", request);
				send(host_sd, request, strlen(request), 0);
				//Send the rest of the headers to host.
				int i = strcasestr(line_buf[1], "Host: ") == line_buf[1] ? 2 : 1; //Start sending after the host header.
		    	for(;i < lines_read; i ++){
		    		printf("Sending: %s", line_buf[i]);
		    		send(host_sd, line_buf[i], strlen(line_buf[i]), 0);
		    		free(line_buf[i]);
		    	}
		    	send(host_sd, "\r\n", 2, 0);
		    	//get_data(hostname, host_port, absPath, host_sd, new_sd);
		    	recv_from_host(host_sd, new_sd, NULL);
	    	}
	    }
    	free(absPath);
    	//free(hostname);
    }

	/* Clean up. */


	char* ending = "\nConnection closing by server.\n";
	//send(new_sd, ending, strlen(ending), 0);
	close(new_sd);
	pthread_exit(NULL);

}

int main(int argc, char **argv) {

	if (argc != 3){
		fprintf(stderr, "Usage: %s [port] [blacklist]\n", argv[0]);
		exit(1);
	}
	/*
	Get address information for local connection.
	*/
	FILE * blacklist = fopen(argv[2], "r");
	if (blacklist == NULL){
		fprintf(stderr,"Failed to open: %s\n", argv[2]);
		exit(1);
	}
	int client_len, sd, new_sd;
	struct sockaddr_storage client;
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int status;
	if ((status = getaddrinfo(NULL, argv[1], &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}

	/*
	Create socket for local connection and listen.
	*/
	sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol); 
	bind(sd, res->ai_addr, res->ai_addrlen);
	listen(sd,5);
	printf("After Listen.\n");
	while (1) {
		printf("In loop.\n");
		if ((new_sd = accept(sd, (struct sockaddr *)&client, &client_len)) == -1) {
			fprintf(stderr, "Can't accept client.\n");
			exit(1);
		}
		pthread_t new_thread;
		struct thread_args args;
		args.new_sd = new_sd;
		args.blacklist = blacklist;
		int rc  = pthread_create(&new_thread, NULL, connect_to_client, (void *)&args);

		
		
	}
	freeaddrinfo(res); 
	fclose(blacklist);
	close(sd);
	return 0;

}
