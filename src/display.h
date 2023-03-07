#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <time.h>
#include "timer.h"

#define COLSTRLEN 11

extern int maxrows;
extern int maxcols;
extern int colwidth;
extern int in;
extern bool dirty;

struct color {
	int r;
	int g;
	int b;
};

extern struct color bg;
extern struct color fg;

void setBGColor(struct color);
void setFGColor(struct color);
void clrScreen();
void disableCursor();
void enableCursor();
void altBuffer();
void stdBuffer();
void initScreen(struct color, struct color);
void resetScreen();
void cntrPrint(int row, int col, int maxlen, char *text);
void leftPrint(int row, int maxlen, char *text);
void rghtPrint(int row, int maxlem, char *text);
void drawHLine(int row, int maxlen);
void drawColumn(char **data, int count, int column, int end);
void drawRow(char **data, int count, int row);
void drawCell(char *data, int column, int row, struct color col);
void setMaxRows(int rows);
void setMaxCols(int cols);
void drawNotif();
void clearNotif();
void drawSegmentNames();
void drawTimeColumn();
void toggleCompact();
void drawDisplay();
void resize(int i);
void ftime(char *timestr, int rms, int decimals, bool sign);

#endif

