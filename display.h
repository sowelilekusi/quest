#include <stdio.h>
#include <string.h>
#include <termios.h>

extern const char *millitime;
extern const char *secondstime;
extern const char *minutestime;
extern const char *hourstime;
extern const char *fulltime;
extern const char *sfulltime;

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

