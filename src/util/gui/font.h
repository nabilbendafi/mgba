/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef GUI_FONT_H
#define GUI_FONT_H

#include "util/common.h"

struct GUIFont;
struct GUIFont* GUIFontCreate(void);
void GUIFontDestroy(struct GUIFont*);

enum GUITextAlignment {
	GUI_TEXT_LEFT = 0,
	GUI_TEXT_CENTER,
	GUI_TEXT_RIGHT
};

struct GUIFontGlyphMetric {
	int width;
	int height;
	struct {
		int top;
		int right;
		int bottom;
		int left;
	} padding;
};

unsigned GUIFontHeight(const struct GUIFont*);
unsigned GUIFontGlyphWidth(const struct GUIFont*, uint32_t glyph);
unsigned GUIFontSpanWidth(const struct GUIFont*, const char* text);

ATTRIBUTE_FORMAT(printf, 6, 7)
void GUIFontPrintf(const struct GUIFont*, int x, int y, enum GUITextAlignment, uint32_t color, const char* text, ...);
void GUIFontPrint(const struct GUIFont*, int x, int y, enum GUITextAlignment, uint32_t color, const char* text);
void GUIFontDrawGlyph(const struct GUIFont*, int x, int y, uint32_t color, uint32_t glyph);

#endif
