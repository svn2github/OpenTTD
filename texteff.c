/* $Id$ */

#include "stdafx.h"
#include "openttd.h"
#include "functions.h"
#include "strings.h"
#include "gfx.h"
#include "viewport.h"
#include "saveload.h"
#include "hal.h"
#include "console.h"
#include "string.h"
#include "variables.h"
#include "table/sprites.h"
#include <stdarg.h> /* va_list */
#include "date.h"

typedef struct TextEffect {
	StringID string_id;
	int32 x;
	int32 y;
	int32 right;
	int32 bottom;
	uint16 duration;
	uint32 params_1;
	uint32 params_2;
} TextEffect;

#define MAX_TEXTMESSAGE_LENGTH 250

typedef struct TextMessage {
	char message[MAX_TEXTMESSAGE_LENGTH];
	uint16 color;
	Date end_date;
} TextMessage;

#define MAX_CHAT_MESSAGES 10
static TextEffect _text_effect_list[30];
static TextMessage _textmsg_list[MAX_CHAT_MESSAGES];
TileIndex _animated_tile_list[256];

static bool _textmessage_dirty = false;
static bool _textmessage_visible = false;

/* The chatbox grows from the bottom so the coordinates are pixels from
 * the left and pixels from the bottom. The height is the maximum height */
static const Oblong _textmsg_box = {10, 30, 400, 150};
static Pixel _textmessage_backup[150 * 400]; // (height * width)

extern void memcpy_pitch(void *d, void *s, int w, int h, int spitch, int dpitch);

// Duration is in game-days
void CDECL AddTextMessage(uint16 color, uint8 duration, const char *message, ...)
{
	char buf[MAX_TEXTMESSAGE_LENGTH];
	va_list va;
	size_t length;
	uint i;

	va_start(va, message);
	vsnprintf(buf, lengthof(buf), message, va);
	va_end(va);

	/* Special color magic */
	if ((color & 0xFF) == 0xC9) color = 0x1CA;

	/* Cut the message till it fits inside the chatbox */
	length = strlen(buf);
	while (GetStringBoundingBox(buf).width > _textmsg_box.width - 9) buf[--length] = '\0';

	/* Find an empty spot and put the message there */
	for (i = 0; i < MAX_CHAT_MESSAGES; i++) {
		if (_textmsg_list[i].message[0] == '\0') {
			// Empty spot
			ttd_strlcpy(_textmsg_list[i].message, buf, sizeof(_textmsg_list[i].message));
			_textmsg_list[i].color = color;
			_textmsg_list[i].end_date = _date + duration;

			_textmessage_dirty = true;
			return;
		}
	}

	// We did not found a free spot, trash the first one, and add to the end
	memmove(&_textmsg_list[0], &_textmsg_list[1], sizeof(_textmsg_list[0]) * (MAX_CHAT_MESSAGES - 1));
	ttd_strlcpy(_textmsg_list[MAX_CHAT_MESSAGES - 1].message, buf, sizeof(_textmsg_list[MAX_CHAT_MESSAGES - 1].message));
	_textmsg_list[MAX_CHAT_MESSAGES - 1].color = color;
	_textmsg_list[MAX_CHAT_MESSAGES - 1].end_date = _date + duration;

	_textmessage_dirty = true;
}

void InitTextMessage(void)
{
	uint i;

	for (i = 0; i < MAX_CHAT_MESSAGES; i++) {
		_textmsg_list[i].message[0] = '\0';
	}
}

// Hide the textbox
void UndrawTextMessage(void)
{
	if (_textmessage_visible) {
		// Sometimes we also need to hide the cursor
		//   This is because both textmessage and the cursor take a shot of the
		//   screen before drawing.
		//   Now the textmessage takes his shot and paints his data before the cursor
		//   does, so in the shot of the cursor is the screen-data of the textmessage
		//   included when the cursor hangs somewhere over the textmessage. To
		//   avoid wrong repaints, we undraw the cursor in that case, and everything
		//   looks nicely ;)
		// (and now hope this story above makes sense to you ;))

		if (_cursor.visible) {
			if (_cursor.draw_pos.x + _cursor.draw_size.x >= _textmsg_box.x &&
				_cursor.draw_pos.x <= _textmsg_box.x + _textmsg_box.width &&
				_cursor.draw_pos.y + _cursor.draw_size.y >= _screen.height - _textmsg_box.y - _textmsg_box.height &&
				_cursor.draw_pos.y <= _screen.height - _textmsg_box.y) {
				UndrawMouseCursor();
			}
		}

		_textmessage_visible = false;
		// Put our 'shot' back to the screen
		memcpy_pitch(
			_screen.dst_ptr + _textmsg_box.x + (_screen.height - _textmsg_box.y - _textmsg_box.height) * _screen.pitch,
			_textmessage_backup,
			_textmsg_box.width, _textmsg_box.height, _textmsg_box.width, _screen.pitch);

		// And make sure it is updated next time
		_video_driver->make_dirty(_textmsg_box.x, _screen.height - _textmsg_box.y - _textmsg_box.height, _textmsg_box.width, _textmsg_box.height);

		_textmessage_dirty = true;
	}
}

// Check if a message is expired every day
void TextMessageDailyLoop(void)
{
	uint i;

	for (i = 0; i < MAX_CHAT_MESSAGES; i++) {
		if (_textmsg_list[i].message[0] == '\0') continue;

		if (_date > _textmsg_list[i].end_date) {
			/* Move the remaining messages over the current message */
			if (i != MAX_CHAT_MESSAGES - 1)
				memmove(&_textmsg_list[i], &_textmsg_list[i + 1], sizeof(_textmsg_list[i]) * (MAX_CHAT_MESSAGES - i - 1));

			/* Mark the last item as empty */
			_textmsg_list[MAX_CHAT_MESSAGES - 1].message[0] = '\0';
			_textmessage_dirty = true;

			/* Go one item back, because we moved the array 1 to the left */
			i--;
		}
	}
}

// Draw the textmessage-box
void DrawTextMessage(void)
{
	int i, j;
	bool has_message;

	if (!_textmessage_dirty) return;

	// First undraw if needed
	UndrawTextMessage();

	if (_iconsole_mode == ICONSOLE_FULL)
		return;

	/* Check if we have anything to draw at all */
	has_message = false;
	for ( i = 0; i < MAX_CHAT_MESSAGES; i++) {
		if (_textmsg_list[i].message[0] == '\0') break;

		has_message = true;
	}
	if (!has_message) return;

	// Make a copy of the screen as it is before painting (for undraw)
	memcpy_pitch(
		_textmessage_backup,
		_screen.dst_ptr + _textmsg_box.x + (_screen.height - _textmsg_box.y - _textmsg_box.height) * _screen.pitch,
		_textmsg_box.width, _textmsg_box.height, _screen.pitch, _textmsg_box.width);

	// Switch to _screen painting
	_cur_dpi = &_screen;

	j = 0;
	// Paint the messages
	for (i = MAX_CHAT_MESSAGES - 1; i >= 0; i--) {
		if (_textmsg_list[i].message[0] == '\0') continue;

		j++;
		GfxFillRect(_textmsg_box.x, _screen.height-_textmsg_box.y-j*13-2, _textmsg_box.x+_textmsg_box.width - 1, _screen.height-_textmsg_box.y-j*13+10, /* black, but with some alpha */ 0x322 | USE_COLORTABLE);

		DoDrawString(_textmsg_list[i].message, _textmsg_box.x + 2, _screen.height - _textmsg_box.y - j * 13 - 1, 0x10);
		DoDrawString(_textmsg_list[i].message, _textmsg_box.x + 3, _screen.height - _textmsg_box.y - j * 13, _textmsg_list[i].color);
	}

	// Make sure the data is updated next flush
	_video_driver->make_dirty(_textmsg_box.x, _screen.height - _textmsg_box.y - _textmsg_box.height, _textmsg_box.width, _textmsg_box.height);

	_textmessage_visible = true;
	_textmessage_dirty = false;
}

static void MarkTextEffectAreaDirty(TextEffect *te)
{
	MarkAllViewportsDirty(
		te->x,
		te->y - 1,
		(te->right - te->x)*2 + te->x + 1,
		(te->bottom - (te->y - 1)) * 2 + (te->y - 1) + 1
	);
}

void AddTextEffect(StringID msg, int x, int y, uint16 duration)
{
	TextEffect *te;
	int w;
	char buffer[100];

	if (_game_mode == GM_MENU)
		return;

	for (te = _text_effect_list; te->string_id != INVALID_STRING_ID; ) {
		if (++te == endof(_text_effect_list)) return;
	}

	te->string_id = msg;
	te->duration = duration;
	te->y = y - 5;
	te->bottom = y + 5;
	te->params_1 = GetDParam(0);
	te->params_2 = GetDParam(4);

	GetString(buffer, msg, lastof(buffer));
	w = GetStringBoundingBox(buffer).width;

	te->x = x - (w >> 1);
	te->right = x + (w >> 1) - 1;
	MarkTextEffectAreaDirty(te);
}

static void MoveTextEffect(TextEffect *te)
{
	if (te->duration < 8) {
		te->string_id = INVALID_STRING_ID;
	} else {
		te->duration -= 8;
		te->y--;
		te->bottom--;
	}
	MarkTextEffectAreaDirty(te);
}

void MoveAllTextEffects(void)
{
	TextEffect *te;

	for (te = _text_effect_list; te != endof(_text_effect_list); te++) {
		if (te->string_id != INVALID_STRING_ID) MoveTextEffect(te);
	}
}

void InitTextEffects(void)
{
	TextEffect *te;

	for (te = _text_effect_list; te != endof(_text_effect_list); te++) {
		te->string_id = INVALID_STRING_ID;
	}
}

void DrawTextEffects(DrawPixelInfo *dpi)
{
	const TextEffect* te;

	switch (dpi->zoom) {
		case 0:
			for (te = _text_effect_list; te != endof(_text_effect_list); te++) {
				if (te->string_id != INVALID_STRING_ID &&
						dpi->left <= te->right &&
						dpi->top  <= te->bottom &&
						dpi->left + dpi->width  > te->x &&
						dpi->top  + dpi->height > te->y) {
					AddStringToDraw(te->x, te->y, te->string_id, te->params_1, te->params_2, 0);
				}
			}
			break;

		case 1:
			for (te = _text_effect_list; te != endof(_text_effect_list); te++) {
				if (te->string_id != INVALID_STRING_ID &&
						dpi->left <= te->right  * 2 - te->x &&
						dpi->top  <= te->bottom * 2 - te->y &&
						dpi->left + dpi->width  > te->x &&
						dpi->top  + dpi->height > te->y) {
					AddStringToDraw(te->x, te->y, (StringID)(te->string_id-1), te->params_1, te->params_2, 0);
				}
			}
			break;
	}
}

void DeleteAnimatedTile(TileIndex tile)
{
	TileIndex* ti;

	for (ti = _animated_tile_list; ti != endof(_animated_tile_list); ti++) {
		if (tile == *ti) {
			/* remove the hole */
			memmove(ti, ti + 1, endof(_animated_tile_list) - 1 - ti);
			/* and clear last item */
			endof(_animated_tile_list)[-1] = 0;
			MarkTileDirtyByTile(tile);
			return;
		}
	}
}

bool AddAnimatedTile(TileIndex tile)
{
	TileIndex* ti;

	for (ti = _animated_tile_list; ti != endof(_animated_tile_list); ti++) {
		if (tile == *ti || *ti == 0) {
			*ti = tile;
			MarkTileDirtyByTile(tile);
			return true;
		}
	}

	return false;
}

void AnimateAnimatedTiles(void)
{
	const TileIndex* ti;

	for (ti = _animated_tile_list; ti != endof(_animated_tile_list) && *ti != 0; ti++) {
		AnimateTile(*ti);
	}
}

void InitializeAnimatedTiles(void)
{
	memset(_animated_tile_list, 0, sizeof(_animated_tile_list));
}

static void SaveLoad_ANIT(void)
{
	// In pre version 6, we has 16bit per tile, now we have 32bit per tile, convert it ;)
	if (CheckSavegameVersion(6)) {
		SlArray(_animated_tile_list, lengthof(_animated_tile_list), SLE_FILE_U16 | SLE_VAR_U32);
	} else {
		SlArray(_animated_tile_list, lengthof(_animated_tile_list), SLE_UINT32);
	}
}


const ChunkHandler _animated_tile_chunk_handlers[] = {
	{ 'ANIT', SaveLoad_ANIT, SaveLoad_ANIT, CH_RIFF | CH_LAST},
};
