#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>

#include <string.h>
#include <time.h>
#include <stdbool.h>

#define NS_PER_S 1000000000

struct timespec finish, delta;
int pausedTime = 0;
bool timerActive = false;
bool paused = false;
bool alive = true;
bool runUnsaved = false;
int timerOffset = 0;
enum event_type {
	START,
	SPLIT,
	SKIP,
	PAUSE,
	RESUME,
	STOP
};
struct run_event {
	enum event_type type;
	struct timespec time;
};

char* current_category = NULL;
char* current_route = NULL;
struct run_event *run;
//Enough to hold a sm64 16 star, can realloc later
int runMaxLength = 12;
int runMarker    = 0;
int runMarker2   = 0;

//save file stuff
char *default_file_name = "untitled.quest";
int files = 0;
char **filePaths = NULL;
char **names, **values;
int valuecount;

//functions
void sub_timespec(struct timespec t1, struct timespec t2, struct timespec* td);
void offset_timespec(int milliseconds, struct timespec* t);
int timespecToMS(struct timespec t);
void extend_run();
void add_event(enum event_type t);
void start();
void stop();
void split();
void skip();
void addPauseTime();
void subtractPauseTime();
void undo();
void redo();
void pause_timer();
void resume();
void appendRunToFile();
void timespecToRFC3339(struct timespec t, char buf[]);
void loadFiles();
void addFile(char *path);
void sendTime(int sock);
void sendValue(int sock, char* name);
void doprocessing (int sock);


void sub_timespec(struct timespec t1, struct timespec t2, struct timespec* td)
{
	td->tv_nsec = t2.tv_nsec - t1.tv_nsec;
	td->tv_sec  = t2.tv_sec  - t1.tv_sec;
	if (td->tv_sec > 0 && td->tv_nsec < 0) {
		td->tv_nsec += NS_PER_S;
		td->tv_sec--;
	} else if (td->tv_sec < 0 && td->tv_nsec > 0) {
		td->tv_nsec -= NS_PER_S;
		td->tv_sec++;
	}
}

void offset_timespec(int milliseconds, struct timespec* t)
{
	//this should leave milliseconds with just the remainder
	int second_offset = milliseconds / 1000;
	milliseconds -= second_offset * 1000;
	
	int nanosecond_offset = milliseconds * 1000000;
	
	t->tv_nsec -= nanosecond_offset;
	if (t->tv_nsec < 0) {
		second_offset++;
		t->tv_nsec += 1000000000;
	}
	t->tv_sec -= second_offset;
}

int timespecToMS(struct timespec t)
{
	return (t.tv_nsec / 1000000) + (t.tv_sec * 1000);
}

void extend_run()
{
	runMaxLength *= 2;
	run = realloc(run, sizeof(struct run_event) * runMaxLength);
}

void add_event(enum event_type t)
{
	if (runMarker == runMaxLength)
		extend_run();
	run[runMarker].type = t;
	clock_gettime(CLOCK_REALTIME, &run[runMarker].time);
	if (t == START)
		offset_timespec(timerOffset, &run[runMarker].time);
	runMarker++;
	runMarker2 = runMarker;
}

void start()
{
	//TODO: Save the old run to the file before the new one starts,
	//the reason to do this here is it gives the runner a chance to undo
	//if they accidentally hit the stop button
	appendRunToFile();
	//TODO: Clear the run data first
	timerActive = true;
	add_event(START);
}

void stop()
{
	timerActive = false;
	add_event(STOP);
	//this makes sure the time clients recieve from time
	//requests match the time on the stop event
	finish = run[runMarker - 1].time;
	runUnsaved = true;
}

void split()
{
	add_event(SPLIT);
}

void skip()
{
	add_event(SKIP);
}

void addPauseTime()
{
	int pauseEvent = 0;
	for (int i = runMarker - 2; i >= 1; i--) {
		if (run[i].type == PAUSE) {
			pauseEvent = i;
			break;
		}
	}
	sub_timespec(run[pauseEvent].time, run[runMarker - 1].time, &delta);
	pausedTime += timespecToMS(delta);
}

void subtractPauseTime()
{
	int pauseEvent = 0;
	for (int i = runMarker - 1; i >= i; i--) {
		if (run[i].type == PAUSE) {
			pauseEvent = i;
			break;
		}
	}
	sub_timespec(run[pauseEvent].time, run[runMarker].time, &delta);
	pausedTime -= timespecToMS(delta);
}

void undo()
{
	if (runMarker > 0) {
		runMarker--;
		if (run[runMarker].type == STOP)
			timerActive = true;
		if (run[runMarker].type == START)
			timerActive = false;
		if (run[runMarker].type == PAUSE)
			paused = false;
		if (run[runMarker].type == RESUME) {
			paused = true;
			subtractPauseTime();
		}
	}
}

void redo()
{
	if (runMarker < runMarker2) {
		runMarker++;
		if (run[runMarker - 1].type == STOP)
			timerActive = false;
		if (run[runMarker - 1].type == START)
			timerActive = true;
		if (run[runMarker - 1].type == PAUSE)
			paused = true;
		if (run[runMarker - 1].type == RESUME) {
			paused = false;
			addPauseTime();
		}
	}
}

//this isnt just called pause() because that would overlap with <unistd.h>
void pause_timer()
{
	if (!paused) {
		add_event(PAUSE);
		paused = true;
	}
}

void resume()
{
	if (paused) {
		add_event(RESUME);
		paused = false;
		addPauseTime();
	}
}

void appendRunToFile()
{
	if (!runUnsaved)
		return;
	char* save_path = NULL;
	if (files <= 0)
		save_path = default_file_name;
	else
		save_path = filePaths[0];
	FILE* fp;

	fp = fopen(save_path, "a+");
	fprintf(fp, "%s\n", "Run");
	if (current_category != NULL) {
		fprintf(fp, "\t%s\n", "Category");
		fprintf(fp, "\t\t%s\n", current_category);
	}
	if (current_route != NULL) {
		fprintf(fp, "\t%s\n", "Route");
		fprintf(fp, "\t\t%s\n", current_route);
	}

	int i = 0;
	bool done = false;
	while (!done) {
		if (run[i].type == STOP) {
			done = true;
		}
		switch (run[i].type) {
			case START:
				fprintf(fp, "\t%s\n", "Start");
				break;
			case SPLIT:
				fprintf(fp, "\t%s\n", "Split");
				break;
			case SKIP:
				fprintf(fp, "\t%s\n", "Skip");
				break;
			case PAUSE:
				fprintf(fp, "\t%s\n", "Pause");
				break;
			case RESUME:
				fprintf(fp, "\t%s\n", "Resume");
				break;
			case STOP:
				fprintf(fp, "\t%s\n", "Stop");
				break;
		}
		if (i == 0) {
			char buf[25];
			timespecToRFC3339(run[i].time, buf);
			fprintf(fp, "\t\t%s\n", buf);
		}
		else {
			sub_timespec(run[i - 1].time, run[i].time, &delta);
			fprintf(fp, "\t\t%d\n", timespecToMS(delta));
		}
		i++;
	}

	fprintf(fp, "\n");
	fclose(fp);
	runUnsaved = false;
}

void timespecToRFC3339(struct timespec t, char buf[])
{
	const int tmpsize = 21;
	struct tm tm;
	gmtime_r(&t.tv_sec, &tm);
	strftime(buf, tmpsize, "%Y-%m-%d %H:%M:%S.", &tm);
	sprintf(buf + tmpsize - 1, "%03luZ", (t.tv_nsec / 1000000));
}

void loadFiles()
{
	FILE* fp;
	//TODO: for now we're just looking for the metadata values
	char buff[255];
	char buff2[255];
	
	for (int i = 0; i < files; i++) {
		printf("loading file: \"%s\"\n", filePaths[i]);
		fp = fopen(filePaths[i], "r");
		while(1) {
			if (!fgets(buff, 255, fp))
				break;
			if (buff[0] == '/' && buff[1] == '/' || buff[0] == '\n')
				continue;
			if (!strcmp(buff, "Segment\n") || !strcmp(buff, "Route\n"))
				break;
			if (!fgets(buff2, 255, fp))
				break;
			if (buff2[0] == '\t') {
				valuecount++;
				
				names = realloc(names, sizeof(char*) * valuecount);
				names[valuecount - 1] = malloc(strlen(buff));
				strncpy(names[valuecount - 1], buff, strlen(buff) - 1);
				names[valuecount - 1][strlen(buff)] = '\0';

				values = realloc(values, sizeof(char*) * valuecount);
				values[valuecount - 1] = malloc(strlen(buff2) - 1);
				strncpy(values[valuecount - 1], buff2 + 1, strlen(buff2) - 1);
				values[valuecount - 1][strlen(buff2)] = '\0';
			}
		}

		fclose(fp);
	}
	
	//Print metadata arrays
	for (int i = 0; i < valuecount; i++) {
		printf("%s | %s", names[i], values[i]);
	}
}

//TODO: eventually file loading should support loading multiple files
void addFile(char *path)
{
	files++;
	filePaths = realloc(filePaths, sizeof(char*) * files);
	filePaths[files - 1] = path;
	loadFiles();
}

void sendTime(int sock)
{
	int n, x;
	if (timerActive)
		clock_gettime(CLOCK_REALTIME, &finish);
	if (paused) {
		sub_timespec(run[0].time, run[runMarker - 1].time, &delta);
	} else {
		sub_timespec(run[0].time, finish, &delta);
	}
	x = timespecToMS(delta) - pausedTime;
	n = write(sock, &x, sizeof(int));

	if (n < 0) {
		perror("ERROR writing to socket");
		exit(1);
	}
}

void sendValue(int sock, char* name)
{
	int n, x;
	bool namefound = false;
	for(int i = 0; i < valuecount; i++) {
		if (!strcmp(names[i], name)) {
			x = i;
			namefound = true;
		}
	}
	if (namefound)
		n = write(sock, values[x], strlen(values[x]));
	else
		n = write(sock, "DATA NOT PRESENT", 17);

	if (n < 0) {
		perror("ERROR writing to socket");
		exit(1);
	}
}

void doprocessing (int sock)
{
	int n;
	char commandcode;
	n = read(sock,&commandcode,1);

	if (n < 0) {
		perror("ERROR reading from socket");
		exit(1);
	}
	if (commandcode == 1) {
		//printf("Recieved time command\n");
		sendTime(sock);
	} else if (commandcode == 2) {
		printf("Recieved start command\n");
		start();
	} else if (commandcode == 3) {
		printf("Recieved stop command\n");
		stop();
	} else if (commandcode == 4) {
		printf("Recieved kill command\n");
		alive = false;
	} else if (commandcode == 5) {
		printf("Recieved split command\n");
		split();
	} else if (commandcode == 6) {
		printf("Recieved skip command\n");
		skip();
	} else if (commandcode == 7) {
		printf("Recieved pause command\n");
		pause_timer();
	} else if (commandcode == 8) {
		printf("Recieved resume command\n");
		resume();
	} else if (commandcode == 9) {
		printf("Recieved undo command\n");
		undo();
	} else if (commandcode == 10) {
		printf("Recieved redo command\n");
		redo();
	} else if (commandcode == 11) {
		printf("Recieved request for foreground color\n");
		sendValue(sock, "Foreground-Color");
	} else if (commandcode == 12) {
		printf("Recieved request for background color\n");
		sendValue(sock, "Background-Color");
	} else if (commandcode == 13) {
		printf("Recieved save command\n");
		appendRunToFile();
	} else {
		printf("Recieved invalid command code, ignoring...\n");
	}
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, clilen;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	int n, pid;
	run = malloc(sizeof(struct run_event) * runMaxLength);
	//TODO: remove this file testing boilerplate
	if (argc > 1)
		addFile(argv[1]);

	/* First call to socket() function */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	/* Initialize socket structure */
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = 8101;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	/* Now bind the host address using bind() call.*/
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR on binding");
		exit(1);
	}

	listen(sockfd,5);
	clilen = sizeof(cli_addr);

	printf("Ready!\n");
	while (alive) {
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) {
			perror("ERROR on accept");
			exit(1);
		}
		doprocessing(newsockfd);
		close(newsockfd);
	}
	free(run);
	close(sockfd);
}

