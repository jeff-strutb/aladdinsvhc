/*
 * level_select.c: the level select (a development mode): L and R held
 * with a direction step the level part and stage, and the level text
 * shows them.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_LevelNumberText(void);
void abi_LevelSelect(void);
void abi_TextLayoutShow(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/* LEVEL_035C's two bytes as the level select shows them: the part (1-7
   on screen) and the stage in it. */
#define LEVEL_PART LEVEL_035C
#define LEVEL_STAGE (LEVEL_035C + 1)
#define PART_LAST 6                   /* the last part (its text a message) */
#define PART_STAGES_LAST 0xCE2Eu      /* by part: its last stage (the DB bank, $80) */
#define PAD_HELD_LR 0x30              /* L or R, in the pad's low byte */
#define PAD_UP 0x08                   /* in the pad's high byte */
#define PAD_DOWN 0x04
#define PAD_LEFT 0x02
#define PAD_RIGHT 0x01
/* The level's number in the text layer: row 16, from column 13: a space,
   the part, '.', the stage, a space (tile words, attribute $20). */
#define LEVEL_NUMBER_AT (TEXT_TILEMAP + 0x41A)
#define TILE_DIGIT_1 0x2031           /* '1': the part and stage shown from 1 */
#define TILE_DOT 0x202E
#define TILE_SPACE 0x2020
#define LEVEL_TEXT_LAST_PART 0x5E     /* text layouts: the last part's title */
#define LEVEL_TEXT_5_4 0x60           /* part 5 stage 3's (shown "6.4") title */

/*
 * @routine 80:A0B1 LevelNumberText
 * WHAT IT DOES: Writes the level's part and stage into the text layer as
 * "P.S" (or a title in place of it for the last part, and for part 5
 * stage 3), and has the NMI upload it.
 * HOW: LEVEL_PART = PART_LAST: layout LEVEL_TEXT_LAST_PART; part and stage
 * $05, $03: layout LEVEL_TEXT_5_4 (by TextLayoutShow). Else each one's low
 * four bits + TILE_DIGIT_1 at LEVEL_NUMBER_AT + 2 and + 6, TILE_DOT
 * between, TILE_SPACE either side; TEXT_TILEMAP_UPLOAD up.
 * Entry: M 8-bit; DB = $80; called by JSR. Exit: M 8-bit; a layout: as
 * TextLayoutShow leaves it; else A = TILE_SPACE, N and Z from the upload
 * count, C and V clear.
 */
void abi_LevelNumberText(void)
{
    uint8_t part = rd8(dba(LEVEL_PART));
    uint16_t both;

    a8_load(part);
    cmp8_of(part, PART_LAST);
    if (part == PART_LAST) {
        a8_load(LEVEL_TEXT_LAST_PART);
        abi_TextLayoutShow();
        return;
    }
    both = rd16(dba(LEVEL_PART));
    cmp16_of(both, 0x0305);
    if (both == 0x0305) {
        cpu.a = both;
        a8_load(LEVEL_TEXT_5_4);
        abi_TextLayoutShow();
        return;
    }
    wr16(LEVEL_NUMBER_AT + 2, (uint16_t)((both & 0x0F) + TILE_DIGIT_1));
    wr16(LEVEL_NUMBER_AT + 6, (uint16_t)((rd16(dba(LEVEL_STAGE)) & 0x0F) + TILE_DIGIT_1));
    wr16(LEVEL_NUMBER_AT + 4, TILE_DOT);
    wr16(LEVEL_NUMBER_AT, TILE_SPACE);
    wr16(LEVEL_NUMBER_AT + 8, TILE_SPACE);
    cpu.a = TILE_SPACE;
    flag_set(P_C, 0);
    flag_set(P_V, 0);
    inc8_at(dba((uint16_t)TEXT_TILEMAP_UPLOAD));
}

/*
 * @routine 80:A05C LevelSelect
 * WHAT IT DOES: With L or R held, steps the level's part (Up, Down) or its
 * stage (Right, Left) within their limits and shows the new level: C
 * clear. C set when neither L nor R is held or no direction is newly
 * pressed.
 * HOW: Up: the part up by 1 unless PART_LAST, stage 0; Down: the part down
 * by 1 unless 0, stage 0 (both may apply). Right: the stage up by 1
 * unless the part's last (PART_STAGES_LAST); else Left: down by 1 unless
 * 0. A step refused ends there; else LevelNumberText.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: M 8-bit, X
 * 8-bit; C as above; a step refused: X = the part or stage, N and Z as its
 * test; else as LevelNumberText leaves them.
 */
void abi_LevelSelect(void)
{
    uint8_t lr = (uint8_t)(rd8(dba(JOY_HELD)) & PAD_HELD_LR), dir, part, stage, last;

    a8_load(lr);
    if (!lr) {
        flag_set(P_C, 1);
        return;
    }
    dir = (uint8_t)(rd8(dba((uint16_t)(JOY_PRESSED + 1))) & 0x0F);
    a8_load(dir);
    if (!dir) {
        flag_set(P_C, 1);
        return;
    }
    if (dir & PAD_UP) {
        part = rd8(dba(LEVEL_PART));
        cpu.x = part;
        cmp8_of(part, PART_LAST);
        if (part == PART_LAST)
            goto refused;
        inc8_at(dba(LEVEL_PART));
        wr8(dba(LEVEL_STAGE), 0);
    }
    if (dir & PAD_DOWN) {
        part = rd8(dba(LEVEL_PART));
        x8_load(part);
        if (!part)
            goto refused;
        dec8_at(dba(LEVEL_PART));
        wr8(dba(LEVEL_STAGE), 0);
    }
    if (dir & PAD_RIGHT) {
        part = rd8(dba(LEVEL_PART));
        cpu.x = part;
        stage = rd8(dba(LEVEL_STAGE));
        a8_load(stage);
        last = rd8(dba(PART_STAGES_LAST) + part);
        cmp8_of(stage, last);
        if (stage == last)
            goto refused;
        inc8_at(dba(LEVEL_STAGE));
    } else if (dir & PAD_LEFT) {
        stage = rd8(dba(LEVEL_STAGE));
        x8_load(stage);
        if (!stage)
            goto refused;
        dec8_at(dba(LEVEL_STAGE));
    }
    call_frame(0xA0AC, abi_LevelNumberText);
refused:
    flag_set(P_C, 0);
}
