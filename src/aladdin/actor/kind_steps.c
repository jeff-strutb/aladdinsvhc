/*
 * kind_steps.c: small per-frame steps of some actor kinds (animate, follow
 * a path), and the path an actor can follow.
 *
 * A path is a list of segments: a velocity (word), then a byte the step
 * count; a segment's velocity is added to the actor's position each step
 * until its frames run out; a zero velocity word ends the path.
 */
#include "machine.h"
#include "ops65.h"
void abi_KindAnimateFreeOff(void);
void abi_KindAnimate9533(void);
void abi_KindAnimate9628(void);
void abi_KindAnimate9EF0(void);
void abi_KindAnimate9F4A(void);
void abi_KindAnimateSend(void);
void abi_ActorPathStep(void);
void abi_ActorPathNext(void);
void abi_AnimStep(void);
void abi_ActorPictureSend(void);
void abi_ActorParityBitsSet(void);
void abi_ActorFreeOutOfRangeLong(void);
#include "actor.h"
#include "actor_anim.h"
#include "wram.h"

/*
 * @routine 84:9387 KindAnimateFreeOff
 * WHAT IT DOES: One frame of a kind that only animates, freed once it is
 * far out of range.
 * HOW: AnimStep, then ActorFreeOutOfRangeLong with range $18.
 * Entry: M 8-bit, X 8-bit; D = the actor; called by JSL. Exit: as
 * ActorFreeOutOfRangeLong.
 */
void abi_KindAnimateFreeOff(void)
{
    abi_AnimStep();  /* 849387 jsl */
    ldx8_v(0x18);  /* 84938B ldx */
    abi_ActorFreeOutOfRangeLong(); return;  /* 84938D jml */
}

/*
 * @routine 84:9533 KindAnimate9533
 * WHAT IT DOES: One frame of an actor kind that only animates.
 * HOW: AnimStep.
 * Entry: D = the actor; called by JSL. Exit: as AnimStep.
 */
void abi_KindAnimate9533(void)
{
    abi_AnimStep();  /* 849533 jsl */
    return;  /* 849537 rtl */
}

/*
 * @routine 84:9628 KindAnimate9628
 * WHAT IT DOES: One frame of an actor kind that only animates.
 * HOW: AnimStep.
 * Entry: D = the actor; called by JSL. Exit: as AnimStep.
 */
void abi_KindAnimate9628(void)
{
    abi_AnimStep();  /* 849628 jsl */
    return;  /* 84962C rtl */
}

/*
 * @routine 84:9EF0 KindAnimate9EF0
 * WHAT IT DOES: One frame of an actor kind that only animates.
 * HOW: AnimStep.
 * Entry: D = the actor; called by JSL. Exit: as AnimStep.
 */
void abi_KindAnimate9EF0(void)
{
    abi_AnimStep();  /* 849EF0 jsl */
    return;  /* 849EF4 rtl */
}

/*
 * @routine 84:9F4A KindAnimate9F4A
 * WHAT IT DOES: One frame of an actor kind that only animates.
 * HOW: AnimStep.
 * Entry: D = the actor; called by JSL. Exit: as AnimStep.
 */
void abi_KindAnimate9F4A(void)
{
    abi_AnimStep();  /* 849F4A jsl */
    return;  /* 849F4E rtl */
}

/*
 * @routine 84:903B KindAnimateSend
 * WHAT IT DOES: One frame of an actor kind that animates its own picture.
 * HOW: AnimStep, then ActorPictureSend.
 * Entry: D = the actor; called by JSL. Exit: as ActorPictureSend.
 */
void abi_KindAnimateSend(void)
{
    abi_AnimStep();  /* 84903B jsl */
    abi_ActorPictureSend();  /* 84903F jsl */
    return;  /* 849043 rtl */
}

/*
 * @routine 84:8CAF ActorPathStep
 * WHAT IT DOES: One step of the actor along its path: its position moves
 * by the segment's velocity, and the path goes on when the step's frames
 * run out.
 * HOW: ActorParityBitsSet; the velocity added to the position; when the
 * path is on, ActorPathNext.
 * Entry: M 8-bit; D = the actor; called by JSR. Exit: M 8-bit; as
 * ActorPathNext when the path is on.
 */
void abi_ActorPathStep(void)
{
    abi_ActorParityBitsSet();  /* 848CAF jsr */
    op_rep(0x20);  /* 848CB2 rep */
    clc();  /* 848CB4 clc */
    lda16(ea_dp(0x48));  /* 848CB5 lda */
    adc16_v(rd16(ea_dp(0x42)));  /* 848CB7 adc */
    sta16(ea_dp(0x42));  /* 848CB9 sta */
    op_sep(0x20);  /* 848CBB sep */
    lda8(ea_dp(0x4C));  /* 848CBD lda */
    if (IF_Z) goto L_848CC4;  /* 848CBF beq */
    abi_ActorPathNext();  /* 848CC1 jsr */
L_848CC4:
    return;  /* 848CC4 rts */
}

/*
 * @routine 84:8DFB ActorPathNext
 * WHAT IT DOES: Counts down the path's step; at its end takes the next
 * step, the next segment when the segment's steps run out, and marks the
 * path done at its end.
 * HOW: The frames count down; at 0: the velocity from the segment
 * (XORed with the speed word $19, plus $1C), the frames reloaded;
 * the segment's steps count down; at 0 the next segment's address from
 * the list: zero ends the path ($18 = 1), else its step count and frames.
 * Entry: M 8-bit; D = the actor; called by JSR. Exit: M and X 8-bit.
 */
void abi_ActorPathNext(void)
{
    lda8(ea_dp(0x47));  /* 848DFB lda */
    if (IF_Z) goto L_848E32;  /* 848DFD beq */
    MEM8(dec, ea_dp(0x47));  /* 848DFF dec */
    if (!IF_Z) goto L_848E32;  /* 848E01 bne */
    op_rep(0x10);  /* 848E03 rep */
    ldy16(ea_dp(0x4A));  /* 848E05 ldy */
    op_rep(0x20);  /* 848E07 rep */
    lda16(ea_absy(0x0000));  /* 848E09 lda */
    eor16_v(rd16(ea_dp(0x19)));  /* 848E0C eor */
    clc();  /* 848E0E clc */
    adc16_v(rd16(ea_dp(0x1C)));  /* 848E0F adc */
    sta16(ea_dp(0x48));  /* 848E11 sta */
    op_sep(0x20);  /* 848E13 sep */
    lda8(ea_dp(0x46));  /* 848E15 lda */
    sta8(ea_dp(0x47));  /* 848E17 sta */
    iny16();  /* 848E19 iny */
    iny16();  /* 848E1A iny */
    MEM8(dec, ea_dp(0x4D));  /* 848E1B dec */
    if (!IF_Z) goto L_848E2E;  /* 848E1D bne */
    lda8(ea_dp(0x45));  /* 848E1F lda */
    sta8(ea_dp(0x46));  /* 848E21 sta */
    ldx16(ea_absy(0x0000));  /* 848E23 ldx */
    if (IF_Z) goto L_848E33;  /* 848E26 beq */
    lda8(ea_absy(SOUND_SEQ));  /* 848E28 lda */
    sta8(ea_dp(0x4D));  /* 848E2B sta */
    txy16();  /* 848E2D txy */
L_848E2E:
    sty16(ea_dp(0x4A));  /* 848E2E sty */
    op_sep(0x30);  /* 848E30 sep */
L_848E32:
    return;  /* 848E32 rts */
L_848E33:
    stz8(ea_dp(0x47));  /* 848E33 stz */
    lda8_v(0x01);  /* 848E35 lda */
    sta8(ea_dp(0x18));  /* 848E37 sta */
    op_sep(0x30);  /* 848E39 sep */
    return;  /* 848E3B rts */
}
