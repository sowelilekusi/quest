#ifndef TIMER_H
#define TIMER_H

#include "display.h"
#include "keys.h"
#include "splitsio.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <wchar.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <math.h>
#include <cjson/cJSON.h>

#define NS_PER_S  1000000000
#define cJSON_GetItem(x, y) cJSON_GetObjectItemCaseSensitive(x, y)

struct segment
{
        char *name;
        int ms;
        bool isSkipped;
	bool isReset;
};

struct pastseg
{
	int ms;
	bool isSkipped;
	bool isReset;
};

extern char *gameTitle;
extern char *categoryTitle;
extern int segCount;
extern struct segment *pbrun;
extern struct segment *bestsegs;
extern struct segment *wrrun;
extern struct segment *segments;

void sub_timespec(struct timespec t1, struct timespec t2, struct timespec* td);
void add_timespec(struct timespec t1, struct timespec t2, struct timespec* td);
void start();
void stop();
void split();
void tpause();
void unsplit();
void skip();
void ftime(char *timestr, int rms, int decimals, bool sign);
int timespecToMS(struct timespec t);
void drawNotif();
void clearNotif();
void drawSegmentNames();
void drawTimeColumn();
void toggleCompact();
void drawDisplay();
void resize(int i);
void calculatePB();
void loadConfig();
void saveConfig(cJSON *config);
void loadFile();
void saveFile();
int main(int argc, char **argv);

#endif

