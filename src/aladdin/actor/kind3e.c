/*
 * kind3e.c: actor kind $3E's start, a maker: while it cannot be hit it
 * makes an actor of look $31 next to it every 128 frames, as long as fewer
 * than three actors hold its tile slot. Its wait is Kind3EWait
 * (kind_waits_rest.c).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorSpawnLookWait(void);

#define K3E_LEFT 0x40u          /* frames left of its pause after a spawn */
#define K3E_PAUSE 0x41u         /* its pause's length (128 frames) */
#define K3E_LOOK 0x4Du          /* ActorSpawnLookWait's look: what it makes */
#define K3E_MADE_LOOK 0x31u     /* the look of what it makes */
#define K3E_SLOT_FULL 0x03u     /* holders of its tile slot at which it makes none */

/*
 * @routine 83:B154 Kind3EStart
 * @actor
 * WHAT IT DOES: Actor kind $3E: each frame while it cannot be hit ($09 bit
 * 6 clear) and fewer than three actors hold its tile slot, it makes an
 * actor of look $31 next to it, then pauses 128 frames.
 * HOW: $08 bit 3 cleared, $09 bits 7 and 4 set, facing and flip 0.
 * K3E_PAUSE = $80; each frame (BRK first) K3E_LEFT = K3E_PAUSE; $09 bit 6
 * set: K3E_PAUSE = $80 again; else the slot's holders ($15E8 by $39) 3 or
 * more: the next frame; else $21 = 0, K3E_LOOK = $31,
 * ActorSpawnLookWait; K3E_LEFT frames (BRK first); the next frame.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $3E in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind3EStart(void)
{
    lda8(ea_dp(ACTOR_08));
    and8_v(0xF7);
    sta8(ea_dp(ACTOR_08));
    lda8(ea_dp(ACTOR_09));
    ora8_v(0x90);
    sta8(ea_dp(ACTOR_09));
    stz8(ea_dp(ACTOR_FACING));
    stz8(ea_dp(ACTOR_FLIP));
hit:
    lda8_v(0x80);
    sta8(ea_dp(K3E_PAUSE));
frame:
    actor_yield(0x83B168u);
    lda8(ea_dp(K3E_PAUSE));
    sta8(ea_dp(K3E_LEFT));
    lda8(ea_dp(ACTOR_09));
    bit8_i(0x40);
    if (!IF_Z)
        goto hit;
    ldx8(ea_dp(ACTOR_TILE_SLOT));
    lda8(ea_absx(ACTOR_TILE_SLOT_USERS));
    cmp8_v(K3E_SLOT_FULL);
    if (IF_C)
        goto frame;
    lda8_v(0x00);
    sta8(ea_dp(ACTOR_SPAWN_AT));
    lda8_v(K3E_MADE_LOOK);
    sta8(ea_dp(K3E_LOOK));
    call_frame(0xB187, abi_ActorSpawnLookWait);
    do {
        actor_yield(0x83B188u);
        MEM8(dec, ea_dp(K3E_LEFT));
    } while (!IF_Z);
    goto frame;
}
