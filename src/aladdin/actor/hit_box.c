/*
 * hit_box.c: whether an actor's box overlaps the box actors are tested
 * against.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "actor.h"
#include "wram.h"

/* The box tested against: centre x and y, half-width and half-height
   (words). Actors' half-sizes: two words an entry by ACTOR_06, in the
   bank DB names. */
#define TEST_BOX_X 0x001484u
#define TEST_BOX_Y 0x001486u
#define TEST_BOX_HALF_W 0x001488u
#define TEST_BOX_HALF_H 0x00148Au
#define HIT_BOXES 0xC29Au

#define ACTOR_X_PIXEL (ACTOR_X + 1)
#define ACTOR_Y_PIXEL (ACTOR_Y + 1)

static uint32_t in_db(uint32_t addr)
{
    return (uint32_t)cpu.db << 16 | addr;
}

static uint32_t dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

/*
 * @routine 83:8312 ActorHitsTestBox
 * WHAT IT DOES: Tells whether the actor, when it can be hit (bit 6 of its
 * byte at $09), overlaps the test box.
 * HOW: The two half-widths and the two half-heights are added (in scratch
 * words, with their doubles); the boxes overlap on an axis when the
 * difference of centres plus the sum is below the double, unsigned. X is
 * tested first.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM, the hit boxes' bank. Exit: C clear when they overlap. Cannot
 * be hit: N, V and Z as BIT of its byte at $09 sets them, A unchanged.
 * Tested: A = the last sum (16 bits), N, Z and C from its compare, V from
 * its addition, Y = the hit box's offset. M and X 8-bit. The other flags
 * unchanged.
 */
void abi_ActorHitsTestBox(void)
{
    uint8_t m = rd8(dp(ACTOR_09));
    uint16_t y;

    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_V | P_Z)) | (m & (P_N | P_V)) | (((uint8_t)cpu.a & m) ? 0 : P_Z));
    if (!(m & 0x40)) {
        op_sep(P_M | P_X | P_C);
        return;
    }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(dp(ACTOR_06)));
    op_rep(P_M | P_X);
    op_and16(0x00FF);
    op_asl16();
    op_asl16();
    y = cpu.a;
    cpu.y = y;
    cpu.a = rd16(in_db(HIT_BOXES + y));
    op_add16(rd16(TEST_BOX_HALF_W));
    wr16(SCRATCH, cpu.a);
    op_asl16();
    wr16(SCRATCH + 2, cpu.a);
    cpu.a = rd16(in_db(HIT_BOXES + 2 + y));
    op_add16(rd16(TEST_BOX_HALF_H));
    wr16(SCRATCH + 4, cpu.a);
    op_asl16();
    wr16(SCRATCH + 6, cpu.a);
    cpu.a = rd16(dp(ACTOR_X_PIXEL));
    op_sub16(rd16(TEST_BOX_X));
    op_add16(rd16(SCRATCH));
    op_cmp16(rd16(SCRATCH + 2));
    if (!(cpu.p & P_C)) {
        cpu.a = rd16(dp(ACTOR_Y_PIXEL));
        op_sub16(rd16(TEST_BOX_Y));
        op_add16(rd16(SCRATCH + 4));
        op_cmp16(rd16(SCRATCH + 6));
        op_sep(P_M | P_X);
        return;
    }
    op_sep(P_M | P_X | P_C);
}

/*
 * @routine 83:8300 ActorHitsTestBoxLong
 * WHAT IT DOES: ActorHitsTestBox for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorHitsTestBox; called by JSL. Exit: as ActorHitsTestBox.
 */
void abi_ActorHitsTestBoxLong(void)
{
    abi_ActorHitsTestBox();
}
