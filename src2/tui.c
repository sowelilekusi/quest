#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <signal.h>

//Big numbies
char bignumbers[4][126] = {
//char bignumbers[4][55] = {
	"▄▀▀▄  ▄█  ▄▀▀▄ ▄▀▀▄ ▄  █ █▀▀▀ ▄▀▀  ▀▀▀█  ▄▀▀▄ ▄▀▀▄     ",
	"█  █   █    ▄▀   ▄▀ █▄▄█ █▄▄  █▄▄    ▐▌  ▀▄▄▀ ▀▄▄█ ▀   ",
	"█  █   █  ▄▀   ▄  █    █    █ █  █   █   █  █    █ ▀   ",
	" ▀▀   ▀▀▀ ▀▀▀▀  ▀▀     ▀ ▀▀▀   ▀▀    ▀    ▀▀   ▀▀    ▀ "
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

//This function needs an x and y coordinate because the resulting string
//is supposed to be printed across 4 rows so it cant really just return the
//resulting string
void printbigtimestring(int ms, int x, int y)
{
	//convert the single cell per character string length into the same
	//thing for the big time string, it cant just be a simple multiplication
	//because the : and . digits arent as wide as the numbers
	
	//Example string, printing a blank timer with all digits at x=65, y=40.
	
	//\033[40;65H▄▀▀▄ ▄▀▀▄ ▄▀▀▄   ▄▀▀▄ ▄▀▀▄   ▄▀▀▄ ▄▀▀▄   ▄▀▀▄ ▄▀▀▄ 
	//\033[41;65H█  █ █  █ █  █ ▀ █  █ █  █ ▀ █  █ █  █   █  █ █  █ 
	//\033[42;65H█  █ █  █ █  █ ▀ █  █ █  █ ▀ █  █ █  █   █  █ █  █ 
	//\033[43;65H ▀▀   ▀▀   ▀▀     ▀▀   ▀▀     ▀▀   ▀▀  ▀  ▀▀   ▀▀  
	
	char buffer[256];

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
	//need a while loop; convert to for loop
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
	fp = popen("./quest-log foreground", "r");
	fgets(path, sizeof(path), fp);
	if (strcmp(path, "DATA NOT PRESENT"))
		processColorString(&f, path);
	printf("\033[38;2;%d;%d;%dm", f.r, f.g, f.b);
	pclose(fp);

	//Request background color from config file
	fp = popen("./quest-log background", "r");
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
		fp = popen("./quest-log time", "r");
		if (fp == NULL) {
			printf("Failed to run command\n");
			exit(1);
		}
		while (fgets(path, sizeof(path), fp) != NULL) {
			time = atoi(path);
			printf("\033[2J\n");
			timestring(&ti, time);
			printf("%s\n", ti + (12 - timestringDigits(time)));
		}
		pclose(fp);
		usleep(1000000 / fps);
	}
}
