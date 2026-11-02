/*
 * course_platform.c: pieces of the platform of kind $47 (its start at
 * $83:C96E, its wait routine at $83:CAD1), which flies a course the player
 * rides: the course's waypoints as it is worked out, the stretches where
 * it turns, the dip it makes under the player's weight, and how far it
 * moved in a frame. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

/* The platform's own fields of its record */
#define COURSE_STEPS 0x22u         /* steps left before the next waypoint is kept */
#define COURSE_END 0x24u           /* offset of the next waypoint in COURSE (word) */
#define COURSE_SPEED 0x20u         /* its speed along the course */
#define DIP_PHASE 0x13u            /* what its dip is doing: an entry of the table at $83:CC34 */
#define LAST_X 0x19u               /* x pixels at the frame's start (word) */
#define LAST_Y 0x1Du               /* y pixels at the frame's start (word) */
#define MOVED_X 0x45u              /* x pixels moved in the frame (word) */
#define MOVED_Y 0x47u              /* y pixels moved in the frame (word) */
#define RIDDEN 0x49u               /* nonzero: the player stands on it this frame */
#define DIP 0x4Au                  /* how far it has sunk under the player: 24-bit, fraction first */

#define COURSE_STEPS_APART 0x26    /* steps from one waypoint to the next */
/* The course's waypoints, 8 bytes each: x and y (24-bit, fraction first),
   the direction and the speed. */
#define COURSE 0x7F5000u

/* In the bank DB names: the number of turn stretches, then 6 bytes each:
   the first x pixel and the x pixel past it (words) */
#define TURN_ZONE_COUNT 0xD2F7u
#define TURN_ZONES 0xD2F8u
/* In the bank DB names: the rates (24-bit) it sinks and rises at */
#define DIP_RATES 0xD2EDu

/*
 * @routine 83:CA3F PlatformCourseRecord
 * WHAT IT DOES: Keeps a waypoint of the platform's course every 38 steps
 * while the course is worked out: its position, direction and speed.
 * HOW: The step count goes down; at 0 it starts again at 38 and the 8
 * bytes go to the course at the end offset, which moves on by 8.
 * Entry: M 8-bit, X 8-bit; D = the platform's record. Exit: no waypoint:
 * N and Z from the count; A, X, Y and the other flags unchanged. A
 * waypoint: A low = the speed, A high = the new end offset's high byte; X
 * = the waypoint's offset (its low byte); N and Z from the speed; V and C
 * from the offset's addition. M and X 8-bit; Y unchanged.
 */
void abi_PlatformCourseRecord(void)
{
    MEM8(dec, ea_dp(COURSE_STEPS));
    if (!IF_Z)
        return;
    lda8_v(COURSE_STEPS_APART);
    sta8(ea_dp(COURSE_STEPS));
    op_rep(0x30);
    ldx16(ea_dp(COURSE_END));
    lda16(ea_dp(ACTOR_X));
    sta16(ea_longx(COURSE));
    lda16(ea_dp(ACTOR_X + 2));
    sta16(ea_longx(COURSE + 2));
    lda16(ea_dp(ACTOR_Y + 1));
    sta16(ea_longx(COURSE + 4));
    clc();
    txa16();
    adc16_v(8);
    sta16(ea_dp(COURSE_END));
    op_sep(0x20);
    lda8(ea_dp(ACTOR_FACING));
    sta8(ea_longx(COURSE + 6));
    lda8(ea_dp(COURSE_SPEED));
    sta8(ea_longx(COURSE + 7));
    op_sep(0x10);
}

/*
 * @routine 83:CAC7 PlatformDipClear
 * WHAT IT DOES: Takes away the platform's dip: it is back at its height.
 * HOW: The three bytes of the dip are zeroed.
 * Entry: M 8-bit; D = the platform's record. Exit: registers and flags
 * unchanged.
 */
void abi_PlatformDipClear(void)
{
    stz8(ea_dp(DIP));
    stz8(ea_dp(DIP + 1));
    stz8(ea_dp(DIP + 2));
}

/*
 * @routine 83:CB6D PlatformTurnZoneTest
 * WHAT IT DOES: Tells whether the platform is in a stretch of the course
 * where it turns: C clear when its x pixel is in one.
 * HOW: The stretches, in order of x, each compared unsigned: below a
 * stretch's first pixel, or past the last stretch, is out (C set); below
 * the pixel past it is in (C clear).
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB the table's
 * bank. Exit: A = its x pixel (16 bits); X = 6 times the stretch reached;
 * Y = the stretches left from it (the count less its number); N and Z
 * from the last compare (from the count when the stretches ran out). In:
 * C clear. Out: C set. M 8-bit, X 8-bit; V unchanged.
 */
void abi_PlatformTurnZoneTest(void)
{
    op_rep(0x20);
    ldy8(ea_abs(TURN_ZONE_COUNT));
    ldx8_v(0x00);
    lda16(ea_dp(ACTOR_X + 1));
    do {
        cmp16_v(rd16(ea_absx(TURN_ZONES)));
        if (!IF_C)
            break;
        cmp16_v(rd16(ea_absx(TURN_ZONES + 2)));
        if (!IF_C) {
            op_sep(0x20);
            return;
        }
        inx8();
        inx8();
        inx8();
        inx8();
        inx8();
        inx8();
        dey8();
    } while (!IF_Z);
    op_sep(0x21);
}

/*
 * @routine 83:CC18 PlatformDipIdle
 * WHAT IT DOES: The platform's dip phase 0: nothing to do (no one has
 * stepped on it).
 * HOW: RTS.
 * Entry: any; called by JSR through the dip phase's table. Exit:
 * registers and flags unchanged.
 */
void abi_PlatformDipIdle(void)
{
}

/*
 * @routine 83:CC8D PlatformDipWaitOff
 * WHAT IT DOES: The platform's dip phase 4: waits until the player is
 * off it, then on to the next phase (a last sink and rise).
 * HOW: The ridden flag tested; zero: the phase steps on by one.
 * Entry: M 8-bit; D = the platform's record; called by JSR through the
 * dip phase's table. Exit: A low = the ridden flag; N and Z from it, or
 * (it was zero) from the new phase. M 8-bit; A high, X, Y and the other
 * flags unchanged.
 */
void abi_PlatformDipWaitOff(void)
{
    lda8(ea_dp(RIDDEN));
    if (IF_Z)
        MEM8(inc, ea_dp(DIP_PHASE));
}

/*
 * @routine 83:CCBA PlatformDipStep
 * WHAT IT DOES: Moves the platform's dip by a frame's rate: sinking
 * (X = 0) or rising (X = 3).
 * HOW: A 24-bit addition, a byte at a time, of the rate X picks to the
 * dip.
 * Entry: M 8-bit; X = the rate's offset (0 or 3); D = the platform's
 * record; DB the rates' bank. Exit: A low = the dip's top byte; N, V, Z
 * and C from its addition. M 8-bit; A high, X and Y unchanged.
 */
void abi_PlatformDipStep(void)
{
    clc();
    lda8(ea_absx(DIP_RATES));
    adc8_v(rd8(ea_dp(DIP)));
    sta8(ea_dp(DIP));
    lda8(ea_absx(DIP_RATES + 1));
    adc8_v(rd8(ea_dp(DIP + 1)));
    sta8(ea_dp(DIP + 1));
    lda8(ea_absx(DIP_RATES + 2));
    adc8_v(rd8(ea_dp(DIP + 2)));
    sta8(ea_dp(DIP + 2));
}

/*
 * @routine 83:CCD1 PlatformPositionKeep
 * WHAT IT DOES: Keeps the platform's x and y pixels at the start of the
 * frame, to measure later how far it moved.
 * HOW: Two word copies with 16-bit A.
 * Entry: M 8-bit; D = the platform's record. Exit: A = its y pixel (16
 * bits); N and Z from it. M 8-bit; X, Y and the other flags unchanged.
 */
void abi_PlatformPositionKeep(void)
{
    op_rep(0x20);
    lda16(ea_dp(ACTOR_X + 1));
    sta16(ea_dp(LAST_X));
    lda16(ea_dp(ACTOR_Y + 1));
    sta16(ea_dp(LAST_Y));
    op_sep(0x20);
}

/*
 * @routine 83:CCDE PlatformMoveMeasure
 * WHAT IT DOES: Measures how far the platform moved in the frame, in x
 * and y pixels, for carrying its rider.
 * HOW: Its pixels less those kept at the frame's start, with 16-bit A.
 * Entry: M 8-bit; D = the platform's record. Exit: A = the y move (16
 * bits); N, V, Z and C from its subtraction. M 8-bit; X and Y unchanged.
 */
void abi_PlatformMoveMeasure(void)
{
    op_rep(0x20);
    sec();
    lda16(ea_dp(ACTOR_X + 1));
    sbc16_v(rd16(ea_dp(LAST_X)));
    sta16(ea_dp(MOVED_X));
    sec();
    lda16(ea_dp(ACTOR_Y + 1));
    sbc16_v(rd16(ea_dp(LAST_Y)));
    sta16(ea_dp(MOVED_Y));
    op_sep(0x20);
}
