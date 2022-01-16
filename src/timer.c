#include "timer.h"

#define COLSTRLEN 11

//Timekeeping
struct timespec timestart, finish, notif;
int currentMS = 0;
int timeSave  = 0;
bool timerActive = false;

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

//Run data
char *filepath;
char *configpath;
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

void start()
{
	if (timerActive || segCount == 0)
		return;
	clock_gettime(CLOCK_REALTIME, &timestart);
	timerActive    = true;
	//Reset state of timer
	dirty = true;
	for(int i = 0; i < segCount; i++) {
		segments[i].ms        = 0;
		segments[i].isSkipped = false;
		segments[i].isReset   = false;
	}
	currSeg = 0;
}

void stop()
{
	if (!timerActive)
		return;
	if (currSeg < segCount)
		segments[currSeg].isReset = true;
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

void skip()
{
	if (!timerActive)
		return;
	if (currSeg < segCount)
		segments[currSeg].isSkipped = true;
	currSeg++;
	if (currSeg >= segCount)
		stop();
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

int timespecToMS(struct timespec t)
{
	return (t.tv_nsec / 1000000) + (t.tv_sec * 1000);
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

void calculatePB()
{
	bool valid      = false;
	int bestMS      = INT_MAX;
	int bestAttempt = 0;

	if (attempts == 0)
		return;
	for (int i = 0; i < attempts; i++) {
		int run = i * segCount;
		valid = true;
		for (int j = 0; j < segCount; j++) {
			if (pastRuns[run + j].isReset == true)
				valid = false;
		}
		if (valid && pastRuns[run + segCount - 1].ms < bestMS) {
			bestAttempt = i;
			bestMS = pastRuns[run + segCount - 1].ms;
		}
	}
	if (valid)
		for (int i = 0; i < segCount; i++)
			pbrun[i].ms = pastRuns[(bestAttempt * segCount) + i].ms;
}

void calculateBestSegs()
{
	if (attempts == 0)
		return;
	for (int i = 0; i < segCount; i++) {
		int bestDuration = INT_MAX;
		for (int j = 0; j < attempts; j++) {
			int duration = pastRuns[(j * segCount) + i].ms;
			if (i != 0)
				duration -= bestsegs[i-1].ms;
			if (duration != 0 && duration < bestDuration)
				bestDuration = duration;
		}
		bestsegs[i].ms = bestDuration;
		if (i != 0)
			bestsegs[i].ms += bestsegs[i-1].ms;
	}
}

void loadConfig()
{
	cJSON *config = NULL;
	char path[256];
	strcat(strcpy(path, getenv("HOME")), "/.config/quest");
	mkdir(path, 0777);
	strcat(strcpy(path, getenv("HOME")), "/.config/quest/keymaps");
	mkdir(path, 0777);
	strcat(strcpy(path, getenv("HOME")), "/.config/quest/keymaps/default");
	configpath = malloc(strlen(path));
	strcpy(configpath, path);
	
	long length;
	FILE* f = fopen(path, "rb");
	if (f != NULL) {
		char *buffer = NULL;
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer = malloc(length + 1);
		if (buffer != NULL)
			fread(buffer, 1, length, f);
		fclose(f);
		buffer[length] = '\0';

		config = cJSON_Parse(buffer);
		free(buffer);

		cJSON *startkey = cJSON_GetItem(config, "start");
		cJSON *stopkey = cJSON_GetItem(config, "stop");
		cJSON *pausekey = cJSON_GetItem(config, "pause");
		cJSON *splitkey = cJSON_GetItem(config, "split");
		cJSON *hotkskey = cJSON_GetItem(config, "toggle hotkeys");
		cJSON *uspltkey = cJSON_GetItem(config, "unsplit");
		cJSON *skipkey = cJSON_GetItem(config, "skip");

		if (cJSON_IsString(startkey) && (startkey->valuestring != NULL))
			km.START = keystringToKeycode(startkey->valuestring);
		if (cJSON_IsString(stopkey) && (stopkey->valuestring != NULL))
			km.STOP = keystringToKeycode(stopkey->valuestring);
		if (cJSON_IsString(pausekey) && (pausekey->valuestring != NULL))
			km.PAUSE = keystringToKeycode(pausekey->valuestring);
		if (cJSON_IsString(splitkey) && (splitkey->valuestring != NULL))
			km.SPLIT = keystringToKeycode(splitkey->valuestring);
		if (cJSON_IsString(hotkskey) && (hotkskey->valuestring != NULL))
			km.HOTKS = keystringToKeycode(hotkskey->valuestring);
		if (cJSON_IsString(uspltkey) && (uspltkey->valuestring != NULL))
			km.USPLT = keystringToKeycode(uspltkey->valuestring);
		if (cJSON_IsString(skipkey) && (skipkey->valuestring != NULL))
			km.SKIP = keystringToKeycode(skipkey->valuestring);
	} else {
		config = cJSON_CreateObject();
		km.START = VC_R;
		km.STOP  = VC_F;
		km.PAUSE = VC_D;
		km.SPLIT = VC_E;
		km.HOTKS = VC_T;
		km.USPLT = VC_G;
		km.SKIP  = VC_V;
		cJSON *startkey = cJSON_CreateString("r");
		cJSON_AddItemToObject(config, "start", startkey);
		cJSON *stopkey = cJSON_CreateString("f");
		cJSON_AddItemToObject(config, "stop", stopkey);
		cJSON *pausekey = cJSON_CreateString("d");
		cJSON_AddItemToObject(config, "pause", pausekey);
		cJSON *splitkey = cJSON_CreateString("e");
		cJSON_AddItemToObject(config, "split", splitkey);
		cJSON *hotkskey = cJSON_CreateString("t");
		cJSON_AddItemToObject(config, "toggle hotkeys", hotkskey);
		cJSON *uspltkey = cJSON_CreateString("g");
		cJSON_AddItemToObject(config, "unsplit", uspltkey);
		cJSON *skipkey = cJSON_CreateString("v");
		cJSON_AddItemToObject(config, "skip", skipkey);
		saveConfig(config);
	}
	cJSON_Delete(config);
}

void saveConfig(cJSON *config)
{
	char *string = cJSON_Print(config);
	if (string != NULL) {
		FILE *f = fopen(configpath, "w");
		if (f == NULL)
			return;
		
		fwrite(string, 1, strlen(string), f);
		
		fclose(f);
	}
}

//TODO: it'll be more efficent if all the segments pointers point at the same
//instance of the segments name, instead of copying the contents over
void loadFile()
{
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
				
				cJSON *rms = cJSON_GetItem(iIterator, "m");
				cJSON *skp = cJSON_GetItem(iIterator, "s");
				cJSON *rst = cJSON_GetItem(iIterator, "r");

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
	calculateBestSegs();
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

			cJSON_AddItemToObject(seg, "m", tim);
			cJSON_AddItemToObject(seg, "s", skp);
			cJSON_AddItemToObject(seg, "r", rst);

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
	filepath = argv[1];
	hook_set_logger_proc(&logger_proc);
	hook_set_dispatch_proc(&dispatch_proc);

	//IPC pipe
	pid_t cpid;

	pipe(pipefd);
	fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
	loadConfig();
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
			struct timespec delta;
			clock_gettime(CLOCK_REALTIME, &finish);
			sub_timespec(notif, finish, &delta);
			if (delta.tv_sec == 3)
				clearNotif();
			if (timerActive) {
				sub_timespec(timestart, finish, &delta);
				currentMS = timespecToMS(delta);
			}
			drawDisplay();
			usleep(4000);
		}
		resetScreen();
		kill(cpid, SIGTERM);
	}

	return 0;
}

