#ifndef KEYS_H
#define KEYS_H

#include "uiohook.h"
#include <unistd.h>
#include "timer.h"
#include "display.h"

#define K_START 1
#define K_STOP  2
#define K_PAUSE 3
#define K_SPLIT 4
#define K_HOTKS 5
#define K_USPLT 6
#define K_SKIP  7

#define KEYNUM 77

extern bool hotkeys_enabled;

struct keymap
{
	uint16_t START;
	uint16_t STOP;
	uint16_t PAUSE;
	uint16_t SPLIT;
	uint16_t HOTKS;
	uint16_t USPLT;
	uint16_t SKIP;
};

extern char buf;
extern int pipefd[2];
extern struct keymap km;

extern char *keystrings[77];

bool logger_proc(unsigned int level, const char *format, ...);
void dispatch_proc(uiohook_event * const event);
int handleInput();
void loadKeymap();
uint16_t keystringToKeycode(char *keystring);

#endif

