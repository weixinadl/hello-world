/* http_server.c - http 1.0 server  */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include "config.h"
//#include "helpers.h"
#include "httpreq.h"

#include <sys/stat.h>

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>


/*------------------------------------------------------------------------
 * Program:   http server
 *
 * Purpose:   allocate a socket and then repeatedly execute the following:
 *              (1) wait for the next connection from a client
 *              (2) read http request, reply to http request
 *              (3) close the connection
 *              (4) go back to step (1)
 *
 * Syntax:    http_server [ port ]
 *
 *               port  - protocol port number to use
 *
 * Note:      The port argument is optional.  If no port is specified,
 *            the server uses the port specified in config.h
 *
 *------------------------------------------------------------------------
 */
bool Parse_HTTP_Request(int socket, struct http_request * request_values) ;
bool Is_Valid_Resource(char * URI) ;
void Send_Resource(int socket, char * URI) ;
int main(int argc, char *argv[])
{
  struct  sockaddr_in serv_addr; /* structure to hold server's address  */
  int     listen_socket, connection_socket;
  int     port;
  pid_t   pid;  /* id of child process to handle request */
  char    response_buffer[MAX_HTTP_RESP_SIZE];
  int     status_code;
  char *  status_phrase;

  /* 1) Create a socket */

  listen_socket = socket(AF_INET, SOCK_STREAM, 0);
  /* Check command-line argument for port and extract    */
  /* port number if one is specified.  Otherwise, use default  */

  if (argc > 1) {                 /* if argument specified        */
    port = atoi(argv[1]);   /* convert from string to integer   */
  } else {
    port = DEFAULT_PORT;
  }

  if (port <= 0) {             /* test for legal value       */
    fprintf(stderr, "bad port number %d", port);
    exit(EXIT_FAILURE);
  }

  /* 2) Set the values for the server  address structure:  serv_addr */
  memset(&serv_addr,0,sizeof(serv_addr)); /* clear sockaddr structure */

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(8080);

  /* 3) Bind the socket to the address information set in serv_addr */
  if(bind(listen_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
  {
    perror("bind failed. Error");
    return 1;
  }

  puts("bind success");

  /* 4) Start listening for connections */
  listen(listen_socket, (int)QLEN);
  puts("listen success");
  /* Main server loop - accept and handle requests */

  while (true) {

    /* 5) Accept a connection */
    connection_socket = accept(listen_socket, NULL, NULL);

    /* Fork a child process to handle this request */

    if ((pid = fork()) == 0) {

      /*----------START OF CHILD CODE----------------*/
      /* we are now in the child process */

      /* child does not need access to listen_socket */
      if ( close(listen_socket) < 0) {
    	fprintf(stderr, "child couldn't close listen socket");
    	exit(EXIT_FAILURE);
      }

      struct http_request new_request; // defined in httpreq.h
      /* 6) call helper function to read the request         *
       * this will fill in the struct new_request for you *
       * see helper.h and httpreq.h                       */
      bool aPrint = Parse_HTTP_Request(connection_socket, &new_request);

      /*** PART A:  PRINT OUT
       *   URI, METHOD and return value of  Parse_HTTP_Request()
       */
      printf("%s\n%s\n%d\n", new_request.URI, new_request.method, (int)aPrint);
      /****  PART B ONLY *******/
      /* 7) Decide which status_code and reason phrase to return to client */
          status_code = 200;
          status_phrase = "OK";
      // set the reply to send
      sprintf(response_buffer, "HTTP/1.0 %d %s\r\n", status_code, status_phrase);
      printf("Sending response line: %s\n", response_buffer);
      send(connection_socket,response_buffer,strlen(response_buffer),0);

      // send resource if requested, under what condition will the server send an
      // entity body?

      if (Is_Valid_Resource(new_request.URI)/** FILL IN CONDITION **/)
	    Send_Resource(connection_socket, new_request.URI);
      else {
	    // don't need to send resource.  end HTTP headers
	    send(connection_socket, "\r\n\r\n", strlen("\r\n\r\n"), 0);
      }


      /***** END PART B  ****/

      /* child's work is done, close remaining descriptors and exit */

      if ( close(connection_socket) < 0) {
	    fprintf(stderr, "closing connected socket failed");
    	exit(EXIT_FAILURE);
      }

      /* all done return to parent */
      exit(EXIT_SUCCESS);

    }
    /*----------END OF CHILD CODE----------------*/

    /* back in parent process  */
    /* close parent's reference to connection socket, */
    /* then back to top of loop waiting for next request */
    if ( close(connection_socket) < 0) {
      fprintf(stderr, "closing connected socket failed");
      exit(EXIT_FAILURE);
    }

    /* if child exited, wait for resources to be released */
    waitpid(-1, NULL, WNOHANG);

  } // end while(true)
}


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "config.h"
#include "httpreq.h"

/*----------------------------------------------------------
 * Function: Parse_HTTP_Request
 *
 * Purpose:  Reads HTTP request from a socket and fills in a data structure
 *           (see httpreq.h with the request values (method and URI)
 *
 * Parameters:  socket         : the socket to read the request from
 *              request_value  : address of struct to write the values to
 *
 * Returns:  true if successfull, false if request is not a valid HTTP request
 *
 *-----------------------------------------------------------
 */

bool Parse_HTTP_Request(int socket, struct http_request * request_values) {

  char buffer[MAX_HTTP_REQ_SIZE];
  char request[MAX_HTTP_REQ_SIZE];
  ssize_t recvdBytes;

  // read request
  request[0] = '\0';
  do {
    recvdBytes = recv(socket, buffer, sizeof(buffer), 0);
    if (recvdBytes > 0) {
      strncat(request, buffer, recvdBytes);
    }
  } while (recvdBytes > 0 && (strstr(request, "\r\n\r\n") == NULL));
  printf("received request: %s\n", request);

  // parse request
  char *line, *method;
  char *line_ptr;

  line = strtok_r(request, "\r\n", &line_ptr);

  method = strtok(line, " ");
  request_values->method = malloc (strlen(method) + 1);
  printf("Method is: %s\n", method);
  if (method == NULL)
    return false;
  strcpy(request_values->method, method);

  // parse the requested URI
  char * request_URI = strtok(NULL, " ");
  printf("URI is: %s\n", request_URI);
  if (request_URI == NULL)
    return false;
  request_values->URI = malloc (strlen(request_URI)+1);
  strcpy(request_values->URI, request_URI);

  char * version =  strtok(NULL, " ");
  if (version == NULL)
    return false;
  printf("version is: %s\n", version);

  // we can ignore headers, so just check that the blank line exists
  if ((strstr(request, "\r\n\r\n") == NULL))
    return true;
  else
    return false;
}

/*----------------------------------------------------------
 * Function: Is_Valid_Resource
 *
 * Purpose:  Checks if URI is a valid resource
 *
 * Parameters:  URI  : the URI of the requested resource, both absolute
 *                     and relative URIs are accepted
 *
 * Returns:  false : the URI does not refer to a resource on the server
 *           true  : the URI is available on the server
 *
 *-----------------------------------------------------------
 */

bool Is_Valid_Resource(char * URI) {

  char * server_directory, * location;
  char * resource;

  /* set the root server directory */

  if ( (server_directory = (char *) malloc(PATH_MAX)) != NULL)
    getcwd(server_directory, PATH_MAX);

  /* remove http://domain/ from URI */

  resource = strstr(URI, "http://");
  if (resource == NULL) {
    /* no http:// check if first character is /, if not add it */
    if (URI[0] != '/')
      resource = strcat("/", URI);
    else
      resource = URI;
  }
  else
    /* if http:// resource must start with '/' */
    resource = strchr(resource, '/');

  if (resource == NULL)
    /* invalid resource format */
    return false;

  /* append root server directory *
   * for example if request is for /images/myphoto.jpg          *
   * and directory for server resources is /var/www/            *
   * then the resource location is /var/www/images/myphoto.jpg  */

  strcat(server_directory, RESOURCE_PATH);
  location = strcat(server_directory, resource);
  printf("server resource location: %s\n", location);

  /* check file access */

  if (!(access(location, R_OK))) {
    puts("access OK\n");
    free(server_directory);
    return true;
  } else {
    puts("access failed\n");
    free(server_directory);
    return false;
  }
}


/*----------------------------------------------------------
 * Function: Send_Resource
 *
 * Purpose:  Sends the contents of the file referred to in URI on the socket
 *
 * Parameters:  socket  : the socket to send the content on
 *                URI   : the Universal Resource Locator, both absolute and
 *                        relative URIs are accepted
 *
 * Returns:  void - errors will cause exit with error printed to stderr
 *
 *-----------------------------------------------------------
 */

void Send_Resource(int socket, char * URI) {

  char * server_directory,  * resource;
  char * location;

  /* set the root server directory */

  if ( (server_directory = (char *) malloc(PATH_MAX)) != NULL)
    getcwd(server_directory, PATH_MAX);

  /* remove http://domain/ from URI */
  resource = strstr(URI, "http://");
  if (resource == NULL) {
    /* no http:// check if first character is /, if not add it */
    if (URI[0] != '/')
      resource = strcat("/", URI);
    else
      resource = URI;
  }
  else
    /* if http:// resource must start with '/' */
    resource = strchr(resource, '/');

  /* append root server directory *
   * for example if request is for /images/myphoto.jpg          *
   * and directory for server resources is /var/www/            *
   * then the resource location is /var/www/images/myphoto.jpg  */

  strcat(server_directory, RESOURCE_PATH);
  location = strcat(server_directory, resource);
  /* open file and send contents on socket */

  FILE * file = fopen(location, "r");

  if (file < 0) {
    fprintf(stderr, "Error opening file.\n");
    exit(EXIT_FAILURE);
  }

  char c;
  long sz;
  char content_header[MAX_HEADER_LENGTH];

  /* get size of file for content_length header */
  fseek(file, 0L, SEEK_END);
  sz = ftell(file);
  rewind(file);

  sprintf(content_header, "Content-Length: %ld\r\n\r\n", sz);
  printf("Sending headers: %s\n", content_header);
  send(socket, content_header, strlen(content_header), 0);

  printf("Sending file contents of %s\n", location);
  free(server_directory);

  while ( (c = fgetc(file)) != EOF ) {
    if ( send(socket, &c, 1, 0) < 1 ) {
      fprintf(stderr, "Error sending file.");
      exit(EXIT_FAILURE);
    }
    printf("%c", c);
  }
  puts("\nfinished reading file\n");
  fclose(file);
}
