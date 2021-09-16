#ifndef KEYS_H
#define KEYS_H

#include <uiohook.h>
#include <unistd.h>
#include "timer.h"

#define K_START 1
#define K_STOP  2
#define K_PAUSE 3
#define K_SPLIT 4
#define K_CLOSE 5
#define K_HOTKS 6

struct keymap
{
	uint16_t START;
	uint16_t STOP;
	uint16_t PAUSE;
	uint16_t SPLIT;
	uint16_t CLOSE;
	uint16_t HOTKS;
};

extern char buf;
extern int pipefd[2];
extern struct keymap km;

bool logger_proc(unsigned int level, const char *format, ...);
void dispatch_proc(uiohook_event * const event);
int handleInput();

#endif

