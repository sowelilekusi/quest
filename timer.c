#include "timer.h"

char buf;
int pipefd[2];
struct timespec timestart, finish;
struct keymap km;
int h, w;
char *gameTitle = "title not loaded";
char *categoryTitle = "category not loaded";
int attempts = 0;

bool timerActive;
struct segment *segments;
int segmentCount;
int currentSegment = 0;
char currentTime[10];
int deltasEnabled = 1;
int sgmtdurEnabled = 1;
int pbEnabled = 1;

void sub_timespec(struct timespec t1, struct timespec t2, struct timespec* td)
{
	td->tv_nsec = t2.tv_nsec - t1.tv_nsec;
	td->tv_sec  = t2.tv_sec  - t1.tv_sec;
	if (td->tv_sec > 0 && td->tv_nsec < 0) {
		td->tv_nsec += NS_PER_S;
		td->tv_sec--;
	} else if (td->tv_sec < 0 && td->tv_nsec > 0) {
		td->tv_nsec -= NS_PER_S;
		td->tv_sec++;
	}
}

void add_timespec(struct timespec t1, struct timespec t2, struct timespec* td)
{	
	td->tv_nsec = t2.tv_nsec + t1.tv_nsec;
	td->tv_sec  = t2.tv_sec  + t1.tv_sec;
	if (td->tv_nsec < 0) {
		td->tv_nsec += NS_PER_S;
		td->tv_sec++;
	}
}


bool logger_proc(unsigned int level, const char *format, ...) {
	return 0;
}
void dispatch_proc(uiohook_event * const event) {
	switch (event->type) {
		case EVENT_KEY_PRESSED:
			if (event->data.keyboard.keycode == km.START)
				buf = K_START;
			if (event->data.keyboard.keycode == km.STOP)
				buf = K_STOP;
			if (event->data.keyboard.keycode == km.PAUSE)
				buf = K_PAUSE;
			if (event->data.keyboard.keycode == km.SPLIT)
				buf = K_SPLIT;
			if (event->data.keyboard.keycode == km.CLOSE)
				buf = K_CLOSE;
			write(pipefd[1], &buf, 1);
		default:
			break;
	}
}

int handleInput()
{
	if (read(pipefd[0], &buf, 1) == -1)
		return 0;
	if (buf == K_SPLIT)
		split();
	if (buf == K_START)
		start();
	if (buf == K_STOP)
		stop();
	if (buf == K_PAUSE)
		tpause();
	if (buf == K_CLOSE)
		return 1;
	return 0;
}

void start()
{
	if (timerActive)
		return;
	clock_gettime(CLOCK_REALTIME, &timestart);
	timerActive = true;
}

void stop()
{
	if (!timerActive)
		return;
	timerActive = false;
}

void split()
{
	/*
	struct timespec *temp = malloc(sizeof(struct timespec) * (splitCount + 1));
	for (int i = 0; i < splitCount; i++) {
		temp[i] = splits[i];
	}
	clock_gettime(CLOCK_REALTIME, &temp[splitCount]);
	free(splits);
	splits = temp;
	splitCount++;
	*/
}

void tpause()
{

}

void loadKeymap()
{
	km.START = VC_R;
	km.STOP  = VC_F;
	km.PAUSE = VC_D;
	km.SPLIT = VC_E;
	km.CLOSE = VC_C;
	//char path[256];
	//strcat(strcpy(path, getenv("HOME")), "/.config/qtimer");
	//mkdir(path, 0777);
	//strcat(strcpy(path, getenv("HOME")), "/.config/qtimer/keymaps");
	//mkdir(path, 0777);
	//strcat(strcpy(path, getenv("HOME")), "/.config/qtimer/keymaps/default");
	
	//FILE* fp = fopen(path, "r");
	
	//if (fp == NULL) {
		//km.START = VC_R;
		//km.STOP  = VC_F;
		//km.PAUSE = VC_D;
		//km.SPLIT = VC_E;
		//fp = fopen(path, "w");
		//fprintf(fp, "START = R\n");
		//fprintf(fp, "STOP  = F\n");
		//fprintf(fp, "PAUSE = D\n");
		//fprintf(fp, "SPLIT = E\n");
		//fclose(fp);
	//} else {
		
	//}

	//fclose(fp);
}

void drawHLine(int row)
{
	for (int i = 0; i <= w; i++)
		printf("\033[%d;%dH─", row, i);
}

int ftime(char *timestr, int ms)
{
	int displayMS = (ms % 1000) / 10;
	int seconds = ms / 1000;
	int minutes = seconds / 60;
	if (minutes)
		sprintf(timestr, "%2d:%02d.%02d", minutes, seconds % 60, displayMS);
	else
		sprintf(timestr, "%2d.%02d", seconds % 60, displayMS);
	return 0;
}

int timespecToMS(struct timespec t)
{
	int ms = t.tv_nsec / 1000000;
	ms += t.tv_sec * 1000;
	return ms;
}

void drawSegments()
{
	char data[(deltasEnabled * 10) + (sgmtdurEnabled * 10) + (pbEnabled * 10) + 11];
	char segmentTime[10];
	char zeroStr[10];
	ftime(zeroStr, 0);
	for(int i = 0; i < segmentCount; i++) {
		if (!ftime(segmentTime, segments[i].realtimeMS)) {
			sprintf(data, "%10s%10s%10s%10s", zeroStr, zeroStr, zeroStr, segmentTime);
		} else {
			sprintf(data, "%s", "Failed to format time");
		}
		rghtPrint(6 + i, w, data);
		leftPrint(6 + i, w, segments[i].name);
	}
}

void drawDisplay()
{
	clrScreen();
	rghtPrint(1, w, "Attempts");
	char atmpt[10];
	sprintf(atmpt, "%9d", attempts);
	rghtPrint(2, w, atmpt);
	cntrPrint(1, w / 2, w, gameTitle);
	cntrPrint(2, w / 2, w, categoryTitle);
	char cols[41];
	sprintf(cols, "%10s%10s%10s%10s", "Delta", "Sgmt", "Time", "PB");
	rghtPrint(4, w, cols);
	drawHLine(5);
	printf("\033[5;%dH[dsp]", 2);
	drawSegments();
	drawHLine(segmentCount + 6);
	struct timespec delta;
	sub_timespec(timestart, finish, &delta);
	ftime(currentTime, timespecToMS(delta));
	rghtPrint(segmentCount + 7, w, currentTime);
	fflush(stdout);
}

void resize(int i)
{
	struct winsize ws;
	ioctl(1, TIOCGWINSZ, &ws);
	w = ws.ws_col;
	h = ws.ws_row;
}

void loadFile(char *path)
{
	char *buffer = NULL;
	long length;
	FILE *f = fopen(path, "rb");
	if (f == NULL)
		return;

	fseek(f, 0, SEEK_END);
	length = ftell(f);
	fseek(f, 0, SEEK_SET);
	buffer = malloc(length + 1);
	if (buffer != NULL) {
		fread(buffer, 1, length, f);
	}
	fclose(f);
	buffer[length] = '\0';
	
	cJSON *splitfile = cJSON_Parse(buffer);
	cJSON *game = NULL;
	cJSON *category = NULL;
	cJSON *attempt = NULL;
	cJSON *segs = NULL;
	game = cJSON_GetObjectItemCaseSensitive(splitfile, "game");
	category = cJSON_GetObjectItemCaseSensitive(splitfile, "category");
	attempt = cJSON_GetObjectItemCaseSensitive(splitfile, "attempts");
	segs = cJSON_GetObjectItemCaseSensitive(splitfile, "segments");
	if (game) {
		cJSON *title = cJSON_GetObjectItemCaseSensitive(game, "longname");
		if (cJSON_IsString(title) && (title->valuestring != NULL)) {
			gameTitle = malloc(strlen(title->valuestring));
			strcpy(gameTitle, title->valuestring);
		}
	}
	if (category) {
		cJSON *title = cJSON_GetObjectItemCaseSensitive(category, "longname");
		if (cJSON_IsString(title) && (title->valuestring != NULL)) {
			categoryTitle = malloc(strlen(title->valuestring));
			strcpy(categoryTitle, title->valuestring);
		}
	}
	if (attempt) {
		cJSON *total = cJSON_GetObjectItemCaseSensitive(attempt, "total");
		if (cJSON_IsNumber(total))
			attempts = total->valueint;
	}
	if (segs) {
		int segm = cJSON_GetArraySize(segs);
		segmentCount = segm;
		segments = malloc(segmentCount * sizeof(struct segment));
		int it = 0;
		cJSON *iterator = NULL;
		cJSON *segname = NULL;
		cJSON *segtime = NULL;
		cJSON_ArrayForEach(iterator, segs) {
			segname = cJSON_GetObjectItemCaseSensitive(iterator, "name");
			if (cJSON_IsString(segname) && (segname->valuestring != NULL)) {
				segments[it].name = malloc(strlen(segname->valuestring));
				strcpy(segments[it].name, segname->valuestring);
			}
			segtime = cJSON_GetObjectItemCaseSensitive(iterator, "endedAt");
			if (segtime) {
				cJSON *time = cJSON_GetObjectItemCaseSensitive(segtime, "realtimeMS");
				cJSON *gtime = cJSON_GetObjectItemCaseSensitive(segtime, "gametimeMS");
				if (cJSON_IsNumber(time))
					segments[it].realtimeMS = time->valueint;
				if (cJSON_IsNumber(gtime))
					segments[it].gametimeMS = gtime->valueint;
			}
			it++;
		}
	}
	cJSON_Delete(splitfile);
}

int main(int argc, char **argv)
{
	timerActive = false;
	hook_set_logger_proc(&logger_proc);
	hook_set_dispatch_proc(&dispatch_proc);

	//IPC pipe
	pid_t cpid;

	pipe(pipefd);
	fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
	loadKeymap();
	cpid = fork();

	if (cpid == 0) {
		close(pipefd[0]);
		hook_run();
	} else {
		close(pipefd[1]);
		signal(SIGWINCH, resize);
		resize(0);
		struct color bg = { 47,  53,  66};
		struct color fg = {247, 248, 242};
		initScreen(bg, fg);
		loadFile(argv[1]);
		while(!handleInput()) {
			drawDisplay();
			if (timerActive) {
				clock_gettime(CLOCK_REALTIME, &finish);
			}
			usleep(4000);
		}
		resetScreen();
		kill(cpid, SIGTERM);
	}
	return 0;
}

