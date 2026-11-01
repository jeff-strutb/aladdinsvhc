/*
 * continue_screen.c: the screen between a lost life and the next try,
 * where the continues left are shown and one can be spent.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"
#include "wram_state.h"
#include "wram_video.h"

/* Where the continues digit sits in the text buffer: row 26, column 29. */
#define CONTINUES_DIGIT (TEXT_TILEMAP + 1722)
#define TILE_DIGIT_0 0x30

/*
 * @routine 80:B441 ContinuesShow
 * WHAT IT DOES: Shows the number of continues left on the continue screen.
 * HOW: The digit's tile is $30 plus the count (decimal when D is set, as
 * the CPU adds); it goes into the text buffer and the buffer's upload is
 * asked for.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: A low =
 * the tile; V and C from the addition; N and Z from the pending flag's
 * new value. A high, X, Y and the other flags unchanged.
 */
void abi_ContinuesShow(void)
{
    uint8_t a;

    cpu.p &= (uint8_t)~P_C;
    a = rd8(CONTINUES);
    a = op_adc8(a, TILE_DIGIT_0, &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    wr8(CONTINUES_DIGIT, a);
    a = (uint8_t)(rd8_rmw(TEXT_TILEMAP_UPLOAD) + 1);
    wr8(TEXT_TILEMAP_UPLOAD, a);
    cpu.p = nz8(cpu.p, a);
}

/*
 * @routine 80:B44F WramC800Clear
 * WHAT IT DOES: Clears the 4 KB buffer at $7E:C800 before a level starts.
 * HOW: Zeroes $7E:C800-$7E:D7FF a byte at a time with 16-bit X; P comes
 * back from the stack.
 * Entry: M 8-bit. Exit: A low = 0; X = 0 when it was 8-bit at entry,
 * $1000 when 16-bit; P as at entry. A high and Y unchanged.
 */
void abi_WramC800Clear(void)
{
    uint8_t p = cpu.p;
    uint16_t x;

    op_rep(P_X);
    for (x = 0; x < 0x1000; x++)
        wr8(TILE_BEHAVIOUR + x, 0);
    cpu.x = x;
    cpu.a &= 0xFF00;
    op_plp(p);
}
