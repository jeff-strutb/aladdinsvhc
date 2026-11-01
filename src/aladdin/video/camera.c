/*
 * camera.c: where the play field may scroll: empty blocks stop it.
 *
 * The level's block map marks the blocks the camera may not show with a
 * negative entry. After the scroll moves, the screen's leading edge is
 * checked against the map; over an empty block the scroll goes back to
 * the block's edge.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram_video.h"

/* The block map, from its first block inside the border */
#define BLOCK_MAP_INNER (0x7FF000u + 66)

#define SCRATCH_10 0x000010u

/* The play field's scroll, as the bytes around its whole pixels */
#define SCROLL_X (LAYER0 + LAYER_SCROLL_X)
#define SCROLL_Y (LAYER0 + LAYER_SCROLL_Y)

/*
 * @routine 81:CB24 BlockAt
 * WHAT IT DOES: Gives the block map entry of the 256-pixel block at a
 * position in the level (negative: an empty block).
 * HOW: The block's column from the X position's high byte, its row from
 * the Y position's, into the map inside its border.
 * Entry: M 16-bit, X 16-bit; A = X position; Y = Y position. Exit: A =
 * the entry; N and Z from it; X = its offset; C = Y bit 1; $0010 = the
 * column part; Y and the other flags unchanged.
 */
void abi_BlockAt(void)
{
    op_xba();
    op_asl16();
    op_and16(0x003E);
    wr16(SCRATCH_10, cpu.a);
    op_lda16_imm(cpu.y);
    op_lsr16();
    op_lsr16();
    op_and16(0x03C0);
    cpu.a |= rd16(SCRATCH_10);
    cpu.p = nz16(cpu.p, cpu.a);
    op_tax16();
    op_lda16(BLOCK_MAP_INNER + cpu.x);
}

/* The block at (x, y): BlockAt with A = x */
static int empty_at(uint16_t x)
{
    cpu.a = x;
    abi_BlockAt();
    return (cpu.p & P_N) != 0;
}

/* TYA, ADC #$FF, TAY with the carry as it stands */
static void y_plus_ff(void)
{
    op_lda16_imm(cpu.y);
    cpu.a = op_adc16(cpu.a, 0x00FF, &cpu.p);
    op_tay16();
}

/*
 * @routine 81:CA84 CameraStopAcross
 * WHAT IT DOES: Stops the play field scrolling into an empty block across:
 * after a move left or right, when the screen's leading edge is over an
 * empty block, the scroll goes back to the block's edge.
 * HOW: Moving left, the blocks at the screen's left edge at its top and
 * bottom rows; moving right (or not at all), at its right edge (255
 * pixels on). Over an empty block the scroll X becomes the next block's
 * start (left) or its block's start (right), and the stopped flag is set.
 * Entry: M 16-bit, X 16-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: A, X, Y and the flags as the last check or stop leaves them.
 */
void abi_CameraStopAcross(void)
{
    uint16_t v;

    wr16(CAMERA_STOPPED_X, 0);
    op_lda16(SCROLL_X);
    op_cmp16(rd16(SCROLL_X_BEFORE));
    if (!(cpu.p & P_C)) {
        cpu.y = rd16(SCROLL_Y_BEFORE);
        cpu.p = nz16(cpu.p, cpu.y);
        if (!empty_at(cpu.a)) {
            y_plus_ff();
            op_lda16(SCROLL_X);
            if (!empty_at(cpu.a))
                return;
        }
        op_lda16(SCROLL_X + 1);
        cpu.a++;
        cpu.p = nz16(cpu.p, cpu.a);
        wr16(SCROLL_X + 1, cpu.a);
        wr16(SCROLL_X - 1, 0);
    } else {
        op_lda16(SCROLL_X);
        op_add16(0x00FF);
        cpu.y = rd16(SCROLL_Y_BEFORE);
        cpu.p = nz16(cpu.p, cpu.y);
        if (!empty_at(cpu.a)) {
            op_lda16_imm(cpu.y);
            op_add16(0x00FF);
            op_tay16();
            op_lda16(SCROLL_X);
            op_add16(0x00FF);
            if (!empty_at(cpu.a))
                return;
        }
        wr16(SCROLL_X - 1, 0);
    }
    v = (uint16_t)(rd16_rmw(CAMERA_STOPPED_X) + 1);
    wr16_rmw(CAMERA_STOPPED_X, v);
    cpu.p = nz16(cpu.p, v);
}

/*
 * @routine 81:CADA CameraStopUpDown
 * WHAT IT DOES: Stops the play field scrolling into an empty block up or
 * down: after the move, when the screen's leading edge is over an empty
 * block, the scroll goes back to the block's edge.
 * HOW: Moving up, the blocks at the screen's top edge at its left and
 * right columns; moving down (or not at all), at its bottom edge (255
 * pixels down). Over an empty block the scroll Y becomes the next block's
 * start (up) or its block's start (down), and the stopped flag steps on.
 * Entry: M 16-bit, X 16-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: A, X, Y and the flags as the last check or stop leaves them.
 */
void abi_CameraStopUpDown(void)
{
    uint16_t v;

    cpu.y = rd16(SCROLL_Y);
    cpu.p = nz16(cpu.p, cpu.y);
    cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(cpu.y - rd16(SCROLL_Y_BEFORE))) & ~P_C) |
                      (cpu.y >= rd16(SCROLL_Y_BEFORE) ? P_C : 0));
    if (!(cpu.p & P_C)) {
        op_lda16(SCROLL_X);
        if (!empty_at(cpu.a)) {
            op_lda16(SCROLL_X);
            op_add16(0x00FF);
            if (!empty_at(cpu.a))
                return;
        }
        op_lda16_imm(cpu.y);
        op_xba();
        cpu.a++;
        cpu.p = nz16(cpu.p, cpu.a);
        wr16(SCROLL_Y + 1, cpu.a);
        wr16(SCROLL_Y - 1, 0);
    } else {
        op_lda16_imm(cpu.y);
        op_add16(0x00FF);
        op_tay16();
        op_lda16(SCROLL_X);
        if (!empty_at(cpu.a)) {
            op_lda16(SCROLL_X);
            op_add16(0x00FF);
            if (!empty_at(cpu.a))
                return;
        }
        wr16(SCROLL_Y - 1, 0);
    }
    v = (uint16_t)(rd16_rmw(CAMERA_STOPPED_Y) + 1);
    wr16_rmw(CAMERA_STOPPED_Y, v);
    cpu.p = nz16(cpu.p, v);
}
