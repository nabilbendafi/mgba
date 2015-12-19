/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef GUI_H
#define GUI_H

#include "util/common.h"

#include "util/vector.h"

struct GUIFont;

enum GUIInput {
	GUI_INPUT_NONE = -1,
	GUI_INPUT_SELECT = 0,
	GUI_INPUT_BACK,
	GUI_INPUT_CANCEL,

	GUI_INPUT_UP,
	GUI_INPUT_DOWN,
	GUI_INPUT_LEFT,
	GUI_INPUT_RIGHT,

	GUI_INPUT_USER_START = 0x10,

	GUI_INPUT_MAX = 0x20
};

enum GUICursorState {
	GUI_CURSOR_NOT_PRESENT,
	GUI_CURSOR_UP,
	GUI_CURSOR_DOWN,
	GUI_CURSOR_CLICKED,
	GUI_CURSOR_DRAGGING
};

enum {
	BATTERY_EMPTY = 0,
	BATTERY_LOW = 1,
	BATTERY_HALF = 2,
	BATTERY_HIGH = 3,
	BATTERY_FULL = 4,

	BATTERY_CHARGING = 8
};

struct GUIBackground {
	void (*draw)(struct GUIBackground*, void* context);
};

struct GUIParams {
	unsigned width;
	unsigned height;
	const struct GUIFont* font;
	const char* basePath;

	void (*drawStart)(void);
	void (*drawEnd)(void);
	uint32_t (*pollInput)(void);
	enum GUICursorState (*pollCursor)(int* x, int* y);
	int (*batteryState)(void);
	void (*guiPrepare)(void);
	void (*guiFinish)(void);

	// State
	int inputHistory[GUI_INPUT_MAX];
	enum GUICursorState cursorState;
	int cx, cy;

	// Directories
	char currentPath[PATH_MAX];
	size_t fileIndex;
};

#define GUI_PARAMS_TRAIL {}, GUI_CURSOR_NOT_PRESENT, 0, 0, "", 0

void GUIInit(struct GUIParams* params);
void GUIPollInput(struct GUIParams* params, uint32_t* newInput, uint32_t* heldInput);
enum GUICursorState GUIPollCursor(struct GUIParams* params, int* x, int* y);
void GUIInvalidateKeys(struct GUIParams* params);

#endif
