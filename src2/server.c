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
bool hasUndoneAtLeastOnce = false;
bool runUnsaved = false;
int timerOffset = 0;
enum event_type {
	START,
	SPLIT,
	SKIP,
	PAUSE,
	RESUME,
	RESET,
	STOP
};
struct run_event {
	enum event_type type;
	struct timespec time;
};
struct segment {
	char *shortname;
	char *longname;
	char *description;
};
struct route {
	char *name;
	int segment_count;
	struct segment *segments;
};

struct run_event *run;
//Enough to hold a sm64 16 star, can realloc later
int runMaxLength = 12;
int runMarker    = 0;
int runMarker2   = 0;

//save file stuff
char *default_file_name = "untitled.quest";
int run_count = 0;
int files = 0;
char **filePaths = NULL;
char **meta_keys, **meta_values;
int valuecount;
struct segment *segments;
int segment_count = 0;
struct route *routes;
int route_count = 0;
struct route current_route;

//functions
void sub_timespec(struct timespec t1, struct timespec t2, struct timespec* td);
void offset_timespec(int milliseconds, struct timespec* t);
int timespecToMS(struct timespec t);
void extend_run();
void add_event(enum event_type t);
void appendRunToFile();
void timespecToRFC3339(struct timespec t, char buf[]);
void loadFiles();
void add_segment(char *sname, char *lname, char *desc);
void addFile(char *path);
void sendInt(int sock, int value);
void sendValue(int sock, char* name);
void sendString(int sock, char* str);
void process_socket_input(int sock);
void addPauseTime();
void subtractPauseTime();
void set_metadata(char *key, char *value);
void save_metadata_to_file(char *token, char *token2);
void reset_timer();
int current_ms();

//basic timer commands
void start();
void split();
void stop();
void skip();
void undo();
void redo();
void pause_timer();
void resume();
void reset();

//convenient combination commands
void start_split_stop();
void start_reset();
void start_split();
void split_stop();
void start_stop();
void pause_resume();
void undo_redo();


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
	hasUndoneAtLeastOnce = false;
}

void reset_timer()
{
	pausedTime = 0;
	runMarker = 0;
	runMarker2 = 0;
}

void start()
{
	if (timerActive) return;
	//Save the old run to the file before the new one starts,
	//the reason to do this here is it gives the runner a chance to undo
	//if they accidentally hit the stop button
	appendRunToFile();
	reset_timer();
	timerActive = true;
	add_event(START);
}

void stop()
{
	if (!timerActive) return;
	timerActive = false;
	add_event(STOP);
	//this makes sure the time clients recieve from time
	//requests match the time on the stop event
	finish = run[runMarker - 1].time;
	runUnsaved = true;
}

//Identical function to stop() but with a RESET event
void reset()
{
	if (!timerActive) return;
	timerActive = false;
	add_event(RESET);
	finish = run[runMarker - 1].time;
	runUnsaved = true;
}

void start_split_stop()
{
	if (!timerActive) {
		start();
	} else {
		if (runMarker < current_route.segment_count) {
			split();
		} else {
			stop();
		}
	}
}

void start_split()
{
	if (!timerActive) start();
	else split();
}

void split_stop()
{
	if (runMarker < current_route.segment_count) split();
	else stop();
}

void start_stop()
{
	if (!timerActive) start();
	else stop();
}

void start_reset()
{
	if (!timerActive) start();
	else reset();
}

void split()
{
	if (!timerActive) return;
	add_event(SPLIT);
}

void skip()
{
	if (!timerActive) return;
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
		if (run[runMarker].type == STOP) {
			timerActive = true;
			runUnsaved = false;
		}
		if (run[runMarker].type == START)
			timerActive = false;
		if (run[runMarker].type == PAUSE)
			paused = false;
		if (run[runMarker].type == RESUME) {
			paused = true;
			subtractPauseTime();
		}
		hasUndoneAtLeastOnce = true;
	}
}

void redo()
{
	if (!timerActive) return;
	if (runMarker < runMarker2) {
		runMarker++;
		if (run[runMarker - 1].type == STOP) {
			timerActive = false;
			runUnsaved = true;
			finish = run[runMarker - 1].time;
		}
		if (run[runMarker - 1].type == START)
			timerActive = true;
		if (run[runMarker - 1].type == PAUSE)
			paused = true;
		if (run[runMarker - 1].type == RESUME) {
			paused = false;
			addPauseTime();
		}
	}
	if (runMarker == runMarker2) {
		hasUndoneAtLeastOnce = false;
	}
}

void undo_redo()
{
	if (hasUndoneAtLeastOnce) redo();
	else undo();
}

//this isnt just called pause() because that would overlap with <unistd.h>
void pause_timer()
{
	if (!timerActive) return;
	if (!paused) {
		add_event(PAUSE);
		paused = true;
	}
}

void resume()
{
	if (!timerActive) return;
	if (paused) {
		add_event(RESUME);
		paused = false;
		addPauseTime();
	}
}

void pause_resume()
{
	if (paused) resume();
	else pause_timer();
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
	if (current_route.name != NULL) {
		fprintf(fp, "\t%s\n", "Route");
		fprintf(fp, "\t\t%s\n", current_route.name);
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
			case RESET:
				fprintf(fp, "\t%s\n", "Reset");
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
	run_count++;
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
	char buff[255];
	char buff2[255];
	
	for (int i = 0; i < files; i++) {
		fp = fopen(filePaths[i], "r+");
		while(1) {
			if (!fgets(buff, 255, fp))
				break;
			if (buff[0] == '/' && buff[1] == '/' || buff[0] == '\n' || buff[0] == '\t')
				continue;
			if (!strcmp(buff, "Segment\n")) {
				char *s = NULL;
				char *l = NULL;
				char *d = NULL;
				for (int x = 0; x < 3; x++) {
					if (!fgets(buff2, 255, fp))
						break;
					if (!strcmp(buff2, "\tShortname\n")) {
						if (!fgets(buff2, 255, fp))
							break;
						s = malloc(strlen(buff2) - 2);
						s = strncpy(s, buff2 + 2, strlen(buff2) - 2);
						s[strlen(s) - 1] = '\0';
					} else if (!strcmp(buff2, "\tLongname\n")) {
						if (!fgets(buff2, 255, fp))
							break;
						l = malloc(strlen(buff2) - 2);
						l = strncpy(l, buff2 + 2, strlen(buff2) - 2);
						l[strlen(l) - 1] = '\0';
					} else if (!strcmp(buff2, "\tDescription\n")) {
						if (!fgets(buff2, 255, fp))
							break;
						d = malloc(strlen(buff2) - 2);
						d = strncpy(d, buff2 + 2, strlen(buff2) - 2);
						d[strlen(d) - 1] = '\0';
					}
				}
				add_segment(s, l, d);
				continue;
			}
			if (!strcmp(buff, "Route\n"))
				continue;
			if (!strcmp(buff, "Run\n")) {
				run_count++;
				continue;
			}
			if (!fgets(buff2, 255, fp))
				break;
			if (buff2[0] == '\t') {
				buff[strlen(buff) - 1] = '\0';
				buff2[strlen(buff2) - 1] = '\0';
				set_metadata(buff, buff2 + 1);
			}
		}

		fclose(fp);
	}
}

void add_segment(char *sname, char *lname, char *desc)
{
	segment_count++;
	segments = realloc(segments, sizeof(struct segment) * segment_count);
	segments[segment_count - 1].shortname = sname;
	segments[segment_count - 1].longname = lname;
	segments[segment_count - 1].description = desc;
}

//TODO: eventually file loading should support loading multiple files
void addFile(char *path)
{
	files++;
	filePaths = realloc(filePaths, sizeof(char*) * files);
	filePaths[files - 1] = path;
	loadFiles();
}

int current_ms()
{
	if (timerActive)
		clock_gettime(CLOCK_REALTIME, &finish);
	if (paused) {
		sub_timespec(run[0].time, run[runMarker - 1].time, &delta);
	} else {
		sub_timespec(run[0].time, finish, &delta);
	}
	return timespecToMS(delta) - pausedTime;	
}

void sendInt(int sock, int value)
{
	char buffer[256];
	sprintf(buffer, "%d", value);
	int n = write(sock, &buffer, 256);
	if (n < 0) {
		perror("ERROR writing to socket");
		exit(1);
	}
}

void sendValue(int sock, char* name)
{
	char buffer[256];
	int n, x;
	bool namefound = false;
	if (name == NULL) {
		strcpy(buffer, "DATA NOT PRESENT");
	} else {
		for(int i = 0; i < valuecount; i++) {
			if (!strcmp(meta_keys[i], name)) {
				x = i;
				namefound = true;
			}
		}
		if (namefound)
			strcpy(buffer, meta_values[x]);
		else
			strcpy(buffer, "DATA NOT PRESENT");
	}
	n = write(sock, &buffer, 256);
	if (n < 0) {
		perror("ERROR writing to socket");
		exit(1);
	}
}

void sendString(int sock, char* str)
{
	char buffer[256];
	strcpy(buffer, str);
	int n = write(sock, &buffer, 256);
	if (n < 0) {
		perror("ERROR writing to socket");
		exit(1);
	}
}

void set_metadata(char *key, char *value)
{
	char key_pos = -1;
	for (int i = 0; i < valuecount; i++)
		if (!strcmp(meta_keys[i], key))
			key_pos = i;
	if (key_pos > -1) {
		meta_values[key_pos] = realloc(meta_values[key_pos], strlen(value));
		strncpy(meta_values[key_pos], value, strlen(value));
		meta_values[key_pos][strlen(value)] = '\0';
	} else {
		valuecount++;
				
		meta_keys = realloc(meta_keys, sizeof(char*) * valuecount);
		meta_keys[valuecount - 1] = malloc(strlen(key));
		strncpy(meta_keys[valuecount - 1], key, strlen(key));
		meta_keys[valuecount - 1][strlen(key)] = '\0';

		meta_values = realloc(meta_values, sizeof(char*) * valuecount);
		meta_values[valuecount - 1] = malloc(strlen(value));
		strncpy(meta_values[valuecount - 1], value, strlen(value));
		meta_values[valuecount - 1][strlen(value)] = '\0';
	}
}

void save_metadata_to_file(char *token, char *token2)
{
	char* save_path = NULL;
	if (files <= 0)
		save_path = default_file_name;
	else
		save_path = filePaths[0];
	FILE* fp;

	fp = fopen(save_path, "r+");
	fprintf(fp, "%s\n", token);
	fprintf(fp, "\t%s\n\n", token2);
	fclose(fp);
}

void process_socket_input(int sock)
{
	int n;
	char buffer[256];
	n = read(sock, &buffer, 256);
	if (n < 0) {
		perror("ERROR reading from socket");
		exit(1);
	}
	char *token = strtok(buffer, " ");

	//Imperative commands
	if (!strcmp(token, "start")) {
		start();
	} else if (!strcmp(token, "stop")) {
		stop();
	} else if (!strcmp(token, "reset")) {
		reset();
	} else if (!strcmp(token, "kill")) {
		alive = false;
	} else if (!strcmp(token, "split")) {
		split();
	} else if (!strcmp(token, "skip")) {
		skip();
	} else if (!strcmp(token, "pause")) {
		pause_timer();
	} else if (!strcmp(token, "resume")) {
		resume();
	} else if (!strcmp(token, "undo")) {
		undo();
	} else if (!strcmp(token, "redo")) {
		redo();
	} else if (!strcmp(token, "save")) {
		appendRunToFile();
	} else if (!strcmp(token, "start-split-stop")) {
		start_split_stop();
	} else if (!strcmp(token, "pause-resume")) {
		pause_resume();
	} else if (!strcmp(token, "start-stop")) {
		start_stop();
	} else if (!strcmp(token, "start-reset")) {
		start_reset();
	} else if (!strcmp(token, "start-split")) {
		start_split();
	} else if (!strcmp(token, "split-stop")) {
		split_stop();
	} else if (!strcmp(token, "undo-redo")) {
		undo_redo();
	
	//Getters
	} else if (!strcmp(token, "get")) {
		token = strtok(NULL, " ");
		if (!strcmp(token, "current_time")) {
			sendInt(sock, current_ms());
		} else if (!strcmp(token, "run_count")) {
			sendInt(sock, run_count);
		} else if (!strcmp(token, "segment_count")) {
			sendInt(sock, segment_count);
		} else if (!strcmp(token, "route_count")) {
			sendInt(sock, route_count);
		} else if (!strcmp(token, "event_count")) {
			sendInt(sock, runMarker);
		} else if (!strcmp(token, "segment_shortname")) {
			token = strtok(NULL, " ");
			int x = atoi(token);
			sendString(sock, segments[x].shortname);
		} else if (!strcmp(token, "segment_longname")) {
			token = strtok(NULL, " ");
			int x = atoi(token);
			sendString(sock, segments[x].longname);
		} else if (!strcmp(token, "segment_description")) {
			token = strtok(NULL, " ");
			int x = atoi(token);
			sendString(sock, segments[x].description);
		} else if (!strcmp(token, "route_name")) {
			token = strtok(NULL, " ");
			int x = atoi(token);
			sendString(sock, routes[x].name);
		} else if (!strcmp(token, "route_segment_count")) {
			token = strtok(NULL, " ");
			int x = atoi(token);
			sendInt(sock, routes[x].segment_count);
		} else if (!strcmp(token, "route_segment_shortname")) {
			token = strtok(NULL, " ");
			int x = atoi(token);
			token = strtok(NULL, " ");
			int y = atoi(token);
			sendString(sock, routes[x].segments[y].shortname);
		} else if (!strcmp(token, "event_time")) {
			token = strtok(NULL, " ");
			int x;
			if (!strcmp(token, "last"))
				x = runMarker - 1;
			else if (!strcmp(token, "first"))
				x = 0;
			else
				x = atoi(token);
			struct timespec t;
			sub_timespec(run[0].time, run[x].time, &t);
			sendInt(sock, timespecToMS(t));
		} else if (!strcmp(token, "event_type")) {
			token = strtok(NULL, " ");
			int x;
			if (!strcmp(token, "last"))
				x = runMarker - 1;
			else if (!strcmp(token, "first"))
				x = 0;
			else
				x = atoi(token);
			char *reply;
			switch (run[x].type) {
				case START:
					reply = "START";
					break;
				case SPLIT:
					reply = "SPLIT";
					break;
				case SKIP:
					reply = "SKIP";
					break;
				case PAUSE:
					reply = "PAUSE";
					break;
				case RESUME:
					reply = "RESUME";
					break;
				case RESET:
					reply = "RESET";
					break;
				case STOP:
					reply = "STOP";
					break;
			}
			sendString(sock, reply);
		} else if (!strcmp(token, "meta")) {
			token = strtok(NULL, " ");
			sendValue(sock, token);
		}
	
	//Setters
	} else if (!strcmp(token, "set")) {
		token = strtok(NULL, " ");
		if (!strcmp(token, "meta")) {
			token = strtok(NULL, " ");
			char *token2 = strtok(NULL, " ");
			set_metadata(token, token2);
			save_metadata_to_file(token, token2);
		}
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
		process_socket_input(newsockfd);
		close(newsockfd);
	}
	free(run);
	close(sockfd);
}

