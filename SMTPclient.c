#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#define BUFF_SIZE 1024
#define EMAIL_SIZE 900

void die(char *issue) {
  perror(issue);
  exit(EXIT_FAILURE);
}


/* Prépare l'adresse du serveur */
void prepare_address( struct sockaddr_in *address, const char *host, int port ) {
  size_t addrSize = sizeof( address );
  memset(address, 0, addrSize);
  address->sin_family = AF_INET;
  inet_pton( AF_INET, (char*) host, &(address->sin_addr) );
  address->sin_port = htons(port);
}

/* Construit le socket client */
int makeSocket( const char *host, int port ) {
  struct sockaddr_in address;
  int sock = socket(PF_INET, SOCK_STREAM, 0);

  prepare_address( &address, host, port );
  if( connect(sock, (struct sockaddr *) &address, sizeof(address)) < 0) {
    die("Failed to connect with server");
  }
  return sock;
}

int connectToServer(const char *host, int port) {
  int sockfd;

  // this idiom's not mine
  if( ((sockfd = makeSocket( host, port )) < 0) &&
      ((sockfd = makeSocket( host, 587 )) < 0) &&
      ((sockfd = makeSocket( host, 25 )) < 0) &&
      ((sockfd = makeSocket( host, 465 )) < 0)) {
    die("Failed to create socket");
  }

  return sockfd;
}

int talkToServer(int sockfd, char *msg) {
  printf("\t%s", msg); // print the client's message

  if( write(sockfd, msg, strlen(msg)) < 0 ) {
    die( "Couldn't talk to the server." );
  }
  return 0;
}

int listenToServer(int sockfd, char *buf) {
  memset(buf, 0, BUFF_SIZE); // clear the buffer
  if( read( sockfd, buf, BUFF_SIZE ) < 0 ) { // read from the server
    die( "Couldn't listen to the server." );
  }
  printf("%s", buf); // print the server's message

  return 0;
}

int abortOnServerError(char *server_msg) {
  char error_code_string[3];
  int error_code;

  for(int i=0; i<3; i++){ // retrieves the first the characters (the error code)
    error_code_string[i] = server_msg[i];
  }

  error_code = atoi(error_code_string); // convert the string to an int

  if ((error_code <= 200) || (error_code >= 355)) { // abort if !(200 <= error_code <= 355)
    printf("Aborting on error %d.\n", error_code);
    exit(EXIT_FAILURE);
  }

  return 0;
}

int readEmailContent(char *email_filename, char *email_content) {
  // snippet found on the internet
  FILE *fp;
  fp = fopen(email_filename, "r");

  if (fp != NULL) {
      size_t newLen = fread(email_content, sizeof(char), EMAIL_SIZE, fp);
      if ( ferror( fp ) != 0 ) {
          die("Error reading file");
      } else {
          email_content[newLen++] = '\0'; /* Just to be safe. */
      }
    }
    return 0;
}

int sendMail(int sockfd, char *sender, char *receiver, \
              char *subject, char *email_filename) {
  char buf[BUFF_SIZE];
  char email_content[EMAIL_SIZE+1];

  listenToServer(sockfd, buf);

  talkToServer(sockfd, "HELO xx\n");
  listenToServer(sockfd, buf);
  abortOnServerError(buf);

  talkToServer(sockfd, "MAIL FROM: <");
  talkToServer(sockfd, sender);
  talkToServer(sockfd, ">\n");
  listenToServer(sockfd, buf);
  abortOnServerError(buf);

  talkToServer(sockfd, "RCPT TO: <");
  talkToServer(sockfd, receiver);
  talkToServer(sockfd, ">\n");
  listenToServer(sockfd, buf);
  abortOnServerError(buf);

  talkToServer(sockfd, "DATA\n");
  listenToServer(sockfd, buf);
  abortOnServerError(buf);


  readEmailContent(email_filename, email_content);

  sprintf(buf, "From: <%s>\n"
                "To: <%s>\n"
                "Subject: %s\n"
                "%s"
                ".\n", sender, receiver, subject, email_content);
  talkToServer(sockfd,  buf);
  listenToServer(sockfd, buf);
  abortOnServerError(buf);

  talkToServer(sockfd, "QUIT\n");
  listenToServer(sockfd, buf);
  abortOnServerError(buf);

  return 0;
}

int main(int argc, char *argv[]) {
  int sock;    /* Socket */
  char *host;  /* Adresse IP du serveur */
  int port;    /* Port du service */
  char *email_filename; /* Nom du fichier contenant le mail */
  char *sender;
  char *receiver;
  char *subject;

  /* Initialisation */

  if (argc != 7) {
    fprintf(stderr, "USAGE: %s <host> <port> <path/to/mail> <sender> <receiver> <subject>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  host = argv[1];
  port = atoi(argv[2]);
  email_filename = argv[3];
  sender = argv[4];
  receiver = argv[5];
  subject = argv[6];

  /* Connection */
  sock = connectToServer( host, port );

  /* Envoi du mail */
  sendMail(sock, sender, receiver, subject, email_filename);

  /* Libération des ressources */
  close(sock);

  exit(EXIT_SUCCESS);
}
