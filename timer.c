#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <uiohook.h>
#include <wchar.h>
#include <fcntl.h>
#include <sys/stat.h>

#define NS_PER_S 1000000000
#define K_START 1
#define K_STOP  2
#define K_PAUSE 3
#define K_SPLIT 4

struct keymap
{
	uint16_t START;
	uint16_t STOP;
	uint16_t PAUSE;
	uint16_t SPLIT;
};

char buf;
int pipefd[2];
struct timespec start, finish, delta;
char *gameTitle, *catagoryTitle;
char *splitPath;
struct keymap km;

bool timerActive;

void loadKeymap();
void startTimer();
void stopTimer();
void splitTimer();
void handleInput();

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

void add_timespec(struct timespec t1, struct timespec t2, struct timespec* td)
{	
	td->tv_nsec = t2.tv_nsec + t1.tv_nsec;
	td->tv_sec  = t2.tv_sec  + t1.tv_sec;
	if (td->tv_nsec < 0) {
		td->tv_nsec += NS_PER_S;
		td->tv_sec++;
	}
}


bool logger_proc(unsigned int level, const char *format, ...) {
	return 0;
}
void dispatch_proc(uiohook_event * const event) {
	switch (event->type) {
		case EVENT_KEY_PRESSED:
			if (event->data.keyboard.keycode == VC_R)
				buf = K_START;
			if (event->data.keyboard.keycode == VC_F)
				buf = K_STOP;
			if (event->data.keyboard.keycode == VC_D)
				buf = K_PAUSE;
			if (event->data.keyboard.keycode == VC_E)
				buf = K_SPLIT;
			write(pipefd[1], &buf, 1);
		default:
			break;
	}
}

void handleInput()
{
	if (read(pipefd[0], &buf, 1) == -1)
		return;
	switch(buf) {
	case K_SPLIT:
		splitTimer();
		break;
	case K_START:
		startTimer();
		break;
	case K_STOP:
		stopTimer();
		break;
	case K_PAUSE:
		break;
	}
}

void ncDisplay()
{
	erase();
	int minutes = (int)delta.tv_sec / 60;
	int hours   = minutes / 60;
	printw("%2d:%02d:%02d.%ld\n", hours, minutes, (int)delta.tv_sec%60, delta.tv_nsec/1000000);
	refresh();
}

void startTimer()
{
	if (timerActive)
		return;
	clock_gettime(CLOCK_REALTIME, &start);
	timerActive = true;
}

void stopTimer()
{
	if (!timerActive)
		return;
	timerActive = false;
}

void splitTimer()
{

}

void loadKeymap()
{
	char path[256];
	strcat(strcpy(path, getenv("HOME")), "/.config/qtimer");
	mkdir(path, 0777);
	strcat(strcpy(path, getenv("HOME")), "/.config/qtimer/keymaps");
	mkdir(path, 0777);
	strcat(strcpy(path, getenv("HOME")), "/.config/qtimer/keymaps/default");
	
	FILE* fp = fopen(path, "r");
	
	if (fp == NULL) {
		fp = fopen(path, "w");
		fprintf(fp, "some text");
		fclose(fp);
	}
	km.START = VC_R;
	km.STOP  = VC_F;
	km.PAUSE = VC_D;
	km.SPLIT = VC_E;
}

int main(int argc, char* argv[])
{
	timerActive = false;
	hook_set_logger_proc(&logger_proc);
	hook_set_dispatch_proc(&dispatch_proc);

	//IPC pipe
	pid_t cpid;

	pipe(pipefd);
	fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
	loadKeymap();
	cpid = fork();

	if (cpid == 0) {
		close(pipefd[0]);
		hook_run();
	} else {
		close(pipefd[1]);
		initscr();
		while(1) {
			handleInput();
			if (timerActive) {
				clock_gettime(CLOCK_REALTIME, &finish);
				sub_timespec(start, finish, &delta);
			}
			ncDisplay();
		}
		endwin();
	}
	return 0;
}
