/*
 * parallax.c: the backdrop's scroll, from the play field's.
 *
 * Each frame the scroll update ends in one of a table of handlers, chosen
 * by the level ($083F), that set the backdrop layer's scroll from the play
 * field's: at the same speed, at half speed, or not at all, so the
 * backdrop looks farther away.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram_video.h"

#define SCRATCH_0C 0x00000Cu

/*
 * @routine 81:C7A5 ParallaxNone
 * WHAT IT DOES: Leaves the backdrop where it is: for levels whose backdrop
 * does not scroll with the play field.
 * HOW: Returns at once (the shared return of ParallaxHalf).
 * Entry: any. Exit: everything unchanged.
 */
void abi_ParallaxNone(void)
{
}

/*
 * @routine 81:C797 ParallaxHalf
 * WHAT IT DOES: Scrolls the backdrop at half the play field's speed, both
 * ways, so it looks far away.
 * HOW: The backdrop's scroll X and Y are the play field's, halved.
 * Entry: M 16-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: A = the
 * play field's scroll Y halved; N clear, Z from it, C the bit shifted out;
 * X, Y and the other flags unchanged.
 */
void abi_ParallaxHalf(void)
{
    op_lda16(LAYER0 + LAYER_SCROLL_X);
    op_lsr16();
    wr16(LAYER1 + LAYER_SCROLL_X, cpu.a);
    op_lda16(LAYER0 + LAYER_SCROLL_Y);
    op_lsr16();
    wr16(LAYER1 + LAYER_SCROLL_Y, cpu.a);
    abi_ParallaxNone();
}

/*
 * @routine 81:C7A6 ParallaxFull
 * WHAT IT DOES: Scrolls the backdrop with the play field, at its speed.
 * HOW: The backdrop's scroll X and Y are the play field's.
 * Entry: M 16-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: A = the
 * play field's scroll Y; N and Z from it; X, Y and the other flags
 * unchanged.
 */
void abi_ParallaxFull(void)
{
    op_lda16(LAYER0 + LAYER_SCROLL_X);
    wr16(LAYER1 + LAYER_SCROLL_X, cpu.a);
    op_lda16(LAYER0 + LAYER_SCROLL_Y);
    wr16(LAYER1 + LAYER_SCROLL_Y, cpu.a);
}

/*
 * @routine 81:C7D4 ParallaxHalfX
 * WHAT IT DOES: Scrolls the backdrop at half the play field's speed across
 * and at its full speed up and down.
 * HOW: The backdrop's scroll X is the play field's halved, its scroll Y
 * the play field's.
 * Entry: M 16-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: A = the
 * play field's scroll Y; N and Z from it; C the bit shifted out of scroll
 * X; X, Y and the other flags unchanged.
 */
void abi_ParallaxHalfX(void)
{
    op_lda16(LAYER0 + LAYER_SCROLL_X);
    op_lsr16();
    wr16(LAYER1 + LAYER_SCROLL_X, cpu.a);
    op_lda16(LAYER0 + LAYER_SCROLL_Y);
    wr16(LAYER1 + LAYER_SCROLL_Y, cpu.a);
}

/*
 * @routine 81:C7E7 ParallaxDrift
 * WHAT IT DOES: Scrolls a backdrop of 11 horizontal bands, each drifting
 * at its own speed, one band (the tenth) fixed to the play field.
 * HOW: Each band's position steps by its speed; the tenth's becomes the
 * play field's scroll X. The positions go into the bands' HDMA table, and
 * the first band's into the backdrop's scroll.
 * Entry: M 16-bit, X 16-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: M 16-bit; A = the first band's position; X = 44, Y = 33; $000C =
 * 0; N and Z from DB; DB unchanged; C from the last step's high word.
 */
void abi_ParallaxDrift(void)
{
    uint16_t x, y, n;
    uint8_t db = cpu.db;

    cpu.x = 0;
    cpu.p = nz16(cpu.p, 0);
    cpu.y = 0x000B;
    cpu.p = nz16(cpu.p, 0x000B);
    do {
        x = cpu.x;
        op_lda16(BAND_POS + x);
        op_add16(rd16(BAND_SPEED + x));
        wr16(BAND_POS + x, cpu.a);
        op_lda16(BAND_POS + 2 + x);
        cpu.a = op_adc16(cpu.a, 0, &cpu.p);
        wr16(BAND_POS + 2 + x, cpu.a);
        cpu.x = (uint16_t)(x + 4);
        cpu.p = nz16(cpu.p, cpu.x);
        cpu.y--;
        cpu.p = nz16(cpu.p, cpu.y);
    } while (cpu.y != 0);
    op_lda16(LAYER0 + LAYER_SCROLL_X);
    wr16(BAND_POS + 9 * 4 + 1, cpu.a);
    cpu.x = 0;
    cpu.p = nz16(cpu.p, 0);
    cpu.y = 0;
    cpu.p = nz16(cpu.p, 0);
    op_sep(0x20);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x7E);
    cpu.p = nz8(cpu.p, 0x7E);
    cpu.db = 0x7E;
    op_rep(0x20);
    op_lda16_imm(0x000B);
    wr16(SCRATCH_0C, cpu.a);
    do {
        x = cpu.x;
        y = cpu.y;
        op_lda16(BAND_POS + 1 + x);
        wr16(BAND_HDMA + 1 + y, cpu.a);
        cpu.x = (uint16_t)(x + 4);
        cpu.p = nz16(cpu.p, cpu.x);
        cpu.y = (uint16_t)(y + 3);
        cpu.p = nz16(cpu.p, cpu.y);
        n = (uint16_t)(rd16(SCRATCH_0C) - 1);
        cpu.a = n;
        cpu.p = nz16(cpu.p, n);
        wr16(SCRATCH_0C, n);
    } while (n != 0);
    op_lda16(BAND_POS + 1);
    wr16(0x7E0000u | BG2_X, cpu.a);     /* DB $7E */
    cpu.db = db;
    cpu.p = nz8(cpu.p, db);
}
