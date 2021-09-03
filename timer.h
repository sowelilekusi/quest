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
#include <math.h>
#include "cJSON.h"

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
        int realtimeMS;
        int gametimeMS;
        bool isSkipped;
};

void sub_timespec(struct timespec t1, struct timespec t2, struct timespec* td);
void add_timespec(struct timespec t1, struct timespec t2, struct timespec* td);
bool logger_proc(unsigned int level, const char *format, ...);
void dispatch_proc(uiohook_event * const event);
int handleInput();
void start();
void stop();
void split();
void tpause();
void loadKeymap();
void drawHLine(int row);
int ftime(char *timestr, int ms);
int timespecToMS(struct timespec t);
void drawSegments();
void drawDisplay();
void resize(int i);
void loadFile(char *path);
int main(int argc, char **argv);

