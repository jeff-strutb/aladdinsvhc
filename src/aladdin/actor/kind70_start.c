/*
 * kind70_start.c: the start of actor kind $70 (and of kind $6E in state
 * $4C): a scripted actor that either rises, flies off and carries the
 * escort actor once it meets it, or waits to be hit and then cues the
 * scene on. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorEndOutOfRange(void);
void abi_ActorFly(void);
void abi_ActorFlyAgainst(void);
void abi_ActorMotionPreset(void);
void abi_AnimStart(void);

#define KIND70_CARRYING 0x40u  /* frames of the first flight; then nonzero while it carries the escort */
#define SCENE_CUE 0x146Cu  /* counted up by scripted actors as the scene goes on */
#define SCENE_CUE_C2 0x00C2u  /* counted up when the hit one has waited; another actor waits for it */

/* The flyer's carry of the escort, tested each frame: within 8 of its
   hold point across and 16 down, it takes hold. 0: not yet. */
static int escort_in_reach(void)
{
    uint16_t d;

    op_rep(P_M);
    flag_set(P_C, 0);
    d = op_adc16(dp16(ACTOR_X + 1), 0xFFE8, &cpu.p);
    flag_set(P_C, 1);
    d = op_sbc16(d, rd16(dba(ACTOR_ESCORT + ACTOR_X + 1)), &cpu.p);
    cpu.a = d;
    cmp16_of(d, 0x0008);
    if (d >= 0x0008)
        return 0;
    flag_set(P_C, 0);
    d = op_adc16(dp16(ACTOR_Y + 1), 0x0010, &cpu.p);
    flag_set(P_C, 1);
    d = op_sbc16(d, rd16(dba(ACTOR_ESCORT + ACTOR_Y + 1)), &cpu.p);
    cpu.a = d;
    cmp16_of(d, 0x0010);
    return d < 0x0010;
}

/*
 * @routine 83:F296 Kind70Start
 * @actor
 * WHAT IT DOES: A scripted actor's start. $07 zero: it rises (animations
 * 0, 1, 2 with waits), flies off for $28 frames, then flies on under
 * gravity until out of range; when the escort actor, not rising, comes
 * within 8 pixels across and 16 down of it, it counts SCENE_CUE up and
 * carries the escort 30 pixels to its left from then on. $07 nonzero: it
 * faces left and waits until it can be hit, counts SCENE_CUE up, waits $80
 * frames, counts SCENE_CUE_C2 up and plays animations 4 and 5, then stays.
 * HOW: Bits $90 set in $09. First way: animation 0, COP $20; animation 1,
 * COP $17; position +16 x, -13 y; animation 2, ActorMotionPreset $4F; $28
 * frames of ActorFly; gravity $18; then each frame ActorFlyAgainst and
 * ActorEndOutOfRange (box 0); carrying: the escort's x = its x - 30, y =
 * its y; not: the escort's speed y not negative, (x - 24) - its x below 8
 * and (y + 16) - its y below 16, then SCENE_CUE and $40 up. Second way:
 * facing and flip 1, animation 3; BRK until bit 6 of $09; SCENE_CUE up,
 * COP $80; SCENE_CUE_C2 up, animation 4, COP $20, animation 5, then BRK
 * for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $70 in state $0C and kind $6E in state
 * $4C. Exit: never returns: its first way ends in ActorEndOutOfRange.
 */
void abi_Kind70Start(void)
{
    uint8_t flags, carrying;
    uint16_t y;

    a8_load(0x90);
    tsb_at(dpa(ACTOR_09), 0x90);
    a8_load(rd8(dpa(ACTOR_07)));
    if ((uint8_t)cpu.a) {
        /* the hit one: when it is shown, the scene's cues */
        a8_load(0x01);
        wr8(dpa(ACTOR_FACING), 0x01);
        wr8(dpa(ACTOR_FLIP), 0x01);
        a8_load(0x03);
        abi_AnimStart();
        do {
            actor_yield(0x83F345u);
            flags = rd8(dpa(ACTOR_09));
            a8_load(flags);
            flag_set(P_Z, !(flags & 0x40));
        } while (!(flags & 0x40));
        inc8_at(dba(SCENE_CUE));
        a8_load(0x80);
        actor_wait(0x83F352u);
        inc8_at(dba(SCENE_CUE_C2));
        a8_load(0x04);
        abi_AnimStart();
        a8_load(0x20);
        actor_wait(0x83F35Fu);
        a8_load(0x05);
        abi_AnimStart();
        for (;;)
            actor_yield(0x83F367u);
    }
    /* the flyer: two poses, a hop up and right, $28 frames of flight */
    wr8(dpa(KIND70_CARRYING), 0);
    a8_load(0x00);
    abi_AnimStart();
    a8_load(0x20);
    actor_wait(0x83F2ABu);
    a8_load(0x01);
    abi_AnimStart();
    a8_load(0x17);
    actor_wait(0x83F2B5u);
    op_rep(0x21);
    cpu.a = op_adc16(dp16(ACTOR_X + 1), 0x0010, &cpu.p);
    dp16_set(ACTOR_X + 1, cpu.a);
    flag_set(P_C, 1);
    cpu.a = op_sbc16(dp16(ACTOR_Y + 1), 0x000D, &cpu.p);
    dp16_set(ACTOR_Y + 1, cpu.a);
    op_sep(P_M);
    a8_load(0x02);
    abi_AnimStart();
    y8_load(0x4F);
    abi_ActorMotionPreset();
    a8_load(0x28);
    wr8(dpa(KIND70_CARRYING), 0x28);
    do {
        actor_yield(0x83F2DAu);
        abi_ActorFly();
    } while (dec8_at(dpa(KIND70_CARRYING)));
    /* then up, till out of range; it takes the escort (falling) when in
       reach, and carries it */
    a8_load(0x18);
    wr8(dpa(ACTOR_ACCEL_Y), 0x18);
    for (;;) {
        actor_yield(0x83F2E8u);
        abi_ActorFlyAgainst();
        x8_load(0x00);
        call_frame(0xF2F2, abi_ActorEndOutOfRange);
        carrying = rd8(dpa(KIND70_CARRYING));
        a8_load(carrying);
        if (carrying) {
            op_rep(P_M);
            flag_set(P_C, 0);
            cpu.a = op_adc16(dp16(ACTOR_X + 1), 0xFFE2, &cpu.p);
            wr16(dba(ACTOR_ESCORT + ACTOR_X + 1), cpu.a);
            y = dp16(ACTOR_Y + 1);
            wr16(dba(ACTOR_ESCORT + ACTOR_Y + 1), y);
            a16_load(y);
            op_sep(P_M);
            continue;
        }
        a8_load(rd8(dba(ACTOR_ESCORT + ACTOR_SPEED_Y + 2)));
        if ((uint8_t)cpu.a & 0x80)
            continue;
        if (!escort_in_reach()) {
            op_sep(P_M);
            continue;
        }
        op_sep(P_M);
        inc8_at(dba(SCENE_CUE));
        inc8_at(dpa(KIND70_CARRYING));
    }
}
