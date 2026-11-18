/*
 * guard_chase.c: the bank $A5 guard's chase: keeping pace with the
 * player, and jumping up or down to a ledge when he is well above or
 * below.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"
#include "wram_player.h"

void abi_ActorMotionPreset(void);
void abi_ActorProbeUpTowardPlayer(void);
void abi_PointInGroundLong(void);
void abi_RandomFromList(void);

#define POINT_X SCRATCH                  /* the probed point (words), PointInGroundLong's */
#define POINT_Y (SCRATCH + 2)
#define TILE_BEHAVIOUR V_0012            /* the probed tile's behaviour: bits 0-2 its kind of ground */
#define PROBES 0x002Cu                   /* probes left (word, data bank) */
#define JUMP_MOTIONS 0xD054u             /* the motion for a jump up, by the probes left (data bank) */
#define GUARD_CODE 0x03u                 /* where its code goes on (word) */
#define GUARD_NO_JUMP_DOWN 0x44u         /* zero: it does not jump down to him */
#define GUARD_KEEPS_ON 0x49u             /* nonzero: it never gives up the chase */
#define GUARD_CODE_SET 0x146Bu           /* which code it was last sent to: 1 stop, 2 run, $40 give up */
#define GUARD_PACE_ONCE 0x146Eu          /* nonzero: one close step keeps pace instead of slowing */
#define PLAYER_X_AT (PLAYER + ACTOR_X + 1)
#define PLAYER_Y_AT (PLAYER + ACTOR_Y + 1)
#define PLAYER_SPEED_AT (PLAYER + ACTOR_SPEED_X)

#define CODE_JUMP 0xE6A0u                /* its code for a jump */
#define CODE_STOP 0xE5B6u
#define CODE_RUN 0xE5E8u
#define CODE_GIVE_UP 0xE794u

/*
 * @routine A5:F284 ActorLedgeAbove
 * WHAT IT DOES: Looks for the top of the ground at a point offset from
 * the actor, going up a tile at a time: C clear when a ground tile with
 * an empty tile above it is found within X probes.
 * HOW: The point's y = the actor's + A, its x = the actor's + Y (- Y
 * facing left). PointInGroundLong (a ground tile has a kind in the
 * behaviour's low 3 bits): while empty, a tile up and a probe less; once
 * in ground, a tile up at a time until a probe finds none (C clear). C
 * set when the probes run out.
 * Entry: M 16-bit, X 8-bit; A = the y offset; X = the probes; Y = the x
 * offset; D = the actor; DB = $81; called by JSR. Exit: M 16-bit, X
 * 8-bit; C as above; $000C, $000E the last point; $002C the probes left.
 */
void abi_ActorLedgeAbove(void)
{
    uint16_t x;

    op_rep(0x20);
    flag_set(P_C, 0);
    wr16(dba(POINT_Y), op_adc16(cpu.a, dp16(ACTOR_Y + 1), &cpu.p));
    wr16(dba(PROBES), (uint8_t)cpu.x);
    x = cpu.y;
    y8_load(rd8(dpa(ACTOR_FACING)));
    if (cpu.y)
        x = (uint16_t)-x;
    flag_set(P_C, 0);
    cpu.a = op_adc16(x, dp16(ACTOR_X + 1), &cpu.p);
    wr16(dba(POINT_X), cpu.a);
    /* up a tile at a time through empty tiles until one is ground */
    for (;;) {
        abi_PointInGroundLong();
        op_sep(0x30);
        a8_load((uint8_t)(rd8(dba(TILE_BEHAVIOUR)) & 0x07));
        if ((uint8_t)cpu.a)
            break;
        op_rep(0x20);
        if (!dec16_at(dba(PROBES))) {
            flag_set(P_C, 1);
            return;
        }
        flag_set(P_C, 0);
        cpu.a = op_adc16(rd16(dba(POINT_Y)), 0xFFF0, &cpu.p);
        wr16(dba(POINT_Y), cpu.a);
    }
    /* then up through ground until a tile is empty: its top is found */
    for (;;) {
        op_rep(0x20);
        flag_set(P_C, 0);
        cpu.a = op_adc16(rd16(dba(POINT_Y)), 0xFFF0, &cpu.p);
        wr16(dba(POINT_Y), cpu.a);
        abi_PointInGroundLong();
        op_sep(0x30);
        a8_load((uint8_t)(rd8(dba(TILE_BEHAVIOUR)) & 0x07));
        if (!(uint8_t)cpu.a) {
            op_rep(0x20);
            flag_set(P_C, 0);
            return;
        }
        if (!dec8_at(dba(PROBES)))
            break;
    }
    op_rep(0x20);
    flag_set(P_C, 1);
}

/* ActorMotionPreset by the JSL at `site`, once the return address is
   dropped: the original's JSL writes its own return address where the
   dropped one was (the caller's frame), so the C writes it too, then
   steps S back up as the callee's RTL would. */
static void preset_past_return(uint32_t site)
{
    jsl_frame(site);
    abi_ActorMotionPreset();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/* GuardLedgeJump's work, run with its JSR's return on the stack: returns
   nonzero when the guard jumps, its own return address pulled (the caller
   then makes its RTS, which returns from the caller's caller). */
static int ledge_jump(void)
{
    uint16_t d;
    uint8_t face, v;

    flag_set(P_C, 0);
    cpu.a = op_adc16(rd16(dba(PLAYER_Y_AT)), 0x0010, &cpu.p);
    flag_set(P_C, 1);
    d = op_sbc16(cpu.a, dp16(ACTOR_Y + 1), &cpu.p);
    cpu.a = d;
    if (d & 0x8000) {
        /* he is above it */
        cmp16_of(d, 0xFFC0);
        if (d >= 0xFFC0)
            return 0;                            /* by 64 or less */
        cmp16_of(d, 0xFF80);
        if (d >= 0xFF80) {                       /* by 65 to 128: on a random pick */
            x8_load(0x4A);
            abi_RandomFromList();
            op_rep(0x20);
            if (cpu.p & P_Z)
                goto none;
        }
        a16_load(0xFFD0);
        x8_load(0x05);
        y8_load(0x18);
        abi_ActorProbeUpTowardPlayer();
        if (cpu.p & P_C)
            goto none;
        x8_load(0x02);
        v = rd8(dba(PROBES));
        cmp8_of(0x02, v);
        if (v <= 0x02) {
            /* few probes left: only onto a tile with behaviour bit 3 */
            flag_set(P_Z, !(cpu.a & 0x0008));
            if (!(cpu.a & 0x0008))
                goto none;
        }
        a16_load(CODE_JUMP);
        dp16_set(GUARD_CODE, CODE_JUMP);
        a16_load(pull16());                      /* its own return dropped */
        a16_load(dp16(ACTOR_X + 1));
        cmp16_of(cpu.a, rd16(dba(PLAYER_X_AT)));
        op_sep(0x20);
        face = (cpu.p & P_C) ? 1 : 0;            /* LDA #0, ROL: the compare's carry */
        a8_load(face);
        flag_set(P_C, 0);
        wr8(dpa(ACTOR_FACING), face);
        wr8(dpa(ACTOR_FLIP), face);
        x8_load(rd8(dba(PROBES)));
        y8_load(rd8(dba((uint16_t)(JUMP_MOTIONS + cpu.x))));
        preset_past_return(0xA5F201u);
        v = (uint8_t)(rd8(dba(TILE_BEHAVIOUR)) & 0x08);
        a8_load(v);
        if (v) {
            op_rep(0x20);
            flag_set(P_C, 0);
            cpu.a = op_adc16(dp16(ACTOR_SPEED_X), 0x0080, &cpu.p);
            dp16_set(ACTOR_SPEED_X, cpu.a);
            op_sep(0x20);
        }
        return 1;
    }
    /* he is below it */
    x8_load(rd8(dpa(GUARD_NO_JUMP_DOWN)));
    if (!cpu.x)
        return 0;
    x8_load(0x00);
    wr8(dpa(GUARD_NO_JUMP_DOWN), 0x00);
    cmp16_of(d, 0x0040);
    if (d < 0x0040) {                            /* by less than 64: on a random pick */
        x8_load(0x4C);
        abi_RandomFromList();
        op_rep(0x20);
        if (cpu.p & P_Z)
            goto none;
    }
    a16_load(0x0060);
    x8_load(0x05);
    y8_load(0x20);
    abi_ActorLedgeAbove();
    if (cpu.p & P_C)
        goto none;
    a16_load(CODE_JUMP);
    dp16_set(GUARD_CODE, CODE_JUMP);
    a16_load(pull16());                          /* its own return dropped */
    op_sep(0x20);
    y8_load(0x83);
    preset_past_return(0xA5F23Bu);
    return 1;
none:
    flag_set(P_C, 1);
    return 0;
}

/*
 * @routine A5:F19D GuardLedgeJump
 * WHAT IT DOES: When the player is well above or below the guard, picks a
 * ledge toward him and starts the guard's jump to it, leaving its caller;
 * C set when it does not jump (or returns at once when he is near its
 * height).
 * HOW: The player's y + 16 less the guard's. Above by more than 128, or
 * by 65-128 and a random pick (RandomFromList, list $4A) allows:
 * ActorProbeUpTowardPlayer (offset -48, 5 probes, 24 across); found with
 * more than 2 probes left or on a tile with behaviour bit 3: the guard's
 * code word ($03) = $E6A0, the caller's return dropped, facing the
 * player, ActorMotionPreset by the probes left (table $D054), on a bit-3
 * tile its speed + $80. Below with $44 set ($44 then cleared): by less
 * than 64 only on a random pick (list $4C); then ActorLedgeAbove (96
 * below, 5 probes, 32 ahead); found: code word $E6A0, return dropped,
 * ActorMotionPreset $83.
 * Entry: M 16-bit, X 8-bit; D = the guard; DB = $81; called by JSR from
 * GuardChase. Exit: jumping: to GuardChase's caller, M 8-bit; else M
 * 16-bit and C set, or C as the compare left it.
 */
void abi_GuardLedgeJump(void)
{
    if (ledge_jump()) {
        uint16_t back = pull16();                /* its RTS, through its caller's frame */

        cpu.pc = 0xA50000u | (uint16_t)(back + 1);
        cpu.unwound = 1;
    }
}

/*
 * @routine A5:F0F4 GuardChase
 * WHAT IT DOES: The guard's chase step: GuardLedgeJump, then its speed
 * and facing set by how far the player is ahead of it.
 * HOW: GuardLedgeJump; then the player's x less the guard's (dx). Within
 * 48 either way ($146E cleared if set, then as within 96): a moving guard
 * slows by $30 (to 0); a stopped one gets code word $E5B6 and $146B = 1
 * (once) and nothing more. Within 96: above a player speed of $C0 it
 * closes on his speed by 8 a frame (0 if that goes negative), else takes
 * $C0. Within 368: it speeds up by 8, to at most $300. Farther, with $49
 * clear: code word $E794, $146B = $40, and back. Each new speed: code
 * word $E5E8 and $146B = 2 (once), the speed's high byte cleared, the
 * facing (and flip) toward the player.
 * Entry: M 16-bit, X 8-bit; D = the guard; DB = $81; called by JSR. Exit:
 * as GuardLedgeJump when it jumps; else M 8-bit when the facing was set,
 * M 16-bit otherwise.
 */
void guard_chase_from(uint16_t at);

void abi_GuardChase(void)
{
    push16(0xF0F6);                              /* JSR GuardLedgeJump */
    if (ledge_jump())
        return;                                  /* its RTS was this routine's */
    cpu.s = (uint16_t)(cpu.s + 2);
    guard_chase_from(0);
}

/* GuardChase after its ledge jump: from the start (at 0), or from its
   speed-up step ($A5:F127) or its close step ($A5:F172), which the
   guard's states reach by JSR (GuardRunOff, GuardHoldLeft and
   GuardHoldRight); M 16-bit. */
void guard_chase_from(uint16_t at)
{
    uint16_t speed, pace, px;
    uint8_t set;

    if (at == 0xF127u)
        goto speed_up;
    if (at == 0xF172u)
        goto close;

    /* dx = his x less its own, shifted up through the ranges */
    flag_set(P_C, 1);
    px = rd16(dba(PLAYER_X_AT));
    cpu.a = op_sbc16(px, dp16(ACTOR_X + 1), &cpu.p);
    flag_set(P_C, 0);
    cpu.a = op_adc16(cpu.a, 0x0030, &cpu.p);
    cmp16_of(cpu.a, 0x0060);
    if (cpu.a < 0x0060)
        goto close;                              /* within 48 */
    flag_set(P_C, 0);
    cpu.a = op_adc16(cpu.a, 0x0030, &cpu.p);
    cmp16_of(cpu.a, 0x00C0);
    if (cpu.a < 0x00C0)
        goto keep_pace;                          /* within 96 */
    flag_set(P_C, 0);
    cpu.a = op_adc16(cpu.a, 0x0110, &cpu.p);
    cmp16_of(cpu.a, 0x02E0);
    if (cpu.a >= 0x02E0) {                       /* farther than 368 */
        x8_load(rd8(dpa(GUARD_KEEPS_ON)));
        if (!cpu.x) {
            x8_load(0x40);
            wr8(dba(GUARD_CODE_SET), 0x40);
            a16_load(CODE_GIVE_UP);
            dp16_set(GUARD_CODE, CODE_GIVE_UP);
            return;
        }
    }
speed_up:
    /* speed up by 8, to at most $300 */
    speed = dp16(ACTOR_SPEED_X);
    a16_load(0x0300);
    cmp16_of(0x0300, speed);
    if (0x0300 < speed)
        goto store;
faster:
    flag_set(P_C, 0);
    cpu.a = op_adc16(0x0008, dp16(ACTOR_SPEED_X), &cpu.p);
    goto store;

keep_pace:
    /* toward his speed by 8, or $C0 when he is slower than that */
    pace = rd16(dba(PLAYER_SPEED_AT));
    a16_load(0x00C0);
    cmp16_of(0x00C0, pace);
    if (0x00C0 >= pace)
        goto store;
    speed = dp16(ACTOR_SPEED_X);
    a16_load(speed);
    cmp16_of(speed, pace);
    if (speed < pace)
        goto faster;
    flag_set(P_C, 1);
    cpu.a = op_sbc16(speed, 0x0008, &cpu.p);
    if (cpu.a & 0x8000)
        a16_load(0x0000);
    goto store;

close:
    x8_load(rd8(dba(GUARD_PACE_ONCE)));
    if (cpu.x) {
        x8_load(0x00);
        wr8(dba(GUARD_PACE_ONCE), 0x00);
        goto keep_pace;
    }
    speed = dp16(ACTOR_SPEED_X);
    a16_load(speed);
    if (!speed) {
        /* stopped: sent to its stop code, once */
        x8_load(0x01);
        set = rd8(dba(GUARD_CODE_SET));
        cmp8_of(0x01, set);
        if (set != 0x01) {
            wr8(dba(GUARD_CODE_SET), 0x01);
            a16_load(CODE_STOP);
            dp16_set(GUARD_CODE, CODE_STOP);
        }
        return;
    }
    flag_set(P_C, 1);
    cpu.a = op_sbc16(speed, 0x0030, &cpu.p);
    if (cpu.a & 0x8000)
        a16_load(0x0000);

store:
    dp16_set(ACTOR_SPEED_X, cpu.a);
    x8_load(0x02);
    set = rd8(dba(GUARD_CODE_SET));
    cmp8_of(0x02, set);
    if (set != 0x02) {
        wr8(dba(GUARD_CODE_SET), 0x02);
        a16_load(CODE_RUN);
        dp16_set(GUARD_CODE, CODE_RUN);
    }
    x8_load(0x00);
    wr8(dpa(ACTOR_SPEED_X + 2), 0x00);
    a16_load(dp16(ACTOR_X + 1));
    cmp16_of(cpu.a, rd16(dba(PLAYER_X_AT)));
    op_sep(0x20);
    set = (cpu.p & P_C) ? 1 : 0;                 /* LDA #0, ROL: facing toward him */
    a8_load(set);
    flag_set(P_C, 0);
    wr8(dpa(ACTOR_FACING), set);
    wr8(dpa(ACTOR_FLIP), set);
}

/*
 * @routine A5:F127 GuardChaseSpeedUp
 * WHAT IT DOES: GuardChase's speed-up step, which GuardRunOff also calls:
 * the guard speeds up by 8, to at most $300, then (as every step of the
 * chase) runs on (code word $E5E8 once) facing the player.
 * HOW: 16-bit: $0300 below ACTOR_SPEED_X: kept; else + 8. Each new speed
 * ($A5:F134): stored; GUARD_CODE_SET 2 and code word $E5E8 unless already
 * 2; the speed's high byte cleared; its x against the player's: the carry
 * to the facing and flip (8-bit).
 * Entry: M 16-bit, X 8-bit; D = the guard; DB a bank whose $0000-$1FFF is
 * work RAM; reached by GuardChase's branch, and called by JSR from
 * GuardRunOff. Exit: M 8-bit; A = the facing; X = 0.
 */
void abi_GuardChaseSpeedUp(void)
{
    guard_chase_from(0xF127u);
}

/*
 * @routine A5:F172 GuardChaseClose
 * WHAT IT DOES: GuardChase's close step (the player within 48), which
 * GuardHoldLeft and GuardHoldRight also call: once after a pace kept
 * ($146E), it keeps his pace; a moving guard slows by $30 (to 0); a
 * stopped one is sent to GuardStop (code word $E5B6, once).
 * HOW: GUARD_PACE_ONCE set: cleared, on as the pace step
 * (GuardChaseSpeedUp_lF158). ACTOR_SPEED_X nonzero: less $30, at least
 * 0, on as a new speed (GuardChaseSpeedUp_lF134). Zero: GUARD_CODE_SET 1
 * and code word $E5B6 unless already 1.
 * Entry: M 16-bit, X 8-bit; D = the guard; DB a bank whose $0000-$1FFF is
 * work RAM; reached by GuardChase's branch, and called by JSR from
 * GuardHoldLeft and GuardHoldRight. Exit: M 16-bit.
 */
void abi_GuardChaseClose(void)
{
    guard_chase_from(0xF172u);
}
