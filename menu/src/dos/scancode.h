/*
colcycle - color cycling image viewer
Copyright (C) 2016  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef KEYB_C_
#error "do not include scancode.h anywhere..."
#endif

/* table with rough translations from set 1 scancodes to ASCII-ish */
static int scantbl[] = {
	0, KEY_ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',		/* 0 - e */
	'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',			/* f - 1c */
	KEY_LCTRL, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',				/* 1d - 29 */
	KEY_LSHIFT, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', KEY_RSHIFT,			/* 2a - 36 */
	KEY_NUM_MUL, KEY_LALT, ' ', KEY_CAPSLK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,			/* 37 - 44 */
	KEY_NUMLK, KEY_SCRLK, KEY_NUM_7, KEY_NUM_8, KEY_NUM_9, KEY_NUM_MINUS, KEY_NUM_4, KEY_NUM_5, KEY_NUM_6, KEY_NUM_PLUS,	/* 45 - 4e */
	KEY_NUM_1, KEY_NUM_2, KEY_NUM_3, KEY_NUM_0, KEY_NUM_DOT, KEY_SYSRQ, 0, 0, KEY_F11, KEY_F12,						/* 4d - 58 */
	0, 0, 0, 0, 0, 0, 0,															/* 59 - 5f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,									/* 60 - 6f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0									/* 70 - 7f */
};

static int scantbl_shift[] = {
	0, KEY_ESC, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',		/* 0 - e */
	'\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',			/* f - 1c */
	KEY_LCTRL, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',				/* 1d - 29 */
	KEY_LSHIFT, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', KEY_RSHIFT,			/* 2a - 36 */
	KEY_NUM_MUL, KEY_LALT, ' ', KEY_CAPSLK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,			/* 37 - 44 */
	KEY_NUMLK, KEY_SCRLK, KEY_NUM_7, KEY_NUM_8, KEY_NUM_9, KEY_NUM_MINUS, KEY_NUM_4, KEY_NUM_5, KEY_NUM_6, KEY_NUM_PLUS,	/* 45 - 4e */
	KEY_NUM_1, KEY_NUM_2, KEY_NUM_3, KEY_NUM_0, KEY_NUM_DOT, KEY_SYSRQ, 0, 0, KEY_F11, KEY_F12,						/* 4d - 58 */
	0, 0, 0, 0, 0, 0, 0,															/* 59 - 5f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,									/* 60 - 6f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0									/* 70 - 7f */
};


/* extended scancodes, after the 0xe0 prefix */
static int scantbl_ext[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			/* 0 - f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\r', KEY_RCTRL, 0, 0,			/* 10 - 1f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			/* 20 - 2f */
	0, 0, 0, 0, 0, KEY_NUM_MINUS, 0, KEY_SYSRQ, KEY_RALT, 0, 0, 0, 0, 0, 0, 0,			/* 30 - 3f */
	0, 0, 0, 0, 0, 0, 0, KEY_HOME, KEY_UP, KEY_PGUP, 0, KEY_LEFT, 0, KEY_RIGHT, 0, KEY_END,	/* 40 - 4f */
	KEY_DOWN, KEY_PGDN, KEY_INS, KEY_DEL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			/* 50 - 5f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			/* 60 - 6f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			/* 70 - 7f */
};

