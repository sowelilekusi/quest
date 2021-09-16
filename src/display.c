#include "display.h"

const char *millitime   = "%8d.%d";
const char *secondstime = "%7d.%02d";
const char *minutestime = "%7d:%02d";
const char *hourstime   = "%5d:%02d:%02d";
const char *fulltime    = "%2d:%02d:%02d.%02d";
const char *sfulltime   = "%4d:%02d.%02d";
#define MVCUR "\033[%d;%dH%s"
int maxrows = INT_MAX;
int maxcols = INT_MAX;
int colwidth = 10;

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

//Column 0 is the left justified column, all following columns columns count
//from the right hand side towards the left.
void drawColumn(char **data, int count, int column)
{
	int x = maxcols - (column * 10);
	int y = 6;
	if (column == 1)
		x = 1;
	for (int i = 0; i < count; i++) {
		printf(MVCUR, y + i, x, data[i]);
	}
}

void drawRow(char **data, int count, int row)
{
	int x = 1;
	int y = row;
	for (int i = 1; i <= count; i++) {
		if (i != 1)
			x = maxcols - (i * 10);
		printf(MVCUR, y, x, data[i]);
	}
}

void drawCell(char *data, int column, int row)
{
	int x = maxcols - (column * 10);
	if (column == 1)
		x = 1;
	printf(MVCUR, row, x, data);
}

void setMaxRows(int rows)
{
	maxrows = rows;
}

void setMaxCols(int cols)
{
	maxcols = cols;
}
