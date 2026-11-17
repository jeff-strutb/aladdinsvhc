/*
 * story_text.c: the tasks that type the story's text onto the text layer,
 * a character every few frames: one text of set 0 (the opening story's
 * scenes) or of set 1 (the story's later scenes), picked by the task's
 * parameter. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"

void abi_TaskExit(void);
void abi_TaskWait(void);
void abi_TextLayerClear(void);

/* In bank $81 (the data bank): per set (0, 1) a byte, the offset from
   here of the set's list; each list is per text its offset in STORY_TEXT
   (words) */
#define STORY_TEXT_SETS 0xFD8Du
/* The texts: characters (tiles below $FB), and the codes: $FB place (the
   tilemap byte offset of the text's first row and column, a word), $FC
   palette (a byte, its low 3 bits), $FD wait (a byte, frames), $FE page
   (wait 160 frames, clear the layer, back to the first row and column),
   $FF next row; 0 ends */
#define STORY_TEXT 0xA50000u
#define STORY_TEXT_PLACE 0xFB
#define STORY_TEXT_UP_MARK_1 0x00BA   /* marks put on the row above, at the column the next character takes */
#define STORY_TEXT_UP_MARK_2 0x00B7
#define STORY_TEXT_PAGE_WAIT 0xA0
#define STORY_TILE (SCRATCH + 32)     /* $002C: the tilemap word being put (first the parameter times 2) */
#define STORY_TILE_AT (SCRATCH + 34)  /* $002E: the row's start plus the column */
/* The task's own bytes in its slot (the direct page) */
#define ST_FRAMES 0x06                /* frames between characters (on entry, the parameter) */
#define ST_PALETTE 0x07               /* the palette bits of the tilemap word's high byte */
#define ST_COLUMN 0x08
#define ST_ROW 0x09                   /* each row two of the layer's rows */
#define ST_ORIGIN 0x0A                /* the tilemap byte offset of the first row and column (word) */
#define ST_CHAR_FRAMES 0x04
#define TEXT_PRIORITY 0x20            /* the high byte's priority bit */

/* Both tasks from their LDX: D = the slot, the text of set X picked by
   the parameter, then typed until its 0, where the task ends. */
static void story_text_type(uint8_t set)
{
    uint8_t c;

    op_rep(P_M);
    clc();
    tya16();
    adc16_v(TASK_STATE);
    tcd();
    op_sep(P_M);
    ldx8_v(set);
    ldy8_v(ST_CHAR_FRAMES);
    lda8(ea_dp(ST_FRAMES));
    ACC8(asl);
    sta8(ea_abs(STORY_TILE));
    sty8(ea_dp(ST_FRAMES));
    stz8(ea_dp(ST_PALETTE));
    stz8(ea_dp(ST_COLUMN));
    stz8(ea_dp(ST_ROW));
    lda8(ea_absx(STORY_TEXT_SETS));
    clc();
    adc8_v(rd8(ea_abs(STORY_TILE)));
    tay8();
    op_rep(P_X);
    abi_TextLayerClear();
    ldx16(ea_absy(STORY_TEXT_SETS));

    for (;;) {
        lda8(ea_longx(STORY_TEXT));
        c = (uint8_t)cpu.a;
        if (!c) {                         /* the end */
            op_sep(0x30);
            abi_TaskExit();
            return;
        }
        cmp8_v(STORY_TEXT_PLACE);
        if (c < STORY_TEXT_PLACE) {       /* a character */
            phx16();
            sta8(ea_abs(STORY_TILE));
            lda8(ea_dp(ST_PALETTE));
            ora8_v(TEXT_PRIORITY);
            sta8(ea_abs(STORY_TILE + 1));
            op_rep(P_M);
            lda16(ea_dp(ST_COLUMN));
            and16_v(0x00FF);
            ACC16(asl);
            clc();
            adc16_v(rd16(ea_dp(ST_ORIGIN)));
            sta16(ea_abs(STORY_TILE_AT));
            lda16(ea_dp(ST_ROW));
            and16_v(0x00FF);
            xba();
            ACC16(lsr);
            clc();
            adc16_v(rd16(ea_abs(STORY_TILE_AT)));
            tax16();
            lda16(ea_abs(STORY_TILE));
            and16_v(0x00FF);
            cmp16_v(STORY_TEXT_UP_MARK_1);
            if (!IF_Z)
                cmp16_v(STORY_TEXT_UP_MARK_2);
            if (IF_Z) {
                /* a mark on the row above: no column step, no wait (no
                   script types one: written from its bytes) */
                txa16();
                clc();
                adc16_v(0xFFC0);
                tax16();
                lda16(ea_abs(STORY_TILE));
                sta16(ea_longx(TEXT_TILEMAP));
                op_sep(P_M);
            } else {
                lda16(ea_abs(STORY_TILE));
                sta16(ea_longx(TEXT_TILEMAP));
                op_sep(P_M);
                MEM8(inc, ea_abs(TEXT_TILEMAP_UPLOAD));
                MEM8(inc, ea_dp(ST_COLUMN));
                lda8(ea_dp(ST_FRAMES));
                jsl_frame(0x81F42Au);
                abi_TaskWait();
            }
            plx16();
            inx16();
            continue;
        }
        switch (c) {
        case 0xFF:                        /* next row */
            ACC8(inc);
            stz8(ea_dp(ST_COLUMN));
            MEM8(inc, ea_dp(ST_ROW));
            inx16();
            break;
        case 0xFE:                        /* page */
            ACC8(inc);
            ACC8(inc);
            stz8(ea_dp(ST_COLUMN));
            stz8(ea_dp(ST_ROW));
            lda8_v(STORY_TEXT_PAGE_WAIT);
            jsl_frame(0x81F3ABu);
            abi_TaskWait();
            phx16();
            abi_TextLayerClear();
            plx16();
            inx16();
            break;
        case 0xFD:                        /* wait */
            ACC8(inc);
            ACC8(inc);
            ACC8(inc);
            lda8(ea_longx(STORY_TEXT + 1));
            jsl_frame(0x81F3BCu);
            abi_TaskWait();
            inx16();
            inx16();
            break;
        case 0xFC:                        /* palette */
            ACC8(inc);
            ACC8(inc);
            ACC8(inc);
            ACC8(inc);
            lda8(ea_longx(STORY_TEXT + 1));
            and8_v(0x07);
            ACC8(asl);
            ACC8(asl);
            sta8(ea_dp(ST_PALETTE));
            inx16();
            inx16();
            break;
        default:                          /* $FB: place */
            ACC8(inc);
            ACC8(inc);
            ACC8(inc);
            ACC8(inc);
            lda8(ea_longx(STORY_TEXT + 1));
            sta8(ea_dp(ST_ORIGIN));
            lda8(ea_longx(STORY_TEXT + 2));
            sta8(ea_dp(ST_ORIGIN + 1));
            inx16();
            inx16();
            inx16();
            break;
        }
    }
}

/*
 * @routine 81:F332 StoryTextTask
 * WHAT IT DOES: A task that types text of set 0 (the opening story's) onto
 * the text layer, a character every 4 frames, then ends.
 * HOW: D = its slot; the text picked by its parameter from STORY_TEXT_SETS
 * set 0; the layer cleared (TextLayerClear); then each byte of the text:
 * a character (below $FB) put at the row and column with the palette and
 * priority bits, the upload marked, the column stepped, TaskWait 4
 * (STORY_TEXT_UP_MARK_1 and _2 go a row up with no step and no wait); $FB
 * sets the place of the first row and column, $FC the palette, $FD waits
 * its byte's frames, $FE waits 160 frames and clears the layer back to
 * the first row and column, $FF goes to the next row; at 0, TaskExit. No
 * script types a mark ($BA, $B7): that path is written from its bytes.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the text; DB a bank whose $0000-$1FFF is work RAM and whose
 * $FD8D holds STORY_TEXT_SETS (bank $81). Exit: none: the task ends
 * (TaskExit with M and X 8-bit).
 */
void abi_StoryTextTask(void)
{
    story_text_type(0);
}

/*
 * @routine 81:F344 StoryTextTask1
 * WHAT IT DOES: The text-typing task of StoryTextTask for set 1 (the
 * story's scenes, kind $9C's).
 * HOW: As StoryTextTask, with set 1.
 * Entry: as StoryTextTask. Exit: none: the task ends (TaskExit with M and
 * X 8-bit).
 */
void abi_StoryTextTask1(void)
{
    story_text_type(1);
}
