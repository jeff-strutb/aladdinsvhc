/*
 * story_text.c: the task that types the opening story's text onto the text
 * layer, a character every few frames: one text of set 0, picked by the
 * task's parameter; StoryTextTask1 (81:F344) types set 1, for the ending's
 * story's scenes.
 */
#include <stdio.h>

#include "data/game_tables.h"
#include "game/game.h"
#include "system/state.h"
#include "system/task.h"
#include "video/video.h"

#define TEXT_SET_OPENING 0
#define TEXT_SET_SCENES 1
#define CHAR_FRAMES 4           /* frames between characters */
#define PAGE_FRAMES 160         /* a page's wait before the layer is cleared */
#define TEXT_PRIORITY 0x20      /* the tile's high byte: its priority bit */
#define TEXT_MAP_BYTES (32 * 32 * 2)
#define ROW_BYTES 0x80          /* a text row: two of the layer's rows of 32 words */
#define ROW_UP_BYTES 0x40       /* a mark goes one of the layer's rows up */
/* marks put on the row above, at the column the next character takes */
#define MARK_1 0xBA
#define MARK_2 0xB7

enum {                          /* the text's codes; below CODE_PLACE a character, 0 the end */
    CODE_PLACE = 0xFB,          /* then a word: the text layer's byte offset of the first row and column */
    CODE_PALETTE = 0xFC,        /* then a byte: its low 3 bits the palette */
    CODE_WAIT = 0xFD,           /* then a byte: frames */
    CODE_PAGE = 0xFE,           /* wait PAGE_FRAMES, clear the layer, back to the first row and column */
    CODE_ROW = 0xFF             /* the next row */
};

enum { TEXT_START, TEXT_TYPING, TEXT_PAGE };

/* A tile into the text layer's map at byte offset `at`, as the original's
   long store into its buffer. */
static void text_map_put(uint16_t at, uint16_t tile)
{
    static int said;

    if (at >= TEXT_MAP_BYTES) {
        if (!said)
            fprintf(stderr, "StoryTextTask: a character past the text layer (offset %u)\n", (unsigned)at);
        said = 1;
        return;
    }
    g.text_map[at >> 1] = tile;
}

/*
 * @routine 81:F332 StoryTextTask
 * @routine 81:F344 StoryTextTask1
 * WHAT IT DOES: A task that types text of set 0 (the opening story's) onto
 * the text layer, a character every 4 frames, then ends.
 * HOW: The text picked by its parameter; the parameter then holds the
 * frames between characters (4), as the original's. The layer cleared;
 * then each byte of the text: a character put at the row and column with
 * the palette and priority bits, the map's upload asked for, the column
 * stepped and the frames waited (MARK_1 and MARK_2 go a layer row up with
 * no step, no upload and no wait; no script types one); CODE_PLACE sets the
 * place of the first row and column, CODE_PALETTE the palette, CODE_WAIT
 * waits its byte's frames, CODE_PAGE waits 160 frames and clears the layer
 * back to the first row and column, CODE_ROW goes to the next row; at 0
 * the task ends. StoryTextTask1 (81:F344) types the same from set 1, the
 * later scenes' texts.
 */
static TaskRun story_text_type(Task *t, unsigned set)
{
    uint16_t *at = task_enter(t);

    for (;;) {
        uint8_t c;

        switch (*at) {
        case TEXT_START:
            t->own.story_text.at = story_text_start(set, t->param);
            t->param = CHAR_FRAMES;
            t->own.story_text.palette = 0;
            t->own.story_text.column = 0;
            t->own.story_text.row = 0;
            TextLayerClear();
            *at = TEXT_TYPING;
            break;
        case TEXT_PAGE:
            TextLayerClear();
            *at = TEXT_TYPING;
            break;
        case TEXT_TYPING:
            c = story_text_byte(t->own.story_text.at);
            if (c == 0)
                return task_exit(t);
            if (c < CODE_PLACE) {
                uint16_t tile = (uint16_t)(c | (t->own.story_text.palette | TEXT_PRIORITY) << 8);
                uint16_t place = (uint16_t)(t->own.story_text.origin + 2u * t->own.story_text.column
                                            + ROW_BYTES * t->own.story_text.row);

                t->own.story_text.at++;
                if (c == MARK_1 || c == MARK_2) {
                    text_map_put((uint16_t)(place - ROW_UP_BYTES), tile);
                    break;
                }
                text_map_put(place, tile);
                g.text_map_upload++;
                t->own.story_text.column++;
                return task_wait(t, t->param);
            }
            switch (c) {
            case CODE_ROW:
                t->own.story_text.column = 0;
                t->own.story_text.row++;
                t->own.story_text.at++;
                break;
            case CODE_PAGE:
                t->own.story_text.column = 0;
                t->own.story_text.row = 0;
                t->own.story_text.at++;
                *at = TEXT_PAGE;
                return task_wait(t, PAGE_FRAMES);
            case CODE_WAIT:
                c = story_text_byte((uint16_t)(t->own.story_text.at + 1));
                t->own.story_text.at = (uint16_t)(t->own.story_text.at + 2);
                return task_wait(t, c);
            case CODE_PALETTE:
                t->own.story_text.palette = (uint8_t)((story_text_byte((uint16_t)(t->own.story_text.at + 1)) & 0x07) << 2);
                t->own.story_text.at = (uint16_t)(t->own.story_text.at + 2);
                break;
            default:                    /* CODE_PLACE */
                t->own.story_text.origin = (uint16_t)(story_text_byte((uint16_t)(t->own.story_text.at + 1))
                                                      | story_text_byte((uint16_t)(t->own.story_text.at + 2)) << 8);
                t->own.story_text.at = (uint16_t)(t->own.story_text.at + 3);
                break;
            }
            break;
        }
    }
}

TaskRun StoryTextTask(Task *t)
{
    return story_text_type(t, TEXT_SET_OPENING);
}

TaskRun StoryTextTask1(Task *t)
{
    return story_text_type(t, TEXT_SET_SCENES);
}
