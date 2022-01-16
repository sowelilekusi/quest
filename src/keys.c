#include "keys.h"

bool hotkeys_enabled = true;
char buf;
int pipefd[2];
struct keymap km;

char *keystrings[KEYNUM] = {
	  "a",   "b",   "c",   "d",   "e",   "f",
	  "g",   "h",   "i",   "j",   "k",   "l",
	  "m",   "n",   "o",   "p",   "q",   "r",
	  "s",   "t",   "u",   "v",   "w",   "x",
	  "y",   "z",   "1",   "2",   "3",   "4",
	  "5",   "6",   "7",   "8",   "9",   "0",
	 "F1",  "F2",  "F3",  "F4",  "F5",  "F6",
	 "F7",  "F8",  "F9", "F10", "F11", "F12",
	"F13", "F14", "F15", "F16", "F17", "F18",
	"F19", "F20", "F21", "F22", "F23", "F24",
	"ESC",   "`",   "-",   "=", "BSP", "TAB",
	"CAP",   "[",   "]",  "\\",   ";",  "\"",
	"ENT",   ",",   ".",   "/",   " ",
};

uint16_t uiohookKeycodes[KEYNUM] = {
	        VC_A,            VC_B,             VC_C,          VC_D,         VC_E,  VC_F,
	        VC_G,            VC_H,             VC_I,          VC_J,         VC_K,  VC_L,
	        VC_M,            VC_N,             VC_O,          VC_P,         VC_Q,  VC_R,
	        VC_S,            VC_T,             VC_U,          VC_V,         VC_W,  VC_X,
	        VC_Y,            VC_Z,             VC_1,          VC_2,         VC_3,  VC_4,
	        VC_5,            VC_6,             VC_7,          VC_8,         VC_9,  VC_0,
	       VC_F1,           VC_F2,            VC_F3,         VC_F4,        VC_F5, VC_F6,
	       VC_F7,           VC_F8,            VC_F9,        VC_F10,       VC_F11, VC_F12,
	      VC_F13,          VC_F14,           VC_F15,        VC_F16,       VC_F17, VC_F18,
	      VC_F19,          VC_F20,           VC_F21,        VC_F22,       VC_F23, VC_F24,
	   VC_ESCAPE,    VC_BACKQUOTE,         VC_MINUS,     VC_EQUALS, VC_BACKSPACE,   VC_TAB,
	VC_CAPS_LOCK, VC_OPEN_BRACKET, VC_CLOSE_BRACKET, VC_BACK_SLASH, VC_SEMICOLON, VC_QUOTE,
	    VC_ENTER,        VC_COMMA,        VC_PERIOD,      VC_SLASH,     VC_SPACE
};

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

void loadKeymap()
{
	km.START = VC_R;
	km.STOP  = VC_F;
	km.PAUSE = VC_D;
	km.SPLIT = VC_E;
	km.HOTKS = VC_T;
	km.USPLT = VC_G;
	km.SKIP  = VC_V;
}

uint16_t keystringToKeycode(char *keystring) {
	for (int i = 0; i < KEYNUM; i++)
		if (strcmp(keystring, keystrings[i]) == 0)
			return uiohookKeycodes[i];
	return NULL;
}

