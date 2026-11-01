/*
 * oam.c: the sprite (OAM) image in work RAM.
 *
 * Each frame the game builds the 544-byte OAM image at $7E:FA80 (128
 * sprites of 4 bytes, then the 32-byte high table) and the NMI copies it to
 * the PPU. A sprite at y $E0 is off the screen.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

static uint32_t dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

/* Bytes of the frame's high-table accumulator (direct page). */
#define OAM_HIGH_BITS 0x61u        /* the bits gathered for the next high-table byte (word) */
#define OAM_HIGH_COUNT 0x63u       /* sprites left before it is full (word) */

/*
 * @routine 81:95D5 OamBufferClear
 * WHAT IT DOES: Takes every sprite off the screen.
 * HOW: Fills the 512 sprite bytes with $E0 (y below the screen) and the
 * high table with 0, last byte first.
 * Entry: M 8-bit. Exit: A low byte = 0; X = $FF (X 8-bit) or $FFFF; Y and
 * the flags as at entry (P pulled back).
 */
void abi_OamBufferClear(void)
{
    int x;

    op_push8(cpu.p);
    op_rep(P_X);
    for (x = 0x01FF; x >= 0; x--)
        wr8(OAM_BUFFER + x, 0xE0);
    for (x = 0x001F; x >= 0; x--)
        wr8(OAM_HIGH + x, 0x00);
    cpu.a &= 0xFF00;
    cpu.x = 0xFFFF;
    op_plp(op_pull8());
}

/*
 * @routine 81:9230 OamBuildBegin
 * WHAT IT DOES: Starts the frame's sprites: none used, all 128 free, the
 * high table cleared.
 * HOW: Clears the sprite and high-table offsets and $0857, sets the
 * high-table accumulator to empty (8 sprites to go, no bits), clears the
 * high table a word at a time and sets 128 free sprites.
 * Entry: M 8-bit, X 8-bit; D = 0; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: M 8-bit; A = $0080; X = $FE; N, Z clear; Y, C, V unchanged.
 */
void abi_OamBuildBegin(void)
{
    int x;

    op_rep(P_M);
    wr16(abs_db(OAM_0857 & 0xFFFF), 0);
    wr16(abs_db(OAM_NEXT & 0xFFFF), 0);
    wr16(abs_db(OAM_HIGH_NEXT & 0xFFFF), 0);
    wr16(dp(OAM_HIGH_COUNT), 0x0008);
    wr16(dp(OAM_HIGH_BITS), 0);
    for (x = 0x1E; x >= 0; x -= 2)
        wr16(OAM_HIGH + x, 0);
    cpu.x = 0xFE;
    cpu.a = 0x0080;
    cpu.p = nz16(cpu.p, 0x0080);
    wr16(abs_db(OAM_FREE & 0xFFFF), 0x0080);
    op_sep(P_M);
}
