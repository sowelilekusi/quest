#include "display.h"

#define MVCUR "\033[%d;%dH%s"
int maxrows = INT_MAX;
int maxcols = INT_MAX;
int colwidth = 10;
int in;

//UI
struct color bg   = { 47,  53,  66};
struct color fg   = {247, 248, 242};
struct color fade = {210, 210, 210};
struct color gold = {249, 255,  79};
struct color good = { 79, 255,  85};
struct color bad  = {255,  79,  79};
int h, w;
bool compact = false;
bool dirty   = false;

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

void drawSegmentNames()
{
	char *names[segCount];
	for(int i = 0; i < segCount; i++) {
		names[i] = segments[i].name;
	}
	drawColumn(names, segCount, 0, segCount);
}

//TODO: Fix up all this commented garbage
//Really the entire display system needs rethinking first but yea
void drawDeltaColumn(int column)
{
	char *times[segCount];
	for (int i = 0; i < segCount; i++) {
		times[i] = calloc(1, COLSTRLEN);
		int time = 0;
		if (i == currSeg)
			time = currentMS - pbrun[i].ms;
		else if (i < currSeg)
			time = segments[i].ms - pbrun[i].ms;
		ftime(times[i], time, 1, true);
		struct color col = {0};
		if (time <= 0)
			col = good;
		else
			col = bad;
		if (i < currSeg)
			drawCell(times[i], column, i + 6, col);
		if (i == currSeg && time >= -5000)
			drawCell(times[i], column, i + 6, col);
	}
	//drawColumn(times, segCount, column, currSeg);
	//Use drawCell because we're doing colors.
	//for (int i = 0; i < segCount; i++) {
	//	if (i <= currSeg)
	//		drawCell(times[i], column, i + 6, good);
	//}
	setFGColor(fg);
	for (int i = 0; i < segCount; i++) {
		free(times[i]);
	}
}

//TODO: try to clean the branching up
void drawTimeColumn(int timeoption, int column)
{
	char *times[segCount];
	int drawEnd = currSeg;
	for (int i = 0; i < segCount; i++) {
		times[i] = calloc(1, COLSTRLEN);
		int time = 0;
		switch (timeoption) {
		case 0:
			time = pbrun[i].ms;
			drawEnd = segCount;
			break;
		case 2:
			if (i > 0 && i < currSeg)
				time = segments[i].ms - segments[i - 1].ms;
			else if (i > 0 && i == currSeg)
				time = currentMS - segments[i - 1].ms;
			else if (i == 0 && i == currSeg)
				time = currentMS;
			else
				time = segments[i].ms;
			break;
		case 3:
			if (i == currSeg)
				time = currentMS;
			else
				time = segments[i].ms;
		}
		ftime(times[i], time, 1, false);
	}
	drawColumn(times, segCount, column, drawEnd);
	for (int i = 0; i < segCount; i++) {
		free(times[i]);
	}
}

void drawNotif(char* text)
{
	clock_gettime(CLOCK_REALTIME, &notif);
	clearNotif();
	leftPrint(maxrows, w, text);
}

void clearNotif()
{
	leftPrint(maxrows, w, "\033[2K");
}

void toggleCompact()
{
	compact = !compact;
	//Clears the screen rather than dirtying it so the notif doesnt clear
	clrScreen();
	if (compact)
		drawNotif("Compact mode enabled");
	else
		drawNotif("Compact mode disabled");
}

void drawDisplay()
{
	if (dirty) {
		clrScreen();
		dirty = false;
	}
	rghtPrint(1, w, "Attempts");
	char atmpt[10];
	sprintf(atmpt, "%9d", attempts);
	rghtPrint(2, w, atmpt);
	cntrPrint(1, w / 2, w, gameTitle);
	cntrPrint(2, w / 2, w, categoryTitle);
	setFGColor(fade);
	drawHLine(5, w);
	printf("\033[5;3H");
	if (hotkeys_enabled || compact)
		printf("[");
	if (hotkeys_enabled)
		printf("h");
	if (compact)
		printf("c");
	if (hotkeys_enabled || compact)
		printf("]");
	setFGColor(fg);
	drawSegmentNames();
	//TODO: The column names stuff has to be more dynamic, part of the
	//drawColumn function probably
	if (!compact) {
		char cols[41];
		sprintf(cols, "%10s%10s%10s%10s", "Delta", "Sgmt", "Time", "PB");
		setFGColor(fade);
		rghtPrint(4, w, cols);
		setFGColor(fg);
		drawTimeColumn(0, 1);
		drawTimeColumn(3, 2);
		drawTimeColumn(2, 3);
		drawDeltaColumn(4);
	} else {	
		char cols[21];
		sprintf(cols, "%10s%10s", "Delta", "Time/PB");
		setFGColor(fade);
		rghtPrint(4, w, cols);
		setFGColor(fg);
		drawTimeColumn(0, 1);
		drawTimeColumn(3, 1);
		drawDeltaColumn(2);
	}
	setFGColor(fade);
	drawHLine(segCount + 6, w);
	setFGColor(fg);
	ftime(currentTime, currentMS, 2, false);
	rghtPrint(segCount + 7, w, currentTime);
	fflush(stdout);
}

void resize(int i)
{
	struct winsize ws;
	ioctl(1, TIOCGWINSZ, &ws);
	w = ws.ws_col;
	h = ws.ws_row;
	setMaxCols(w);
	setMaxRows(h);
	dirty = true;
}

void ftime(char *timestr, int rms, int decimals, bool sign)
{
	if (decimals > 3 || decimals < 0)
		decimals = 0;
	int seconds   = rms / 1000;
	int minutes   = seconds / 60;
	int hours     = minutes / 60;
	//A few better formatted variables for displaying these numbers
	int thr = rms % 1000;
	int two = thr / 10;
	int one = two / 10;
	int s   = seconds % 60;
	int m   = minutes % 60;
	int h   = hours;
	int d   = 0;
	switch (decimals) {
	case 1:
		d = one;
		break;
	case 2:
		d = two;
		break;
	case 3:
		d = thr;
		break;
	}

	char tformat[22];
	int i = 0;
	int decimalspace = decimals + (decimals != 0);
	if (hours) {
		tformat[i++] = '%';
		if (sign)
			tformat[i++] = '+';
		tformat[i++] = (colwidth - 6 - decimalspace) + 48;
		tformat[i++] = 'd';
		tformat[i++] = ':';
	}
	if (minutes) {
		tformat[i++] = '%';
		if (sign && !hours)
			tformat[i++] = '+';
		if (hours) {
			tformat[i++] = '0';
			tformat[i++] = '2';
		} else {
			tformat[i++] = (colwidth - 3 - decimalspace) + 48;
		}
		tformat[i++] = 'd';
		tformat[i++] = ':';
	}

	tformat[i++] = '%';
	if (s != 0 && sign && !hours && !minutes)
		tformat[i++] = '+';
	if (minutes) {
		tformat[i++] = '0';
		tformat[i++] = '2';
	} else {
		//This value can push the resulting char out of the numbers
		//section of the ascii table so we gotta clamp it
		int n = colwidth - decimalspace + 48;
		if (n >= 58)
			n = 57;
		tformat[i++] = n;
	}
	tformat[i++] = 'd';

	if (decimals) {
		tformat[i++] = '.';
		tformat[i++] = '%';
		tformat[i++] = '0';
		tformat[i++] = decimals + 48;
		tformat[i++] = 'd';
	}
	tformat[i] = 0;

	if (hours) {
		if (!decimals)
			sprintf(timestr, tformat, h, abs(m), abs(s));
		else
			sprintf(timestr, tformat, h, abs(m), abs(s), abs(d));
	} else if (minutes) {
		if (!decimals)
			sprintf(timestr, tformat, m, abs(s));
		else
			sprintf(timestr, tformat, m, abs(s), abs(d));
	} else {
		if (!decimals) {
			sprintf(timestr, tformat, s);
		} else {
			sprintf(timestr, tformat, s, abs(d));
			if (sign && s == 0 && d < 0)
				timestr[COLSTRLEN - 4 - decimals] = '-';
			if (sign && s == 0 && d >= 0)
				timestr[COLSTRLEN - 4 - decimals] = '+';
		}
	}
}
