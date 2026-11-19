/*
 * backdrop_drift.c: the actor of kind $67 (its start at $83:E5F1) moves
 * the backdrop layer up and down the screen, by its speed a frame, in
 * turns. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

/* The backdrop's height against the play field's, added to the play
   field's scroll Y for the backdrop's ($81:CA6A): 32-bit, a fraction byte
   first. */
#define BACK_Y_OFFSET 0x001A2Bu
#define DRIFT_SPEED 0x42u          /* the actor's: fraction pixels a frame (word) */
#define DRIFT_FRAMES 0x40u         /* the actor's: frames left of the drift at full speed (word) */

void abi_BackdropDriftDown(void);
void abi_BackdropDriftUp(void);
void abi_ActorCount42Down(void);

/*
 * @routine 83:E5F1 Kind67Start
 * @actor
 * WHAT IT DOES: Actor kind $67: drifts the backdrop down the screen for
 * 128 frames at $80 fraction pixels a frame, slows to a stop by 1 a
 * frame, rests 48 frames; then the same upwards; for good.
 * HOW: $09 = $80. Then each turn, 16-bit: DRIFT_FRAMES = $80, DRIFT_SPEED
 * = $80; DRIFT_FRAMES frames (BRK first) of BackdropDriftDown; frames
 * (BRK first) of ActorCount42Down and BackdropDriftDown until
 * DRIFT_SPEED is 0; 8-bit, COP $30. The same with BackdropDriftUp (the
 * speed set first, then the frames); 8-bit, COP $30; again.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $67
 * in state $0C (the table at $83:8CAB). Exit: never returns.
 */
void abi_Kind67Start(void)
{
    lda8_v(0x80);
    sta8(ea_dp(ACTOR_09));
    for (;;) {
        op_rep(0x20);
        lda16_v(0x0080);
        sta16(ea_dp(DRIFT_FRAMES));
        lda16_v(0x0080);
        sta16(ea_dp(DRIFT_SPEED));
        do {
            actor_yield(0x83E601u);
            call_frame(0xE605, abi_BackdropDriftDown);
            MEM16(dec, ea_dp(DRIFT_FRAMES));
        } while (!IF_Z);
        do {
            actor_yield(0x83E60Au);
            call_frame(0xE60E, abi_ActorCount42Down);
            call_frame(0xE611, abi_BackdropDriftDown);
            lda16(ea_dp(DRIFT_SPEED));
        } while (!IF_Z);
        op_sep(0x20);
        lda8_v(0x30);
        actor_wait(0x83E61Au);
        op_rep(0x20);
        lda16_v(0x0080);
        sta16(ea_dp(DRIFT_SPEED));
        lda16_v(0x0080);
        sta16(ea_dp(DRIFT_FRAMES));
        do {
            actor_yield(0x83E628u);
            call_frame(0xE62C, abi_BackdropDriftUp);
            MEM16(dec, ea_dp(DRIFT_FRAMES));
        } while (!IF_Z);
        do {
            actor_yield(0x83E631u);
            call_frame(0xE635, abi_ActorCount42Down);
            call_frame(0xE638, abi_BackdropDriftUp);
            lda16(ea_dp(DRIFT_SPEED));
        } while (!IF_Z);
        op_sep(0x20);
        lda8_v(0x30);
        actor_wait(0x83E641u);
    }
}

/*
 * @routine 83:E653 BackdropDriftDown
 * WHAT IT DOES: Moves the backdrop down the screen by the actor's speed:
 * its offset against the play field's scroll falls.
 * HOW: A 32-bit subtraction of the speed from the offset, a word at a
 * time.
 * Entry: M 16-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: A = the offset's top word; N, V, Z and C from its
 * subtraction. M 16-bit; X and Y unchanged.
 */
void abi_BackdropDriftDown(void)
{
    uint16_t low;

    flag_set(P_C, 1);
    low = rd16(dba(BACK_Y_OFFSET));
    low = op_sbc16(low, dp16(DRIFT_SPEED), &cpu.p);
    wr16(dba(BACK_Y_OFFSET), low);
    cpu.a = op_sbc16(rd16(dba(BACK_Y_OFFSET + 2)), 0x0000, &cpu.p);
    wr16(dba(BACK_Y_OFFSET + 2), cpu.a);
}

/*
 * @routine 83:E666 BackdropDriftUp
 * WHAT IT DOES: Moves the backdrop up the screen by the actor's speed:
 * its offset against the play field's scroll grows.
 * HOW: A 32-bit addition of the speed to the offset, a word at a time.
 * Entry: M 16-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: A = the offset's top word; N, V, Z and C from its
 * addition. M 16-bit; X and Y unchanged.
 */
void abi_BackdropDriftUp(void)
{
    uint16_t low;

    flag_set(P_C, 0);
    low = rd16(dba(BACK_Y_OFFSET));
    low = op_adc16(low, dp16(DRIFT_SPEED), &cpu.p);
    wr16(dba(BACK_Y_OFFSET), low);
    cpu.a = op_adc16(rd16(dba(BACK_Y_OFFSET + 2)), 0x0000, &cpu.p);
    wr16(dba(BACK_Y_OFFSET + 2), cpu.a);
}
