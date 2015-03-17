#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>

#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#define BUFLEN 256

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

    while((n = read(new_sd, bp, bytes_to_read)) > 0) {
      if (*bp == '\n') {
        break;
      }
	  
	  checker = strstr(bp, "GET");
	  if (checker != bp){
		  printf("405\n");
	  } else {
		  http = strstr(bp, "http://");
		  http += 7;
		  path = strstr(http, "/");
		  
		  printf("Check bp: %s\n", bp);
		  printf("check http: %s\n", http);
		  printf("check /: %s\n", path);
		  
		  printf("check http: %d\n", &http);
		  
		  printf("check /: %d\n", &path);
		  
		  printf("check host: %d\n", &http-&path);
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
