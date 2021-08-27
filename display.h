#include <stdio.h>
#include <string.h>

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
