#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>

#include <string.h>

int main(int argc, char *argv[]) {
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	char buffer[256];

	if (argc < 2) {
		fprintf(stderr,"usage %s command\n", argv[0]);
		exit(0);
	}

	portno = 8101;

	/* Create a socket point */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	server = gethostbyname("localhost");

	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);

	/* Now connect to the server */
	if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR connecting");
		exit(1);
	}

	bzero(buffer, 256);
	for (int i = 1; i < argc; i++) {
		strcat(buffer, argv[i]);
		strcat(buffer, " ");
	}

	/* Send message to the server */
	n = write(sockfd, &buffer, 256);
	if (n < 0) {
		perror("ERROR writing to socket");
		exit(1);
	}

	/* Now read server response */
	bzero(buffer,256);
	n = read(sockfd, &buffer, 256);
	if (n < 0) {
		perror("ERROR reading from socket");
		exit(1);
	}
	if (buffer != NULL)
		printf("%s\n", buffer);
	return 0;
}
