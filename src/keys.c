#include "keys.h"

bool hotkeys_enabled = true;
char buf;
int pipefd[2];
struct keymap km;

char *keystrings[2] = {"a", "b"};

bool logger_proc(unsigned int level, const char *format, ...) {
	return 0;
}

void dispatch_proc(uiohook_event * const event) {
	switch (event->type) {
		case EVENT_KEY_PRESSED:
			buf = -1;
			if (event->data.keyboard.keycode == km.START)
				buf = K_START;
			if (event->data.keyboard.keycode == km.STOP)
				buf = K_STOP;
			if (event->data.keyboard.keycode == km.PAUSE)
				buf = K_PAUSE;
			if (event->data.keyboard.keycode == km.SPLIT)
				buf = K_SPLIT;
			if (event->data.keyboard.keycode == km.HOTKS)
				buf = K_HOTKS;
			if (event->data.keyboard.keycode == km.USPLT)
				buf = K_USPLT;
			if (event->data.keyboard.keycode == km.SKIP)
				buf = K_SKIP;
			write(pipefd[1], &buf, 1);
		default:
			break;
	}
}

int handleInput()
{
	//Non global hotkeys	
	char t;
	read(in, &t, 1);
	if (t == 'c')
		toggleCompact();
	if (t == 'q')
		return 1;

	//Global hotkeys
	ssize_t rd = read(pipefd[0], &buf, 1);
	if ((!hotkeys_enabled && buf != K_HOTKS) || rd == -1)
		return 0;
	if (buf == K_SPLIT)
		split();
	if (buf == K_START)
		start();
	if (buf == K_STOP)
		stop();
	if (buf == K_PAUSE)
		tpause();
	if (buf == K_HOTKS) {
		hotkeys_enabled = !hotkeys_enabled;
		if (hotkeys_enabled)
			drawNotif("Global hotkeys enabled");
		else
			drawNotif("Global hotkeys disabled");
	}
	if (buf == K_USPLT)
		unsplit();
	if (buf == K_SKIP)
		skip();
	return 0;
}
