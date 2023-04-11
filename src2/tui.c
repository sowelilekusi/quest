#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <signal.h>

char numbermap[5][44] = {
	"xxx..x..xxx.xxx.x.x.xxx.xxx.xxx.xxx.xxx.....",
	"x.x..x....x...x.x.x.x...x.....x.x.x.x.x.x...",
	"x.x..x..xxx.xxx.xxx.xxx.xxx...x.xxx.xxx.....",
	"x.x..x..x.....x...x...x.x.x...x.x.x...x.x...",
	"xxx..x..xxx.xxx...x.xxx.xxx...x.xxx...x...x."
};
struct termios base;
int fps = 60;
struct color {
	int r;
	int g;
	int b;
};
struct color  b = { 47,  53,  66}; //Background color
struct color  f = {247, 248, 242}; //Text foreground color
struct color  g = {249, 255,  79}; //Best ever segment time color
struct color ag = { 24, 240,  31}; //Ahead, and gaining time segment color
struct color al = { 79, 255,  85}; //Ahead, but losing time segment color
struct color bg = {255,  79,  79}; //Behind, but gaining time segment color
struct color bl = {224,  34,  34}; //Behind, and losing time segment color

int w, h;

int timestringDigits(int ms)
{
	int chars = 4;
	if (ms >= 10000)
		chars += 1;
	if (ms >= 60000)
		chars += 2;
	if (ms >= 600000)
		chars += 1;
	if (ms >= 3600000)
		chars += 2;
	if (ms >= 36000000)
		chars += 1;
	if (ms >= 360000000)
		chars += 1;
	return chars;
}

//Attempt 2 at thinking through how to print the big numbies
void printbig(int x, int y, int ms)
{
	char small[13];
	timestring(&small, ms);
	if (w < strlen(small)) {
		printf("2smol\n");
		return;
	}
	int bigstringw = 42; //Minimum width and height the big timer string
	int bigstringh = 5;  //bigger sizes are just multiples of these numbers.
	int bigstrings = 1;
	x = (w - (bigstringw - 1 * bigstrings)) / 2; //theres a -1 because theres extra whitespace on the last printed digit
	y = (h - (bigstringh * bigstrings)) / 2;
	for (int sy = 0; sy < 5; sy++) {             //for every row
		printf("\033[%d;%dH", y + sy, x);    //go to position
		for (int cc = 0; cc < 12; cc++) {    //then, for every character
			int c = small[cc];           //check what character we're on
			if (c >= 48 && c <= 57) {    //if its a number, print 4 pixels
				for (int xx = 0; xx < 4; xx++) {
					int xxx = c - 48;
					if (numbermap[sy][(xxx * 4) + xx] == 'x')
						printf("\033[48;2;%d;%d;%dm ", f.r, f.g, f.b);
					if (numbermap[sy][(xxx * 4) + xx] == '.')
						printf("\033[48;2;%d;%d;%dm ", b.r, b.g, b.b);
				}
			}
			if (c == 46 || c == 58) {                 //if its punctuation, print 2 pixels
				for (int xx = 0; xx < 2; xx++) {
					if (c == 46) {
						if (numbermap[sy][42 + xx] == 'x')
							printf("\033[48;2;%d;%d;%dm ", f.r, f.g, f.b);
						if (numbermap[sy][42 + xx] == '.')
							printf("\033[48;2;%d;%d;%dm ", b.r, b.g, b.b);
					}
					if (c == 58) {
						if (numbermap[sy][40 + xx] == 'x')
							printf("\033[48;2;%d;%d;%dm ", f.r, f.g, f.b);
						if (numbermap[sy][40 + xx] == '.')
							printf("\033[48;2;%d;%d;%dm ", b.r, b.g, b.b);
					}
				}
			}
		}
	}
	printf("\n");
	//printf("\033[%d;%dH%s\n", y, x, small + (12 - timestringDigits(time)));
}

void timestring(char *str, int ms)
{
	int msdigits = (ms / 10) % 100;
	int seconds  = (ms / 1000) % 60;
	int minutes  = ((ms / 1000) / 60) % 60;
	int hours    = ((ms / 1000) / 60) / 60;
	sprintf(str, "%03d:%02d:%02d.%02d", hours, minutes, seconds, msdigits);
}

void resize(int i)
{
	struct winsize ws;
	ioctl(1, TIOCGWINSZ, &ws);
	w = ws.ws_col;
	h = ws.ws_row;
}

void initScreen()
{
	struct termios t;
	tcgetattr(1, &base);
	t = base;
	t.c_lflag &= (~ECHO & ~ICANON);
	tcsetattr(1, TCSANOW, &t);
	//TODO:Figure out why i did this
	dup(0);
	fcntl(0, F_SETFL, O_NONBLOCK);
	printf("\033[?1049h\n");         //Switch to TUI mode (alternate buffer)
	printf("\033[?25l\n");           //Hide text cursor
	printf("\033[2J\n");             //Clear screen
}

void resetScreen()
{
	tcsetattr(1, TCSANOW, &base);
	printf("\033[2J\n");             //Clear screen
	printf("\033[?1049l\n");         //Switch back to regular mode
	printf("\033[?25h\n");           //Show cursor
}

void die(int i)
{
	exit(1);
}

void processColorString(struct color *c, char* s)
{
	int i = 0;
	int length = strlen(s);
	char comp[4];
	int compcount = 0;
	int colorcompsdone = 0;
	//TODO: if we know the length now that we're not doing fgetc we dont 
	//need a while loop; convert to for loop.
	//Why? what makes a for loop better than a while loop?
	while (1) {
		char x = s[i++];
		if (x >= 48 && x <= 57) {
			comp[compcount] = x;
			compcount++;
		}
		if (x == 44 || i == length) {
			comp[compcount] = '\0';
			switch(colorcompsdone) {
				case 0:
					c->r = atoi(comp);
					break;
				case 1:
					c->g = atoi(comp);
					break;
				case 2:
					c->b = atoi(comp);
			}
			colorcompsdone++;
			compcount = 0;
		}
		if (i == length)
			break;
	}
}

int main (int argc, char *argv[])
{
	initScreen();
	atexit(resetScreen);
	signal(SIGTERM, die);
	signal(SIGINT, die);
	signal(SIGWINCH, resize);
	resize(0);

	FILE *fp;
	char path[1000];
	char ti[13];

	//Request foreground color from config file
	fp = popen("./result/bin/quest-log foreground", "r");
	fgets(path, sizeof(path), fp);
	if (strcmp(path, "DATA NOT PRESENT"))
		processColorString(&f, path);
	printf("\033[38;2;%d;%d;%dm", f.r, f.g, f.b);
	pclose(fp);

	//Request background color from config file
	fp = popen("./result/bin/quest-log background", "r");
	fgets(path, sizeof(path), fp);
	if (strcmp(path, "DATA NOT PRESENT"))
		processColorString(&b, path);
	printf("\033[48;2;%d;%d;%dm", b.r, b.g, b.b);
	pclose(fp);

	//Set fps from command line argument
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-fps"))
			fps = atoi(argv[i + 1]);
	}
	while (1) {
		int time = 0;
		fp = popen("./result/bin/quest-log time", "r");
		if (fp == NULL) {
			printf("Failed to run command\n");
			exit(1);
		}
		//TODO: why is this a while loop?
		while (fgets(path, sizeof(path), fp) != NULL) {
			time = atoi(path);
		}
		pclose(fp);

		printf("\033[2J\n");
		//timestring(&ti, time);
		//printf("%s\n", ti + (12 - timestringDigits(time)));
		printbig(3, 4, time);
		usleep(1000000 / fps);
	}
}
