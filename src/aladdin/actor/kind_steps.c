/*
 * kind_steps.c: small per-frame steps of some actor kinds (animate, follow
 * a path), and the path an actor can follow.
 *
 * A path is a list of segments: a velocity (word), then a byte the step
 * count; a segment's velocity is added to the actor's position each step
 * until its frames run out; a zero velocity word ends the path.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
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

/* An actor on a path: words of steps, each taken for some frames */
#define PATH_AT 0x42u              /* its place along it (word) */
#define PATH_FRAMES_FIRST 0x45u    /* frames each step of a new run is taken for */
#define PATH_FRAMES_EACH 0x46u     /* frames each step is taken for */
#define PATH_FRAMES 0x47u          /* frames left of this step; 0: stopped */
#define PATH_STEP 0x48u            /* the step added each frame (word) */
#define PATH_NEXT 0x4Au            /* the next step in the path (word, the DB bank) */
#define PATH_ON 0x4Cu              /* nonzero: it follows the path */
#define PATH_STEPS 0x4Du           /* steps left of this run */

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
    abi_AnimStep();
    x8_load(0x18);
    abi_ActorFreeOutOfRangeLong();
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
    uint8_t on;

    abi_ActorParityBitsSet();
    flag_set(P_C, 0);
    cpu.a = op_adc16(dp16(PATH_STEP), dp16(PATH_AT), &cpu.p);
    dp16_set(PATH_AT, cpu.a);
    on = rd8(dpa(PATH_ON));
    a8_load(on);
    if (on)
        abi_ActorPathNext();
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
    uint8_t frames = rd8(dpa(PATH_FRAMES));
    uint16_t next;

    a8_load(frames);
    if (!frames || dec8_at(dpa(PATH_FRAMES)))
        return;
    /* this step done: the next one (turned by $19, plus $1C) */
    op_rep(P_X);
    cpu.y = dp16(PATH_NEXT);
    nz16_of(cpu.y);
    flag_set(P_C, 0);
    cpu.a = op_adc16((uint16_t)(rd16(dba(0) + cpu.y) ^ dp16(0x19)), dp16(0x1C), &cpu.p);
    dp16_set(PATH_STEP, cpu.a);
    frames = rd8(dpa(PATH_FRAMES_EACH));
    a8_load(frames);
    wr8(dpa(PATH_FRAMES), frames);
    cpu.y = (uint16_t)(cpu.y + 2);
    if (!dec8_at(dpa(PATH_STEPS))) {
        /* this run done: the next run, or the path's end (0) */
        frames = rd8(dpa(PATH_FRAMES_FIRST));
        a8_load(frames);
        wr8(dpa(PATH_FRAMES_EACH), frames);
        next = rd16(dba(0) + cpu.y);
        cpu.x = next;
        nz16_of(next);
        if (!next) {
            wr8(dpa(PATH_FRAMES), 0);
            a8_load(0x01);
            wr8(dpa(ACTOR_18), 0x01);
            op_sep(0x30);
            return;
        }
        a8_load(rd8(dba(2) + cpu.y));
        wr8(dpa(PATH_STEPS), (uint8_t)cpu.a);
        cpu.y = cpu.x;
        nz16_of(cpu.y);
    }
    dp16_set(PATH_NEXT, cpu.y);
    op_sep(0x30);
}
