#include "keys.h"

char buf;
int pipefd[2];
struct keymap km;

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
