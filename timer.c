#include "display.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <uiohook.h>
#include <wchar.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>

#define NS_PER_S 1000000000
#define K_START 1
#define K_STOP  2
#define K_PAUSE 3
#define K_SPLIT 4
#define K_CLOSE 5

struct keymap
{
	uint16_t START;
	uint16_t STOP;
	uint16_t PAUSE;
	uint16_t SPLIT;
	uint16_t CLOSE;
};

struct segment
{
	char *name;
	int *startMS;
	int *endMS;
};

char buf;
int pipefd[2];
struct timespec timestart, finish;
char *gameTitle, *catagoryTitle;
char *splitPath;
struct keymap km;
int h, w;

bool timerActive;

void loadKeymap();
void start();
void stop();
void split();
void tpause();
int handleInput();

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
			if (event->data.keyboard.keycode == km.START)
				buf = K_START;
			if (event->data.keyboard.keycode == km.STOP)
				buf = K_STOP;
			if (event->data.keyboard.keycode == km.PAUSE)
				buf = K_PAUSE;
			if (event->data.keyboard.keycode == km.SPLIT)
				buf = K_SPLIT;
			if (event->data.keyboard.keycode == km.CLOSE)
				buf = K_CLOSE;
			write(pipefd[1], &buf, 1);
		default:
			break;
	}
}

int handleInput()
{
	if (read(pipefd[0], &buf, 1) == -1)
		return 0;
	if (buf == K_SPLIT)
		split();
	if (buf == K_START)
		start();
	if (buf == K_STOP)
		stop();
	if (buf == K_PAUSE)
		tpause();
	if (buf == K_CLOSE)
		return 1;
	return 0;
}

void start()
{
	if (timerActive)
		return;
	clock_gettime(CLOCK_REALTIME, &timestart);
	timerActive = true;
}

void stop()
{
	if (!timerActive)
		return;
	timerActive = false;
}

void split()
{

}

void tpause()
{

}

void loadKeymap()
{
	km.START = VC_R;
	km.STOP  = VC_F;
	km.PAUSE = VC_D;
	km.SPLIT = VC_E;
	km.CLOSE = VC_C;
	//char path[256];
	//strcat(strcpy(path, getenv("HOME")), "/.config/qtimer");
	//mkdir(path, 0777);
	//strcat(strcpy(path, getenv("HOME")), "/.config/qtimer/keymaps");
	//mkdir(path, 0777);
	//strcat(strcpy(path, getenv("HOME")), "/.config/qtimer/keymaps/default");
	
	//FILE* fp = fopen(path, "r");
	
	//if (fp == NULL) {
		//km.START = VC_R;
		//km.STOP  = VC_F;
		//km.PAUSE = VC_D;
		//km.SPLIT = VC_E;
		//fp = fopen(path, "w");
		//fprintf(fp, "START = R\n");
		//fprintf(fp, "STOP  = F\n");
		//fprintf(fp, "PAUSE = D\n");
		//fprintf(fp, "SPLIT = E\n");
		//fclose(fp);
	//} else {
		
	//}

	//fclose(fp);
}

void drawHLine(int row)
{
	for (int i = 0; i <= w; i++)
		printf("\033[%d;%dH─", row, i);
}

void printTime(int row, struct timespec t1, struct timespec t2)
{
	struct timespec delta;
	sub_timespec(timestart, finish, &delta);
	int minutes = delta.tv_sec / 60;
	int hours   = minutes / 60;
	printf("\033[%d;1H%01d:%02d:%02ld.%02ld", row, hours, minutes, delta.tv_sec, delta.tv_nsec / 10000000);
}

void drawDisplay()
{
	clrScreen();
	cntrPrint(1, w / 2, w, "Game Name");
	cntrPrint(2, w / 2, w, "Catagory Name");
	char cols[41];
	sprintf(cols, "%10s%10s%10s%10s", "Delta", "Sgmt", "Time", "PB");
	rghtPrint(4, w, cols);
	drawHLine(5);
	printTime(6, timestart, finish);
	fflush(stdout);
}

void resize(int i)
{
	struct winsize ws;
	ioctl(1, TIOCGWINSZ, &ws);
	w = ws.ws_col;
	h = ws.ws_row;
}

int main(int argc, char **argv)
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
		signal(SIGWINCH, resize);
		resize(0);
		struct color bg = { 47,  53,  66};
		struct color fg = {247, 248, 242};
		initScreen(bg, fg);
		while(!handleInput()) {
			drawDisplay();
			if (timerActive) {
				clock_gettime(CLOCK_REALTIME, &finish);
			}
			usleep(3000);
		}
		resetScreen();
		kill(cpid, SIGTERM);
	}
	return 0;
}
