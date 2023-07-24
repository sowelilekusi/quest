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
	char commandcode;

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

	if (!strcmp(argv[1], "time")) {
		commandcode = 1;
	} else if (!strcmp(argv[1], "start")) {
		commandcode = 2;
	} else if (!strcmp(argv[1], "stop")) {
		commandcode = 3;
	} else if (!strcmp(argv[1], "kill")) {
		commandcode = 4;
	} else if (!strcmp(argv[1], "split")) {
		commandcode = 5;
	} else if (!strcmp(argv[1], "skip")) {
		commandcode = 6;
	} else if (!strcmp(argv[1], "pause")) {
		commandcode = 7;
	} else if (!strcmp(argv[1], "resume")) {
		commandcode = 8;
	} else if (!strcmp(argv[1], "undo")) {
		commandcode = 9;
	} else if (!strcmp(argv[1], "redo")) {
		commandcode = 10;
	} else if (!strcmp(argv[1], "foreground")) {
		commandcode = 11;
	} else if (!strcmp(argv[1], "background")) {
		commandcode = 12;
	} else if (!strcmp(argv[1], "save")) {
		commandcode = 13;
	} else if (!strcmp(argv[1], "runs")) {
		commandcode = 14;
	} else if (!strcmp(argv[1], "segments")) {
		commandcode = 15;
	} else if (!strcmp(argv[1], "start-split-stop")) {
		commandcode = 16;
	} else if (!strcmp(argv[1], "pause-resume")) {
		commandcode = 17;
	} else if (!strcmp(argv[1], "start-stop")) {
		commandcode = 18;
	} else if (!strcmp(argv[1], "start-split")) {
		commandcode = 19;
	} else if (!strcmp(argv[1], "split-stop")) {
		commandcode = 20;
	} else if (!strcmp(argv[1], "undo-redo")) {
		commandcode = 21;
	} else {
		perror("No valid command given");
		exit(1);
	}

	/* Send message to the server */
	n = write(sockfd, &commandcode, 1);

	if (n < 0) {
		perror("ERROR writing to socket");
		exit(1);
	}

	/* Now read server response */
	//bzero(buffer,256);
	
	//read an int response
	if (commandcode < 11 || commandcode == 14 || commandcode == 15) {
		int x = -1;
		n = read(sockfd, &x, sizeof(int));

		if (n < 0) {
			perror("ERROR reading from socket");
			exit(1);
		}

		if (x != -1)
			printf("%d\n",x);
	}
	//read a string response
	else {
		bzero(buffer,256);
		n = read(sockfd, &buffer, 255);

		if (n < 0) {
			perror("ERROR reading from socket");
			exit(1);
		}

		if (buffer != NULL)
			printf("%s", buffer);
	}
	return 0;
}
