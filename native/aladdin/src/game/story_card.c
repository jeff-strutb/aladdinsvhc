/*
 * story_card.c: the story card shown between acts: a screen loaded and a
 * text typed out, by a list of commands for each part of the game.
 *
 * A part's list (the cartridge's, read through data/) is a run of commands,
 * each a byte: 0 loads a screen (sets of graphics, VRAM copies and
 * palettes, then its song), 2 types a text (its number follows), 4 ends
 * the card (a fade, then the next part starts), 6 fades and goes on with
 * the list, 8 goes on to the final act's scene. A text is lines, each a
 * tilemap offset (word), an attribute byte, then characters: 0 ends the
 * line, $FF clears the layer and ends it, $FE sets the wait before the
 * next character (word), $FC and $FD end the text; $BA and $B7 are marks
 * put on the row above the next character.
 */
#include <stdio.h>

#include "audio/sound.h"
#include "data/game_tables.h"
#include "game/game.h"
#include "system/state.h"
#include "system/system.h"
#include "system/task.h"
#include "video/video.h"
#include "video/video_tasks.h"

#define CARD_LEVEL 0x30             /* the level whose RAM the card uses */
#define LAST_PART 7                 /* a card of part 0 is part 7's */
#define SETUP_FRAMES 0x30           /* frames before the first command */
#define SCREEN_FRAMES 0x40          /* after a screen has faded in */
#define END_FRAMES 0x30             /* round the end's fade */
#define PART_FADE_FRAMES 0x40       /* round a part's fade */
#define TYPE_FRAMES 4               /* frames between characters */
#define TEXT_MARK_ROW 32            /* a mark goes a row (32 words) above its character */
#define TYPE_BUTTONS (PAD_A | PAD_X | PAD_B | PAD_Y)
#define SONG_SPEED 1                /* SoundQueueF5WithA's speed after a song starts (the original leaves A = 1) */
#define MARK_A 0xBA
#define MARK_B 0xB7

enum { CMD_SCREEN = 0, CMD_TEXT = 2, CMD_END = 4, CMD_PART_FADE = 6, CMD_ENDING = 8 };
enum {
    CARD_START, CARD_SETUP, CARD_COUNTDOWN, CARD_SCREEN_FADE, CARD_TEXT_LINE, CARD_TEXT_TYPE,
    CARD_END_1, CARD_END_FADE, CARD_END_2, CARD_PART_1, CARD_PART_FADE
};

static uint8_t list_byte(const Task *t) { return story_card_list_byte(t->own.story_card.list); }
static void list_next(Task *t) { t->own.story_card.list++; }
static uint8_t text_byte(const Task *t) { return story_card_text_byte(t->own.story_card.text); }

/* a word of the text map by its byte offset (a place past it is the
   original's write into the bytes after the map: reported, left out) */
static void text_map_put(uint16_t offset, uint16_t word)
{
    if (offset / 2 >= sizeof g.text_map / sizeof g.text_map[0])
        return;
    g.text_map[offset / 2] = word;
}

/* command 0: a screen loaded: its sets, copies and palettes (a trio each,
   one more while the byte after is 0), then its song; returns once its
   song has started */
static void screen_load(Task *t)
{
    uint8_t song;

    NmiDisable();
    ScreenBlank();
    VramFillMap(VIDEO_MAP_BACK, 0);
    VramFillMap(VIDEO_MAP_FRONT, 0);
    TextLayerClear();
    do {
        UnpackSet(0x0E);
        VramCopy(0x7E / 7);
        list_next(t);
        UnpackSet(list_byte(t));
        list_next(t);
        VramCopy(list_byte(t) / 7);
        list_next(t);
        PaletteLoad(list_byte(t));
        list_next(t);
        song = list_byte(t);
    } while (song == 0);
    OamBufferClear();
    ScrollStateClear();
    NmiEnable();
    SongPlay(song);
    SoundQueueF5WithA(SONG_SPEED);
    list_next(t);
}

/*
 * @routine 81:F10D StoryCardTask
 * WHAT IT DOES: The story card between acts: for the part being started
 * (the progress sequence; part 0 as part 7) it runs that part's list: a
 * screen loaded and brightened, a text typed a character every 4 frames
 * (any of B, Y, A, X held types at once; Start ends the card), then a
 * fade, and the next part starts (or the list goes on, or the final act's
 * scene).
 * HOW: The list from the part; TextLayerClearFront, a frame; the display
 * shut; the text layer's upload at $5C00, $800 bytes; level $30's RAM,
 * graphics set 1 with copy 0; 48 frames. Then for each command, after
 * `count` frames, the main screen's layers 1 and 3 and the command.
 * (The record's table of commands has a routine for each byte; the list
 * holds only 0, 2, 4, 6 and 8.)
 */
TaskRun StoryCardTask(Task *t)
{
    uint16_t *at = task_enter(t);
    TaskRun r;
    uint8_t c;
    unsigned part;

    for (;;) {
        switch (*at) {
        case CARD_START:
            part = g.progress_sequence ? g.progress_sequence : LAST_PART;
            t->own.story_card.list = story_card_list_start((uint8_t)part);
            TextLayerClearFront();
            *at = CARD_SETUP;
            return task_wait(t, 1);
        case CARD_SETUP:
            NmiDisable();
            ScreenBlank();
            g.text_upload_to = 0x5C00;
            g.text_upload_bytes = 0x0800;
            g.level = CARD_LEVEL;
            LevelRamSetup();
            UnpackSet(1);
            VramCopy(0);
            NmiEnable();
            t->own.story_card.count = SETUP_FRAMES;
            *at = CARD_COUNTDOWN;
            break;
        case CARD_COUNTDOWN:
            if (--t->own.story_card.count & 0xFF)
                return task_wait(t, 1);
            g.main_layers = 0x05;
            switch (list_byte(t)) {
            case CMD_SCREEN:
                screen_load(t);
                *at = CARD_SCREEN_FADE;
                break;
            case CMD_TEXT:
                list_next(t);
                t->own.story_card.text = story_card_text_start(list_byte(t));
                *at = CARD_TEXT_LINE;
                break;
            case CMD_END:
                SoundQueueF6With1();
                t->own.story_card.count = END_FRAMES;
                *at = CARD_END_1;
                return task_wait(t, 1);
            case CMD_PART_FADE:
                list_next(t);
                SoundQueueF6With1();
                t->own.story_card.count = PART_FADE_FRAMES;
                *at = CARD_PART_1;
                return task_wait(t, 1);
            case CMD_ENDING:
                return task_exec(t, TASK_ENTRY_ENDING_STORY);
            default:
                fprintf(stderr, "story card: command $%02X is not in its list's commands\n", list_byte(t));
                return task_exit(t);
            }
            break;
        case CARD_SCREEN_FADE:
            r = BrightnessUpAndWait(t);
            if (r != TASK_GOES_ON)
                return r;
            t->own.story_card.count = SCREEN_FRAMES;
            *at = CARD_COUNTDOWN;
            break;
        case CARD_TEXT_LINE:
            t->own.story_card.text_at = story_card_text_word(t->own.story_card.text);
            t->own.story_card.text += 2;
            t->own.story_card.attr = text_byte(t);
            t->own.story_card.count = TYPE_FRAMES;
            *at = CARD_TEXT_TYPE;
            /* fall through */
        case CARD_TEXT_TYPE:
            if (g.pad1_held & PAD_START) {
                SoundQueueF6With1();
                t->own.story_card.count = END_FRAMES;
                *at = CARD_END_1;
                return task_wait(t, 1);
            }
            if (!(g.pad1_held & TYPE_BUTTONS) && --t->own.story_card.count != 0)
                return task_wait(t, 1);
            t->own.story_card.count = TYPE_FRAMES;
            for (;;) {
                uint16_t word;

                t->own.story_card.text++;
                c = text_byte(t);
                word = (uint16_t)((uint16_t)t->own.story_card.attr << 8 | c);
                if (c == 0 || c == 0xFF) {              /* the line ends (the layer cleared with $FF) */
                    if (c == 0xFF)
                        TextLayerClear();
                    t->own.story_card.text++;
                    *at = CARD_TEXT_LINE;
                    break;
                }
                if (c == 0xFE) {                        /* a wait before the next character */
                    t->own.story_card.text++;
                    t->own.story_card.count = story_card_text_byte(t->own.story_card.text);
                    t->own.story_card.text++;
                    t->own.story_card.count |= (uint16_t)story_card_text_byte(t->own.story_card.text) << 8;
                    break;
                }
                if (c >= 0xFC) {                        /* $FC, $FD: the text ends */
                    list_next(t);
                    t->own.story_card.count = 1;
                    *at = CARD_COUNTDOWN;
                    break;
                }
                if (c == MARK_A || c == MARK_B) {       /* a mark on the row above */
                    text_map_put((uint16_t)(t->own.story_card.text_at - 2 * TEXT_MARK_ROW), word);
                    continue;
                }
                text_map_put(t->own.story_card.text_at, word);
                t->own.story_card.text_at += 2;
                g.text_map_upload++;
                return task_wait(t, 1);
            }
            break;
        case CARD_END_1:
            if (--t->own.story_card.count & 0xFF)
                return task_wait(t, 1);
            *at = CARD_END_FADE;
            break;
        case CARD_END_FADE:
            r = BrightnessDownAndWait(t);
            if (r != TASK_GOES_ON)
                return r;
            t->own.story_card.count = END_FRAMES;
            *at = CARD_END_2;
            return task_wait(t, 1);
        case CARD_END_2:
            if (--t->own.story_card.count & 0xFF)
                return task_wait(t, 1);
            return task_exec(t, TASK_ENTRY_NEXT_PART);
        case CARD_PART_1:
            if (--t->own.story_card.count & 0xFF)
                return task_wait(t, 1);
            *at = CARD_PART_FADE;
            break;
        case CARD_PART_FADE:
            r = BrightnessDownAndWait(t);
            if (r != TASK_GOES_ON)
                return r;
            t->own.story_card.count = PART_FADE_FRAMES;
            *at = CARD_COUNTDOWN;
            break;
        default:
            return task_exit(t);
        }
    }
}
