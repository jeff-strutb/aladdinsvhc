/*
 * course_platform.c: pieces of the platform of kind $47 (its start at
 * $83:C96E, its wait routine at $83:CAD1), which flies a course the player
 * rides: the course's waypoints as it is worked out, the stretches where
 * it turns, the dip it makes under the player's weight and its phases,
 * how far it moved in a frame, and the platforms started along the course
 * and kept within its live stretch. Drafted by cdraft.py; named and
 * described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"
#include "wram_player.h"

void abi_ActorCreateChild(void);
void abi_ActorMoveScaled(void);
void abi_ActorSpawnFacingY(void);
void abi_SoundQueueAdd(void);

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

/*
 * @routine 83:CA11 PlatformSpawnOnCourse
 * WHAT IT DOES: Starts one more platform on the course: a child of kind
 * $47 at the waypoint the course maker's offset ($37) names; the next is
 * six waypoints on.
 * HOW: ActorCreateChild with kind $47; the child's x and y (6 bytes) from
 * the waypoint, its own $37 the waypoint's offset; the maker's offset
 * moves on $30.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $83; D = the
 * course maker; called by JSR, four times, once the course is worked out.
 * Exit: M 8-bit, X 8-bit; Y = the child (low byte); A low = the new
 * offset's low byte.
 */
void abi_PlatformSpawnOnCourse(void)
{
    lda8_v(0x47);  /* 83CA11 lda */
    abi_ActorCreateChild();  /* 83CA13 jsl */
    op_rep(0x20);  /* 83CA17 rep */
    txy16();  /* 83CA19 txy */
    ldx16(ea_dp(0x37));  /* 83CA1A ldx */
    clc();  /* 83CA1C clc */
    lda16(ea_longx(0x7F5000u));  /* 83CA1D lda */
    sta16(ea_absy(0x000B));  /* 83CA21 sta */
    lda16(ea_longx(0x7F5002u));  /* 83CA24 lda */
    sta16(ea_absy(0x000D));  /* 83CA28 sta */
    lda16(ea_longx(0x7F5004u));  /* 83CA2B lda */
    sta16(ea_absy(0x000F));  /* 83CA2F sta */
    txa16();  /* 83CA32 txa */
    sta16(ea_absy(0x0037));  /* 83CA33 sta */
    clc();  /* 83CA36 clc */
    adc16_v(0x0030);  /* 83CA37 adc */
    sta16(ea_dp(0x37));  /* 83CA3A sta */
    op_sep(0x30);  /* 83CA3C sep */
    return;  /* 83CA3E rts */
}


/*
 * @routine 83:CAC1 PlatformDipStart
 * WHAT IT DOES: Starts the platform's dip as the player lands on it: sound
 * A kept for it, dip phase 1, the dip back to 0.
 * HOW: A to $17, the phase to 1, then on into PlatformDipClear.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $83; A = the
 * sound command; called by JSR. Exit: as PlatformDipClear; A low = 1.
 */
void abi_PlatformDipStart(void)
{
    sta8(ea_dp(0x17));  /* 83CAC1 sta */
    lda8_v(0x01);  /* 83CAC3 lda */
    sta8(ea_dp(0x13));  /* 83CAC5 sta */
    abi_PlatformDipClear(); return;
}


/*
 * @routine 83:CB24 PlatformCourseStep
 * WHAT IT DOES: Moves the platform a step along its direction at its
 * course speed, while the course is worked out; the step's y goes to its
 * height without the dip.
 * HOW: The height without the dip ($4D-$4F) put in its y; the direction's
 * 6-byte step (8 bytes a direction at $D304) to $000C-$0012;
 * ActorMoveScaled with the course speed; the new y kept back at $4D-$4F.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $83; called by
 * JSR (by the slowing and speeding steps at 83:CAFC and 83:CB10). Exit: M
 * 8-bit, X 8-bit (as ActorMoveScaled); A low = the y's top byte.
 */
void abi_PlatformCourseStep(void)
{
    lda8(ea_dp(0x4D));  /* 83CB24 lda */
    sta8(ea_dp(0x0E));  /* 83CB26 sta */
    lda8(ea_dp(0x4E));  /* 83CB28 lda */
    sta8(ea_dp(0x0F));  /* 83CB2A sta */
    lda8(ea_dp(0x4F));  /* 83CB2C lda */
    sta8(ea_dp(0x10));  /* 83CB2E sta */
    lda8(ea_dp(0x14));  /* 83CB30 lda */
    ACC8(asl);  /* 83CB32 asl */
    ACC8(asl);  /* 83CB33 asl */
    ACC8(asl);  /* 83CB34 asl */
    tax8();  /* 83CB35 tax */
    lda8(ea_absx(0xD304));  /* 83CB36 lda */
    sta8(ea_abs(SCRATCH));  /* 83CB39 sta */
    lda8(ea_absx(0xD305));  /* 83CB3C lda */
    sta8(ea_abs(0x000D));  /* 83CB3F sta */
    lda8(ea_absx(0xD306));  /* 83CB42 lda */
    sta8(ea_abs(0x000E));  /* 83CB45 sta */
    lda8(ea_absx(0xD307));  /* 83CB48 lda */
    sta8(ea_abs(0x0010));  /* 83CB4B sta */
    lda8(ea_absx(0xD308));  /* 83CB4E lda */
    sta8(ea_abs(0x0011));  /* 83CB51 sta */
    lda8(ea_absx(0xD309));  /* 83CB54 lda */
    sta8(ea_abs(V_0012));  /* 83CB57 sta */
    lda8(ea_dp(0x20));  /* 83CB5A lda */
    abi_ActorMoveScaled();  /* 83CB5C jsl */
    lda8(ea_dp(0x0E));  /* 83CB60 lda */
    sta8(ea_dp(0x4D));  /* 83CB62 sta */
    lda8(ea_dp(0x0F));  /* 83CB64 lda */
    sta8(ea_dp(0x4E));  /* 83CB66 sta */
    lda8(ea_dp(0x10));  /* 83CB68 lda */
    sta8(ea_dp(0x4F));  /* 83CB6A sta */
    return;  /* 83CB6C rts */
}


/*
 * @routine 83:CB93 PlatformCourseFollow
 * WHAT IT DOES: Follows the course: on to the next waypoint once the
 * platform passes its x; and when the platform is out of the stretch the
 * course keeps live, moves it 24 waypoints on or back to the course's
 * other end, with that waypoint's direction and speed.
 * HOW: Its x pixels against the next waypoint's: past it, its offset ($37)
 * moves on 8. The stretch: 12 waypoints either side of the live one
 * (COURSE_LIVE, $0400); outside it, $C0 added (x below $1A44) or taken
 * away; a negative offset leaves it; else x, y and height without the dip
 * from the waypoint, its speed, the direction (to $40 and its facing), its
 * resume address from the direction's entry at $D312, $1C = $FF, $42 and
 * the dip phase zeroed, PlatformDipClear, $16 counted up. Its early
 * return, SEP #$30 and RTS, lies before its entry, at 83:CB90.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $83; called by
 * JSR from the platform's wait routine (83:CAD1). Exit: M 8-bit, X 8-bit;
 * A, X and Y as left by the last step taken.
 */
void abi_PlatformCourseFollow(void)
{
    goto L_83CB93;
L_83CB90:
    op_sep(0x30);  /* 83CB90 sep */
    return;  /* 83CB92 rts */
L_83CB93:
    op_rep(0x30);  /* 83CB93 rep */
    ldx16(ea_dp(0x37));  /* 83CB95 ldx */
    lda16(ea_dp(0x0C));  /* 83CB97 lda */
    cmp16_v(rd16(ea_longx(0x7F5009u)));  /* 83CB99 cmp */
    if (!IF_C) goto L_83CB90;  /* 83CB9D bcc */
    clc();  /* 83CB9F clc */
    txa16();  /* 83CBA0 txa */
    adc16_v(0x0008);  /* 83CBA1 adc */
    sta16(ea_dp(0x37));  /* 83CBA4 sta */
    lda16(ea_abs(0x0400));  /* 83CBA6 lda */
    ACC16(asl);  /* 83CBA9 asl */
    ACC16(asl);  /* 83CBAA asl */
    ACC16(asl);  /* 83CBAB asl */
    tax16();  /* 83CBAC tax */
    lda16(ea_dp(0x0C));  /* 83CBAD lda */
    cmp16_v(rd16(ea_longx(0x7F4FA1u)));  /* 83CBAF cmp */
    if (!IF_C) goto L_83CBBB;  /* 83CBB3 bcc */
    cmp16_v(rd16(ea_longx(0x7F5061u)));  /* 83CBB5 cmp */
    if (!IF_C) goto L_83CC15;  /* 83CBB9 bcc */
L_83CBBB:
    op_rep(0x30);  /* 83CBBB rep */
    lda16_v(0x00C0);  /* 83CBBD lda */
    ldy16(ea_dp(0x0C));  /* 83CBC0 ldy */
    cpy16_v(rd16(ea_abs(0x1A44)));  /* 83CBC2 cpy */
    if (!IF_C) goto L_83CBCA;  /* 83CBC5 bcc */
    lda16_v(0xFF40);  /* 83CBC7 lda */
L_83CBCA:
    clc();  /* 83CBCA clc */
    adc16_v(rd16(ea_dp(0x37)));  /* 83CBCB adc */
    if (IF_N) goto L_83CC15;  /* 83CBCD bmi */
    sta16(ea_dp(0x37));  /* 83CBCF sta */
    tax16();  /* 83CBD1 tax */
    lda16(ea_longx(0x7F5000u));  /* 83CBD2 lda */
    sta16(ea_dp(0x0B));  /* 83CBD6 sta */
    lda16(ea_longx(0x7F5002u));  /* 83CBD8 lda */
    sta16(ea_dp(0x0D));  /* 83CBDC sta */
    lda16(ea_longx(0x7F5004u));  /* 83CBDE lda */
    sta16(ea_dp(0x0F));  /* 83CBE2 sta */
    sta16(ea_dp(0x4E));  /* 83CBE4 sta */
    op_sep(0x20);  /* 83CBE6 sep */
    lda8(ea_dp(0x0E));  /* 83CBE8 lda */
    sta8(ea_dp(0x4D));  /* 83CBEA sta */
    lda8(ea_longx(0x7F5007u));  /* 83CBEC lda */
    sta8(ea_dp(0x20));  /* 83CBF0 sta */
    lda8(ea_longx(0x7F5006u));  /* 83CBF2 lda */
    op_sep(0x10);  /* 83CBF6 sep */
    sta8(ea_dp(0x40));  /* 83CBF8 sta */
    sta8(ea_dp(0x14));  /* 83CBFA sta */
    ACC8(asl);  /* 83CBFC asl */
    tax8();  /* 83CBFD tax */
    lda8(ea_absx(0xD312));  /* 83CBFE lda */
    sta8(ea_dp(0x03));  /* 83CC01 sta */
    lda8(ea_absx(0xD313));  /* 83CC03 lda */
    sta8(ea_dp(0x04));  /* 83CC06 sta */
    lda8_v(0xFF);  /* 83CC08 lda */
    sta8(ea_dp(0x1C));  /* 83CC0A sta */
    stz8(ea_dp(0x42));  /* 83CC0C stz */
    stz8(ea_dp(0x13));  /* 83CC0E stz */
    abi_PlatformDipClear();  /* 83CC10 jsr */
    MEM8(inc, ea_dp(0x16));  /* 83CC13 inc */
L_83CC15:
    op_sep(0x30);  /* 83CC15 sep */
    return;  /* 83CC17 rts */
}


/*
 * @routine 83:CC42 PlatformDipLand
 * WHAT IT DOES: Dip phase 1: the platform is landed on: starts a child
 * (hit box $4B) that keeps the platform as its $37, plays the landing's
 * sound when the platform can be hit, and moves on to the next phase.
 * HOW: ActorSpawnFacingY with hit box $4B, facing 0 and the platform's
 * $40; started: the platform's address (TDC) to the child's $37-$38; bit 6
 * of $09: SoundQueueAdd with the sound kept at $17. The phase steps up.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $83; called by
 * JSR through the dip phase's table. Exit: M 8-bit; a child started: X
 * 8-bit; none: X as ActorSpawnFacingY left it.
 */
void abi_PlatformDipLand(void)
{
    lda8_v(0x4B);  /* 83CC42 lda */
    ldx8_v(0x00);  /* 83CC44 ldx */
    ldy8(ea_dp(0x40));  /* 83CC46 ldy */
    abi_ActorSpawnFacingY();  /* 83CC48 jsl */
    if (IF_N) goto L_83CC62;  /* 83CC4C bmi */
    tdc();  /* 83CC4E tdc */
    sta8(ea_absx(0x0037));  /* 83CC4F sta */
    xba();  /* 83CC52 xba */
    sta8(ea_absx(0x0038));  /* 83CC53 sta */
    op_sep(0x10);  /* 83CC56 sep */
    bit8_v(rd8(ea_dp(0x09)));  /* 83CC58 bit */
    if (!IF_V) goto L_83CC62;  /* 83CC5A bvc */
    lda8(ea_dp(0x17));  /* 83CC5C lda */
    abi_SoundQueueAdd();  /* 83CC5E jsl */
L_83CC62:
    MEM8(inc, ea_dp(0x13));  /* 83CC62 inc */
    return;  /* 83CC64 rts */
}


/*
 * @routine 83:CC65 PlatformDipSink
 * WHAT IT DOES: Dip phase 2: the platform sinks; at 11 pixels down, on to
 * the next phase.
 * HOW: PlatformDipStep with the sinking rate (X = 0); the dip's pixels
 * (16-bit) 11 or more: the phase steps up (a 16-bit increment of $13).
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $83; called by
 * JSR through the dip phase's table. Exit: M 8-bit; X = 0; A = the dip's
 * pixels; C from the comparison.
 */
void abi_PlatformDipSink(void)
{
    ldx8_v(0x00);  /* 83CC65 ldx */
    abi_PlatformDipStep();  /* 83CC67 jsr */
    op_rep(0x20);  /* 83CC6A rep */
    clc();  /* 83CC6C clc */
    lda16(ea_dp(0x4B));  /* 83CC6D lda */
    cmp16_v(0x000B);  /* 83CC6F cmp */
    if (!IF_C) goto L_83CC76;  /* 83CC72 bcc */
    MEM16(inc, ea_dp(0x13));  /* 83CC74 inc */
L_83CC76:
    op_sep(0x20);  /* 83CC76 sep */
    return;  /* 83CC78 rts */
}


/*
 * @routine 83:CC79 PlatformDipRise
 * WHAT IT DOES: Dip phase 3: the platform rises; under 3 pixels down, on
 * to the next phase (waiting for the player to step off).
 * HOW: PlatformDipStep with the rising rate (X = 3); the dip's pixels
 * (16-bit) below 3: the phase steps up (a 16-bit increment of $13).
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $83; called by
 * JSR through the dip phase's table. Exit: M 8-bit; X = 3; A = the dip's
 * pixels; C from the comparison.
 */
void abi_PlatformDipRise(void)
{
    ldx8_v(0x03);  /* 83CC79 ldx */
    abi_PlatformDipStep();  /* 83CC7B jsr */
    op_rep(0x20);  /* 83CC7E rep */
    clc();  /* 83CC80 clc */
    lda16(ea_dp(0x4B));  /* 83CC81 lda */
    cmp16_v(0x0003);  /* 83CC83 cmp */
    if (IF_C) goto L_83CC8A;  /* 83CC86 bcs */
    MEM16(inc, ea_dp(0x13));  /* 83CC88 inc */
L_83CC8A:
    op_sep(0x20);  /* 83CC8A sep */
    return;  /* 83CC8C rts */
}


/*
 * @routine 83:CC94 PlatformDipSinkAgain
 * WHAT IT DOES: Dip phase 5: once the player is off, the platform sinks
 * again; at 11 pixels down, on to the last phase.
 * HOW: As PlatformDipSink: PlatformDipStep with X = 0; 11 or more: the
 * phase steps up (a 16-bit increment of $13).
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $83; called by
 * JSR through the dip phase's table. Exit: M 8-bit; X = 0; A = the dip's
 * pixels; C from the comparison.
 */
void abi_PlatformDipSinkAgain(void)
{
    ldx8_v(0x00);  /* 83CC94 ldx */
    abi_PlatformDipStep();  /* 83CC96 jsr */
    op_rep(0x20);  /* 83CC99 rep */
    clc();  /* 83CC9B clc */
    lda16(ea_dp(0x4B));  /* 83CC9C lda */
    cmp16_v(0x000B);  /* 83CC9E cmp */
    if (!IF_C) goto L_83CCA5;  /* 83CCA1 bcc */
    MEM16(inc, ea_dp(0x13));  /* 83CCA3 inc */
L_83CCA5:
    op_sep(0x20);  /* 83CCA5 sep */
    return;  /* 83CCA7 rts */
}


/*
 * @routine 83:CCA8 PlatformDipRiseEnd
 * WHAT IT DOES: Dip phase 6: the platform rises back; once past its
 * height, the dip is zeroed and the phase back to 0 (idle).
 * HOW: PlatformDipStep with the rising rate (X = 3); the dip's top byte
 * negative: its three bytes and the phase zeroed.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $83; called by
 * JSR through the dip phase's table. Exit: M 8-bit; X = 3; A low = the
 * dip's top byte.
 */
void abi_PlatformDipRiseEnd(void)
{
    ldx8_v(0x03);  /* 83CCA8 ldx */
    abi_PlatformDipStep();  /* 83CCAA jsr */
    lda8(ea_dp(0x4C));  /* 83CCAD lda */
    if (!IF_N) goto L_83CCB9;  /* 83CCAF bpl */
    stz8(ea_dp(0x4A));  /* 83CCB1 stz */
    stz8(ea_dp(0x4B));  /* 83CCB3 stz */
    stz8(ea_dp(0x4C));  /* 83CCB5 stz */
    stz8(ea_dp(0x13));  /* 83CCB7 stz */
L_83CCB9:
    return;  /* 83CCB9 rts */
}

/*
 * @routine 83:CAFC PlatformCourseSlower
 * WHAT IT DOES: A step of the course while it is worked out, slowing: the
 * course speed down by 1 (not below $20), the platform moved a step, and
 * whether it is in a turn stretch.
 * HOW: The speed less 1, held at $20 at least; PlatformCourseStep;
 * PlatformTurnZoneTest.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $83; called by
 * JSR. Exit: as PlatformTurnZoneTest: C clear in a turn stretch.
 */
void abi_PlatformCourseSlower(void)
{
    sec();  /* 83CAFC sec */
    lda8(ea_dp(0x20));  /* 83CAFD lda */
    sbc8_v(0x01);  /* 83CAFF sbc */
    cmp8_v(0x20);  /* 83CB01 cmp */
    if (IF_C) goto L_83CB07;  /* 83CB03 bcs */
    lda8_v(0x20);  /* 83CB05 lda */
L_83CB07:
    sta8(ea_dp(0x20));  /* 83CB07 sta */
    abi_PlatformCourseStep();  /* 83CB09 jsr */
    abi_PlatformTurnZoneTest();  /* 83CB0C jsr */
    return;  /* 83CB0F rts */
}


/*
 * @routine 83:CB10 PlatformCourseFaster
 * WHAT IT DOES: A step of the course while it is worked out, speeding: the
 * course speed up by 1 (not above $80), the platform moved a step, and
 * whether it is in a turn stretch.
 * HOW: The speed plus 1, held at $80 at most; PlatformCourseStep;
 * PlatformTurnZoneTest.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $83; called by
 * JSR. Exit: as PlatformTurnZoneTest: C clear in a turn stretch.
 */
void abi_PlatformCourseFaster(void)
{
    clc();  /* 83CB10 clc */
    lda8(ea_dp(0x20));  /* 83CB11 lda */
    adc8_v(0x01);  /* 83CB13 adc */
    cmp8_v(0x80);  /* 83CB15 cmp */
    if (!IF_C) goto L_83CB1B;  /* 83CB17 bcc */
    lda8_v(0x80);  /* 83CB19 lda */
L_83CB1B:
    sta8(ea_dp(0x20));  /* 83CB1B sta */
    abi_PlatformCourseStep();  /* 83CB1D jsr */
    abi_PlatformTurnZoneTest();  /* 83CB20 jsr */
    return;  /* 83CB23 rts */
}


/*
 * @routine 83:CC19 PlatformDipUpdate
 * WHAT IT DOES: Runs the platform's dip for the frame: its phase's
 * routine, then its y set to its height without the dip plus the dip.
 * HOW: JSR through the dip phase table at $83:CC34 by the phase; then the
 * 24-bit sum of BASE_Y and DIP, a byte at a time, to its y.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $83; called by
 * JSR from the platform's wait routine (83:CAD1). Exit: M 8-bit; A low =
 * the y's top byte; X as the phase routine left it; flags from the last
 * add.
 */
void abi_PlatformDipUpdate(void)
{
    lda8(ea_dp(0x13));  /* 83CC19 lda */
    ACC8(asl);  /* 83CC1B asl */
    tax8();  /* 83CC1C tax */
    call_routine(0x830000u | rd16(0x830000u | (uint16_t)(0xCC34 + cpu.x)), 0);  /* 83CC1D jsr */
    clc();  /* 83CC20 clc */
    lda8(ea_dp(0x4D));  /* 83CC21 lda */
    adc8_v(rd8(ea_dp(0x4A)));  /* 83CC23 adc */
    sta8(ea_dp(0x0E));  /* 83CC25 sta */
    lda8(ea_dp(0x4E));  /* 83CC27 lda */
    adc8_v(rd8(ea_dp(0x4B)));  /* 83CC29 adc */
    sta8(ea_dp(0x0F));  /* 83CC2B sta */
    lda8(ea_dp(0x4F));  /* 83CC2D lda */
    adc8_v(rd8(ea_dp(0x4C)));  /* 83CC2F adc */
    sta8(ea_dp(0x10));  /* 83CC31 sta */
    return;  /* 83CC33 rts */
}

