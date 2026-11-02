/*
 * small_actor_helpers.c: small actor routines: kind $89, which sets
 * Aladdin's sprite priority by where he is; a waiting actor that sends out
 * a kind $8E child every 8 frames; and a child of kind $3D started at a
 * spawn point and kept in its parent's list. Drafted by cdraft.py; named
 * and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorCreateChild(void);
void abi_ActorCreateChild8(void);
void abi_ActorPictureSend(void);
void abi_AnimStep(void);

#define PLAYER_09 0x08E4u  /* the player's byte at $09 (bits 4-5: his sprite priority) */
#define PRIORITY_BY_SIDE 0xD7C8u  /* his priority bits: left of x $0934, then right of it (in the bank DB names) */
#define V_032D 0x032Du  /* the high byte of the word at $032C */
#define CHILD_COUNT 0x44u  /* children started so far; the list of their records is at $40 */

/*
 * @routine 84:8F9C Kind89PlayerPriority
 * @actor
 * WHAT IT DOES: Actor kind $89: each frame sets Aladdin's sprite priority
 * (bits 4-5 of his $09) from PRIORITY_BY_SIDE by whether he is left of x
 * $0934 or not, for good.
 * HOW: Each frame (BRK first): X = 0, or 1 when his x pixels are $0934 or
 * more; PLAYER_09 and $CF, OR PRIORITY_BY_SIDE at X.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $89 in state $0C (through the stub at
 * $83:9326). Exit: never returns.
 */
void abi_Kind89PlayerPriority(void)
{
    uint16_t px;
    uint8_t bits;

    for (;;) {
        actor_yield(0x848F9Cu);
        /* his priority bits by the side of x $0934 he is on */
        x8_load(0x00);
        px = rd16(dba(PLAYER + ACTOR_X + 1));
        cpu.a = px;
        cmp16_of(px, 0x0934);
        if (px >= 0x0934)
            x8_load(0x01);
        bits = (uint8_t)((rd8(dba(PLAYER_09)) & 0xCF) | rd8(dba(PRIORITY_BY_SIDE) + cpu.x));
        a8_load(bits);
        wr8(dba(PLAYER_09), bits);
    }
}


/*
 * @routine 84:9B4D SenderWait8E
 * WHAT IT DOES: What a sending actor runs each frame it waits: its
 * animation on and its picture sent; when the low three bits of V_032D are
 * 0, a child of kind $8E ($07 = 1) at its spawn point $80.
 * HOW: AnimStep, ActorPictureSend; V_032D and 7 zero: $21 = $80, $07 = 1,
 * ActorCreateChild8 with kind $8E.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSL from the
 * actor loop's stub at $83:9348. Exit: as ActorCreateChild8 or
 * ActorPictureSend.
 */
void abi_SenderWait8E(void)
{
    uint8_t phase;

    abi_AnimStep();
    abi_ActorPictureSend();
    phase = rd8(dba(V_032D)) & 0x07;
    a8_load(phase);
    if (phase)
        return;
    a8_load(0x80);
    wr8(dpa(ACTOR_SPAWN_AT), 0x80);
    a8_load(0x01);
    wr8(dpa(ACTOR_07), 0x01);
    a8_load(0x8E);
    abi_ActorCreateChild8();
}


/*
 * @routine 83:C4EC ChildStartListed
 * WHAT IT DOES: Starts a child of kind $3D at spawn point X with $07 = Y,
 * and keeps its record in the actor's list of children ($40 + 2 * $44),
 * counting $44 up.
 * HOW: $21 = X, $07 = Y; ActorCreateChild $3D; the record (X) to Y; at $40
 * + $44 * 2 (direct page, X-indexed) the record (16-bit); M and X 8-bit;
 * $44 up.
 * Entry: M 8-bit, X 8-bit; X = the spawn point; Y = the child's $07; D =
 * the actor's record; DB = $83; called by JSR (83:C32E, 83:C358, 83:C35F).
 * Exit: M and X 8-bit; X = $44 * 2 before the count (low byte); Y = the
 * record's low byte.
 */
void abi_ChildStartListed(void)
{
    uint16_t at;
    uint8_t n;

    wr8(dpa(ACTOR_SPAWN_AT), (uint8_t)cpu.x);
    wr8(dpa(ACTOR_07), (uint8_t)cpu.y);
    a8_load(0x3D);
    abi_ActorCreateChild();
    cpu.y = cpu.x;                        /* the child */
    op_rep(P_M);
    at = (uint16_t)((dp16(CHILD_COUNT) & 0x00FF) << 1);
    cpu.a = cpu.x = at;
    nz16_of(at);
    wr16(dpa((uint16_t)(ACTOR_40 + at)), cpu.y);   /* kept in the list */
    op_sep(0x30);
    n = (uint8_t)(rd8(dpa(CHILD_COUNT)) + 1);
    wr8(dpa(CHILD_COUNT), n);
    nz8_of(n);
}
