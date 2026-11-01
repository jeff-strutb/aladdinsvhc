/*
 * bg_follow.c: the BG positions taken from the scroll layers each frame.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"
#include "wram_video.h"

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

#define PLAY_X (LAYER0 + LAYER_SCROLL_X)
#define PLAY_Y (LAYER0 + LAYER_SCROLL_Y)
#define BACK_X (LAYER1 + LAYER_SCROLL_X)
#define BACK_Y (LAYER1 + LAYER_SCROLL_Y)

static void copy16(uint16_t from, uint16_t to)
{
    cpu.a = rd16(abs_db(from));
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(abs_db(to), cpu.a);
}

/*
 * @routine 81:8568 BgFollowLayers
 * WHAT IT DOES: Takes this frame's BG1 and BG2 positions from the scroll
 * layers, as the level sets them to follow.
 * HOW: BG1 takes the play field's x and y unless held. BG2 by its follow
 * mode: $80, the play field's x; $40, the backdrop's y; 0, the backdrop's
 * x and y; anything else, neither.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSR. Exit: M and X 8-bit; X = the BG2 mode; A = the last
 * position copied, else as the play field's y or the entry's; N, Z from
 * the last load or compare; C from the mode compares; Y = the mode when
 * neither $80 nor $40, else unchanged; V unchanged.
 */
void abi_BgFollowLayers(void)
{
    uint8_t mode;

    op_rep(P_M);
    cpu.x = rd8(abs_db(BG1_HELD));
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    if (cpu.x == 0) {
        copy16(PLAY_X, BG1_X);
        copy16(PLAY_Y, BG1_Y);
    }
    mode = rd8(abs_db(BG2_FOLLOWS));
    cpu.x = mode;
    cpu.p = cmp8_flags(cpu.p, mode, 0x80);
    if (mode == 0x80) {
        copy16(PLAY_X, BG2_X);
    } else {
        cpu.p = cmp8_flags(cpu.p, mode, 0x40);
        if (mode != 0x40) {
            cpu.y = mode;
            cpu.p = nz8(cpu.p, mode);
            if (mode != 0)
                goto done;
            copy16(BACK_X, BG2_X);
        }
        copy16(BACK_Y, BG2_Y);
    }
done:
    op_sep(P_M | P_X);
}
