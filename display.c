#include "display.h"

const char *millitime   = "%8d.%d";
const char *secondstime = "%7d.%02d";
const char *minutestime = "%7d:%02d";
const char *hourstime   = "%5d:%02d:%02d";
const char *fulltime    = "%2d:%02d:%02d.%02d";
const char *sfulltime   = "%4d:%02d.%02d";

struct termios base;

void setBGColor(struct color c)
{
	printf("\033[48;2;%d;%d;%dm", c.r, c.g, c.b);
}

void setFGColor(struct color c)
{
	printf("\033[38;2;%d;%d;%dm", c.r, c.g, c.b);
}

void clrScreen()
{
	printf("\033[2J\n");
}

void disableCursor()
{
	printf("\033[?25l\n");
}

void enableCursor()
{
	printf("\033[?25h\n");
}

void altBuffer()
{
	printf("\033[?1049h\n");
}

void stdBuffer()
{
	printf("\033[?1049l\n");
}

void initScreen(struct color bg, struct color fg)
{
	struct termios t;
	tcgetattr(1, &base);
	t = base;
	t.c_lflag &= (~ECHO & ~ICANON);
	tcsetattr(1, TCSANOW, &t);
	altBuffer();
	setBGColor(bg);
	setFGColor(fg);
	disableCursor();
	clrScreen();
}

void resetScreen()
{
	tcsetattr(1, TCSANOW, &base);
	clrScreen();
	enableCursor();
	stdBuffer();
}

void cntrPrint(int row, int col, int maxlen, char *text)
{
	printf("\033[%d;%ldH%.*s", row, col - (strlen(text) / 2), maxlen, text);
}

void leftPrint(int row, int maxlen, char *text)
{
	printf("\033[%d;1H%.*s", row, maxlen, text);
}

void rghtPrint(int row, int maxlen, char* text)
{
	if (strlen(text) < maxlen)
		printf("\033[%d;1H%*s", row, maxlen, text);
	else
		printf("\033[%d;1H%.*s", row, maxlen, text);
}

void drawHLine(int row, int maxlen)
{
	for (int i = 0; i <= maxlen; i++) {
		printf("\033[%d;%dH─", row, i);
	}
}
