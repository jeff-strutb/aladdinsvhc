/*
 * screen_tasks.c: tasks that change the screen over many frames (fades,
 * palette sequences, layer shakes), and runs of text-layer words.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_TextLayoutDraw(void);
void abi_BrightnessUpTask(void);
void abi_BrightnessDownTask(void);
void abi_TextLayerCopyWords(void);
void abi_TextLayerBlankWords(void);
void abi_RandomFromList(void);
void abi_TaskWait(void);
void abi_TaskExit(void);
void abi_ShakeUp(void);
void abi_ShakeDown(void);
void abi_ScreenFullBright(void);
void abi_PaletteLoad(void);
void abi_ColourFadeStep(void);
void abi_ColourFadeBegin(void);
void abi_RandomNext(void);
#include "wram.h"
#include "wram_video.h"

/*
 * @routine 81:9774 TextLayerCopyWords
 * WHAT IT DOES: Copies a run of words into the text layer's tilemap, or
 * repeats one word along it.
 * HOW: Scratch $0C words: each word at the data bank's Y goes to the
 * tilemap at X; X steps a word each time, Y only while C is clear (C set:
 * the same word all along).
 * Entry: M 16-bit, X 16-bit; X = the tilemap's byte offset; Y = the
 * source; C set to repeat; scratch $0C = the count (nonzero); called by
 * JSR. Exit: X, Y past the run; scratch $0C = 0; Z set, N clear; A = the
 * last word; C unchanged.
 */
void abi_TextLayerCopyWords(void)
{
    int repeat = (cpu.p & P_C) != 0;

    do {                                  /* the run's words (one, repeated, with C) */
        cpu.a = rd16(dba(0) + cpu.y);
        wr16((TEXT_TILEMAP + cpu.x) & 0xFFFFFF, cpu.a);
        if (!repeat)
            cpu.y = (uint16_t)(cpu.y + 2);
        cpu.x = (uint16_t)(cpu.x + 2);
    } while (dec16_at(dba(SCRATCH)));
}

/*
 * @routine 81:9787 TextLayerBlankWords
 * WHAT IT DOES: Blanks a run of the text layer's tilemap.
 * HOW: Scratch $0C words of $0020 (the blank tile) from X; X steps a word
 * each time, Y too while C is clear.
 * Entry: M 16-bit, X 16-bit; X = the tilemap's byte offset; scratch $0C =
 * the count (nonzero); called by JSR. Exit: X past the run; Y stepped as
 * X when C is clear; scratch $0C = 0; A = $0020; Z set, N clear; C
 * unchanged.
 */
void abi_TextLayerBlankWords(void)
{
    int repeat = (cpu.p & P_C) != 0;

    cpu.a = 0x0020;
    do {
        wr16((TEXT_TILEMAP + cpu.x) & 0xFFFFFF, 0x0020);
        if (!repeat)
            cpu.y = (uint16_t)(cpu.y + 2);
        cpu.x = (uint16_t)(cpu.x + 2);
    } while (dec16_at(dba(SCRATCH)));
}





/*
 * @routine 81:AE8A RandomFromList
 * WHAT IT DOES: Picks a random entry from list X.
 * HOW: The list (its offset at $DCE7 + X) starts with a mask; RandomNext
 * masked by it picks the entry, a byte after the mask.
 * Entry: X 8-bit; X = the list's number * 2; DB = $81. Exit: M and X
 * 8-bit; Y = A low byte = the entry; A high byte = the byte after it;
 * scratch $1C = the masked random (word); N, Z from the entry; C, V from
 * the ADC.
 */
void abi_RandomFromList(void)
{
    uint8_t mask;
    uint16_t pick;

    op_rep(0x30);
    cpu.y = rd16(dba(0xDCE7) + cpu.x);   /* the list: its mask, then its entries */
    op_sep(P_M);
    mask = rd8(dba(0xDCE7) + cpu.y);
    a8_load(mask);
    wr8(dba(0x001C), mask);
    abi_RandomNext();
    pick = (uint8_t)((uint8_t)cpu.a & rd8(dba(0x001C)));
    cpu.a = pick;
    wr16(dba(0x001C), pick);
    flag_set(P_C, 0);
    cpu.y = op_adc16(cpu.y, pick, &cpu.p);
    a16_load(rd16(dba(0xDCE8) + cpu.y));
    op_sep(0x30);
    y8_load((uint8_t)cpu.a);
}

/*
 * @routine 81:9734 TextLayoutDraw
 * WHAT IT DOES: Draws text layout A (low 7 bits) into the text layer's
 * tilemap, or blanks the places it covers (bit 7 set).
 * HOW: With DB = $82, the layout's offset from the word table at $B575;
 * then runs until a 0 byte: a count byte (low 6 bits; bit 6 set: one word
 * repeated), the tilemap offset (word), and the words (one when
 * repeated), copied by TextLayerCopyWords or blanked by
 * TextLayerBlankWords; A is kept at scratch $0F for the blank test.
 * Entry: M 8-bit, X 8-bit; A low byte = the layout; called by JSR. Exit:
 * M and X 8-bit; DB as at entry, N and Z from it (PLB); X, Y as the last
 * run left them (low bytes); scratch $0C = 0.
 */
void abi_TextLayoutDraw(void)
{
    uint8_t layout = (uint8_t)cpu.a, run;

    wr8(dba(SCRATCH + 3), layout);        /* bit 7: blank instead of copy */
    cpu.x = layout & 0x7F;
    phb();
    a8_load(0x82);
    pha8();
    plb();
    op_rep(P_X);
    cpu.y = rd16(dba(0xB575) + cpu.x);
    /* each run: a byte (bit 6 repeat, low 6 bits the count), its place
       (word), then its words (one when repeated); 0 ends */
    for (;;) {
        op_sep(P_M);
        run = rd8(dba(0) + cpu.y);
        a8_load(run);
        if (!run)
            break;
        cpu.y = (uint16_t)(cpu.y + 1);
        pha8();
        flag_set(P_C, run & 0x40);
        pla8();
        op_rep(P_M);
        cpu.a = run & 0x3F;
        wr16(dba(SCRATCH), cpu.a);
        cpu.x = rd16(dba(0) + cpu.y);
        cpu.y = (uint16_t)(cpu.y + 2);
        a16_load(rd16(dba(SCRATCH + 2)));
        if (cpu.a & 0x8000)
            abi_TextLayerBlankWords();
        else
            abi_TextLayerCopyWords();
        if (cpu.p & P_C)
            cpu.y = (uint16_t)(cpu.y + 2);
    }
    op_sep(0x30);
    plb();
}

/* The task's frames between steps: its parameter halved, at least 1. */
static void steps_halved(void)
{
    uint8_t param = rd8(dba(TASK_PARAM) + cpu.y);

    flag_set(P_C, param & 1);
    a8_load((uint8_t)(param >> 1));
    if (!(uint8_t)cpu.a)
        a8_load(0x01);
    wr8(dba(TASK_PARAM) + cpu.y, (uint8_t)cpu.a);
}

/*
 * @routine 81:9E8E BrightnessUpTask
 * WHAT IT DOES: A task that raises the screen's brightness a step at a
 * time to full, then ends.
 * HOW: The brightness shadow's low 4 bits kept; at 15 already it ends;
 * else the parameter is halved (at least 1) and, for each step, TaskWait
 * for that many frames, then the brightness up by one, until 15; then
 * TaskExit.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the frames between steps (halved); DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: the task ends.
 */
void abi_BrightnessUpTask(void)
{
    uint8_t level = rd8(dba(INIDISP_SHADOW)) & 0x0F;

    a8_load(level);
    wr8(dba(INIDISP_SHADOW), level);
    cmp8_of(level, 0x0F);
    if (level != 0x0F) {
        steps_halved();
        do {                              /* up a step each wait, to full */
            a8_load(rd8(dba(TASK_PARAM) + cpu.y));
            jsl_frame(0x819EA7u);
            abi_TaskWait();
            level = inc8_at(dba(INIDISP_SHADOW));
            a8_load(level);
            cmp8_of(level, 0x0F);
        } while (level != 0x0F);
    }
    abi_TaskExit();
}

/*
 * @routine 81:9EB9 BrightnessDownTask
 * WHAT IT DOES: A task that lowers the screen's brightness a step at a
 * time to 0 (dark), then ends.
 * HOW: The brightness shadow's low 4 bits kept; at 0 already it ends;
 * else the parameter is halved (at least 1) and, for each step, TaskWait
 * for that many frames, then the brightness down by one, until 0; then
 * TaskExit.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the frames between steps (halved); DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: the task ends.
 */
void abi_BrightnessDownTask(void)
{
    uint8_t level = rd8(dba(INIDISP_SHADOW)) & 0x0F;

    a8_load(level);
    wr8(dba(INIDISP_SHADOW), level);
    if (level) {
        steps_halved();
        do {                              /* down a step each wait, to 0 */
            a8_load(rd8(dba(TASK_PARAM) + cpu.y));
            jsl_frame(0x819ED0u);
            abi_TaskWait();
        } while (dec8_at(dba(INIDISP_SHADOW)));
    }
    abi_TaskExit();
}
