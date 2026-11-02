/*
 * colour_bands.c: the colour bands' HDMA table: the fixed colour set
 * line by line down the screen, written into one of two buffers.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_ColourBandsWrite(void);
#include "wram.h"

#define BAND_COUNT 0xBA7Eu     /* the bands: their count, then (lines, value) pairs (in the bank DB names) */
#define BANDS 0xBA7Fu
#define BAND_BUFFERS 0xBAA1u   /* by buffer, its offset in BAND_TABLE */
#define BAND_TABLE 0x7EF500u   /* the HDMA tables: two buffers */
#define BAND_TABLE_USED 0x0356u /* the buffer the HDMA reads */
#define BANDS_DIM 0x06u        /* the task's: the dimming step */
#define BANDS_BUFFER 0x07u     /* the task's: the buffer to write */
#define BANDS_LEFT 0x08u       /* the task's: bands left to copy */
#define BANDS_DIMMED_FROM 0x22 /* the 18th pair on is dimmed */

/*
 * @routine 81:A08A ColourBandsWrite
 * WHAT IT DOES: Writes the colour bands' HDMA table into the task's
 * spare buffer, the bands from the 18th on dimmed by its step, and points
 * the HDMA at it.
 * HOW: The buffer (the task's $07: 0 or 1) is at $7E:F500 + its byte at
 * $BAA1; the band count at $BA7E, then (lines, value) pairs from $BA7F,
 * are copied (counting at the task's $08), each value from the 18th pair
 * on less the step (the task's $06), not below 0; $0356 = the buffer's
 * offset; the buffer flips.
 * Entry: M 8-bit, X 8-bit; D = the colour band task's record (its slot +
 * $6F); DB = $80; called by JSR. Exit: A low byte = the new buffer; X, Y
 * past the table (low bytes); the task's $08 = 0.
 */
void abi_ColourBandsWrite(void)
{
    uint8_t buffer = rd8(dpa(BANDS_BUFFER)), left, value;

    y8_load(buffer);
    phy8();
    x8_load(rd8(dba(BAND_BUFFERS) + buffer));
    phx8();
    left = rd8(dba(BAND_COUNT));
    a8_load(left);
    wr8(dpa(BANDS_LEFT), left);
    cpu.y = 0;
    do {
        wr8(BAND_TABLE + cpu.x, rd8(dba(BANDS) + cpu.y));
        value = rd8(dba(BANDS + 1) + cpu.y);
        a8_load(value);
        cmp8_of((uint8_t)cpu.y, BANDS_DIMMED_FROM);
        if (cpu.y >= BANDS_DIMMED_FROM) {
            flag_set(P_C, 1);
            value = op_sbc8(value, rd8(dpa(BANDS_DIM)), &cpu.p);
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | value);
            if (value & 0x80) {
                value = 0;
                a8_load(0);
            }
        }
        wr8(BAND_TABLE + 1 + cpu.x, value);
        cpu.y = (uint8_t)(cpu.y + 2);
        cpu.x = (uint8_t)(cpu.x + 2);
        left = (uint8_t)(rd8(dpa(BANDS_LEFT)) - 1);
        wr8(dpa(BANDS_LEFT), left);
        nz8_of(left);
    } while (left);
    plx8();
    wr8(dba(BAND_TABLE_USED), (uint8_t)cpu.x);
    pla8();
    a8_load((uint8_t)((uint8_t)cpu.a ^ 0x01));   /* the other buffer next */
    wr8(dpa(BANDS_BUFFER), (uint8_t)cpu.a);
}
