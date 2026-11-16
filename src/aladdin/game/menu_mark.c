/*
 * menu_mark.c: the selection mark of a menu drawn in the text layer: taken
 * away from the choice left; and the title menu's cursor moved. Drafted
 * by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"

void abi_SoundQueueAdd(void);
void abi_TextLayoutShow(void);
void abi_MenuMarkMove(void);
void abi_TitleMenuCursor(void);
void abi_TitleMenuBlink(void);

#define MENU_CHOICE 0x000849u        /* the title menu's choice (0-2) */
#define MENU_CHOICE_WAS 0x00084Au    /* the choice before the last move */
#define MENU_BLINK_HIDDEN 0x000847u  /* nonzero: the chosen line is blanked */
#define MENU_BLINK_FRAMES 0x000848u  /* frames the line has been shown, or frames left blanked */
#define MENU_LINES 0xCE25u           /* by choice: its text layout (the DB bank, $81) */
#define MENU_BLINK_SHOWN 0x20        /* frames a line is shown before it blinks off */
#define PAD_UP 0x08                  /* in the pad's high byte */
#define PAD_DOWN 0x04
#define TITLE_IDLE_LEAST 0x04EC      /* frames the title waits idle, at least, after a move */
#define SOUND_MENU_MOVE 0x0C

/*
 * @routine 81:EF15 MenuMarkErase
 * WHAT IT DOES: Takes the selection mark away from a menu choice: blanks
 * its seven tiles in the text layer's tilemap.
 * HOW: Tile word $0020 (a blank) stored at the choice's offset plus 2 (row
 * 0), $40, $42, $44 (row 1) and $80, $82, $84 (row 2), with 16-bit A; P
 * kept.
 * Entry: X 16-bit = the choice's byte offset in the tilemap; called by JSL
 * (from the menu's cursor move, 81:EE04, which draws the mark at the new
 * choice by 81:EF39). Exit: P as at entry; A = $0020.
 */
void abi_MenuMarkErase(void)
{
    static const uint16_t cells[7] = { 2, 64, 66, 68, 128, 130, 132 };

    php();
    op_rep(P_M);
    a16_load(0x0020);
    for (int i = 0; i < 7; i++)
        wr16((TEXT_TILEMAP + cells[i] + cpu.x) & 0xFFFFFF, 0x0020);
    plp();
}

/* The text layout of a menu choice's line. */
static uint8_t menu_line(uint8_t choice)
{
    return rd8(dba(MENU_LINES) + choice);
}

/*
 * @routine 81:EDA2 TitleMenuCursorLong
 * WHAT IT DOES: TitleMenuCursor for a caller in another bank.
 * HOW: JSR to it, then RTL.
 * Entry and exit: as TitleMenuCursor; called by JSL.
 */
void abi_TitleMenuCursorLong(void)
{
    abi_TitleMenuCursor();
}

/*
 * @routine 81:EDA6 TitleMenuCursor
 * WHAT IT DOES: Moves the title screen menu's cursor on Up or Down
 * (wrapping round the three choices), with its mark, line and sound;
 * otherwise blinks the chosen line.
 * HOW: Pad 1's newly pressed high byte: neither Up nor Down, TitleMenuBlink
 * only. Up, MENU_CHOICE down by 1 (0 wraps to 2); Down, up by 1 (3 wraps
 * to 0). Then MenuMarkMove, TitleMenuBlink, the old choice's line drawn
 * again (TextLayoutShow; it may have been blinked off), the blink
 * restarted, MENU_CHOICE_WAS = the choice, V_0343 (the title's idle
 * countdown) raised to at least TITLE_IDLE_LEAST, and sound
 * SOUND_MENU_MOVE.
 * Entry: M 8-bit, X 8-bit; DB = $81; called by JSR (81:EDA2). Exit: M
 * 8-bit, X 8-bit; as TitleMenuBlink leaves it, or as SoundQueueAdd (A's
 * high byte $04).
 */
void abi_TitleMenuCursor(void)
{
    uint8_t pressed = rd8(dba((uint16_t)(JOY_PRESSED + 1))), choice, was;
    uint16_t idle;

    a8_load(pressed);
    if (!(pressed & (PAD_UP | PAD_DOWN))) {
        abi_TitleMenuBlink();
        return;
    }
    choice = rd8(dba(MENU_CHOICE));
    if (pressed & PAD_UP) {
        choice = (uint8_t)(choice - 1);
        if (choice & 0x80)
            choice = 2;
        a8_load(choice);
    } else {
        choice = (uint8_t)(choice + 1);
        a8_load(choice);             /* A keeps 3 when it wraps */
        if (choice == 3)
            choice = 0;
    }
    wr8(dba(MENU_CHOICE), choice);
    abi_MenuMarkMove();
    abi_TitleMenuBlink();
    was = rd8(dba(MENU_CHOICE_WAS));
    cpu.y = was;
    a8_load(menu_line(was));
    abi_TextLayoutShow();
    wr8(dba(MENU_BLINK_HIDDEN), 0);
    wr8(dba(MENU_BLINK_FRAMES), 0);
    choice = rd8(dba(MENU_CHOICE));
    wr8(dba(MENU_CHOICE_WAS), choice);
    idle = rd16(dba(V_0343));
    cpu.a = TITLE_IDLE_LEAST;
    cmp16_of(TITLE_IDLE_LEAST, idle);
    if (TITLE_IDLE_LEAST >= idle)
        wr16(dba(V_0343), TITLE_IDLE_LEAST);
    a8_load(SOUND_MENU_MOVE);
    abi_SoundQueueAdd();
}

/*
 * @routine 81:EE55 TitleMenuBlink
 * WHAT IT DOES: Blinks the title menu's chosen line: shown for
 * MENU_BLINK_SHOWN frames, then blanked until its count runs out.
 * HOW: Shown (MENU_BLINK_HIDDEN zero): MENU_BLINK_FRAMES counted up; at
 * MENU_BLINK_SHOWN, MENU_BLINK_HIDDEN set and the line of MENU_CHOICE
 * blanked (its layout with bit 7, TextLayoutShow). Blanked:
 * MENU_BLINK_FRAMES counted down; at 0, MENU_BLINK_HIDDEN cleared and the
 * line of MENU_CHOICE_WAS drawn.
 * Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: M 8-bit, X
 * 8-bit; a line drawn or blanked: as TextLayoutShow leaves it, Y = the
 * choice. Else Y = MENU_BLINK_HIDDEN; shown: A = the count, N, Z and C as
 * its compare with MENU_BLINK_SHOWN; blanked: A = the count before, N
 * and Z from the count after.
 */
void abi_TitleMenuBlink(void)
{
    uint8_t frames = rd8(dba(MENU_BLINK_FRAMES)), hidden = rd8(dba(MENU_BLINK_HIDDEN));
    uint8_t choice;

    cpu.y = hidden;
    if (hidden) {
        a8_load(frames);
        if (dec8_at(dba(MENU_BLINK_FRAMES)))
            return;
        wr8(dba(MENU_BLINK_HIDDEN), 0);
        choice = rd8(dba(MENU_CHOICE_WAS));
        cpu.y = choice;
        a8_load(menu_line(choice));
        abi_TextLayoutShow();
        return;
    }
    frames = (uint8_t)(frames + 1);
    a8_load(frames);
    wr8(dba(MENU_BLINK_FRAMES), frames);
    cmp8_of(frames, MENU_BLINK_SHOWN);
    if (frames != MENU_BLINK_SHOWN)
        return;
    inc8_at(dba(MENU_BLINK_HIDDEN));
    choice = rd8(dba(MENU_CHOICE));
    cpu.y = choice;
    a8_load((uint8_t)(menu_line(choice) | 0x80));
    abi_TextLayoutShow();
}
