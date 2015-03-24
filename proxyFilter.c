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

	
  int n, bytes_to_read;
  int sd, new_sd, client_len, port;
  struct sockaddr_in server, client;
  char *bp, buf[BUFLEN], outbuf[BUFLEN];

  // running start up (have 2 argument, so one more to implement)
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

    bp = buf;
    bytes_to_read = BUFLEN;
	char *checker = NULL;
	char *http = NULL;
	char *path = NULL;
	char *ver = NULL;
	char *host = NULL;

    while((n = read(new_sd, bp, bytes_to_read)) > 0) {
      if (*bp == '\n') {
        break;
      }
	  /*
	  // get pointer to HTTPver
	  ver = strstr(bp, "HTTP");
	  // get the HTTPver
	  int vlen = strrchr(bp, '\0') - ver - 1;
	  char *httpVer = malloc(vlen * sizeof(char));
	  strcpy(httpVer, ver);
	  httpVer[vlen] = 0;
	*/
	  char *httpVer = NULL;
	  char *ver = NULL;




	  checker = strstr(bp, "GET"); 
	  if (strncmp(bp, "GET", 3) != 0){
		  printf("%s 405 Method not allowed.\n", httpVer);
	  } else if (strncmp(bp, "GET http://", 11) == 0){
		  // get the pointer to http://
		  http = strstr(bp, "http://") + 7;
		  path = strstr(http, "/");
		  /*
		  if (strstr(http, "/") > ver) {
			  path = strstr(http, " ");
		  }else {
			  path = strstr(http, "/");
		  }
		  
		  // Check statements
		  /*printf("Check bp: %s\n", bp);
		  printf("check http: %s\n", http);
		  printf("check HTTP: %s\n", ver);
		  printf("check path: %s\n", path);*/
		  
		  // get the host
		  size_t hlen = (path - http) + 1;
		  host = malloc(hlen * sizeof(char));
		  strncpy(host, http, hlen-1);
		  host[hlen] = 0;

		  
		  ver = strstr(path, " ");
		  

	  } else {
	  	printf("Error in URL.\n");
	  } if (strncmp(ver, " HTTP/1.1", 8) == 0){
	  	int vlen = strrchr(bp, '\0') - ver - 1;
	  	  char *httpVer = malloc(vlen * sizeof(char));
	  	  strcpy(httpVer, ver);
		  // get the absPath
		  // if it's empty, then should be "/"
		  int plen = (ver - path);
		  char *absPath = malloc(plen * sizeof(char));
		  strncpy(absPath, path, plen);
		  absPath[plen] = 0;
		  if (absPath[0] == ' '){
			  absPath[0] = '/';
		  }
		  printf("GET %s %s \nHOST: %s\n", absPath, httpVer, host);

		  struct addrinfo *servinfo;
		  struct addrinfo hints;
		  memset(&hints, 0, sizeof hints);
    	  hints.ai_family = AF_INET; // AF_INET or AF_INET6 to force version
    	  hints.ai_socktype = SOCK_STREAM;
		  int status;
		  if ((status = getaddrinfo(host, "80", NULL, &servinfo)) != 0) {
    		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    		exit(1);
		}
		int hostsd;
		if ((hostsd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    		fprintf(stderr, "Can't create a socket.\n");
    		exit(1);
    	}

    	connect(hostsd, (const struct addrinfo *)servinfo, servinfo->ai_addrlen);



	  } else {
	  	printf("Error in version.\n");
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
    close(new_sd);
	printf("after close new_sd\n");
  }

  close(sd);
  return 0;

}
