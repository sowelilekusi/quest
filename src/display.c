#include "display.h"

#define MVCUR "\033[%d;%dH%s"
int maxrows = INT_MAX;
int maxcols = INT_MAX;
int colwidth = 10;
int in = 0;

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
	dup(0);
	fcntl(0, F_SETFL, O_NONBLOCK);
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
void drawColumn(char **data, int count, int column, int end)
{
	int x = maxcols - (column * 10) + 1;
	int y = 6;
	if (column == 0)
		x = 1;
	for (int i = 0; i < count; i++) {
		if (i <= end)
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

//In case you need colors, you gotta draw cell by cell
void drawCell(char *data, int column, int row, struct color col)
{
	int x = maxcols - (column * 10) + 1;
	if (column == 1)
		x = 1;
	printf("\033[38;2;%d;%d;%dm\033[%d;%dH%s", col.r, col.g, col.b, row, x, data);
}

void setMaxRows(int rows)
{
	maxrows = rows;
}

void setMaxCols(int cols)
{
	maxcols = cols;
}
