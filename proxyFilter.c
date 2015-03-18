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
  printf("Check before the while loop.\n");
  while (1) {
    memset(buf, 0, sizeof(buf));
    if ((new_sd = accept(sd, (struct sockaddr *)&client, &client_len)) == -1) {
      fprintf(stderr, "Can't accept client.\n");
      exit(1);
    }
	printf("after new_sd if statement\n");

    bp = buf;
    bytes_to_read = BUFLEN;
    int i = 0;
	char *checker = NULL;
	char *http = NULL;
	char *path = NULL;
	char *ver = NULL;

    while((n = read(new_sd, bp, bytes_to_read)) > 0) {
      if (*bp == '\n') {
        break;
      }
	  
	  // get pointer to HTTPver
	  ver = strstr(bp, "HTTP");
	  // get the HTTPver
	  int vlen = strrchr(bp, '\0') - ver - 1;
	  char *httpVer = malloc(vlen * sizeof(char));
	  strcpy(httpVer, ver);
	  httpVer[vlen] = 0;
	  
	  checker = strstr(bp, "GET"); 
	  if (checker != bp){
		  printf("%s 405 Method not allowed.\n", httpVer);
	  } else {
		  // get the pointer to http://
		  http = strstr(bp, "http://");
		  
		  // get the pointer to absPath
		  http += 7;
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
		  int hlen = (path - http) + 1;
		  char *host = malloc(hlen * sizeof(char));
		  strncpy(host, http, hlen-1);
		  host[hlen] = 0;
		  
		  // get the absPath
		  // if it's empty, then should be "/"
		  int plen = (ver - path);
		  char *absPath = malloc(plen * sizeof(char));
		  strncpy(absPath, path, plen);
		  absPath[plen] = 0;
		  if (absPath[0] == ' '){
			  absPath[0] = '/';
		  }
		  
		  // Check statements
		  /*printf("check host: %s\n", host);
		  printf("check absPath: %s\n", absPath);
		  printf("check HTTPver: %s\n", httpVer);*/
		  
		  printf("GET %s %s \nHOST: %s\n", absPath, httpVer, host);
		  
		  //GET http://www.w3.org/pub/WWW/TheProject.html HTTP/1.1
		  //GET http://www.check.com/file/path HTTP/1.3  
		  //GET http://www.check.com HTTP/1.3  
		  //POST http://www.w3.org/pub/WWW/TheProject.html HTTP/1.1
		  
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
