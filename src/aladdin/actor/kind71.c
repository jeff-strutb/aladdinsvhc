/*
 * kind71.c: actor kind $71, by its $07: one that paces left and right,
 * one that steps back and then moves right for good, or one that stays,
 * showing animations 1 and 0 in turn.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_ActorMoveX(void);

#define K71_LEG 0x42u               /* frames left of a pace leg */
#define K71_PACE 0x08u              /* $07 bit: it paces */
#define K71_RUNS 0x10u              /* $07 bit: it steps back and moves right */

/* JSL at site to C that may push: its return on the stack while it runs. */
static void jsl_at(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

static void anim(uint8_t a, uint32_t site)
{
    a8_load(a);
    jsl_at(site, abi_AnimStart);
}

/* Facing and flip both v (STA, STA). */
static void face(uint8_t v)
{
    wr8(dpa(ACTOR_FACING), v);
    wr8(dpa(ACTOR_FLIP), v);
}

/*
 * @routine 83:F370 Kind71Start
 * @actor
 * WHAT IT DOES: Actor kind $71, by its $07: bit 3, it paces, $90 frames
 * one way and $90 the other at speed $80, first to the left; else bit 4,
 * it steps back $171 pixels and moves right at speed $0100 for good; else
 * it stays, facing by bit 0, in animation 1 for $39 frames and 0 for $38,
 * again and again.
 * HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set. Bit 3: facing and flip
 * 1, speed $80, AnimStart 0; each leg K71_LEG $90 frames (BRK first) of
 * ActorMoveX, then facing and flip flipped. Bit 4: facing and flip 0; x
 * pixels less $0170 with the borrow (REP #$21 clears C); AnimStart 0;
 * speed $0100; each frame (BRK first) ActorMoveX. Else: facing and flip =
 * bit 0; AnimStart 1, COP $39, AnimStart 0, COP $38, again.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $71 in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind71Start(void)
{
    uint8_t way, v;
    uint16_t x;

    a8_load(0x02);
    tsb_at(dpa(ACTOR_08), 0x02);
    a8_load(0x90);
    tsb_at(dpa(ACTOR_09), 0x90);
    way = rd8(dpa(ACTOR_07));
    a8_load(way);
    flag_set(P_Z, !(way & K71_PACE));        /* BIT #$08 */
    if (way & K71_PACE) {
        a8_load(0x01);
        face(0x01);
        a8_load(0x80);
        wr8(dpa(ACTOR_SPEED_X), 0x80);
        wr8(dpa(ACTOR_SPEED_X + 1), 0);
        wr8(dpa(ACTOR_SPEED_X + 2), 0);
        anim(0x00, 0x83F3D5u);
        for (;;) {
            a8_load(0x90);
            wr8(dpa(K71_LEG), 0x90);
            do {
                actor_yield(0x83F3DDu);
                jsl_at(0x83F3DFu, abi_ActorMoveX);
            } while (dec8_at(dpa(K71_LEG)));
            v = (uint8_t)(rd8(dpa(ACTOR_FACING)) ^ 0x01);
            a8_load(v);
            face(v);
        }
    }
    flag_set(P_Z, !(way & K71_RUNS));        /* BIT #$10 */
    if (way & K71_RUNS) {
        face(0x00);
        op_rep(0x21);                        /* C clear: the SBC takes one more */
        x = dp16(ACTOR_X + 1);
        cpu.a = op_sbc16(x, 0x0170, &cpu.p);
        dp16_set(ACTOR_X + 1, cpu.a);
        op_sep(0x20);
        anim(0x00, 0x83F393u);
        wr8(dpa(ACTOR_SPEED_X), 0);
        a8_load(0x01);
        wr8(dpa(ACTOR_SPEED_X + 1), 0x01);
        wr8(dpa(ACTOR_SPEED_X + 2), 0);
        for (;;) {
            actor_yield(0x83F39Fu);
            jsl_at(0x83F3A1u, abi_ActorMoveX);
        }
    }
    v = (uint8_t)(rd8(dpa(ACTOR_07)) & 0x01);
    a8_load(v);
    face(v);
    for (;;) {
        anim(0x01, 0x83F3B1u);
        a8_load(0x39);
        actor_wait(0x83F3B7u);
        anim(0x00, 0x83F3BBu);
        a8_load(0x38);
        actor_wait(0x83F3C1u);
    }
}
