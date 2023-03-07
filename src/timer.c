#include "timer.h"

//Timekeeping
struct timespec timestart, finish, notif;
int currentMS = 0;
int timeSave  = 0;
bool timerActive = false;

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
	//for now we do this here for reasons explained in stop()
	calculatePB();
	clock_gettime(CLOCK_REALTIME, &timestart);
	timerActive    = true;
	//Reset state of timer
	dirty = true; //find a way to put this in src/display.c
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
	//If this is done here you wont get a chance to peep your deltas on 
	//a pb coz the ui will just update and zero them all out if your current
	//run is the pb run youre now comparing to
	//calculatePB();
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

int timespecToMS(struct timespec t)
{
	return (t.tv_nsec / 1000000) + (t.tv_sec * 1000);
}

void calculatePB()
{
	bool valid      = false;
	//TODO: come back to this value, this function should be redone a little
	int PBsfound    = 0;

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
			PBsfound++;
			bestAttempt = i;
			bestMS = pastRuns[run + segCount - 1].ms;
		}
	}
	if (PBsfound > 0)
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
	configpath = malloc(strlen(path) + 1);
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
			gameTitle = malloc(strlen(title->valuestring) + 1);
			strcpy(gameTitle, title->valuestring);
		}
	}
	if (cate) {
		cJSON *title = cJSON_GetItem(cate, "name");
		if (cJSON_IsString(title) && (title->valuestring != NULL)) {
			categoryTitle = malloc(strlen(title->valuestring) + 1);
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
				segments[it].name = malloc(strlen(segname->valuestring) + 1);
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

