#include "display.h"

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
	altBuffer();
	setBGColor(bg);
	setFGColor(fg);
	disableCursor();
	clrScreen();
}

void resetScreen()
{
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
