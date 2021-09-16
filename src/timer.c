#include "timer.h"

//Timekeeping
struct timespec timestart, finish;
int currentMS = 0;
bool timerActive;

//UI
struct color bg = { 47,  53,  66};
struct color fg = {247, 248, 242};
int h, w;
int deltaOn     = 1;
int sgmtdurOn   = 1;
int pbOn        = 1;
bool resized    = false;

//Splits.io data
const char *schemaver  = "v1.0.1";
const char *timersname = "quest";
const char *timerlname = "Quinn's Utterly Elegant Speedrun Timer";
const char *timerver   = "v0.5.0";
const char *timerlink  = "https://github.com/SilentFungus/quest";

//Run data
char *filepath;
char *gameTitle     = "title not loaded";
char *categoryTitle = "category not loaded";
int attempts        = 0;
int bestTime        = 0;
int bestAttempt     = 0;
struct segment *segments;
struct segment *pbrun;
struct segment *wrrun;
struct segment *bestsegs;
struct pastseg *pastRuns;
int segCount;
int currSeg = -1;
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

void reset()
{
	if (!timerActive)
		return;
	segments[currSeg].isReset = true;
	stop();
	currSeg = -1;
}

void start()
{
	if (timerActive || segCount == 0)
		return;
	clock_gettime(CLOCK_REALTIME, &timestart);
	timerActive    = true;
	resized        = true;
	currSeg = 0;
}

void stop()
{
	if (!timerActive)
		return;
	timerActive = false;
	attempts++;
	if (pastRuns)
		pastRuns = realloc(pastRuns, attempts * segCount * sizeof(struct pastseg));
	else
		pastRuns = calloc(segCount, sizeof(struct pastseg));
	for (int i = 0; i < segCount; i++) {
		struct pastseg t;
		t.ms = segments[i].ms;
		t.isSkipped  = segments[i].isSkipped;
		t.isReset    = segments[i].isReset;
		pastRuns[((attempts-1) * segCount) + i] = t;
	}
	calculatePB();
	saveFile();
}

void split()
{
	if (!timerActive)
		return;
	segments[currSeg].ms = currentMS;
	currSeg++;
	if (currSeg >= segCount)
		stop();
}

void unsplit()
{
	if (!timerActive)
		return;
	currSeg--;
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
	km.HOTKS = VC_T;
	km.USPLT = VC_G;
}

void ftime(char *timestr, bool withMS, int rms)
{
	int seconds   = rms / 1000;
	int minutes   = seconds / 60;
	int hours     = minutes / 60;
	//A few better formatted variables for displaying these numbers
	int tms = (rms % 1000) / 10;
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
	int rms = t.tv_nsec / 1000000;
	rms += t.tv_sec * 1000;
	return rms;
}

void drawSegments()
{
	char data[(deltaOn * 10) + (sgmtdurOn * 10) + (pbOn * 10) + 11];
	char segmentTime[11];
	char *zeroStr = "-";
	char deltaTime[11];
	char sgmtTime[11];
	char segTime[11];
	for(int i = 0; i < segCount; i++) {
		ftime(segmentTime, false, pbrun[i].ms);
		if (i >= currSeg) {
			sprintf(data, "%10s%10s%10s%10s", zeroStr, zeroStr, zeroStr, segmentTime);
		} else {
			ftime(deltaTime, false, segments[i].ms - pbrun[i].ms);
			ftime(sgmtTime, false, segments[i].ms - segments[i - 1].ms);
			ftime(segTime, false, segments[i].ms);
			sprintf(data, "%10s%10s%10s%10s", deltaTime, sgmtTime, segTime, segmentTime);
		}
		rghtPrint(6 + i, w, data);
		leftPrint(6 + i, w, segments[i].name);
	}
}

void drawCurrentSegment()
{
	char data[(deltaOn * 10) + (sgmtdurOn * 10) + (pbOn * 10) + 11];
	strcpy(data, "");
	char pbTime[11];
	char deltaTime[11];
	char sgmtTime[11];
	char segTime[11];
	if (deltaOn) {
		ftime(deltaTime, false, currentMS - pbrun[currSeg].ms);
		strcat(data, deltaTime);
	}
	if (sgmtdurOn) {
		if (currSeg == 0)
			ftime(sgmtTime, false, currentMS);
		else
			ftime(sgmtTime, false, currentMS - segments[currSeg - 1].ms);
		strcat(data, sgmtTime);
	}
	ftime(segTime, false, currentMS);
	strcat(data, segTime);
	if (pbOn) {
		ftime(pbTime, true, pbrun[currSeg].ms);
		strcat(data, pbTime);
	}
	data[(deltaOn * 10) + (sgmtdurOn * 10) + (pbOn * 10) + 11] = '\0';
	rghtPrint(6 + currSeg, w, data);
	leftPrint(6 + currSeg, w, segments[currSeg].name);
}

void drawDisplay()
{
	if (resized) {
		clrScreen();
		drawSegments();
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
	printf("\033[5;3H[dsph]");
	if (timerActive) {
		drawCurrentSegment();
		struct timespec delta;
		sub_timespec(timestart, finish, &delta);
		currentMS = timespecToMS(delta);
	}
	drawHLine(segCount + 6, w);
	ftime(currentTime, true, currentMS);
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
	resized = true;
}

void calculatePB()
{
	if (attempts == 0)
		return;
	int bestMS      = INT_MAX;
	int bestAttempt = 0;
	for (int i = 0; i < attempts; i++) {
		int run = i * segCount;
		bool valid = true;
		for (int j = 0; j < segCount; j++) {
			if (pastRuns[run + j].isSkipped == true ||
					pastRuns[run + j].isReset == true)
				valid = false;
		}
		if (valid && pastRuns[run + segCount - 1].ms < bestMS) {
			bestAttempt = i;
			bestMS = pastRuns[run + segCount - 1].ms;
		}
	}
	for (int i = 0; i < segCount; i++) {
		pbrun[i].ms = pastRuns[(bestAttempt * segCount) + i].ms;
	}
}

//TODO: it'll be more efficent if all the segments pointers point at the same
//instance of the segments name, instead of copying the contents over
void loadFile()
{
	//char path[256];
	//strcat(strcpy(path, getenv("HOME")), "/.config/qtimer");
	//mkdir(path, 0777);
	//strcat(strcpy(path, getenv("HOME")), "/.config/qtimer/keymaps");
	//mkdir(path, 0777);
	//strcat(strcpy(path, getenv("HOME")), "/.config/qtimer/keymaps/default");
	
	//FILE* fp = fopen(path, "r");
	// READING THE FILE TO A BUFFER
	//fclose(fp);
	

	char *buffer = NULL;
	long length;
	FILE *f = fopen(filepath, "rb");
	if (f == NULL)
		return;

	fseek(f, 0, SEEK_END);
	length = ftell(f);
	fseek(f, 0, SEEK_SET);
	buffer = malloc(length + 1);
	if (buffer != NULL) 
		fread(buffer, 1, length, f);
	fclose(f);
	buffer[length] = '\0';

	cJSON *splitfile = cJSON_Parse(buffer);

	free(buffer);

	cJSON *schema = cJSON_GetItem(splitfile, "_schemaVersion");
	if (schema) {
		importSplitsIO(splitfile);
		return;
	}

	cJSON *game = NULL;
	cJSON *cate = NULL;
	cJSON *atts = NULL;
	cJSON *segs = NULL;
	cJSON *runs = NULL;
	game = cJSON_GetItem(splitfile, "game");
	cate = cJSON_GetItem(splitfile, "category");
	atts = cJSON_GetItem(splitfile, "attempts");
	segs = cJSON_GetItem(splitfile, "segments");
	runs = cJSON_GetItem(splitfile, "history");
	
	if (game) {
		cJSON *title = cJSON_GetItem(game, "name");
		if (cJSON_IsString(title) && (title->valuestring != NULL)) {
			gameTitle = malloc(strlen(title->valuestring));
			strcpy(gameTitle, title->valuestring);
		}
	}
	if (cate) {
		cJSON *title = cJSON_GetItem(cate, "name");
		if (cJSON_IsString(title) && (title->valuestring != NULL)) {
			categoryTitle = malloc(strlen(title->valuestring));
			strcpy(categoryTitle, title->valuestring);
		}
	}
	if (atts) {
		cJSON *total = cJSON_GetItem(atts, "total");
		if (cJSON_IsNumber(total))
			attempts = total->valueint;
	}
	if (segs) {
		segCount = cJSON_GetArraySize(segs);
		segments = calloc(segCount, sizeof(struct segment));
		pbrun    = calloc(segCount, sizeof(struct segment));
		wrrun    = calloc(segCount, sizeof(struct segment));
		bestsegs = calloc(segCount, sizeof(struct segment));
		int it = 0;
		cJSON *iterator = NULL;
		cJSON *segname = NULL;
		cJSON_ArrayForEach(iterator, segs) {
			segname = cJSON_GetItem(iterator, "name");
			if (cJSON_IsString(segname) && (segname->valuestring != NULL)) {
				segments[it].name = malloc(strlen(segname->valuestring));
				strcpy(segments[it].name, segname->valuestring);
			}
			it++;
		}
		
	}
	if (runs) {
		pastRuns = calloc(cJSON_GetArraySize(runs) * segCount, sizeof(struct pastseg));
		int oI = 0;
		cJSON *oIterator = NULL;
		cJSON_ArrayForEach(oIterator, runs) {
			int iI = 0;
			cJSON *iIterator = NULL;
			cJSON_ArrayForEach(iIterator, oIterator) {
				struct pastseg t;
				
				cJSON *rms = cJSON_GetItem(iIterator, "rms");
				cJSON *skp = cJSON_GetItem(iIterator, "skipped");
				cJSON *rst = cJSON_GetItem(iIterator, "reset");

				t.ms = rms->valueint;
				if (cJSON_IsTrue(skp))
					t.isSkipped = true;
				else
					t.isSkipped = false;
				if (cJSON_IsTrue(rst))
					t.isReset = true;
				else
					t.isReset = false;

				pastRuns[(oI * segCount) + iI] = t;
				iI++;
			}
			oI++;
		}
	}
	cJSON_Delete(splitfile);
	calculatePB();
}

//Imports game/catagory names and segment names
void importSplitsIO(cJSON *splitfile)
{
	cJSON *game = NULL;
	cJSON *cate = NULL;
	cJSON *segs = NULL;
	game = cJSON_GetItem(splitfile, "game");
	cate = cJSON_GetItem(splitfile, "category");
	segs = cJSON_GetItem(splitfile, "segments");

	if (game) {
		cJSON *title = cJSON_GetItem(game, "longname");
		if (cJSON_IsString(title) && (title->valuestring != NULL)) {
			gameTitle = malloc(strlen(title->valuestring));
			strcpy(gameTitle, title->valuestring);
		}
	}
	if (cate) {
		cJSON *title = cJSON_GetItem(cate, "longname");
		if (cJSON_IsString(title) && (title->valuestring != NULL)) {
			categoryTitle = malloc(strlen(title->valuestring));
			strcpy(categoryTitle, title->valuestring);
		}
	}
	if (segs) {
		segCount = cJSON_GetArraySize(segs);
		segments = calloc(segCount, sizeof(struct segment));
		pbrun    = calloc(segCount, sizeof(struct segment));
		wrrun    = calloc(segCount, sizeof(struct segment));
		bestsegs = calloc(segCount, sizeof(struct segment));

		cJSON *segname  = NULL;

		int it = 0;
		cJSON *iterator = NULL;
		cJSON_ArrayForEach(iterator, segs) {
			segname = cJSON_GetItem(iterator, "name");
			if (cJSON_IsString(segname) && (segname->valuestring != NULL)) {
				segments[it].name = malloc(strlen(segname->valuestring));
				strcpy(segments[it].name, segname->valuestring);
			}
			it++;
		}
	}
	cJSON_Delete(splitfile);
}

void saveFile()
{
	if (timerActive)
		return;

	cJSON *splitfile = cJSON_CreateObject();
	
	cJSON *game = cJSON_CreateObject();
	cJSON *cate = cJSON_CreateObject();
	cJSON *atts = cJSON_CreateObject();
	cJSON *segs = cJSON_CreateArray();
	cJSON *runs = cJSON_CreateArray();
	
	cJSON *gameName = cJSON_CreateString(gameTitle);
	cJSON *cateName = cJSON_CreateString(categoryTitle);
	cJSON *attCount = cJSON_CreateNumber(attempts);

	cJSON_AddItemToObject(game, "name", gameName);
	cJSON_AddItemToObject(cate, "name", cateName);
	cJSON_AddItemToObject(atts, "total", attCount);

	cJSON_AddItemToObject(splitfile, "game", game);
	cJSON_AddItemToObject(splitfile, "category", cate);
	cJSON_AddItemToObject(splitfile, "attempts", atts);

	for(int i = 0; i < segCount; i++) {
		cJSON *seg  = cJSON_CreateObject();
		cJSON *segn = cJSON_CreateString(segments[i].name);
		cJSON_AddItemToObject(seg, "name", segn);
		cJSON_AddItemToArray(segs, seg);
	}
	cJSON_AddItemToObject(splitfile, "segments", segs);

	for (int i = 0; i < attempts; i++) {
		cJSON *run = cJSON_CreateArray();
		for (int j = 0; j < segCount; j++) {
			struct pastseg t = pastRuns[(i * segCount) + j];
			cJSON *seg = cJSON_CreateObject();

			cJSON *tim = cJSON_CreateNumber(t.ms);
			cJSON *skp = cJSON_CreateBool(t.isSkipped);
			cJSON *rst = cJSON_CreateBool(t.isReset);

			cJSON_AddItemToObject(seg, "rms", tim);
			cJSON_AddItemToObject(seg, "skipped", skp);
			cJSON_AddItemToObject(seg, "reset", rst);

			cJSON_AddItemToArray(run, seg);
		}
		cJSON_AddItemToArray(runs, run);
	}
	cJSON_AddItemToObject(splitfile, "history", runs);

	char *string = cJSON_Print(splitfile);
	if (string != NULL) {
		FILE *f = fopen(filepath, "w");
		if (f == NULL)
			return;
		
		fwrite(string, 1, strlen(string), f);
		
		fclose(f);
	}

	cJSON_Delete(splitfile);
}

int main(int argc, char **argv)
{
	timerActive = false;
	filepath = argv[1];
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
		initScreen(bg, fg);
		loadFile();
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

