/*
 * carpet.c: the magic carpet ride's steering: the joypad's eight
 * directions push the speeds, the carpet tilts when it climbs or dives,
 * and with no direction held it drifts back to its cruising speed and
 * levels off. Two rides use it: the one whose record has $06 = 1 and
 * another (the "Alt" routines), each with its own rates and limits.
 * Drafted by cdraft.py; named and described by hand.
 *
 * A steering direction ($14) is 0 right, then clockwise to 7 (2 down, 4
 * left, 6 up); 8 slows down across, 9 speeds up, 10 neither; negative
 * none. The rate tables (in bank $80) give per direction an across rate
 * and an up-down rate, signed by bit 7 (set: take away).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "cpu_ops.h"
#include "native.h"
#include "actor.h"

void abi_PlayerMoveStart(void);

/* The ride's own fields of the rider's record */
#define CARPET_DIRECTION 0x14u     /* the steering direction (see above) */
#define CARPET_DIVING 0x41u        /* nonzero: tilted to dive (move 3) */
#define CARPET_CLIMBING 0x42u      /* nonzero: tilted to climb (move 1) */
#define CARPET_SPED_UP 0x44u       /* nonzero: it has sped up to cruise (direction 9) */
#define CARPET_SLOWED 0x45u        /* nonzero: it has slowed to cruise (direction 8) */

/* In bank $80: per direction, the across and the up-down rate (bytes) */
#define CARPET_RATES 0xC7B5u       /* the ride with $06 = 1 */
#define CARPET_RATES_ALT 0xC7CBu   /* the other ride */
/* In bank $80: per direction, 0 level, positive dive, negative climb */
#define CARPET_TILT 0xC7E1u

/* The 24-bit speed at `at` up or down by a rate: bit 7 set, down by the
   other 7 bits (kept in $002C). */
static void speed_step(uint16_t at, uint8_t rate)
{
    if (rate & 0x80) {
        rate &= 0x7F;
        wr8(dba(0x002C), rate);
        flag_set(P_C, 1);
        wr8(dpa(at), op_sbc8(rd8(dpa(at)), rate, &cpu.p));
        wr8(dpa(at + 1), op_sbc8(rd8(dpa(at + 1)), 0x00, &cpu.p));
        a8_load(op_sbc8(rd8(dpa(at + 2)), 0x00, &cpu.p));
    } else {
        flag_set(P_C, 0);
        wr8(dpa(at), op_adc8(rate, rd8(dpa(at)), &cpu.p));
        wr8(dpa(at + 1), op_adc8(rd8(dpa(at + 1)), 0x00, &cpu.p));
        a8_load(op_adc8(rd8(dpa(at + 2)), 0x00, &cpu.p));
    }
    wr8(dpa(at + 2), (uint8_t)cpu.a);
}

/* PlayerMoveStart with move `move`. */
static void move_start(uint8_t move)
{
    a8_load(move);
    abi_PlayerMoveStart();
}

/*
 * @routine 81:E555 CarpetLevelOut
 * WHAT IT DOES: Levels the carpet off when it is tilted: out of a dive
 * (move 4) or out of a climb (move 2).
 * HOW: The dive flag set: cleared, PlayerMoveStart with move 4. Else the
 * climb flag set: cleared, PlayerMoveStart with move 2.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Also reached by
 * JMP from CarpetTilt. Exit: not tilted: A low = 0, Z set; levelled: as
 * PlayerMoveStart. M 8-bit, X 8-bit.
 */
void abi_CarpetLevelOut(void)
{
    a8_load(rd8(dpa(CARPET_DIVING)));
    if ((uint8_t)cpu.a) {
        wr8(dpa(CARPET_DIVING), 0);
        move_start(0x04);
        return;
    }
    a8_load(rd8(dpa(CARPET_CLIMBING)));
    if ((uint8_t)cpu.a) {
        wr8(dpa(CARPET_CLIMBING), 0);
        move_start(0x02);
    }
}


/*
 * @routine 81:E56D CarpetTilt
 * WHAT IT DOES: Tilts the carpet the way it is steered: diving (move 3)
 * when steered down, climbing (move 1) when steered up; level steering
 * levels it off.
 * HOW: The tilt table by the direction: 0, on into CarpetLevelOut;
 * positive, unless already diving, the dive flag set, the climb flag
 * cleared and PlayerMoveStart with move 3; negative, the same for a climb
 * with move 1.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). The direction
 * 0-7. Exit: steered up or down: A low = 1, except a new dive: as
 * PlayerMoveStart; level: as CarpetLevelOut. M 8-bit, X 8-bit.
 */
void abi_CarpetTilt(void)
{
    uint8_t tilt;

    x8_load(rd8(dpa(CARPET_DIRECTION)));
    a8_load(tilt = rd8(dba(CARPET_TILT) + cpu.x));
    if (!tilt) {
        abi_CarpetLevelOut();
        return;
    }
    if (!(tilt & 0x80)) {
        /* down: a new dive */
        a8_load(rd8(dpa(CARPET_DIVING)));
        if (!(uint8_t)cpu.a) {
            inc8_at(dpa(CARPET_DIVING));
            wr8(dpa(CARPET_CLIMBING), 0);
            move_start(0x03);
            return;
        }
    } else {
        /* up: a new climb */
        a8_load(rd8(dpa(CARPET_CLIMBING)));
        if (!(uint8_t)cpu.a) {
            inc8_at(dpa(CARPET_CLIMBING));
            wr8(dpa(CARPET_DIVING), 0);
            move_start(0x01);
        }
    }
    a8_load(0x01);
}


/* The x speed by the direction's rate in `rates`. */
static void steer_x(uint16_t rates)
{
    uint8_t dir = rd8(dpa(CARPET_DIRECTION));

    a8_load(dir);
    if (dir & 0x80)
        return;
    flag_set(P_C, dir & 0x80);
    x8_load((uint8_t)(dir << 1));
    a8_load(rd8(dba(rates) + cpu.x));
    speed_step(0x19, (uint8_t)cpu.a);
}

/*
 * @routine 81:E597 CarpetSteerXAlt
 * WHAT IT DOES: Pushes the other ride's speed across by its rate for the
 * steering direction.
 * HOW: No direction (negative): nothing. Else the across rate from the
 * other ride's table, then on in CarpetSteerX: added to the 24-bit speed,
 * or taken away when bit 7 is set.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: as
 * CarpetSteerX.
 */
void abi_CarpetSteerXAlt(void)
{
    steer_x(CARPET_RATES_ALT);
}


/*
 * @routine 81:E5A4 CarpetSteerX
 * WHAT IT DOES: Pushes the ride's speed across by its rate for the
 * steering direction.
 * HOW: No direction (negative): nothing. Else the across rate from the
 * table: bit 7 clear, added to the 24-bit speed; set, its low 7 bits (kept
 * at $002C) taken away. A byte at a time.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
 * 8-bit; X = the direction times 2; A low = the speed's top byte; flags
 * from the last add or subtract. No direction: A low = the direction, N
 * set.
 */
void abi_CarpetSteerX(void)
{
    steer_x(CARPET_RATES);
}


/* Cruise: direction 9 (speed up) below `top`, else 8 (slow). */
static void cruise_x(uint16_t top)
{
    uint16_t speed;

    op_rep(P_M);
    speed = dp16(0x19);
    cpu.a = speed;
    cmp16_of(speed, top);
    op_sep(P_M);
    if (speed >= top) {
        a8_load(0x08);
        wr8(dpa(CARPET_DIRECTION), 0x08);
        wr8(dpa(CARPET_SLOWED), 0x08);
    } else {
        a8_load(0x09);
        wr8(dpa(CARPET_DIRECTION), 0x09);
        wr8(dpa(CARPET_SPED_UP), 0x09);
    }
}

/*
 * @routine 81:E62F CarpetCruiseXAlt
 * WHAT IT DOES: With no direction held, steers the other ride back toward
 * its cruising speed across, once each way.
 * HOW: Once it has both sped up and slowed down to cruise: direction 10
 * (neither). Else the speed (16-bit, from its fraction byte) below $0181:
 * direction 9 and the sped-up flag; at or above: direction 8 and the
 * slowed flag (the two ends of CarpetCruiseX).
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
 * 8-bit; A low = the direction set.
 */
void abi_CarpetCruiseXAlt(void)
{
    a8_load(rd8(dpa(CARPET_SPED_UP)));
    if ((uint8_t)cpu.a) {
        a8_load(rd8(dpa(CARPET_SLOWED)));
        if ((uint8_t)cpu.a) {
            /* both done: steady */
            a8_load(0x0A);
            wr8(dpa(CARPET_DIRECTION), 0x0A);
            return;
        }
    }
    cruise_x(0x0181);
}


/*
 * @routine 81:E643 CarpetCruiseX
 * WHAT IT DOES: With no direction held, steers the ride toward its
 * cruising speed across.
 * HOW: The speed (16-bit, from its fraction byte) below $0300: direction 9
 * (speed up) and the sped-up flag set to 9; else direction 8 (slow down)
 * and the slowed flag set to 8. After its end, direction 10 for
 * CarpetCruiseXAlt.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
 * 8-bit; A low = the direction set; C from the comparison.
 */
void abi_CarpetCruiseX(void)
{
    cruise_x(0x0300);
}


/*
 * @routine 81:E664 CarpetSteadyY
 * WHAT IT DOES: Steadies the carpet up and down: steers against its
 * vertical speed until it is small, then stops it.
 * HOW: The vertical speed (16-bit, from its fraction byte): 7 or more
 * down, direction 6 (up); below -8, direction 2 (down); between, the speed
 * zeroed (all three bytes) and no direction ($FF).
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
 * 8-bit; A low = the direction set.
 */
void abi_CarpetSteadyY(void)
{
    uint16_t speed;

    op_rep(P_M);
    a16_load(speed = dp16(0x1D));
    if (speed & 0x8000) {
        /* rising faster than 8/256: steer down */
        cmp16_of(speed, 0xFFF8);
        if (speed < 0xFFF8) {
            op_sep(P_M);
            a8_load(0x02);
            wr8(dpa(CARPET_DIRECTION), 0x02);
            return;
        }
    } else if (speed) {
        /* falling faster than 7/256: steer up */
        cmp16_of(speed, 0x0007);
        if (speed >= 0x0007) {
            op_sep(P_M);
            a8_load(0x06);
            wr8(dpa(CARPET_DIRECTION), 0x06);
            return;
        }
    }
    /* near still: stopped, no steering */
    dp16_set(0x1D, 0);
    op_sep(P_M);
    wr8(dpa(0x1F), 0);
    a8_load(0xFF);
    wr8(dpa(CARPET_DIRECTION), 0xFF);
}


/* The x speed kept within $0020 and `top`. */
static void limit_x(uint16_t top)
{
    uint16_t speed;

    op_rep(P_M);
    a16_load(speed = dp16(0x19));
    cmp16_of(speed, top);
    if (speed >= top) {
        a16_load(top);
        dp16_set(0x19, top);
    } else {
        cmp16_of(speed, 0x0020);
        if (speed < 0x0020) {
            a16_load(0x0020);
            dp16_set(0x19, 0x0020);
        }
    }
    op_sep(P_M);
}

/*
 * @routine 81:E68F CarpetSpeedXLimitAlt
 * WHAT IT DOES: Keeps the other ride's speed across between its least and
 * most: $0020 and $0380 (an eighth of a pixel and three and a half pixels
 * a frame).
 * HOW: The speed's fraction byte and pixels as a 16-bit word, compared
 * unsigned and set to the limit it passes.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
 * 8-bit; A = the speed word as limited, or as it was; C from the last
 * comparison.
 */
void abi_CarpetSpeedXLimitAlt(void)
{
    limit_x(0x0380);
}


/*
 * @routine 81:E6AE CarpetSpeedXLimit
 * WHAT IT DOES: Keeps the ride's speed across between its least and most:
 * $0020 and $0480 (an eighth of a pixel and four and a half pixels a
 * frame).
 * HOW: The speed's fraction byte and pixels as a 16-bit word, compared
 * unsigned and set to the limit it passes.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
 * 8-bit; A = the speed word as limited, or as it was; C from the last
 * comparison.
 */
void abi_CarpetSpeedXLimit(void)
{
    limit_x(0x0480);
}

/* The y speed by the direction's rate in `rates`; none: CarpetSteadyY
   first. */
static void steer_y(uint16_t rates)
{
    uint8_t dir = rd8(dpa(0x14));

    a8_load(dir);
    if (dir & 0x80)
        return;
    flag_set(P_C, 0);
    x8_load((uint8_t)(dir << 1));
    a8_load(rd8(dba(rates) + cpu.x));
    if (!(uint8_t)cpu.a) {
        abi_CarpetSteadyY();
        a8_load(rd8(dba(rates) + cpu.x));
    }
    speed_step(0x1D, (uint8_t)cpu.a);
}

/*
 * @routine 81:E5DB CarpetSteerYAlt
 * WHAT IT DOES: Pushes the other ride's vertical speed by its rate for the
 * steering direction.
 * HOW: No direction (negative): nothing. Else the up-down rate from the
 * other ride's table; a rate of 0: CarpetSteadyY, then the same rate read
 * again; then on in CarpetSteerY: added to the 24-bit speed, or taken away
 * when bit 7 is set.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: as
 * CarpetSteerY.
 */
void abi_CarpetSteerYAlt(void)
{
    steer_y(0xC7CC);
}


/*
 * @routine 81:E5F0 CarpetSteerY
 * WHAT IT DOES: Pushes the ride's vertical speed by its rate for the
 * steering direction.
 * HOW: No direction (negative): nothing. Else the up-down rate from the
 * table; a rate of 0: CarpetSteadyY, then the same rate read again. Bit 7
 * clear, added to the 24-bit speed; set, its low 7 bits (kept at $002C)
 * taken away. A byte at a time.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
 * 8-bit; X = the direction times 2; A low = the speed's top byte; flags
 * from the last add or subtract. No direction: A low = the direction, N
 * set.
 */
void abi_CarpetSteerY(void)
{
    steer_y(0xC7B6);
}

/*
 * @routine 81:E503 CarpetCruiseSet
 * WHAT IT DOES: Sets the carpet's speed across to cruise: $0300 for the
 * ride with $06 = 1, $0180 for the other.
 * HOW: The record's $06 less 1: zero, the speed's three bytes 0, 3, 0;
 * else $80, 1, 0.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). The game calls
 * it only from the other ride's branch (81:E4EC): the $0300 setting is
 * reached only by unit_carpet_cruise_check.box. Exit: M 8-bit; A low = the
 * speed's pixel byte.
 */
void abi_CarpetCruiseSet(void)
{
    /* the ride with $06 = 1 cruises at 3, the other at 1.5 */
    a8_load((uint8_t)(rd8(dpa(0x06)) - 1));
    if (!(uint8_t)cpu.a) {
        wr8(dpa(0x19), 0x00);
        a8_load(0x03);
    } else {
        wr8(dpa(0x19), 0x80);
        a8_load(0x01);
    }
    wr8(dpa(0x1A), (uint8_t)cpu.a);
    wr8(dpa(0x1B), 0x00);
}


/*
 * @routine 81:E6CD CarpetSpeedYLimit
 * WHAT IT DOES: Keeps the carpet's vertical speed within its limits: for
 * the ride with $06 = 1 from -$01C0 up to $01E0 down; for the other from
 * -$0240 to $0280.
 * HOW: The speed (16-bit, from its fraction byte) signed: down past the
 * limit, or up past the other, set to it.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit; A
 * as the speed left or set.
 */
void abi_CarpetSpeedYLimit(void)
{
    uint16_t speed, top, least;

    /* within -$1C0..$1E0 on the ride with $06 = 1, -$240..$280 on the
       other */
    a8_load((uint8_t)(rd8(dpa(0x06)) - 1));
    top = (uint8_t)cpu.a ? 0x0280 : 0x01E0;
    least = (uint8_t)cpu.a ? 0xFDC0 : 0xFE40;
    op_rep(P_M);
    a16_load(speed = dp16(0x1D));
    if (speed & 0x8000) {
        cmp16_of(speed, least);
        if (speed < least) {
            a16_load(least);
            dp16_set(0x1D, least);
        }
    } else {
        cmp16_of(speed, top);
        if (speed >= top) {
            a16_load(top);
            dp16_set(0x1D, top);
        }
    }
    op_sep(P_M);
}

