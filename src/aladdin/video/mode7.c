/*
 * mode7.c: setting up a Mode 7 screen: its map and a sine table in work
 * RAM.
 *
 * Mode 7's map is VRAM's low bytes (128x128 tiles), its graphics the high
 * bytes. A screen is staged in the 2 KB buffer at $7F:5000: the buffer is
 * read back from VRAM and written into the map.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "hw.h"

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

static uint32_t dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

#define MODE7_BUFFER 0x7F5000u     /* staging buffer */
/* The direct-page words the callers pass VRAM addresses in. */
#define MODE7_MAP_AT 0x0Cu         /* where in the map the buffer goes */
#define MODE7_READ_AT 0x0Eu        /* where VRAM is read from */
/* A quarter turn of sine, 17 words from 0 to $100 (1.0), in bank $81. */
#define SINE_QUARTER 0x81B3E5u
/* The sine table: 64 words a turn, then the cosine table, in bank $7F. */
#define SINE_TABLE 0x4F00u
#define COSINE_TABLE 0x4F80u

/* VMAIN = v, then the VRAM address. */
static void vram_at(uint8_t vmain, uint16_t addr)
{
    wr8(abs_db(VMAIN & 0xFFFF), vmain);
    wr16(abs_db(VMADDL & 0xFFFF), addr);
}

/*
 * @routine 81:AFC0 Mode7MapClear
 * WHAT IT DOES: Clears the whole Mode 7 map to tile 0.
 * HOW: Steps VRAM after each low byte from address 0 and writes 0 to the
 * low byte 16,384 times.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $2115-$2119 are the PPU's;
 * called by JSR. Exit: A low byte = 0; X = Y = 0; Z set, N clear; C, V
 * unchanged.
 */
void abi_Mode7MapClear(void)
{
    uint16_t y;

    cpu.a &= 0xFF00;
    op_rep(P_X);
    vram_at(0x00, 0x0000);
    for (y = 0x4000; y != 0; y--)
        wr8(abs_db(VMDATAL & 0xFFFF), 0x00);
    cpu.x = 0x4000;
    cpu.y = 0;
    cpu.p = nz16(cpu.p, 0);
    op_sep(P_X);
}

/*
 * @routine 81:AFDF Mode7MapFromBuffer
 * WHAT IT DOES: Writes the 32x32 tiles in the staging buffer into the Mode
 * 7 map from the address the caller gives.
 * HOW: Steps VRAM after each low byte and writes the buffer's 1,024 bytes
 * to the low bytes; after every 32 the address moves down a map row
 * (+$80, with the carry as it stands) and is set again.
 * Entry: M 8-bit, X 8-bit; D's $0C (word) = the map address; DB a bank
 * whose $2115-$2119 are the PPU's; called by JSR. Exit: $0C = the address
 * after the last row; A = it (16 bits); X = Y = 0; Z set, N clear; C, V
 * from its last ADC.
 */
void abi_Mode7MapFromBuffer(void)
{
    uint16_t x = 0, y;

    cpu.a = (uint16_t)(cpu.a & 0xFF00);
    op_rep(P_X);
    vram_at(0x00, rd16(dp(MODE7_MAP_AT)));
    for (y = 0x0400; y != 0; y--) {
        uint8_t v = rd8(MODE7_BUFFER + x);

        wr8(abs_db(VMDATAL & 0xFFFF), v);
        x++;
        if ((x & 0x1F) == 0) {
            cpu.a = op_adc16(rd16(dp(MODE7_MAP_AT)), 0x0080, &cpu.p);
            wr16(dp(MODE7_MAP_AT), cpu.a);
            wr16(abs_db(VMADDL & 0xFFFF), cpu.a);
        } else {
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | (x & 0x1F));
        }
    }
    cpu.x = x;
    cpu.y = 0;
    cpu.p = nz16(cpu.p, 0);
    op_sep(P_X);
}

/*
 * @routine 81:B012 Mode7MapToBuffer
 * WHAT IT DOES: Reads 2 KB of the Mode 7 map from the address the caller
 * gives into the staging buffer.
 * HOW: Steps VRAM after each low byte, makes the dummy read the PPU's read
 * latch needs, then reads 2,048 low bytes.
 * Entry: M 8-bit, X 8-bit; D's $0E (word) = the VRAM address; DB a bank
 * whose $2115-$213A are the PPU's; called by JSR. Exit: A low byte = the
 * last byte read; X = Y = 0; Z set, N clear; C, V unchanged.
 */
void abi_Mode7MapToBuffer(void)
{
    uint16_t x, y;
    uint8_t v;

    cpu.a &= 0xFF00;
    op_rep(P_X);
    vram_at(0x00, rd16(dp(MODE7_READ_AT)));
    v = rd8(abs_db(RDVRAML & 0xFFFF));
    for (x = 0, y = 0x0800; y != 0; y--, x++) {
        v = rd8(abs_db(RDVRAML & 0xFFFF));
        wr8(MODE7_BUFFER + x, v);
    }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.x = x;
    cpu.y = 0;
    cpu.p = nz16(cpu.p, 0);
    op_sep(P_X);
}

/*
 * @routine 81:B035 VramWordsToBuffer
 * WHAT IT DOES: Reads 1,024 words of VRAM from the address the caller
 * gives into the staging buffer: Mode 7 graphics, a pixel per high byte.
 * HOW: Steps VRAM after each high byte, makes the dummy read (a word read
 * of $213A, which reads $213B too), then reads 1,024 words.
 * Entry: M 8-bit, X 8-bit; D's $0E (word) = the VRAM address; DB a bank
 * whose $2115-$213B are the PPU's; called by JSR. Exit: M and X 8-bit;
 * A = the last word read; X = Y = 0; Z set, N clear; C, V unchanged.
 */
void abi_VramWordsToBuffer(void)
{
    uint16_t x, y;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x80);
    wr8(abs_db(VMAIN & 0xFFFF), 0x80);
    op_rep(P_M | P_X);
    cpu.a = rd16(dp(MODE7_READ_AT));
    wr16(abs_db(VMADDL & 0xFFFF), cpu.a);
    cpu.a = rd16(abs_db(RDVRAMH & 0xFFFF));
    for (x = 0, y = 0x0400; y != 0; y--, x += 2) {
        cpu.a = rd16(abs_db(RDVRAML & 0xFFFF));
        wr16(MODE7_BUFFER + x, cpu.a);
    }
    cpu.x = x;
    cpu.y = 0;
    cpu.p = nz16(cpu.p, 0);
    op_sep(P_M | P_X);
}

/*
 * @routine 81:B120 SineTableBuild
 * WHAT IT DOES: Builds the sine and cosine tables (64 steps a turn, $100
 * = 1) in bank $7F from a quarter turn in ROM.
 * HOW: Each of the quarter's 17 words goes to its four places in the sine
 * table and its four in the cosine table, mirrored and negated as the
 * quadrant needs.
 * Entry: M 8-bit, X 8-bit. Exit: M and X 8-bit; X = $22; Y = $FE; A = the
 * last value negated (16 bits); DB as at entry, N and Z from it (PLB);
 * C, V unchanged.
 */
void abi_SineTableBuild(void)
{
    uint16_t x = 0, y = 0x20, v;

    op_rep(P_M | P_X);
    do {
        v = rd16(SINE_QUARTER + x);
        wr16(0x7F0000u | (uint16_t)(SINE_TABLE + x), v);
        wr16(0x7F0000u | (uint16_t)(COSINE_TABLE + 0x60 + x), v);
        wr16(0x7F0000u | (uint16_t)(SINE_TABLE + 0x20 + y), v);
        wr16(0x7F0000u | (uint16_t)(COSINE_TABLE + y), v);
        v = (uint16_t)(~v + 1);
        wr16(0x7F0000u | (uint16_t)(SINE_TABLE + 0x40 + x), v);
        wr16(0x7F0000u | (uint16_t)(COSINE_TABLE + 0x20 + x), v);
        wr16(0x7F0000u | (uint16_t)(SINE_TABLE + 0x60 + y), v);
        wr16(0x7F0000u | (uint16_t)(COSINE_TABLE + 0x40 + y), v);
        x += 2;
        y -= 2;
    } while (!(y & 0x8000));
    cpu.a = v;
    cpu.x = x;
    cpu.y = y;
    op_sep(P_M | P_X);
    cpu.p = nz8(cpu.p, cpu.db);               /* PLB */
}
