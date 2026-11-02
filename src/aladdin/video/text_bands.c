/*
 * text_bands.c: a screen of text shown band by band (the story text in
 * bank $82): a page of lines written to the text tilemap, and the bands'
 * window bytes handed to HDMA channel 5 a frame at a time.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_TaskWait(void);
void abi_TextBandsConceal(void);
void abi_TextBandsReveal(void);
void abi_TextBandsCommit(void);
void abi_TextPageDraw(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

#define TEXT_BANDS_HDMA 0x0352u    /* the HDMA table the bands use (word: $CEEF or $CF3B) */
#define TEXT_BANDS_FLIP 0xF9u      /* in the direct page: its low bit picks the buffer */
#define TEXT_BAND_STEP 0xF7u       /* in the direct page: the band being revealed or concealed */
#define TEXT_PAGE_LINES 0xF5u      /* in the direct page: the next line's place (word) */
#define BAND_ROWS_A 0xCF87u        /* by band, its two rows' offsets in TEXT_BANDS (in the bank DB names) */
#define BAND_ROWS_B 0xCF8Bu

/* Band row values from band TEXT_BAND_STEP set to `value`, committed, then
   a wait of 4 frames (returning at `site`). */
static void band_set(uint8_t value, uint32_t site)
{
    x8_load(rd8(dpa(TEXT_BAND_STEP)));
    a8_load(value);
    y8_load(rd8(dba(BAND_ROWS_A) + cpu.x));
    wr8(dba(TEXT_BANDS) + cpu.y, value);
    y8_load(rd8(dba(BAND_ROWS_B) + cpu.x));
    wr8(dba(TEXT_BANDS) + cpu.y, value);
    abi_TextBandsCommit();
    a8_load(0x04);
    jsl_frame(site);
    abi_TaskWait();
}

/*
 * @routine 82:B270 TextBandsCommit
 * WHAT IT DOES: Hands the 16 window bytes of the text bands ($1C8E) to
 * HDMA channel 5, through whichever of its two tables is not in use.
 * HOW: By bit 0 of $F9: clear, the bytes go to $1C9E and channel 5's
 * table address to $CEEF; set, to $1CAE and $CF3B (the addresses into
 * $0352, channel 5's in hdma_table_at). $F9 steps up.
 * Entry: M 8-bit, X 8-bit; D the page holding $F9; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR. Exit: X = $FF; Y = $0F or $1F;
 * A low byte = the last byte copied.
 */
void abi_TextBandsCommit(void)
{
    uint8_t flip = rd8(dpa(TEXT_BANDS_FLIP)), v;

    /* the 16 band values copied into the buffer the flip picks, and the
       HDMA pointed at it */
    x8_load(0x0F);
    y8_load(0x1F);
    a8_load(0xEF);
    wr8(dba(TEXT_BANDS_HDMA), 0xEF);
    a8_load(0xCE);
    wr8(dba(TEXT_BANDS_HDMA + 1), 0xCE);
    a8_load((uint8_t)(flip >> 1));
    flag_set(P_C, flip & 1);
    if (flip & 1) {
        y8_load(0x2F);
        a8_load(0x3B);
        wr8(dba(TEXT_BANDS_HDMA), 0x3B);
        a8_load(0xCF);
        wr8(dba(TEXT_BANDS_HDMA + 1), 0xCF);
    }
    do {
        v = rd8(dba(TEXT_BANDS) + cpu.x);
        a8_load(v);
        wr8(dba(TEXT_BANDS) + cpu.y, v);
        cpu.y = (uint8_t)(cpu.y - 1);
        x8_load((uint8_t)(cpu.x - 1));
    } while (!(cpu.x & 0x80));
    inc8_at(dpa(TEXT_BANDS_FLIP));
}

/*
 * @routine 82:B190 TextPageDraw
 * WHAT IT DOES: Writes a page of text into the text tilemap, each line at
 * the next place from the list at $F5 with the line's attribute byte,
 * then has the NMI upload the tilemap.
 * HOW: The page's first byte is its line count ($000C). Per line: the
 * list's next word (stepping $F5) is its tilemap offset; then the line's
 * attribute byte ($000D) and length ($000E), then its characters, each
 * the low byte of a tilemap word whose high byte is the attribute.
 * text_tilemap_upload steps up.
 * Entry: M 8-bit, X 16-bit; Y = the page's offset; D = 0; DB = the bank of
 * the page and the list; called by JSR. Exit: X, Y past the last line;
 * $000C, $000E = 0.
 */
void abi_TextPageDraw(void)
{
    uint8_t lines = rd8(dba(0) + cpu.y);

    a8_load(lines);
    wr8(dba(SCRATCH), lines);
    cpu.y = (uint16_t)(cpu.y + 1);
    do {
        /* each line: its place from the list, its attribute and length,
           its characters */
        phy16();
        cpu.x = dp16(TEXT_PAGE_LINES);
        cpu.y = rd16(dba(0) + cpu.x);
        cpu.x = (uint16_t)(cpu.x + 2);
        dp16_set(TEXT_PAGE_LINES, cpu.x);
        cpu.x = cpu.y;
        ply16();
        a8_load(rd8(dba(0) + cpu.y));
        wr8(dba(SCRATCH + 1), (uint8_t)cpu.a);
        a8_load(rd8(dba(1) + cpu.y));
        wr8(dba(SCRATCH + 2), (uint8_t)cpu.a);
        cpu.y = (uint16_t)(cpu.y + 2);
        do {
            a8_load(rd8(dba(0) + cpu.y));
            wr8((TEXT_TILEMAP + cpu.x) & 0xFFFFFF, (uint8_t)cpu.a);
            a8_load(rd8(dba(SCRATCH + 1)));
            wr8((TEXT_TILEMAP + 1 + cpu.x) & 0xFFFFFF, (uint8_t)cpu.a);
            cpu.x = (uint16_t)(cpu.x + 2);
            cpu.y = (uint16_t)(cpu.y + 1);
        } while (dec8_at(dpa(SCRATCH + 2)));
    } while (dec8_at(dpa(SCRATCH)));
    inc8_at(dba(TEXT_TILEMAP_UPLOAD));
}

/*
 * @routine 82:B224 TextBandsReveal
 * WHAT IT DOES: Shows the text band by band from the middle out: a pair
 * of bands every 4 frames, over 4 steps.
 * HOW: For step $F7 = 0 to 3: the window bytes of the step's two bands
 * (from $80:CF87 and $80:CF8B) set to 0, TextBandsCommit, TaskWait 4.
 * Entry: M 8-bit, X 8-bit; D the page holding $F7 and $F9; DB = $80;
 * called by JSR from a task. Exit: as TaskWait; $F7 = 4.
 */
void abi_TextBandsReveal(void)
{
    uint8_t step;

    wr8(dpa(TEXT_BAND_STEP), 0);
    do {                                  /* the four bands cleared, one each 4 frames */
        band_set(0x00, 0x82B23Bu);
        step = inc8_at(dpa(TEXT_BAND_STEP));
        a8_load(step);
        cmp8_of(step, 0x04);
    } while (step != 0x04);
}

/*
 * @routine 82:B248 TextBandsConceal
 * WHAT IT DOES: Masks the text band by band from the outside in, a pair
 * of bands every 4 frames, then waits 40 frames.
 * HOW: For step $F7 = 3 down to 0: the window bytes of the step's two
 * bands set to 8 (masked), TextBandsCommit, TaskWait 4; then TaskWait 40.
 * Entry: M 8-bit, X 8-bit; D the page holding $F7 and $F9; DB = $80;
 * called by JSR from a task. Exit: as TaskWait; $F7 = $FF.
 */
void abi_TextBandsConceal(void)
{
    a8_load(0x03);
    wr8(dpa(TEXT_BAND_STEP), 0x03);
    do                                    /* the four bands set to 8, last first */
        band_set(0x08, 0x82B261u);
    while (!(dec8_at(dpa(TEXT_BAND_STEP)) & 0x80));
    a8_load(0x28);
    jsl_frame(0x82B26Bu);
    abi_TaskWait();
}
