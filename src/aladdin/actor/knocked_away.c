/*
 * knocked_away.c: an actor knocked away by Aladdin: it shows a pose,
 * sounds, takes a motion preset and flies off the way he faces until it
 * is out of range, where it ends. Kinds reach it from their second state
 * (kind $20's at 83:9C19). Drafted by cdraft.py; named, completed and
 * described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "wram_video.h"
#include "actor.h"

void abi_ActorEndOutOfRange(void);
void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_ActorPaletteFromRom(void);
void abi_ActorPictureSend(void);
void abi_AnimResume(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);

#define STEPS 0x40u                /* frames left of its first poses */
#define ANIM_AT 0x2Fu              /* the frame AnimResume takes up the animation at */
#define PLAYER_FLIP (PLAYER + ACTOR_FLIP) /* Aladdin's flip: the way he faces */
#define FLY_LEVEL_A 0x0Du          /* on these levels a falling actor drops behind the layers */
#define FLY_LEVEL_B 0x02u

/* From 83:9561: each frame (BRK) its animation on and it flies; on levels
   FLY_LEVEL_A and FLY_LEVEL_B, once it falls (no script runs this), its
   priority bits $30 are cleared; ActorEndOutOfRange with box 0 ends it
   once it is out of range. */
static void fly_off(void)
{
    for (;;) {
        actor_yield(0x839561u);
        abi_AnimStep();
        abi_ActorPictureSend();
        abi_ActorFly();
        lda8(ea_abs(LEVEL));
        cmp8_v(FLY_LEVEL_A);
        if (IF_Z || (cmp8_v(FLY_LEVEL_B), IF_Z)) {
            lda8(ea_dp(ACTOR_SPEED_Y + 2));
            if (!IF_N) {
                lda8_v(0x30);
                trb8(ea_dp(ACTOR_09));
            }
        }
        ldx8_v(0x00);
        push16(0x9586);
        abi_ActorEndOutOfRange();
        cpu.s = (uint16_t)(cpu.s + 2);
    }
}

/*
 * @routine 83:9532 ActorKnockedAway
 * WHAT IT DOES: Knocks the actor away: animation A, sound X, motion preset
 * Y; it faces away from Aladdin, poses 16 frames, then flies off until it
 * is out of range and ends.
 * HOW: X and Y kept on the stack; AnimStart A, ActorPictureSend;
 * ActorMotionPreset Y; SoundQueueAdd X; its facing and flip Aladdin's
 * flip with bit 0 turned; STEPS 16 frames (BRK) of AnimStep and
 * ActorPictureSend; then fly_off.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; A = the
 * animation, X = the sound, Y = the preset; reached by a kind's jump.
 * Exit: never returns.
 */
void abi_ActorKnockedAway(void)
{
    phx8();
    phy8();
    abi_AnimStart();
    abi_ActorPictureSend();
    ply8();
    abi_ActorMotionPreset();
    pla8();
    abi_SoundQueueAdd();
    lda8(ea_abs((uint16_t)PLAYER_FLIP));
    eor8_v(0x01);
    sta8(ea_dp(ACTOR_FACING));
    sta8(ea_dp(ACTOR_FLIP));
    lda8_v(0x10);
    sta8(ea_dp(STEPS));
    do {
        actor_yield(0x839553u);
        abi_AnimStep();
        abi_ActorPictureSend();
        MEM8(dec, ea_dp(STEPS));
    } while (!IF_Z);
    fly_off();
}

/*
 * @routine 83:9589 ActorKnockedAwayResumed
 * WHAT IT DOES: Knocks the actor away with its animation taken up where it
 * is: ANIM_AT = X, motion preset Y, five frames' wait, then it flies off
 * until it is out of range and ends.
 * HOW: Y kept on the stack; ANIM_AT = X; AnimResume, ActorPictureSend;
 * ActorMotionPreset Y; COP 5; then fly_off (into ActorKnockedAway's
 * flight).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; A = the
 * animation, X its frame, Y the preset; reached by a kind's jump.
 * Exit: never returns.
 */
void abi_ActorKnockedAwayResumed(void)
{
    phy8();
    stx8(ea_dp(ANIM_AT));
    abi_AnimResume();
    abi_ActorPictureSend();
    ply8();
    abi_ActorMotionPreset();
    lda8_v(0x05);
    actor_wait(0x83959Bu);
    fly_off();
}

/*
 * @routine 83:9C19 Kind20Hit
 * WHAT IT DOES: Actor kind $20's guard struck: from in front (its facing
 * not negative) knocked away (animation 4, sound $0F, preset 3); from
 * behind turned round, sound 7, and knocked away from where its
 * animation is (animation 4 at frame 3, preset $4C). With PLACE bit 4
 * (no script) its palette from ROM entry 5 first.
 * HOW: PLACE bit 4: ActorPaletteFromRom Y = 5. ACTOR_FACING not
 * negative: A 4, X $0F, Y 3, on to ActorKnockedAway. Else facing = its
 * bit 0, flip the other; SoundQueueAdd 7; A 4, X 3, Y $4C,
 * ActorKnockedAwayResumed.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; reached by
 * the actor loop's jump for kind $20 in a state with bit 7 set (the table
 * at $83:8DFD). Exit: never returns.
 */
void abi_Kind20Hit(void)
{
    lda8(ea_dp(ACTOR_07));
    and8_v(0x10);
    if (!IF_Z) {
        ldy8_v(0x05);
        abi_ActorPaletteFromRom();
    }
    lda8(ea_dp(ACTOR_FACING));
    if (!IF_N) {
        lda8_v(0x04);
        ldx8_v(0x0F);
        ldy8_v(0x03);
        actor_leave(0x839532u);
        return;
    }
    and8_v(0x01);
    sta8(ea_dp(ACTOR_FACING));
    eor8_v(0x01);
    sta8(ea_dp(ACTOR_FLIP));
    lda8_v(0x07);
    abi_SoundQueueAdd();
    lda8_v(0x04);
    ldx8_v(0x03);
    ldy8_v(0x4C);
    jump_routine(0x839589u);
}
