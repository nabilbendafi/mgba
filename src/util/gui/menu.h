/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef GUI_MENU_H
#define GUI_MENU_H

#include "util/vector.h"

struct GUIMenu;
struct GUIMenuItem {
	const char* title;
	void* data;
	unsigned state;
	const char** validStates;
	struct GUIMenu* submenu;
};

DECLARE_VECTOR(GUIMenuItemList, struct GUIMenuItem);

struct GUIBackground;
struct GUIMenu {
	const char* title;
	const char* subtitle;
	struct GUIMenuItemList items;
	size_t index;
	struct GUIBackground* background;
};

enum GUIMenuExitReason {
	GUI_MENU_EXIT_ACCEPT,
	GUI_MENU_EXIT_BACK,
	GUI_MENU_EXIT_CANCEL,
};

struct GUIParams;
enum GUIMenuExitReason GUIShowMenu(struct GUIParams* params, struct GUIMenu* menu, struct GUIMenuItem** item);

void GUIDrawBattery(struct GUIParams* params);
void GUIDrawClock(struct GUIParams* params);

#endif
