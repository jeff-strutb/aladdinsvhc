/*
 * password_screen.c: the password screen: four slots of pictures stepped
 * with the pad and checked against the passwords; a match sets the part
 * and starts it, anything else says so and goes back to the title.
 *
 * Not converted, with their proof: CopierCheck (84:AF7F), which writes and
 * reads back a byte of cartridge RAM at $70:1522 and, finding it kept,
 * sets the lives and health to 1: the retail board has no RAM there, so
 * the read never matches; RegionCheck (84:AFAE) as in game/progress.c. The
 * original also writes 3 to $0370 on a match; nothing reads it.
 */
#include "audio/sound.h"
#include "data/level_tables.h"
#include "game/game.h"
#include "system/state.h"
#include "system/system.h"
#include "system/task.h"
#include "video/video.h"
#include "video/video_tasks.h"

#define PASSWORD_LEVEL 0x2C         /* the level whose RAM and actors the screen uses */
#define SONG 0x2D
#define FADE_SLOT 1
#define MESSAGE_FRAMES 0x80
#define MESSAGE_OK 0x64             /* the text layouts of the message: a password, not one */
#define MESSAGE_NOT 0x62
#define SOUND_STEP 0x0C
#define CONFIRM_BUTTONS (PAD_A | PAD_B | PAD_X | PAD_Y | PAD_START)
#define LEVEL_PART_LAST 7
#define CHEAT_HEALTH 10
#define CHEAT_APPLES 0x99
#define START_LIVES 3
#define START_CONTINUES 1
#define PICTURE_TILE_BITS 0x1000    /* the slot's map word: palette 4 on, the slot's number from bit 10 */

enum { P_START, P_SONG, P_FADE_IN, P_INPUT, P_MESSAGE, P_FADE_OUT, P_LEAVE };

/*
 * @routine 84:AF52 PasswordPictureDraw
 * WHAT IT DOES: Shows picture `picture` in password slot `slot`: its tiles
 * in the slot's place and its palette in the slot's.
 */
static void PasswordPictureDraw(unsigned slot, unsigned picture)
{
    TileBlockQueue(password_slot_vram(slot),
                   (uint16_t)((slot << 10) | PICTURE_TILE_BITS | password_row_bits(picture)));
    PaletteFromRom((uint8_t)picture, (uint8_t)slot);
}

/* the password the four slots show: its number, or -1 */
static int password_found(const Task *t)
{
    for (unsigned p = 0; p < PASSWORDS; p++) {
        unsigned slot;

        for (slot = 0; slot < PASSWORD_SLOTS; slot++)
            if (t->own.password.picture[slot] != password_entry(p, slot))
                break;
        if (slot == PASSWORD_SLOTS)
            return (int)p;
    }
    return -1;
}

/*
 * @routine 84:AD50 PasswordScreen
 * WHAT IT DOES: The password screen: four slots of pictures stepped with
 * the pad, checked against the table of passwords; a match sets the part
 * and starts it, anything else says so and goes back to the title.
 * HOW: The screen set up (the sets, level $2C's RAM, palette $4D), each
 * slot's start picture shown, slot 3 current; the song; the fade in. Each
 * frame: A, B, X, Y or START checks the slots against the passwords: a
 * match sets the part (kept in the third kept option, sealed) and a
 * message, else another message; 128 frames, the fade-out. The step
 * buttons (the original's bits: $08 raises the slot's picture, $04
 * lowers it, 0-5 round; $02 moves the slot on, $01 back, 0-3 round) each
 * with sound $0C. A match then starts its part: with the cheat set, 10
 * health marks, the hover and 99 apples, else the part's health; 3 lives,
 * 1 continue; the part 7 by NextPartTask, else the story card; no match,
 * the title.
 */
TaskRun PasswordScreen(Task *t)
{
    uint16_t *at = task_enter(t);
    uint16_t pressed;
    int found;

    switch (*at) {
    case P_START:
        NmiDisable();
        ScreenBlank();
        ScrollStateClear();
        OamBufferClear();
        DisplayDefaults();
        UnpackSet(0x0D);
        VramCopy(0x70 / 7);
        UnpackSet(0x23);
        VramCopy(0x85 / 7);
        VramCopy2(0x5B / 7);
        UnpackSet(6);
        VramCopy(0x77 / 7);
        g.level = PASSWORD_LEVEL;
        LevelRamSetup();
        PaletteLoad(0x4D);
        for (unsigned slot = 0; slot < PASSWORD_SLOTS; slot++) {
            t->own.password.picture[slot] = password_start_picture(slot);
            PasswordPictureDraw(slot, t->own.password.picture[slot]);
        }
        t->own.password.slot = 3;
        t->own.password.matched = 0;
        NmiEnable();
        *at = P_SONG;
        return task_wait(t, 1);
    case P_SONG:
        SongPlay(SONG);
        g.main_layers = 0x1F;
        g.sub_layers = 0x1F;
        g.camera_window = 1;
        TaskCreate(FADE_SLOT, 2, TASK_ENTRY_COLOUR_FADE_IN);
        *at = P_FADE_IN;
        return task_wait(t, 1);
    case P_FADE_IN:
        if (FrameStep())
            return task_exec_take(t);
        if (task_busy(FADE_SLOT))
            return task_wait(t, 1);
        *at = P_INPUT;
        return task_wait(t, 1);
    case P_INPUT:
        if (FrameStep())
            return task_exec_take(t);
        pressed = g.pad_pressed[0];
        if (pressed & CONFIRM_BUTTONS) {
            found = password_found(t);
            if (found >= 0) {
                g.progress_sequence = password_part((unsigned)found);
                g.kept_options[2] = (uint8_t)g.progress_sequence;
                g.progress_stage = 0;
                KeptOptionsSeal();
                t->own.password.matched++;
                TextLayoutShow(MESSAGE_OK);
            } else {
                TextLayoutShow(MESSAGE_NOT);
            }
            t->own.password.count = MESSAGE_FRAMES;
            *at = P_MESSAGE;
            return task_wait(t, 1);
        }
        if (pressed & (PAD_UP | PAD_DOWN)) {
            uint8_t *picture = &t->own.password.picture[t->own.password.slot];

            if (pressed & PAD_UP)
                *picture = (uint8_t)(*picture + 1 == PASSWORD_PICTURES ? 0 : *picture + 1);
            else
                *picture = (uint8_t)(*picture == 0 ? PASSWORD_PICTURES - 1 : *picture - 1);
            PasswordPictureDraw(t->own.password.slot, *picture);
            SoundQueueAdd(SOUND_STEP);
        } else if (pressed & (PAD_LEFT | PAD_RIGHT)) {
            t->own.password.slot = (uint8_t)((t->own.password.slot + ((pressed & PAD_LEFT) ? 1 : -1)) & 3);
            SoundQueueAdd(SOUND_STEP);
        }
        return task_wait(t, 1);
    case P_MESSAGE:
        if (FrameStep())
            return task_exec_take(t);
        if (--t->own.password.count != 0)
            return task_wait(t, 1);
        SoundQueueF6With1();
        TaskCreate(FADE_SLOT, 1, TASK_ENTRY_COLOUR_FADE_OUT);
        *at = P_FADE_OUT;
        return task_wait(t, 1);
    case P_FADE_OUT:
        if (FrameStep())
            return task_exec_take(t);
        if (task_busy(FADE_SLOT))
            return task_wait(t, 1);
        if (!t->own.password.matched)
            return task_exec(t, TASK_ENTRY_TITLE_AFTER_FADE_OUT);
        if (g.cheat_start) {
            g.hover_on = 1;
            g.health_marks = CHEAT_HEALTH;
            g.health = CHEAT_HEALTH;
            g.apples = CHEAT_APPLES;
        } else {
            g.health_marks = password_health((uint8_t)g.progress_sequence);
            g.health = g.health_marks;
        }
        g.hud_count_2 = 0;
        g.lives = START_LIVES;
        g.continues = START_CONTINUES;
        g.player_flags = 0;
        g.brightness = 0;
        g.blank = 1;
        return task_exec(t, g.progress_sequence == LEVEL_PART_LAST ? TASK_ENTRY_NEXT_PART : TASK_ENTRY_STORY_CARD);
    }
    return task_exit(t);
}
