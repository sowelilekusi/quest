#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>

extern int maxrows;
extern int maxcols;
extern int colwidth;
extern int in;

struct color {
	int r;
	int g;
	int b;
};

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

#endif

