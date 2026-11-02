/*
 * air_steer.c: the player's speed across while it is in the air, and a
 * few small player settings. Drafted by cdraft.py; named and described by
 * hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram_player.h"

void abi_ActorAccelerateX(void);
void abi_ActorDecelerateX(void);

/* Speed tables (in the data bank, $80), by direction and dash: the
   accelerations toward and against the facing, the speeds they stop at,
   and the speed the player settles to with no direction */
#define AIR_ACCEL_TOWARD 0xC781
#define AIR_ACCEL_AGAINST 0xC787
#define AIR_TOP_TOWARD 0xC78A
#define AIR_LEAST_AGAINST 0xC796
#define AIR_SETTLE 0xC7A2

/* The table entry for the held direction's speed, by the dash: twice
   `at` plus the dash twice. */
static uint8_t by_dash(uint8_t at)
{
    uint8_t dash = rd8(dba(PLAYER_DASH));

    flag_set(P_C, 0);
    at = op_adc8(at, dash, &cpu.p);
    flag_set(P_C, 0);
    return op_adc8(at, dash, &cpu.p);
}

/*
 * @routine 81:CF2E PlayerAirSteer
 * WHAT IT DOES: Steers the player across while it is in the air: held
 * toward its facing it speeds up, against it slows down, each to a limit;
 * with no direction its speed settles to a set speed.
 * HOW: The direction bits come from V_1434 (when V_1428 is set; 0 there
 * reads as 2) or V_0012. Bits 0-2 at 0, 2 or 3: no direction; the speed
 * is brought to the settle speed of the dash row (ActorAccelerateX by $28
 * when below, ActorDecelerateX by 8 when above, not under $0180). Else
 * bit 6 against the facing at $14 picks: the same way, ActorAccelerateX
 * by the row's rate, the speed cut to the row's top; the other way,
 * ActorDecelerateX by the row's rate, the speed raised to the row's
 * least. The rows are by the direction's low two bits and PLAYER_DASH.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80. Exit: M and
 * X 8-bit; A, X, Y and the flags as the path taken leaves them.
 */
void abi_PlayerAirSteer(void)
{
    uint8_t v, dir, low, toward;
    uint16_t limit, speed;

    /* the direction held: the pad's, or the scripted one */
    v = rd8(dba(V_1428));
    a8_load(v);
    if (v) {
        x8_load(rd8(dba(V_1434)));
        if (!cpu.x)
            x8_load(0x02);
    } else {
        x8_load(rd8(dba(V_0012)));
    }
    dir = (uint8_t)cpu.x;
    low = dir & 0x07;
    a8_load(low);
    if (!low)
        goto settle;
    cmp8_of(low, 0x02);
    if (low == 0x02)
        goto settle;
    cmp8_of(low, 0x03);
    if (low == 0x03)
        goto settle;
    flag_set(P_C, dir & 0x40);
    a8_load((uint8_t)(((dir >> 7) & 1) ^ rd8(dpa(ACTOR_FACING))));
    toward = (uint8_t)cpu.a;
    if (toward) {
        /* held his way: faster, to the top speed */
        a8_load((uint8_t)((dir & 0x03) << 1));
        flag_set(P_C, 0);
        pha8();
        cpu.y = op_adc8((uint8_t)cpu.a, rd8(dba(PLAYER_DASH)), &cpu.p);
        a8_load(rd8(dba(AIR_ACCEL_TOWARD) + cpu.y));
        wr8(dpa(ACTOR_ACCEL_X), (uint8_t)cpu.a);
        abi_ActorAccelerateX();
        pla8();
        cpu.x = by_dash((uint8_t)((uint8_t)cpu.a << 1));
        limit = rd16(dba(AIR_TOP_TOWARD) + cpu.x);
        speed = dp16(ACTOR_SPEED_X);
        cpu.a = limit;
        cmp16_of(limit, speed);
        if (limit < speed)
            dp16_set(ACTOR_SPEED_X, limit);
        op_sep(P_M);
        return;
    }
    /* held the other way: slower, to the least */
    a8_load(dir & 0x03);
    pha8();
    cpu.y = dir & 0x03;
    a8_load(rd8(dba(AIR_ACCEL_AGAINST) + cpu.y));
    wr8(dpa(ACTOR_ACCEL_X), (uint8_t)cpu.a);
    abi_ActorDecelerateX();
    pla8();
    cpu.x = by_dash((uint8_t)((uint8_t)cpu.a << 2));
    limit = rd16(dba(AIR_LEAST_AGAINST) + cpu.x);
    speed = dp16(ACTOR_SPEED_X);
    cpu.a = limit;
    cmp16_of(limit, speed);
    if (limit >= speed)
        dp16_set(ACTOR_SPEED_X, limit);
    op_sep(P_M);
    return;
settle:
    /* no way held: the speed settles toward its dash's speed */
    op_rep(P_X);
    v = rd8(dba(PLAYER_DASH));
    flag_set(P_C, v & 0x80);
    cpu.a = (uint8_t)(v << 1);
    cpu.y = cpu.a;
    cpu.x = rd16(dba(AIR_SETTLE) + cpu.y);
    speed = dp16(ACTOR_SPEED_X);
    cmp16_of(cpu.x, speed);
    if (cpu.x != speed) {
        if (cpu.x > speed) {
            dp16_set(ACTOR_SPEED_X, cpu.x);
            a8_load(0x28);
            wr8(dpa(ACTOR_ACCEL_X), 0x28);
            abi_ActorAccelerateX();
        } else {
            a8_load(0x08);
            wr8(dpa(ACTOR_ACCEL_X), 0x08);
            abi_ActorDecelerateX();
            cpu.x = 0x0180;
            speed = dp16(ACTOR_SPEED_X);
            cmp16_of(0x0180, speed);
            if (0x0180 >= speed)
                dp16_set(ACTOR_SPEED_X, 0x0180);
        }
    }
    op_sep(P_X);
}

/*
 * @routine 81:D855 PlayerSpeedLeast
 * WHAT IT DOES: Keeps the player moving across at three quarters of a
 * pixel a frame or more, counting the frames it had to.
 * HOW: A speed under $00C0 becomes $00C0 and PLAYER_SLOWED steps up.
 * Entry: M 8-bit; D = the player's record; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: M 8-bit; A = the speed (or $00C0); N, Z, C from the
 * compare, or from the count's step; X, Y unchanged.
 */
void abi_PlayerSpeedLeast(void)
{
    uint16_t speed = dp16(ACTOR_SPEED_X);

    cpu.a = speed;
    cmp16_of(speed, 0x00C0);
    if (speed < 0x00C0) {                 /* held to $C0, counted */
        inc16_at(dba(PLAYER_SLOWED));
        a16_load(0x00C0);
        dp16_set(ACTOR_SPEED_X, 0x00C0);
    }
}

/*
 * @routine 81:DACE PlayerPosRestore
 * WHAT IT DOES: Puts the player back at the position kept for it.
 * HOW: Its X and Y pixels from PLAYER_POS_SAVED.
 * Entry: M 8-bit; D = the player's record; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: M 8-bit; A = the Y; N, Z from it; X, Y unchanged.
 */
void abi_PlayerPosRestore(void)
{
    dp16_set(ACTOR_X + 1, rd16(dba(PLAYER_POS_SAVED)));
    a16_load(rd16(dba(PLAYER_POS_SAVED + 2)));
    dp16_set(ACTOR_Y + 1, cpu.a);
}

/*
 * @routine 81:DDB4 PlayerFlagsClear
 * WHAT IT DOES: Clears four of the player's flags and counts (V_1A08,
 * V_141C, V_142D, V_144D).
 * HOW: Each byte stored 0.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit:
 * everything unchanged.
 */
void abi_PlayerFlagsClear(void)
{
    wr8(dba(V_1A08), 0);
    wr8(dba(V_141C), 0);
    wr8(dba(V_142D), 0);
    wr8(dba(V_144D), 0);
}
