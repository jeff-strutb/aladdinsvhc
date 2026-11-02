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
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_video.h"

void abi_ActorCreateChild(void);
void abi_ActorMoveScaled(void);
void abi_ActorSpawnFacingY(void);
void abi_SoundQueueAdd(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_ActorFreeListSortHigh(void);
void abi_PlatformOnCourseStart(void);
void abi_PlatformCourseSlower(void);
void abi_PlatformCourseFaster(void);
void abi_PlatformSpawnOnCourse(void);

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
#define BASE_Y 0x4Du               /* its y without the dip: 24-bit, fraction first */
#define COURSE_AT 0x37u            /* the course maker's: offset of the waypoint the next platform starts at (word) */
#define LAST_DIRECTION 0x1Cu       /* the direction it last took ($FF: none) */
#define DIRECTION 0x40u            /* the direction it moves along the course: 0 or 1 */
#define DIRECTION_FLIP 0x42u       /* nonzero: the wait routine flips DIRECTION */
#define FIELD_21 0x21u             /* zeroed at its start; no code filed so far reads it */

#define COURSE_STEPS_APART 0x26    /* steps from one waypoint to the next */
/* The course's waypoints, 8 bytes each: x and y (24-bit, fraction first),
   the direction and the speed. */
#define COURSE 0x7F5000u
#define WAYPOINT_SIZE 8
/* The waypoint the course is live around (the platforms are kept within
   12 waypoints of it): the course maker keeps it at the screen's middle. */
#define COURSE_LIVE 0x000400u
/* The course starts at x $2A, y $1C0 and is worked out to x $1000. */
#define COURSE_START_X 0x002A
#define COURSE_START_Y 0x01C0
#define COURSE_END_X_HIGH 0x10
/* Not yet filed (no script reaches it): a platform's part of the course
   in a turn stretch, at $83:CAA8. */
#define PLATFORM_IN_TURN 0x83CAA8u

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
    uint16_t x;

    if (dec8_at(dpa(COURSE_STEPS)))
        return;
    /* every COURSE_STEPS_APART steps, a waypoint: x, y, facing, speed */
    a8_load(COURSE_STEPS_APART);
    wr8(dpa(COURSE_STEPS), COURSE_STEPS_APART);
    op_rep(0x30);
    x = dp16(COURSE_END);
    wr16(COURSE + x, dp16(ACTOR_X));
    wr16(COURSE + 2 + x, dp16(ACTOR_X + 2));
    wr16(COURSE + 4 + x, dp16(ACTOR_Y + 1));
    flag_set(P_C, 0);
    cpu.a = op_adc16(x, WAYPOINT_SIZE, &cpu.p);
    dp16_set(COURSE_END, cpu.a);
    op_sep(P_M);
    wr8(COURSE + 6 + x, rd8(dpa(ACTOR_FACING)));
    a8_load(rd8(dpa(COURSE_SPEED)));
    wr8(COURSE + 7 + x, (uint8_t)cpu.a);
    cpu.x = x;
    op_sep(P_X);
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
    wr8(dpa(DIP), 0);
    wr8(dpa(DIP + 1), 0);
    wr8(dpa(DIP + 2), 0);
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
    uint16_t at;
    uint8_t x = 0;

    /* each stretch: from its first x to its second, 6 bytes apart */
    op_rep(P_M);
    y8_load(rd8(dba(TURN_ZONE_COUNT)));
    cpu.a = at = dp16(ACTOR_X + 1);
    do {
        cpu.x = x;
        cmp16_of(at, rd16(dba((uint16_t)(TURN_ZONES + x))));
        if (at < rd16(dba((uint16_t)(TURN_ZONES + x))))
            break;
        cmp16_of(at, rd16(dba((uint16_t)(TURN_ZONES + 2 + x))));
        if (at < rd16(dba((uint16_t)(TURN_ZONES + 2 + x)))) {
            op_sep(P_M);
            return;
        }
        x = (uint8_t)(x + 6);
        cpu.x = x;
        y8_load((uint8_t)(cpu.y - 1));
    } while ((uint8_t)cpu.y);
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
    a8_load(rd8(dpa(RIDDEN)));
    if (!(uint8_t)cpu.a)
        inc8_at(dpa(DIP_PHASE));
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
    flag_set(P_C, 0);
    wr8(dpa(DIP), op_adc8(rd8(dba((uint16_t)(DIP_RATES + cpu.x))), rd8(dpa(DIP)), &cpu.p));
    wr8(dpa(DIP + 1), op_adc8(rd8(dba((uint16_t)(DIP_RATES + 1 + cpu.x))), rd8(dpa(DIP + 1)), &cpu.p));
    a8_load(op_adc8(rd8(dba((uint16_t)(DIP_RATES + 2 + cpu.x))), rd8(dpa(DIP + 2)), &cpu.p));
    wr8(dpa(DIP + 2), (uint8_t)cpu.a);
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
    op_rep(P_M);
    dp16_set(LAST_X, dp16(ACTOR_X + 1));
    a16_load(dp16(ACTOR_Y + 1));
    dp16_set(LAST_Y, cpu.a);
    op_sep(P_M);
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
    op_rep(P_M);
    flag_set(P_C, 1);
    dp16_set(MOVED_X, op_sbc16(dp16(ACTOR_X + 1), dp16(LAST_X), &cpu.p));
    flag_set(P_C, 1);
    cpu.a = op_sbc16(dp16(ACTOR_Y + 1), dp16(LAST_Y), &cpu.p);
    dp16_set(MOVED_Y, cpu.a);
    op_sep(P_M);
}

/*
 * @routine 83:C96E PlatformCourseStart
 * @actor
 * WHAT IT DOES: Starts a platform of kind $47. The first one (ACTOR_07
 * zero) is the course maker: it works the course out from x $2A, y $1C0
 * to x $1000, starts four platforms along it, and from then on keeps the
 * course's live waypoint (COURSE_LIVE) at the screen's middle as the
 * play field scrolls. A platform it started (ACTOR_07 nonzero) goes on in
 * PlatformOnCourseStart.
 * HOW: The maker: speed $20, the waypoint step count 1, the course empty.
 * It steps the course slowing (PlatformCourseSlower, facing 0) while out
 * of a turn stretch and speeding (PlatformCourseFaster, facing 1) while
 * in one, keeping a waypoint every 38 steps, until its x pixel's high
 * byte is $10. Then the free list sorted for as many passes as half its
 * top offset (ActorFreeListSortHigh), ACTOR_07 to 1 for its children,
 * PlatformSpawnOnCourse four times from waypoint 6 ($30), COURSE_LIVE 18.
 * Each frame after its yield: with the play field's x at or past its x
 * before the move, COURSE_LIVE goes on one when the x plus $80 reaches
 * the next waypoint's x; else back one when the x plus $80 is below the
 * live waypoint's x.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; reached
 * as the kind's start. Exit: the maker never leaves (it yields each
 * frame); a platform it started: as PlatformOnCourseStart.
 */
void abi_PlatformCourseStart(void)
{
    uint8_t child = rd8(dpa(ACTOR_07));

    a8_load(child);
    if (child) {
        abi_PlatformOnCourseStart();
        return;
    }
    /* The course maker: from the start point, a waypoint every 38 steps,
       slowing out of the turn stretches and speeding through them, to
       x $1000. */
    wr8(dpa(COURSE_SPEED), 0x20);
    dp16_set(ACTOR_X + 1, COURSE_START_X);
    dp16_set(ACTOR_Y + 1, COURSE_START_Y);
    dp16_set(BASE_Y + 1, COURSE_START_Y);
    dp16_set(COURSE_END, 0);
    cpu.a = (uint16_t)((COURSE_START_Y & 0xFF00) | 0x01);
    nz8_of(0x01);
    wr8(dpa(COURSE_STEPS), 1);
    for (;;) {
        wr8(dpa(ACTOR_FACING), 0);
        for (;;) {
            uint8_t x_high = rd8(dpa(ACTOR_X + 2));
            a8_load(x_high);
            cmp8_of(x_high, COURSE_END_X_HIGH);
            if (x_high == COURSE_END_X_HIGH)
                goto worked_out;
            abi_PlatformCourseRecord();
            abi_PlatformCourseSlower();
            if (!(cpu.p & P_C))
                break;
        }
        uint8_t facing = (uint8_t)(rd8(dpa(ACTOR_FACING)) + 1);
        wr8(dpa(ACTOR_FACING), facing);
        nz8_of(facing);
        do {
            abi_PlatformCourseRecord();
            abi_PlatformCourseFaster();
        } while (!(cpu.p & P_C));
    }
worked_out:;
    /* Four platforms started six waypoints apart from waypoint 6 */
    uint8_t top = rd8(dba(ACTOR_FREE_TOP));
    a8_load((uint8_t)(top >> 1));
    flag_set(P_C, top & 1);
    abi_ActorFreeListSortHigh();
    wr8(dpa(ACTOR_07), (uint8_t)(rd8(dpa(ACTOR_07)) + 1));
    a8_load(6 * WAYPOINT_SIZE);
    dp16_set(COURSE_AT, 6 * WAYPOINT_SIZE);
    for (int i = 0; i < 4; i++)
        abi_PlatformSpawnOnCourse();
    a8_load(18);
    wr8(dba(COURSE_LIVE), 18);
    /* Each frame the live waypoint follows the screen's middle: on one
       when the play field scrolls right past the next waypoint, back one
       when it scrolls left before the live one. */
    for (;;) {
        op_sep(0x30);
        actor_yield(0x83C9CEu);
        op_rep(0x30);
        uint16_t scroll = rd16(dba(LAYER0 + LAYER_SCROLL_X));
        uint16_t live = rd16(dba(COURSE_LIVE));
        uint16_t at = (uint16_t)(live << 3);
        int right = scroll >= rd16(dba(SCROLL_X_BEFORE));
        uint16_t mid;

        cpu.x = at;
        flag_set(P_C, 0);
        mid = op_adc16(scroll, 0x0080, &cpu.p);
        cpu.a = mid;
        if (right) {
            uint16_t next_x = rd16(COURSE + WAYPOINT_SIZE + 1 + at);
            cmp16_of(mid, next_x);
            if (mid >= next_x) {
                wr16(dba(COURSE_LIVE), ++live);
                nz16_of(live);
            }
        } else {
            uint16_t live_x = rd16(COURSE + 1 + at);
            cmp16_of(mid, live_x);
            if (mid < live_x) {
                wr16(dba(COURSE_LIVE), --live);
                nz16_of(live);
            }
        }
    }
}

/*
 * @routine 83:CA75 PlatformOnCourseStart
 * @actor
 * WHAT IT DOES: Starts a platform the course maker put on the course:
 * nothing riding it, no dip, sprite bits $02 and $90, speed $20, its
 * height without the dip its y; then each frame it steps along the
 * course, slowing, until it reaches a turn stretch.
 * HOW: FIELD_21, DIRECTION, DIRECTION_FLIP and RIDDEN zeroed; PlatformDipClear;
 * LAST_DIRECTION $FF; ACTOR_08 2, ACTOR_09 $90; COURSE_SPEED $20; BASE_Y
 * the 24-bit y; facing 0. Then yield, PlatformPositionKeep and
 * PlatformCourseSlower each frame while C is set (out of a turn stretch).
 * In one it goes on at $83:CAA8, which no script reaches yet: its C is
 * not written, and it runs as the original's (jump_routine). (There it
 * asks for a direction flip each time it enters or leaves a stretch,
 * and PlatformCourseFollow can set its resume address there, $83:CAB0,
 * for direction 1; no script does either.)
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; jumped
 * to from PlatformCourseStart. Exit: never returns to its caller: it
 * yields each frame.
 */
void abi_PlatformOnCourseStart(void)
{
    uint16_t y;

    wr8(dpa(FIELD_21), 0);
    wr8(dpa(DIRECTION), 0);
    wr8(dpa(DIRECTION_FLIP), 0);
    wr8(dpa(RIDDEN), 0);
    abi_PlatformDipClear();
    wr8(dpa(LAST_DIRECTION), 0xFF);
    wr8(dpa(ACTOR_08), 0x02);
    wr8(dpa(ACTOR_09), 0x90);
    wr8(dpa(COURSE_SPEED), 0x20);
    /* its height without the dip: its y, all 24 bits */
    wr8(dpa(BASE_Y), rd8(dpa(ACTOR_Y)));
    y = dp16(ACTOR_Y + 1);
    dp16_set(BASE_Y + 1, y);
    a16_load(y);
    wr8(dpa(ACTOR_FACING), 0);
    do {
        actor_yield(0x83CA9Eu);
        abi_PlatformPositionKeep();
        abi_PlatformCourseSlower();
    } while (cpu.p & P_C);
    jump_routine(PLATFORM_IN_TURN);
}

/*
 * @routine 83:CA11 PlatformSpawnOnCourse
 * WHAT IT DOES: Starts one more platform on the course: a child of kind
 * $47 at the waypoint the course maker's offset ($37) names; the next is
 * six waypoints on.
 * HOW: ActorCreateChild with kind $47; the child's x and y (6 bytes) from
 * the waypoint, its own $37 the waypoint's offset; the maker's offset
 * moves on $30.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; D = the
 * course maker; called by JSR, four times, once the course is worked out.
 * Exit: M 8-bit, X 8-bit; Y = the child (low byte); A low = the new
 * offset's low byte.
 */
void abi_PlatformSpawnOnCourse(void)
{
    uint16_t child, at;

    a8_load(0x47);
    abi_ActorCreateChild();
    /* the child at the waypoint the next platform starts at; the next one
       6 waypoints on */
    op_rep(P_M);
    child = cpu.x;
    cpu.y = child;
    at = dp16(0x37);
    cpu.x = at;
    wr16(dba((uint16_t)(0x000B + child)), rd16(0x7F5000u + at));
    wr16(dba((uint16_t)(0x000D + child)), rd16(0x7F5002u + at));
    wr16(dba((uint16_t)(0x000F + child)), rd16(0x7F5004u + at));
    wr16(dba((uint16_t)(0x0037 + child)), at);
    flag_set(P_C, 0);
    cpu.a = op_adc16(at, 0x0030, &cpu.p);
    dp16_set(0x37, cpu.a);
    op_sep(0x30);
}


/*
 * @routine 83:CAC1 PlatformDipStart
 * WHAT IT DOES: Starts the platform's dip as the player lands on it: sound
 * A kept for it, dip phase 1, the dip back to 0.
 * HOW: A to $17, the phase to 1, then on into PlatformDipClear.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; A = the
 * sound command; called by JSR. Exit: as PlatformDipClear; A low = 1.
 */
void abi_PlatformDipStart(void)
{
    wr8(dpa(0x17), (uint8_t)cpu.a);
    a8_load(0x01);
    wr8(dpa(0x13), 0x01);
    abi_PlatformDipClear();
}


/*
 * @routine 83:CB24 PlatformCourseStep
 * WHAT IT DOES: Moves the platform a step along its direction at its
 * course speed, while the course is worked out; the step's y goes to its
 * height without the dip.
 * HOW: The height without the dip ($4D-$4F) put in its y; the direction's
 * 6-byte step (8 bytes a direction at $D304) to $000C-$0012;
 * ActorMoveScaled with the course speed; the new y kept back at $4D-$4F.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
 * JSR (by the slowing and speeding steps at 83:CAFC and 83:CB10). Exit: M
 * 8-bit, X 8-bit (as ActorMoveScaled); A low = the y's top byte.
 */
void abi_PlatformCourseStep(void)
{
    uint8_t x;

    /* from its y without the dip, a step in its direction at its speed */
    wr8(dpa(0x0E), rd8(dpa(0x4D)));
    wr8(dpa(0x0F), rd8(dpa(0x4E)));
    wr8(dpa(0x10), rd8(dpa(0x4F)));
    x = (uint8_t)(rd8(dpa(0x14)) << 3);
    flag_set(P_C, rd8(dpa(0x14)) & 0x20);
    x8_load(x);
    wr8(dba(SCRATCH), rd8(dba((uint16_t)(0xD304 + x))));
    wr8(dba(0x000D), rd8(dba((uint16_t)(0xD305 + x))));
    wr8(dba(0x000E), rd8(dba((uint16_t)(0xD306 + x))));
    wr8(dba(0x0010), rd8(dba((uint16_t)(0xD307 + x))));
    wr8(dba(0x0011), rd8(dba((uint16_t)(0xD308 + x))));
    wr8(dba(V_0012), rd8(dba((uint16_t)(0xD309 + x))));
    a8_load(rd8(dpa(0x20)));
    abi_ActorMoveScaled();
    wr8(dpa(0x4D), rd8(dpa(0x0E)));
    wr8(dpa(0x4E), rd8(dpa(0x0F)));
    a8_load(rd8(dpa(0x10)));
    wr8(dpa(0x4F), (uint8_t)cpu.a);
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
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
 * JSR from the platform's wait routine (83:CAD1). Exit: M 8-bit, X 8-bit;
 * A, X and Y as left by the last step taken.
 */
void abi_PlatformCourseFollow(void)
{
    uint16_t x, at, step;
    uint8_t dir;

    op_rep(0x30);
    /* the camera past the next waypoint's x: on to it */
    cpu.x = x = dp16(0x37);
    cpu.a = at = dp16(0x0C);
    cmp16_of(at, rd16(0x7F5009u + x));
    if (at < rd16(0x7F5009u + x))
        goto out;
    flag_set(P_C, 0);
    dp16_set(0x37, op_adc16(x, 0x0008, &cpu.p));
    /* within this course's x range ($0400 picks it): nothing more */
    cpu.x = x = (uint16_t)(rd16(dba(0x0400)) << 3);
    flag_set(P_C, rd16(dba(0x0400)) & 0x2000);
    cpu.a = at = dp16(0x0C);
    cmp16_of(at, rd16(0x7F4FA1u + x));
    if (at >= rd16(0x7F4FA1u + x)) {
        cmp16_of(at, rd16(0x7F5061u + x));
        if (at < rd16(0x7F5061u + x))
            goto out;
    }
    /* the platform to the waypoint 24 behind or ahead of the camera */
    step = 0x00C0;
    cpu.y = at;
    cmp16_of(at, rd16(dba(0x1A44)));
    if (at >= rd16(dba(0x1A44)))
        step = 0xFF40;
    flag_set(P_C, 0);
    cpu.a = at = op_adc16(step, dp16(0x37), &cpu.p);
    if (at & 0x8000)
        goto out;
    dp16_set(0x37, at);
    cpu.x = x = at;
    dp16_set(0x0B, rd16(0x7F5000u + x));
    dp16_set(0x0D, rd16(0x7F5002u + x));
    cpu.a = rd16(0x7F5004u + x);
    dp16_set(0x0F, cpu.a);
    dp16_set(0x4E, cpu.a);
    op_sep(P_M);
    wr8(dpa(0x4D), rd8(dpa(0x0E)));
    wr8(dpa(0x20), rd8(0x7F5007u + x));
    dir = rd8(0x7F5006u + x);
    op_sep(P_X);
    wr8(dpa(0x40), dir);
    wr8(dpa(0x14), dir);
    flag_set(P_C, dir & 0x80);
    x8_load((uint8_t)(dir << 1));
    wr8(dpa(0x03), rd8(dba((uint16_t)(0xD312 + cpu.x))));
    wr8(dpa(0x04), rd8(dba((uint16_t)(0xD313 + cpu.x))));
    a8_load(0xFF);
    wr8(dpa(0x1C), 0xFF);
    wr8(dpa(0x42), 0);
    wr8(dpa(0x13), 0);
    abi_PlatformDipClear();
    inc8_at(dpa(0x16));
out:
    op_sep(0x30);
}


/*
 * @routine 83:CC42 PlatformDipLand
 * WHAT IT DOES: Dip phase 1: the platform is landed on: starts a child
 * (hit box $4B) that keeps the platform as its $37, plays the landing's
 * sound when the platform can be hit, and moves on to the next phase.
 * HOW: ActorSpawnFacingY with hit box $4B, facing 0 and the platform's
 * $40; started: the platform's address (TDC) to the child's $37-$38; bit 6
 * of $09: SoundQueueAdd with the sound kept at $17. The phase steps up.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
 * JSR through the dip phase's table. Exit: M 8-bit; a child started: X
 * 8-bit; none: X as ActorSpawnFacingY left it.
 */
void abi_PlatformDipLand(void)
{
    a8_load(0x4B);
    x8_load(0x00);
    y8_load(rd8(dpa(0x40)));
    abi_ActorSpawnFacingY();
    if (!(cpu.p & P_N)) {
        /* the child's parent, and the sound when on screen */
        a16_load(cpu.d);
        wr8(dba((uint16_t)(0x0037 + cpu.x)), (uint8_t)cpu.d);
        a8_load((uint8_t)(cpu.d >> 8));
        cpu.a = (uint16_t)((cpu.d << 8) | (cpu.d >> 8));
        wr8(dba((uint16_t)(0x0038 + cpu.x)), (uint8_t)cpu.a);
        op_sep(P_X);
        bit8_of((uint8_t)cpu.a, rd8(dpa(0x09)));
        if (cpu.p & P_V) {
            a8_load(rd8(dpa(0x17)));
            abi_SoundQueueAdd();
        }
    }
    inc8_at(dpa(0x13));
}


/*
 * @routine 83:CC65 PlatformDipSink
 * WHAT IT DOES: Dip phase 2: the platform sinks; at 11 pixels down, on to
 * the next phase.
 * HOW: PlatformDipStep with the sinking rate (X = 0); the dip's pixels
 * (16-bit) 11 or more: the phase steps up (a 16-bit increment of $13).
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
 * JSR through the dip phase's table. Exit: M 8-bit; X = 0; A = the dip's
 * pixels; C from the comparison.
 */
void abi_PlatformDipSink(void)
{
    x8_load(0x00);
    abi_PlatformDipStep();
    op_rep(P_M);
    a16_load(dp16(0x4B));
    cmp16_of(cpu.a, 0x000B);
    if (cpu.a >= 0x000B)
        inc16_at(dpa(0x13));
    op_sep(P_M);
}


/*
 * @routine 83:CC79 PlatformDipRise
 * WHAT IT DOES: Dip phase 3: the platform rises; under 3 pixels down, on
 * to the next phase (waiting for the player to step off).
 * HOW: PlatformDipStep with the rising rate (X = 3); the dip's pixels
 * (16-bit) below 3: the phase steps up (a 16-bit increment of $13).
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
 * JSR through the dip phase's table. Exit: M 8-bit; X = 3; A = the dip's
 * pixels; C from the comparison.
 */
void abi_PlatformDipRise(void)
{
    x8_load(0x03);
    abi_PlatformDipStep();
    op_rep(P_M);
    a16_load(dp16(0x4B));
    cmp16_of(cpu.a, 0x0003);
    if (cpu.a < 0x0003)
        inc16_at(dpa(0x13));
    op_sep(P_M);
}


/*
 * @routine 83:CC94 PlatformDipSinkAgain
 * WHAT IT DOES: Dip phase 5: once the player is off, the platform sinks
 * again; at 11 pixels down, on to the last phase.
 * HOW: As PlatformDipSink: PlatformDipStep with X = 0; 11 or more: the
 * phase steps up (a 16-bit increment of $13).
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
 * JSR through the dip phase's table. Exit: M 8-bit; X = 0; A = the dip's
 * pixels; C from the comparison.
 */
void abi_PlatformDipSinkAgain(void)
{
    x8_load(0x00);
    abi_PlatformDipStep();
    op_rep(P_M);
    a16_load(dp16(0x4B));
    cmp16_of(cpu.a, 0x000B);
    if (cpu.a >= 0x000B)
        inc16_at(dpa(0x13));
    op_sep(P_M);
}


/*
 * @routine 83:CCA8 PlatformDipRiseEnd
 * WHAT IT DOES: Dip phase 6: the platform rises back; once past its
 * height, the dip is zeroed and the phase back to 0 (idle).
 * HOW: PlatformDipStep with the rising rate (X = 3); the dip's top byte
 * negative: its three bytes and the phase zeroed.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
 * JSR through the dip phase's table. Exit: M 8-bit; X = 3; A low = the
 * dip's top byte.
 */
void abi_PlatformDipRiseEnd(void)
{
    x8_load(0x03);
    abi_PlatformDipStep();
    a8_load(rd8(dpa(0x4C)));
    if ((uint8_t)cpu.a & 0x80) {
        /* risen past level: done */
        wr8(dpa(0x4A), 0);
        wr8(dpa(0x4B), 0);
        wr8(dpa(0x4C), 0);
        wr8(dpa(0x13), 0);
    }
}

/*
 * @routine 83:CAFC PlatformCourseSlower
 * WHAT IT DOES: A step of the course while it is worked out, slowing: the
 * course speed down by 1 (not below $20), the platform moved a step, and
 * whether it is in a turn stretch.
 * HOW: The speed less 1, held at $20 at least; PlatformCourseStep;
 * PlatformTurnZoneTest.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
 * JSR. Exit: as PlatformTurnZoneTest: C clear in a turn stretch.
 */
void abi_PlatformCourseSlower(void)
{
    uint8_t speed;

    /* down one, not below $20 */
    flag_set(P_C, 1);
    speed = op_sbc8(rd8(dpa(0x20)), 0x01, &cpu.p);
    cmp8_of(speed, 0x20);
    if (speed < 0x20)
        speed = 0x20;
    a8_load(speed);
    wr8(dpa(0x20), speed);
    abi_PlatformCourseStep();
    abi_PlatformTurnZoneTest();
}


/*
 * @routine 83:CB10 PlatformCourseFaster
 * WHAT IT DOES: A step of the course while it is worked out, speeding: the
 * course speed up by 1 (not above $80), the platform moved a step, and
 * whether it is in a turn stretch.
 * HOW: The speed plus 1, held at $80 at most; PlatformCourseStep;
 * PlatformTurnZoneTest.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
 * JSR. Exit: as PlatformTurnZoneTest: C clear in a turn stretch.
 */
void abi_PlatformCourseFaster(void)
{
    uint8_t speed;

    /* up one, not above $80 */
    flag_set(P_C, 0);
    speed = op_adc8(rd8(dpa(0x20)), 0x01, &cpu.p);
    cmp8_of(speed, 0x80);
    if (speed >= 0x80)
        speed = 0x80;
    a8_load(speed);
    wr8(dpa(0x20), speed);
    abi_PlatformCourseStep();
    abi_PlatformTurnZoneTest();
}


/*
 * @routine 83:CC19 PlatformDipUpdate
 * WHAT IT DOES: Runs the platform's dip for the frame: its phase's
 * routine, then its y set to its height without the dip plus the dip.
 * HOW: JSR through the dip phase table at $83:CC34 by the phase; then the
 * 24-bit sum of BASE_Y and DIP, a byte at a time, to its y.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
 * JSR from the platform's wait routine (83:CAD1). Exit: M 8-bit; A low =
 * the y's top byte; X as the phase routine left it; flags from the last
 * add.
 */
void abi_PlatformDipUpdate(void)
{
    uint8_t phase = rd8(dpa(0x13));

    /* the dip phase's step; then its y = its base y plus the dip */
    a8_load(phase);
    flag_set(P_C, phase & 0x80);
    x8_load((uint8_t)(phase << 1));
    call_routine(0x830000u | rd16(0x830000u | (uint16_t)(0xCC34 + cpu.x)), 0);
    flag_set(P_C, 0);
    wr8(dpa(0x0E), op_adc8(rd8(dpa(0x4D)), rd8(dpa(0x4A)), &cpu.p));
    wr8(dpa(0x0F), op_adc8(rd8(dpa(0x4E)), rd8(dpa(0x4B)), &cpu.p));
    a8_load(op_adc8(rd8(dpa(0x4F)), rd8(dpa(0x4C)), &cpu.p));
    wr8(dpa(0x10), (uint8_t)cpu.a);
}


