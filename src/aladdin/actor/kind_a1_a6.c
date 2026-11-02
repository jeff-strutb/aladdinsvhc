/*
 * kind_a1_a6.c: actor kinds $A1 in state $4C (hit: it flies off) and $A6
 * (it stays until far above or below the point actors are kept in range
 * of). Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorEndPlaced(void);
void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_AnimStart(void);

#define RANGE_Y 0x1A46u  /* the y of the point actors are kept in range of (word) */

/*
 * @routine 84:A9FC KindA1Hit
 * @actor
 * WHAT IT DOES: Actor kind $A1 in state $4C (hit): it flies off (motion
 * $1F), after 8 frames when its $41 is set, until it can no longer be hit,
 * and ends.
 * HOW: Animation 0, $08 bit 4 set; $41 nonzero: COP 8; ActorMotionPreset
 * $1F; each frame ActorFly while bit 6 of $09 is set; then ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $A1 in state $4C (through the stub at
 * $83:9412). Exit: never returns; ends in ActorEndPlaced.
 */
void abi_KindA1Hit(void)
{
    uint8_t wait, flags;

    a8_load(0x00);
    abi_AnimStart();
    wr8(dpa(ACTOR_08), (uint8_t)(rd8(dpa(ACTOR_08)) | 0x10));
    wait = rd8(dpa(ACTOR_41));
    a8_load(wait);
    if (wait) {
        a8_load(0x08);
        actor_wait(0x84AA0Eu);
    }
    /* it falls away on motion $1F until it is off the screen */
    y8_load(0x1F);
    abi_ActorMotionPreset();
    do {
        actor_yield(0x84AA16u);
        abi_ActorFly();
        flags = rd8(dpa(ACTOR_09));       /* BIT: N, V from it, Z from A AND it */
        flag_set(P_N, flags & 0x80);
        flag_set(P_V, flags & 0x40);
        flag_set(P_Z, !((uint8_t)cpu.a & flags));
    } while (flags & 0x40);
    abi_ActorEndPlaced();
}


/*
 * @routine 84:AC3F KindA6Stay
 * @actor
 * WHAT IT DOES: Actor kind $A6: shown (animation 0, flipped by its $07)
 * where it was placed until it is 256 or more pixels above or below the
 * point actors are kept in range of, then ends.
 * HOW: $08 = 3, AnimStart 0, flip = $07; each frame (BRK first) RANGE_Y
 * less its y plus $0100, unsigned below $0200: again; else ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $A6 in state $0C (through the stub at
 * $83:943E). Exit: never returns; ends in ActorEndPlaced.
 */
void abi_KindA6Stay(void)
{
    uint8_t flip;
    uint16_t d;

    a8_load(0x03);
    wr8(dpa(ACTOR_08), 0x03);
    a8_load(0x00);
    abi_AnimStart();
    flip = rd8(dpa(ACTOR_07));
    a8_load(flip);
    wr8(dpa(ACTOR_FLIP), flip);
    op_rep(P_M);
    /* until it is 256 pixels or more above or below the range point */
    do {
        actor_yield(0x84AC4Fu);
        flag_set(P_C, 1);
        d = op_sbc16(rd16(dba(RANGE_Y)), dp16(ACTOR_Y + 1), &cpu.p);
        flag_set(P_C, 0);
        d = op_adc16(d, 0x0100, &cpu.p);
        cpu.a = d;
        cmp16_of(d, 0x0200);
    } while (d < 0x0200);
    op_sep(P_M);
    abi_ActorEndPlaced();
}
