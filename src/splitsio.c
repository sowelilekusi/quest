#include "splitsio.h"

//Splits.io data
const char *schemaver  = "v1.0.1";
const char *timersname = "quest";
const char *timerlname = "Quinn's Utterly Elegant Speedrun Timer";
const char *timerver   = "v0.5.1";
const char *timerlink  = "https://github.com/SilentFungus/quest";

//Imports game/catagory names and segment names
void importSplitsIO(cJSON *splitfile)
{
	cJSON *p = NULL;
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

void exportSplitsIO()
{
	//cJSON root node
	cJSON *export          = NULL;

	//Schema version
	cJSON *schema          = NULL;

	//Links
	cJSON *links_root      = NULL;
	cJSON *speedruncom_id  = NULL;
	cJSON *splitsio_id     = NULL;
	
	//Timer
	cJSON *timer_root      = NULL;
	cJSON *timer_shortname = NULL;
	cJSON *timer_longname  = NULL;
	cJSON *timer_version   = NULL;
	cJSON *timer_website   = NULL;

	//Attempts
	cJSON *attempts_root   = NULL;
	cJSON *attempts_total  = NULL;
	cJSON *histories       = NULL;
	cJSON *history_root    = NULL;
	cJSON *history_attmpt  = NULL;
	cJSON *history_dur     = NULL;
	cJSON *history_dur_rms = NULL;
	cJSON *history_dur_gms = NULL;

	//Supplementary data
	cJSON *image_url       = NULL;
	cJSON *video_url       = NULL;

	//Time
	cJSON *started_at      = NULL;
	cJSON *ended_at        = NULL;

	//Pauses
	cJSON *pauses_root     = NULL;
	cJSON *pause_started   = NULL;
	cJSON *pause_ended     = NULL;

	//Game
	cJSON *game_root       = NULL;
	cJSON *game_longname   = NULL;
	cJSON *game_shortname  = NULL;
	cJSON *game_links      = NULL;
	cJSON *game_srcom_id   = NULL;
	cJSON *game_splits_id  = NULL;

	//Catagory
	cJSON *cate_root       = NULL;
	cJSON *cate_longname   = NULL;
	cJSON *cate_shortname  = NULL;
	cJSON *cate_links      = NULL;
	cJSON *cate_splits_id  = NULL;
	cJSON *cate_spdrun_id  = NULL;

	//Runners
	cJSON *runner_root     = NULL;
	cJSON *runner_longname = NULL;
	cJSON *runner_shrtname = NULL;
	cJSON *runner_links    = NULL;
	cJSON *runner_twitch   = NULL;
	cJSON *runner_spltsio  = NULL;
	cJSON *runner_spdrun   = NULL;
	cJSON *runner_twitter  = NULL;

	//Segments
	cJSON *seg_root        = NULL;
	cJSON *seg_name        = NULL;
	cJSON *seg_ended       = NULL;
	cJSON *seg_ended_rms   = NULL;
	cJSON *seg_ended_gms   = NULL;
	cJSON *seg_best        = NULL;
	cJSON *seg_best_rms    = NULL;
	cJSON *seg_best_gms    = NULL;
	cJSON *seg_is_skipped  = NULL;
	cJSON *seg_is_reset    = NULL;
	cJSON *seg_histories   = NULL;
	cJSON *seg_hst_attmp   = NULL;
	cJSON *seg_hst_end     = NULL;
	cJSON *seg_hst_end_rms = NULL;
	cJSON *seg_hst_end_gms = NULL;
	cJSON *seg_hst_skp     = NULL;
	cJSON *seg_hst_rst     = NULL;
}
