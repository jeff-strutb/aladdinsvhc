/*
 * kind7b.c: actor kind $7B, a thing that waits for the player to land on
 * it, then sends him on. Written from the bytes; named and described by
 * hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram_player.h"
#include "wram_state.h"

void abi_ActorHitsTestBoxLong(void);
void abi_AnimStart(void);
void abi_SoundQueueAdd(void);
void abi_ActorEndPlaced(void);

#define K7B_PLAYER_1F (PLAYER + 0x1Fu)        /* bit 7 set: the player is not taken */
#define K7B_PLAYER_RESUME (PLAYER + ACTOR_03) /* the player's resume address (word) */
#define K7B_Y_PIXELS (PLAYER + ACTOR_Y + 1)   /* his y pixels (word) */
#define K7B_SENT_TO 0xDA14                     /* where it sends the player's code */
#define K7B_SOUND 0x41

/*
 * @routine 83:C572 Kind7BStart
 * @actor
 * WHAT IT DOES: Actor kind $7B, a thing that waits for the player to land
 * on it from above: then it changes pose (animation 1), sends the player
 * on in his code at $DA14, waits 32 frames, sounds $41, waits 16 more and
 * goes back to waiting; without health left the touch sets its own state
 * to $8C instead.
 * HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set; AnimStart 0. Each frame
 * (BRK first): on when V_141C is nonzero, the player's $1F has bit 7
 * clear and ActorHitsTestBoxLong says C clear. Health 0: ACTOR_STATE =
 * $8C, AnimStart 0, again. Else, his y pixels + 16 less its y pixels
 * below 0 (he is above it): AnimStart 1, his resume address ($03-$04) =
 * $DA14, COP $20, SoundQueueAdd $41, COP $10, again; otherwise again. No
 * script runs past the first BRK: written from its bytes. Its state
 * $8C sends it, in the loop, to Kind7BSecond ($83:C5D1), which follows.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $7B
 * in state $0C (the table at $83:8CAB). Exit: never returns.
 */
void abi_Kind7BStart(void)
{
    lda8(ea_dp(ACTOR_08));
    ora8_v(0x02);
    sta8(ea_dp(ACTOR_08));
    lda8(ea_dp(ACTOR_09));
    ora8_v(0x90);
    sta8(ea_dp(ACTOR_09));
    for (;;) {
        lda8_v(0x00);
        abi_AnimStart();
        for (;;) {
            actor_yield(0x83C584u);
            lda8(ea_abs(V_141C));
            if (IF_Z)
                continue;
            lda8(ea_abs(K7B_PLAYER_1F));
            if (IF_N)
                continue;
            abi_ActorHitsTestBoxLong();
            if (IF_C)
                continue;
            lda8(ea_abs(HEALTH));
            if (IF_Z) {
                lda8_v(0x8C);
                sta8(ea_dp(ACTOR_STATE));
                break;
            }
            op_rep(P_M);
            lda16(ea_abs(K7B_Y_PIXELS));
            clc();
            adc16_v(0x0010);
            sec();
            sbc16_v(rd16(ea_dp(ACTOR_Y + 1)));
            op_sep(P_M);
            if (IF_C)
                continue;
            lda8_v(0x01);
            abi_AnimStart();
            lda8_v(K7B_SENT_TO & 0xFF);
            sta8(ea_abs(K7B_PLAYER_RESUME));
            lda8_v(K7B_SENT_TO >> 8);
            sta8(ea_abs(K7B_PLAYER_RESUME + 1));
            lda8_v(0x20);
            actor_wait(0x83C5C3u);
            lda8_v(K7B_SOUND);
            abi_SoundQueueAdd();
            lda8_v(0x10);
            actor_wait(0x83C5CDu);
        }
    }
}

/*
 * @routine 83:C5D1 Kind7BSecond
 * @actor
 * WHAT IT DOES: Where actors of kind $7B go on from the actor loop in a
 * state with bit 7 set (the table at $83:8DFD): ActorEndPlaced, the
 * actor ended.
 * HOW: JML to ActorEndPlaced ($83:96CA).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind7BSecond(void)
{
    abi_ActorEndPlaced();
}
