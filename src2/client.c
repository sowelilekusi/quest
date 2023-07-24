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

	if (!strcmp(argv[1], "time")) {
		strcpy(buffer, "current_time");
	} else if (!strcmp(argv[1], "start")) {
		strcpy(buffer, "start");
	} else if (!strcmp(argv[1], "stop")) {
		strcpy(buffer, "stop");
	} else if (!strcmp(argv[1], "kill")) {
		strcpy(buffer, "kill");
	} else if (!strcmp(argv[1], "split")) {
		strcpy(buffer, "split");
	} else if (!strcmp(argv[1], "skip")) {
		strcpy(buffer, "skip");
	} else if (!strcmp(argv[1], "pause")) {
		strcpy(buffer, "pause");
	} else if (!strcmp(argv[1], "resume")) {
		strcpy(buffer, "resume");
	} else if (!strcmp(argv[1], "undo")) {
		strcpy(buffer, "undo");
	} else if (!strcmp(argv[1], "redo")) {
		strcpy(buffer, "redo");
	} else if (!strcmp(argv[1], "foreground")) {
		strcpy(buffer, "Foreground-Color");
	} else if (!strcmp(argv[1], "background")) {
		strcpy(buffer, "Background-Color");
	} else if (!strcmp(argv[1], "save")) {
		strcpy(buffer, "save");
	} else if (!strcmp(argv[1], "runs")) {
		strcpy(buffer, "run_count");
	} else if (!strcmp(argv[1], "segments")) {
		strcpy(buffer, "segment_count");
	} else if (!strcmp(argv[1], "start-split-stop")) {
		strcpy(buffer, "start-split-stop");
	} else if (!strcmp(argv[1], "pause-resume")) {
		strcpy(buffer, "pause-resume");
	} else if (!strcmp(argv[1], "start-stop")) {
		strcpy(buffer, "start-stop");
	} else if (!strcmp(argv[1], "start-split")) {
		strcpy(buffer, "start-split");
	} else if (!strcmp(argv[1], "split-stop")) {
		strcpy(buffer, "split-stop");
	} else if (!strcmp(argv[1], "undo-redo")) {
		strcpy(buffer, "undo-redo");
	} else {
		perror("No valid command given");
		exit(1);
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
	
	//read an int response
	if (!strcmp(argv[1], "time") || !strcmp(argv[1], "runs") || !strcmp(argv[1], "segments")) {
		int x = -1;
		x = *(int*)&buffer;
		if (n < 0) {
			perror("ERROR reading from socket");
			exit(1);
		}
		if (x != -1)
			printf("%d\n",x);
	}
	//read a string response
	else {
		if (n < 0) {
			perror("ERROR reading from socket");
			exit(1);
		}
		if (buffer != NULL)
			printf("%s", buffer);
	}
	return 0;
}
