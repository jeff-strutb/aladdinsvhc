/*
 * options.c: the options screen: its task (the screen set up, then each
 * frame the cheat code, the cursor's moves and the items' steps), the
 * settings' words, the cursor, the sound test and the sound output.
 */
#include "actor/actor.h"
#include "audio/sound.h"
#include "data/game_tables.h"
#include "game/game.h"
#include "system/state.h"
#include "system/system.h"
#include "system/task.h"
#include "video/video.h"
#include "video/video_tasks.h"

#define OPTIONS_LEVEL 0x2B          /* the level whose RAM and actors the screen uses */
#define FADE_SLOT 1
#define FIGURE_KIND 0x82
#define FIGURE_LOOK 0x0900
#define FIGURE_PLACE 0x0080
#define SETTINGS 4
#define SOUND_TEST_LAST 0x22        /* the sound test's last number, less one */
#define SONG_FIRST 0x10             /* the sound test's number 1 is song $10 */
#define CODE_WAIT_MAX 0x20
#define CODE_LENGTH 8
#define CURSOR_ABOVE_BYTES 62       /* the cursor's tile above its place: 31 words before it */
#define CONFIRM_BUTTONS (PAD_A | PAD_B | PAD_X | PAD_Y)
#define ITEM_SETTING 0
#define ITEM_SOUND_TEST 2
#define ITEM_OUTPUT 4
#define ITEM_EXIT 6
#define SOUND_STEP 0x0C
#define SOUND_CHEAT 0x28
#define SOUND_STEREO 0xF7           /* the command for stereo; the next, monaural */
#define TEXT_STEREO 0x68
#define TEXT_MONAURAL 0x66
#define SOUND_TEST_PLACE 0x04A8     /* the sound test's number: a byte offset in the text map */
#define DIGIT_ATTR 0x2800
#define CODE_FIRST_PRESS PAD_L      /* the code starts with this press, on pad 2 */

/* where the setting's five words go: column 21 of rows 6, 9, 11, 13 and 15 */
static const uint16_t setting_at[5] = { 426, 618, 746, 874, 1002 };

static void text_put(unsigned offset, uint16_t word)
{
    if (offset / 2 < sizeof g.text_map / sizeof g.text_map[0])
        g.text_map[offset / 2] = word;
}

/*
 * @routine 83:8C16 OptionSettingShow
 * WHAT IT DOES: Shows a setting on the options screen: five map words down
 * one column, and the text layer's upload asked for.
 */
static void OptionSettingShow(unsigned setting)
{
    for (unsigned i = 0; i < 5; i++)
        text_put(setting_at[i], option_setting_word(i, setting));
    g.text_map_upload++;
}

/*
 * @routine 83:8C43 OptionCursorDraw
 * WHAT IT DOES: Draws the options screen's cursor at item `item` (times 2):
 * three tiles on each of two lines, and one on the line above, and asks
 * for the text layer's upload.
 */
static void OptionCursorDraw(unsigned item)
{
    unsigned place = option_cursor_place(item / 2);

    text_put(place - CURSOR_ABOVE_BYTES, 0x207E);
    text_put(place, 0x2802);
    text_put(place + 2, 0x2003);
    text_put(place + 4, 0x2004);
    text_put(place + 64, 0x2812);
    text_put(place + 66, 0x2013);
    text_put(place + 68, 0x2014);
    g.text_map_upload++;
}

/*
 * @routine 83:8C7B OptionCursorClear
 * WHAT IT DOES: Clears the cursor beside every item and asks for the
 * upload.
 */
static void OptionCursorClear(void)
{
    static const int cells[7] = { -CURSOR_ABOVE_BYTES, 0, 2, 4, 64, 66, 68 };

    for (int item = OPTION_ITEMS - 1; item >= 0; item--)
        for (int i = 0; i < 7; i++)
            text_put((unsigned)((int)option_cursor_place((unsigned)item) + cells[i]), 0);
    g.text_map_upload++;
}

/*
 * @routine 83:8B9D SoundTestShow
 * WHAT IT DOES: Shows the sound test's number (1 up) as two decimal digits.
 */
static void SoundTestShow(const Task *t)
{
    unsigned number = (unsigned)t->own.options.sound_test + 1;

    text_put(SOUND_TEST_PLACE, (uint16_t)(DIGIT_ATTR | ('0' + number / 10)));
    text_put(SOUND_TEST_PLACE + 2, (uint16_t)(DIGIT_ATTR | ('0' + number % 10)));
    g.text_map_upload++;
}

/*
 * @routine 83:8BB4 OptionStereoShow
 * WHAT IT DOES: Shows the sound output, STEREO or MONAURAL by bit 7 of the
 * second kept option, and sends the sound driver the matching command.
 */
static void OptionStereoShow(void)
{
    int stereo = g.kept_options[1] & 0x80;

    TextLayoutShow(stereo ? TEXT_STEREO : TEXT_MONAURAL);
    SoundQueueAdd((uint8_t)(stereo ? SOUND_STEREO : SOUND_STEREO + 1));
}

/* the item's step, right (or left): 83:8B61 OptionSettingStep, 83:8B7E
   SoundTestStep, 83:8BA8 OptionStereoToggle, 83:8BCE OptionExitStep (nothing) */
static void item_step(Task *t, int right)
{
    switch (t->own.options.item) {
    case ITEM_SETTING:
        t->own.options.setting = (uint8_t)((t->own.options.setting + (right ? 1 : -1)) & 3);
        g.kept_options[0] = t->own.options.setting;
        OptionSettingShow(t->own.options.setting);
        KeptOptionsSeal();
        break;
    case ITEM_SOUND_TEST:
        if (right) {
            if (++t->own.options.sound_test > SOUND_TEST_LAST)
                t->own.options.sound_test = 0;
        } else if ((int8_t)--t->own.options.sound_test < 0) {
            t->own.options.sound_test = SOUND_TEST_LAST;
        }
        SoundQueueF2();
        SoundTestShow(t);
        break;
    case ITEM_OUTPUT:
        g.kept_options[1] ^= 0x80;
        KeptOptionsSeal();
        OptionStereoShow();
        break;
    default:
        break;
    }
}

/* the cheat code: L on pad 2, then the presses of the table one after
   another, each within 32 frames; the whole code sets the cheat */
static void cheat_code(Task *t)
{
    uint16_t now = g.pad_pressed[1];

    if (t->own.options.code_at == 0) {
        if (now & CODE_FIRST_PRESS) {
            t->own.options.code_at = 1;
            t->own.options.code_wait = 0;
        }
        return;
    }
    if (++t->own.options.code_wait >= CODE_WAIT_MAX) {
        t->own.options.code_wait = 0;
        t->own.options.code_at = 0;
        return;
    }
    if (now == 0)
        return;
    t->own.options.code_wait = 0;
    if (now != option_code_press(t->own.options.code_at)) {
        t->own.options.code_at = 0;
        return;
    }
    if (++t->own.options.code_at == CODE_LENGTH) {
        g.cheat_start = 1;
        SoundQueueAdd(SOUND_CHEAT);
    }
}

enum { O_START, O_FRAME };

/*
 * @routine 83:8979 OptionsTask
 * @routine 83:8BCF OptionsLeave
 * WHAT IT DOES: The options screen's task. Sets the screen up: its
 * graphics and palettes, its figure (an actor of kind $82), the fade-in,
 * its text, the colour maths, the cursor on the first item and each item's
 * setting shown. Then every frame: the cheat code (L on pad 2, then the
 * presses of its table one after another, each within 32 frames, sets the
 * cheat and plays sound $28); A, B, X or Y on the sound test plays its
 * song, on the exit item leaves; Start leaves; up and down move the cursor
 * (wrapping), with sounds $F2 and $0C; left and right step the item, with
 * sound $0C. Leaving stops the sound and starts the title's task after
 * its fade-out.
 */
TaskRun OptionsTask(Task *t)
{
    uint16_t *at = task_enter(t);
    uint16_t pressed;
    Actor *figure;

    switch (*at) {
    case O_START:
        NmiDisable();
        ScreenBlank();
        ScrollStateClear();
        OamBufferClear();
        DisplayDefaults();
        TextLayerClear();
        g.text_upload_to = 0x5C00;
        g.text_upload_bytes = 0x0800;
        g.level = OPTIONS_LEVEL;
        LevelRamSetup();
        g.camera_window = -1;
        UnpackSet(1);
        VramCopy(0);
        UnpackSet(0x0D);
        VramCopy(0x70 / 7);
        UnpackSet(2);
        VramCopy(0x77 / 7);
        UnpackSet(0x22);
        VramCopy(0xFC / 7);
        NmiEnable();
        g.main_layers = 0x16;
        PaletteLoad(0x08);
        PaletteLoad(0x0B);
        PaletteLoad(0x57);
        figure = ActorCreate(FIGURE_KIND);
        if (figure) {
            figure->state = 0x0C;
            figure->look = FIGURE_LOOK;
            figure->x = (figure->x & 0xFFu) | (uint32_t)FIGURE_PLACE << 8;
            figure->y = (figure->y & 0xFFu) | (uint32_t)FIGURE_PLACE << 8;
        }
        TaskCreate(FADE_SLOT, 4, TASK_ENTRY_BRIGHTNESS_UP);
        TextLayoutShow(0x2E);
        g.blend_direct_colour = 0;
        g.blend_use_sub = 0;
        g.blend_prevent = 0;
        g.blend_clip = 0;
        g.blend_layers = 0x10;
        g.blend_half = 0;
        g.blend_subtract = 1;
        g.fixed_red = 2;
        g.fixed_green = 1;
        g.fixed_blue = 6;
        t->own.options.item = ITEM_SETTING;
        t->own.options.sound_test = 0;
        t->own.options.setting = g.kept_options[0];
        OptionCursorClear();
        OptionCursorDraw(t->own.options.item);
        OptionSettingShow(t->own.options.setting);
        SoundTestShow(t);
        OptionStereoShow();
        t->own.options.code_wait = 0;
        t->own.options.code_at = 0;
        *at = O_FRAME;
        return task_wait(t, 1);
    case O_FRAME:
        if (FrameStep())
            return task_exec_take(t);
        if (!g.cheat_start)
            cheat_code(t);
        pressed = g.pad_pressed[0];
        if (pressed & CONFIRM_BUTTONS) {
            if (t->own.options.item == ITEM_SOUND_TEST) {
                SongPlay((uint8_t)(t->own.options.sound_test + SONG_FIRST));
                return task_wait(t, 1);
            }
            if (t->own.options.item != ITEM_EXIT)
                return task_wait(t, 1);
        } else if (!(pressed & PAD_START)) {
            if (pressed & PAD_UP) {
                t->own.options.item = (uint8_t)(t->own.options.item == 0 ? ITEM_EXIT : t->own.options.item - 2);
            } else if (pressed & PAD_DOWN) {
                t->own.options.item = (uint8_t)(t->own.options.item + 2 >= 2 * OPTION_ITEMS ? 0 : t->own.options.item + 2);
            } else if (pressed & (PAD_LEFT | PAD_RIGHT)) {
                item_step(t, (pressed & PAD_LEFT) ? 0 : 1);
                SoundQueueAdd(SOUND_STEP);
                return task_wait(t, 1);
            } else {
                return task_wait(t, 1);
            }
            SoundQueueF2();
            SoundQueueAdd(SOUND_STEP);
            OptionCursorClear();
            OptionCursorDraw(t->own.options.item);
            return task_wait(t, 1);
        }
        SoundQueueF2();
        return task_exec(t, TASK_ENTRY_TITLE_AFTER_FADE_OUT);
    }
    return task_exit(t);
}
