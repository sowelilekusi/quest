#include "timer.h"

//Timekeeping
struct timespec timestart, finish;
int currentMS = 0;
bool timerActive;

//Global hotkeys
char buf;
int pipefd[2];
struct keymap km;

//UI
int h, w;
int deltasEnabled = 1;
int sgmtdurEnabled = 1;
int pbEnabled = 1;
bool resized = false;

//Run data
const char *schemaver  = "v1.0.1";
const char *timersname = "quest";
const char *timerlname = "Quinn's Utterly Elegant Speedrun Timer";
const char *timerver   = "v0.4.0";
const char *timerlink  = "https://github.com/SilentFungus/quest";
char *gameTitle = "title not loaded";
char *categoryTitle = "category not loaded";
int attempts = 0;
struct segment *segments;
int segmentCount;
int currentSegment = -1;
char currentTime[10];

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
	currentSegment = 0;
}

void stop()
{
	if (!timerActive)
		return;
	timerActive = false;
	currentSegment = -1;
}

void split()
{
	if (!timerActive)
		return;
	segments[currentSegment].realtimeMS = currentMS;
	segments[currentSegment].gametimeMS = currentMS;
	currentSegment++;
	if (currentSegment >= segmentCount)
		stop();
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

void ftime(char *timestr, bool withMS, int ms)
{
	int seconds   = ms / 1000;
	int minutes   = seconds / 60;
	int hours     = minutes / 60;
	//A few better formatted variables for displaying these numbers
	int tms = (ms % 1000) / 10;
	int oms = tms / 10;
	int s   = seconds % 60;
	int m   = minutes % 60;
	int h   = hours;

	if (hours) {
		if (withMS)
			sprintf(timestr, fulltime, h, abs(h), abs(m), abs(s), abs(tms));
		else
			sprintf(timestr, hourstime, h, abs(m), abs(s));
	} else if (minutes) {
		if (withMS)
			sprintf(timestr, sfulltime, m, abs(s), abs(tms));
		else
			sprintf(timestr, minutestime, m, abs(s));
	} else {
		if (withMS)
			sprintf(timestr, secondstime, s, abs(tms));
		else
			sprintf(timestr, millitime, s, abs(oms));
	}
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
	char segmentTime[11];
	char zeroStr[11];
	char deltaTime[11];
	char sgmtTime[11];
	char segTime[11];
	ftime(zeroStr, false, 0);
	for(int i = 0; i < segmentCount; i++) {
		ftime(segmentTime, true, segments[i].pbrealtimeMS);
		if (i >= currentSegment) {
			sprintf(data, "%10s%10s%10s%10s", zeroStr, zeroStr, zeroStr, segmentTime);
		} else {
			ftime(deltaTime, false, segments[i].realtimeMS - segments[i].pbrealtimeMS);
			ftime(sgmtTime, false, segments[i].realtimeMS - segments[i - 1].realtimeMS);
			ftime(segTime, false, segments[i].realtimeMS);
			sprintf(data, "%10s%10s%10s%10s", deltaTime, sgmtTime, segTime, segmentTime);
		}
		rghtPrint(6 + i, w, data);
		leftPrint(6 + i, w, segments[i].name);
	}
}

void drawCurrentSegment()
{
	char data[(deltasEnabled * 10) + (sgmtdurEnabled * 10) + (pbEnabled * 10) + 11];
	strcpy(data, "");
	char pbTime[11];
	char deltaTime[11];
	char sgmtTime[11];
	char segTime[11];
	if (deltasEnabled) {
		ftime(deltaTime, false, currentMS - segments[currentSegment].pbrealtimeMS);
		strcat(data, deltaTime);
	}
	if (sgmtdurEnabled) {
		if (currentSegment == 0)
			ftime(sgmtTime, false, currentMS);
		else
			ftime(sgmtTime, false, currentMS - segments[currentSegment - 1].realtimeMS);
		strcat(data, sgmtTime);
	}
	ftime(segTime, false, currentMS);
	strcat(data, segTime);
	if (pbEnabled) {
		ftime(pbTime, true, segments[currentSegment].pbrealtimeMS);
		strcat(data, pbTime);
	}
	data[(deltasEnabled * 10) + (sgmtdurEnabled * 10) + (pbEnabled * 10) + 11] = '\0';
	rghtPrint(6 + currentSegment, w, data);
	leftPrint(6 + currentSegment, w, segments[currentSegment].name);
}

void drawDisplay()
{
	if (resized) {
		clrScreen();
		resized = false;
	}
	rghtPrint(1, w, "Attempts");
	char atmpt[10];
	sprintf(atmpt, "%9d", attempts);
	rghtPrint(2, w, atmpt);
	cntrPrint(1, w / 2, w, gameTitle);
	cntrPrint(2, w / 2, w, categoryTitle);
	char cols[41];
	sprintf(cols, "%10s%10s%10s%10s", "Delta", "Sgmt", "Time", "PB");
	rghtPrint(4, w, cols);
	drawHLine(5, w);
	printf("\033[5;3H[dsp]");
	drawSegments();
	if (timerActive) {
		drawCurrentSegment();
		struct timespec delta;
		sub_timespec(timestart, finish, &delta);
		currentMS = timespecToMS(delta);
	}
	drawHLine(segmentCount + 6, w);
	ftime(currentTime, true, currentMS);
	rghtPrint(segmentCount + 7, w, currentTime);
	fflush(stdout);
}

void resize(int i)
{
	struct winsize ws;
	ioctl(1, TIOCGWINSZ, &ws);
	w = ws.ws_col;
	h = ws.ws_row;
	resized = true;
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
					segments[it].pbrealtimeMS = time->valueint;
				if (cJSON_IsNumber(gtime))
					segments[it].pbgametimeMS = gtime->valueint;
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
			usleep(5000);
		}
		resetScreen();
		kill(cpid, SIGTERM);
	}
	return 0;
}

