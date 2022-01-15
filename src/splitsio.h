#ifndef SPLITSIO
#define SPLITSIO

#include "cJSON.h"
#include "timer.h"

const char *schemaver;
const char *timersname;
const char *timerlname;
const char *timerver;
const char *timerlink;

void importSplitsIO(cJSON *splitfile);
void exportSplitsIO();

#endif
