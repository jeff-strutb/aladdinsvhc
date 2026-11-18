/*
 * guard_pieces.c: the bank $A5 guard's (actor kind $08) moves: the
 * pieces its code word sends it to (stop, run, jump, give up, walk), the
 * run's throw of up to three kind $90 children, its climb of a wall and
 * its landing.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"
#include "wram_player.h"

void abi_ActorAnimByTile(void);
void abi_ActorAnimChange(void);
void abi_ActorAnimRiseFall(void);
void abi_ActorFly(void);
void abi_ActorLedgeAhead(void);
void abi_ActorMotionPreset(void);
void abi_ActorMoveX(void);
void abi_ActorMoveY(void);
void abi_ActorOutOfGround(void);
void abi_ActorOutOfWall8(void);
void abi_ActorOutOfWallProbe4(void);
void abi_ActorSettleOnFloor(void);
void abi_ActorSnapToGround(void);
void abi_ActorSpawn(void);
void abi_ActorSpeedXClear(void);
void abi_ActorSpeedXFromA5(void);
void abi_ActorSpeedYFromA5(void);
void abi_AnimStart(void);
void abi_GroundBehindBit3(void);
void abi_GuardSnapToTile(void);
void abi_PointInGroundLong(void);
void abi_GuardJump(void);
void abi_GuardStop(void);
static uint32_t guard_step_ahead(void);

#define G_POINT_X SCRATCH               /* the probe point PointInGround tests: x (word) */
#define G_POINT_Y (SCRATCH + 2)         /* its y (word) */
#define G_TILE (SCRATCH + 6)            /* PointInGround's tile behaviour: bit 3 a slope's side, bit 5 an edge, bit 6 a ledge, bit 7 the slope's way */
#define G_MOVING 0x41u                  /* counted up while a move is under way, cleared when it ends */
#define G_LANDING 0x42u                 /* frames left to settle after landing */
#define G_IN_AIR 0x44u                  /* set for the frame it leaves the ground */
#define G_BACK_OFF 0x46u                /* nonzero: a wall it cannot climb turns it back */
#define G_SLOTS 0x1478u                 /* the three kind $90 children's slots (by their $07) */
#define G_PRESETS_FROM 0x7Au            /* the motion preset for a throw seen by slot 0 (slots 1, 2: $7B, $7C) */
#define G_PRESET_NONE 0x7Du             /* the motion preset with no throw seen */
#define G_PRESET_NO_LEDGE 0x80u         /* the motion preset with no ledge below */
#define G_HIDE_A 0x141Cu                /* nonzero: the guard waits before it comes back */
#define G_HIDE_B 0x1427u
#define PLAYER_X_AT (PLAYER + ACTOR_X + 1)
#define G_STEP_AHEAD_X 0xCFF4u          /* (data bank) by speed and facing: x of the point ahead (word) */
#define G_STEP_AHEAD_Y 0xD010u          /* (data bank) by speed and facing: y of the point ahead (word) */

void abi_GuardClimb(void);
void abi_GuardFlip(void);
void abi_GuardLand(void);
void abi_GuardRunDown(void);
static void jump_air(void);

static void anim(uint8_t a)
{
    a8_load(a);
    abi_ActorAnimChange();
}

/* A JSL at `site` to fn, its return address on the stack while fn runs, as
   the original's (frame_step.c's model). */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/* ActorAnimChange with A = a by the JSR whose last byte is `ret`. */
static void anim_at(uint8_t a, uint16_t ret)
{
    a8_load(a);
    call_frame(ret, abi_ActorAnimChange);
}

static void speed_x(uint8_t y)
{
    y8_load(y);
    abi_ActorSpeedXFromA5();
}

static void speed_y(uint8_t y)
{
    y8_load(y);
    abi_ActorSpeedYFromA5();
}

/*
 * @routine A5:E5B6 GuardStop
 * @actor
 * WHAT IT DOES: The guard stopped (its code word $E5B6): each frame on the
 * ground, it shows the pose for facing the player, facing away, or
 * standing at an edge.
 * HOW: Each frame (BRK): ActorSnapToGround; G_TILE bit 5 set: animation
 * $16; else (16-bit X) its x against the player's, the carry against
 * its flip: equal, animation 0; else 6 (ActorAnimChange).
 * Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; resumed by
 * the actor loop at its code word. Exit: never returns.
 */
void abi_GuardStop(void)
{
    uint16_t x, px;
    uint8_t tile, side;

    for (;;) {
        actor_yield(0xA5E5B6u);
        jsl_call(0xA5E5B8u, abi_ActorSnapToGround);
        tile = rd8(dba(G_TILE));
        a8_load(tile);
        if (tile & 0x20) {
            anim_at(0x16, 0xE5E5);
            continue;
        }
        op_rep(0x10);
        x = dp16(ACTOR_X + 1);
        cpu.x = x;
        cpu.p = nz16(cpu.p, x);
        px = rd16(dba(PLAYER_X_AT));
        cmp16_of(x, px);
        op_sep(0x10);
        side = x >= px;                          /* LDA #0, ROL: the carry */
        a8_load(side);
        flag_set(P_C, 0);
        a8_load((uint8_t)(side ^ rd8(dpa(ACTOR_FLIP))));
        if ((uint8_t)cpu.a)
            anim_at(0x06, 0xE5DE);
        else
            anim_at(0x00, 0xE5D7);
    }
}

/*
 * @routine A5:E5E8 GuardRun
 * @actor
 * WHAT IT DOES: The guard running (its code word $E5E8): it runs along
 * the ground, posed by the ground under it; at a wall it climbs; where
 * the ground ahead rises it jumps; off an edge it drops, and at a ledge
 * below it throws up to three kind $90 children to find its way, then
 * leaps by what they report.
 * HOW: ActorSpeedXFromA5 9, animation 1. Each frame (BRK): ActorMoveX;
 * ActorOutOfWall8 C set: on at GuardClimb; ActorSnapToGround zero: off
 * the edge; else ActorAnimByTile and GuardStepAhead. Off the edge:
 * AnimStart $0A, G_IN_AIR up, BRK, G_IN_AIR 0, G_MOVING up;
 * ActorLedgeAhead C clear: preset $80. Else BRK; the spawn place and
 * G_SLOTS cleared; up to three ActorSpawn of kind $90 with $07 0, 1, 2,
 * each slot counted up. Each frame (BRK) until a slot has a high nibble
 * (or all are 0: preset G_PRESET_NONE): low nibbles cleared, preset
 * G_PRESETS_FROM + the first slot set. ActorMotionPreset, and on in the
 * air (GuardJump after its start).
 * Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; resumed by
 * the actor loop at its code word. Exit: never returns.
 */
void abi_GuardRun(void)
{
    y8_load(0x09);
    call_frame(0xE5EC, abi_ActorSpeedXFromA5);
    anim_at(0x01, 0xE5F1);
    for (;;) {
        actor_yield(0xA5E5F2u);
        jsl_call(0xA5E5F4u, abi_ActorMoveX);
        call_frame(0xE5FA, abi_ActorOutOfWall8);
        if (cpu.p & P_C) {
            abi_GuardClimb();
            return;
        }
        jsl_call(0xA5E5FDu, abi_ActorSnapToGround);
        if (cpu.p & P_Z)
            break;
        call_frame(0xE605, abi_ActorAnimByTile);
        push16(0xE608);                          /* JSR GuardStepAhead */
        if (guard_step_ahead()) {
            abi_GuardJump();                     /* its JMP: the piece goes on */
            return;
        }
        cpu.s = (uint16_t)(cpu.s + 2);
    }
    /* off the edge */
    a8_load(0x0A);
    jsl_call(0xA5E610u, abi_AnimStart);
    (void)inc8_at(dpa(G_IN_AIR));
    actor_yield(0xA5E616u);
    abi_GuardRunDown();
}

/*
 * @routine A5:E618 GuardRunDown
 * @actor
 * WHAT IT DOES: The guard off an edge (GuardRun's drop, also GuardFlip's):
 * with no ledge below, it drops with preset $80; at a ledge it throws up to
 * three kind $90 children to find its way and leaps by what they report.
 * HOW: G_IN_AIR 0, G_MOVING up; ActorLedgeAhead C clear: Y = $80, on at
 * the preset. Else BRK; the spawn place and G_SLOTS cleared; up to three
 * ActorSpawn of kind $90 with $07 0, 1, 2, each slot counted up. Each
 * frame (BRK) until a slot has a high nibble (or all are 0: Y =
 * G_PRESET_NONE): low nibbles cleared, Y = G_PRESETS_FROM + the first slot
 * set. ActorMotionPreset, and on in the air (GuardJump after its start).
 * Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; reached from
 * GuardRun's fall off an edge and by GuardFlip's jump. Exit: never
 * returns.
 */
void abi_GuardRunDown(void)
{
    static const uint16_t slot[3] = { G_SLOTS, G_SLOTS + 1, G_SLOTS + 2 };
    uint8_t seen, i;

    wr8(dpa(G_IN_AIR), 0x00);
    (void)inc8_at(dpa(G_MOVING));
    abi_ActorLedgeAhead();
    if (!(cpu.p & P_C)) {
        y8_load(G_PRESET_NO_LEDGE);
        goto preset;
    }
    actor_yield(0xA5E626u);
    wr8(dpa(ACTOR_SPAWN_AT), 0x00);
    for (i = 0; i < 3; i++)
        wr8(dba(slot[i]), 0x00);
    for (i = 0; i < 3; i++) {
        a8_load(0x90);
        abi_ActorSpawn();
        if (cpu.p & P_N)
            break;
        a8_load(i);
        wr8(dba((uint16_t)(ACTOR_07 + cpu.x)), i);
        (void)inc8_at(dba(slot[i]));
        op_sep(0x10);
    }
    for (;;) {
        actor_yield(0xA5E669u);
        seen = rd8(dba(G_SLOTS));
        seen |= rd8(dba(G_SLOTS + 1));
        seen |= rd8(dba(G_SLOTS + 2));
        a8_load(seen);
        if (!seen) {
            y8_load(G_PRESET_NONE);
            goto preset;
        }
        a8_load((uint8_t)(seen & 0xF0));
        if ((uint8_t)cpu.a)
            break;
    }
    a8_load(0x0F);
    for (i = 0; i < 3; i++)
        trb_at(dba(slot[i]), 0x0F);
    y8_load(G_PRESETS_FROM);
    for (i = 0; i < 3; i++) {
        a8_load(rd8(dba(slot[i])));
        if ((uint8_t)cpu.a)
            goto preset;
        if (i < 2)
            y8_load((uint8_t)(cpu.y + 1));
    }
    y8_load(G_PRESET_NONE);
preset:
    abi_ActorMotionPreset();
    jump_air();
}

/*
 * @routine A5:E6A0 GuardJump
 * @actor
 * WHAT IT DOES: The guard in the air (its code word $E6A0): it flies with
 * gravity, posed rising or falling; landing on a slope it turns down it
 * and lands; at a wall it climbs it (or turns back, or lands on a ledge
 * it can stand on); on the ground it settles for up to 10 frames, until
 * the player is within 48, and stops.
 * HOW: G_MOVING up; gravity $38. Each frame (BRK): ActorAnimRiseFall,
 * ActorFly. Falling, ActorSettleOnFloor nonzero and G_TILE bit 3: facing
 * by its bit 7 (flip), G_MOVING 0, on at GuardLand. ActorOutOfWall8 C
 * set: G_TILE bit 3 set: as a slope's side below; bit 6 clear: G_BACK_OFF
 * set: GuardFlip, else GuardClimb. Else GroundBehindBit3 nonzero: G_TILE
 * bit 7 against its flip: equal, GuardLand. Rising, or ActorOutOfGround
 * zero: again. Landed: G_LANDING $0A, animation $0A, ActorSpeedXClear;
 * each frame ActorSnapToGround, BRK, until the player is within 48 or
 * G_LANDING runs out; G_MOVING 0, on at GuardStop.
 * Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; resumed by
 * the actor loop at its code word; also reached by jumps from the
 * guard's other moves. Exit: never returns.
 */
void abi_GuardJump(void)
{
    (void)inc8_at(dpa(G_MOVING));
    a8_load(0x38);
    wr8(dpa(ACTOR_ACCEL_Y), 0x38);
    jump_air();
}

/* GuardJump from its first frame in the air ($A5:E6A6). */
static void jump_air(void)
{
    uint8_t tile, up, side;
    uint16_t d;

    for (;;) {
        actor_yield(0xA5E6A6u);
        abi_ActorAnimRiseFall();
        abi_ActorFly();
        up = rd8(dpa(ACTOR_SPEED_Y + 2));
        a8_load(up);
        if (up & 0x80) {
            abi_ActorSettleOnFloor();
            if (!(cpu.p & P_Z)) {
                tile = rd8(dba(G_TILE));
                a8_load(tile);
                if (tile & 0x08) {
                    side = tile >> 7;            /* ROL, LDA #0, ROL */
                    a8_load(side);
                    flag_set(P_C, 0);
                    wr8(dpa(ACTOR_FLIP), side);
                    wr8(dpa(ACTOR_FLIP), side);
                    wr8(dpa(G_MOVING), 0x00);
                    abi_GuardLand();
                    return;
                }
            }
        }
        abi_ActorOutOfWall8();
        if (cpu.p & P_C) {
            tile = rd8(dba(G_TILE));
            a8_load(tile);
            if (tile & 0x08)
                goto slope;
            if (!(tile & 0x40)) {
                a8_load(rd8(dpa(G_BACK_OFF)));
                if ((uint8_t)cpu.a) {
                    abi_GuardFlip();
                    return;
                }
                wr8(dpa(G_MOVING), 0x00);
                abi_GuardClimb();
                return;
            }
        }
        abi_GroundBehindBit3();
        if (cpu.p & P_Z)
            goto fall;
slope:
        tile = rd8(dba(G_TILE));
        side = tile >> 7;
        a8_load(side);
        flag_set(P_C, 0);
        a8_load((uint8_t)(side ^ rd8(dpa(ACTOR_FLIP))));
        if (!(uint8_t)cpu.a) {
            wr8(dpa(G_MOVING), 0x00);
            abi_GuardLand();
            return;
        }
fall:
        a8_load(rd8(dpa(ACTOR_SPEED_Y + 2)));
        if (cpu.a & 0x80)
            continue;
        abi_ActorOutOfGround();
        if (!(cpu.p & P_Z))
            break;
    }
    /* landed */
    a8_load(0x0A);
    wr8(dpa(G_LANDING), 0x0A);
    anim(0x0A);
    abi_ActorSpeedXClear();
    do {
        abi_ActorSnapToGround();
        actor_yield(0xA5E71Du);
        op_rep(0x20);
        flag_set(P_C, 1);
        d = rd16(dba(PLAYER_X_AT));
        d = op_sbc16(d, dp16(ACTOR_X + 1), &cpu.p);
        flag_set(P_C, 0);
        d = op_adc16(d, 0x0030, &cpu.p);
        a16_load(d);
        cmp16_of(d, 0x0060);
        op_sep(0x20);
        if (d >= 0x0060)
            break;
    } while (dec8_at(dpa(G_LANDING)));
    wr8(dpa(G_MOVING), 0x00);
    abi_GuardStop();
}

/*
 * @routine A5:E73B GuardClimb
 * @actor
 * WHAT IT DOES: The guard's climb of a wall: up it while the wall goes
 * on; at its top, over it (GuardJump); where the point above is solid,
 * it turns back (GuardFlip).
 * HOW: G_MOVING up; animation $0B; ActorSpeedXFromA5 0, ActorSpeedYFromA5
 * 0; each frame (BRK) ActorMoveY, ActorMoveX, ActorOutOfWallProbe4: C
 * clear, over ($A5:E785: G_MOVING 0, ActorSpeedXFromA5 3,
 * ActorSpeedYFromA5 3, on at GuardJump); else the point 24 above its x
 * (G_POINT_X, G_POINT_Y), PointInGroundLong C set: on at GuardFlip.
 * Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; reached by
 * jumps from GuardRun and GuardJump. Exit: never returns.
 */
void abi_GuardClimb(void)
{
    (void)inc8_at(dpa(G_MOVING));
    anim(0x0B);
    speed_x(0x00);
    speed_y(0x00);
    for (;;) {
        actor_yield(0xA5E74Cu);
        abi_ActorMoveY();
        abi_ActorMoveX();
        abi_ActorOutOfWallProbe4();
        if (!(cpu.p & P_C))
            break;
        op_rep(0x20);
        a16_load(dp16(ACTOR_X + 1));
        wr16(dba(G_POINT_X), cpu.a);
        flag_set(P_C, 0);
        a16_load(op_adc16(dp16(ACTOR_Y + 1), 0xFFE8, &cpu.p));
        wr16(dba(G_POINT_Y), cpu.a);
        abi_PointInGroundLong();                 /* leaves M 8-bit */
        if (cpu.p & P_C) {
            abi_GuardFlip();
            return;
        }
    }
    /* over the top */
    wr8(dpa(G_MOVING), 0x00);
    speed_x(0x03);
    speed_y(0x03);
    abi_GuardJump();
}

/*
 * @routine A5:E774 GuardFlip
 * @actor
 * WHAT IT DOES: The guard turned back: facing and flip swapped, animation
 * 8, and down off the edge (GuardRunDown).
 * HOW: ACTOR_FACING EOR 1 to the facing and flip; AnimStart 8; on at
 * GuardRunDown.
 * Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; reached by
 * jumps from GuardJump and GuardClimb. Exit: never returns.
 */
void abi_GuardFlip(void)
{
    uint8_t f = (uint8_t)(rd8(dpa(ACTOR_FACING)) ^ 0x01);

    a8_load(f);
    wr8(dpa(ACTOR_FACING), f);
    wr8(dpa(ACTOR_FLIP), f);
    a8_load(0x08);
    abi_AnimStart();
    abi_GuardRunDown();
}

/*
 * @routine A5:E794 GuardGiveUp
 * @actor
 * WHAT IT DOES: The guard giving up the chase (its code word $E794): it
 * drops out of sight and can't be hit; once nothing holds it back it is
 * put where the player is, and once it can be hit again it comes back
 * into view facing him and leaps.
 * HOW: ACTOR_09 bit 7 and ACTOR_08 bit 3 cleared; BRK while G_HIDE_A or
 * G_HIDE_B; its x and y = the player's; BRK while ACTOR_09 bit 6 is set;
 * ACTOR_08 bit 3 and ACTOR_09 bit 7 set; facing and flip toward the
 * player; G_MOVING 0; ActorSpeedXFromA5 3, ActorSpeedYFromA5 3; on at
 * GuardJump.
 * Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; resumed by
 * the actor loop at its code word. Exit: never returns.
 */
void abi_GuardGiveUp(void)
{
    uint16_t x, px;
    uint8_t side;

    a8_load(0x80);
    trb_at(dpa(ACTOR_09), 0x80);
    a8_load(0x08);
    trb_at(dpa(ACTOR_08), 0x08);
    for (;;) {
        uint8_t hide = rd8(dba(G_HIDE_A));
        hide |= rd8(dba(G_HIDE_B));
        a8_load(hide);
        if (!(uint8_t)cpu.a)
            break;
        actor_yield(0xA5E7A4u);
    }
    a8_load(rd8(dba(PLAYER + ACTOR_X + 1)));
    wr8(dpa(ACTOR_X + 1), (uint8_t)cpu.a);
    a8_load(rd8(dba(PLAYER + ACTOR_X + 2)));
    wr8(dpa(ACTOR_X + 2), (uint8_t)cpu.a);
    a8_load(rd8(dba(PLAYER + ACTOR_Y + 1)));
    wr8(dpa(ACTOR_Y + 1), (uint8_t)cpu.a);
    a8_load(rd8(dba(PLAYER + ACTOR_Y + 2)));
    wr8(dpa(ACTOR_Y + 2), (uint8_t)cpu.a);
    do {
        actor_yield(0xA5E7BCu);
        a8_load(rd8(dpa(ACTOR_09)));
    } while (cpu.a & 0x40);
    a8_load(0x08);
    tsb_at(dpa(ACTOR_08), 0x08);
    a8_load(0x80);
    tsb_at(dpa(ACTOR_09), 0x80);
    op_rep(0x20);
    x = dp16(ACTOR_X + 1);
    a16_load(x);
    px = rd16(dba(PLAYER_X_AT));
    cmp16_of(x, px);
    op_sep(0x20);
    side = x >= px;
    a8_load(side);
    flag_set(P_C, 0);
    wr8(dpa(ACTOR_FACING), side);
    wr8(dpa(ACTOR_FLIP), side);
    wr8(dpa(G_MOVING), 0x00);
    speed_x(0x03);
    speed_y(0x03);
    abi_GuardJump();
}

/*
 * @routine A5:E7EB GuardLand
 * @actor
 * WHAT IT DOES: The guard landed on a slope or ledge: set at the tile's
 * centre, pose 7 for 32 frames, its speed cleared and a small hop
 * (ActorSpeedYFromA5 6), then stopped.
 * HOW: G_MOVING up; GuardSnapToTile; ActorAnimChange 7; COP $20;
 * ActorSpeedXClear; ActorSpeedYFromA5 6; G_MOVING 0; on at GuardStop.
 * Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; reached by
 * jumps from GuardJump. Exit: never returns.
 */
void abi_GuardLand(void)
{
    (void)inc8_at(dpa(G_MOVING));
    abi_GuardSnapToTile();
    anim(0x07);
    a8_load(0x20);
    actor_wait(0xA5E7F7u);
    abi_ActorSpeedXClear();
    speed_y(0x06);
    wr8(dpa(G_MOVING), 0x00);
    abi_GuardStop();
}

/*
 * @routine A5:E807 GuardWalk
 * @actor
 * WHAT IT DOES: The guard once the player has no health left (its code
 * word $E807): it walks in place for good.
 * HOW: G_MOVING up; animation 4; BRK for good.
 * Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; resumed by
 * the actor loop at its code word. Exit: never returns.
 */
void abi_GuardWalk(void)
{
    (void)inc8_at(dpa(G_MOVING));
    anim(0x04);
    for (;;)
        actor_yield(0xA5E80Eu);
}

/* GuardStepAhead's work, for a caller that has pushed its JSR's return:
   0 when the ground ahead is clear (the routine's RTS, the return not yet
   pulled); else, with the return pulled and the rise's speed set, where
   the guard goes on ($A5:E6A0, GuardJump). */
static uint32_t guard_step_ahead(void)
{
    uint8_t x;
    uint16_t at;

    x = (uint8_t)(rd8(dpa(ACTOR_SPEED_X + 1)) << 1);
    x = (uint8_t)((x ^ rd8(dpa(ACTOR_FACING))) << 1);
    a8_load(x);
    x8_load(x);
    op_rep(0x20);
    flag_set(P_C, 0);
    at = rd16(dba((uint16_t)(G_STEP_AHEAD_X + x)));
    a16_load(op_adc16(at, dp16(ACTOR_X + 1), &cpu.p));
    wr16(dba(G_POINT_X), cpu.a);
    flag_set(P_C, 0);
    at = rd16(dba((uint16_t)(G_STEP_AHEAD_Y + x)));
    a16_load(op_adc16(at, dp16(ACTOR_Y + 1), &cpu.p));
    wr16(dba(G_POINT_Y), cpu.a);
    abi_PointInGroundLong();                     /* leaves M 8-bit */
    a8_load((uint8_t)(rd8(dba(G_TILE)) & 0x07));
    if (!(uint8_t)cpu.a)
        return 0;
    a8_load((uint8_t)(pull16() >> 8));           /* PLA, PLA: its JSR's return; A its high byte */
    y8_load(0x09);
    abi_ActorSpeedYFromA5();
    return 0xA5E6A0u;
}

/*
 * @routine A5:EAE6 GuardStepAhead
 * WHAT IT DOES: The running guard's look ahead: where the ground rises in
 * front of it, it jumps (it leaves its caller and goes to GuardJump).
 * HOW: X = ((speed's middle byte * 2) EOR facing) * 2; the point = its x
 * + G_STEP_AHEAD_X, its y + G_STEP_AHEAD_Y (16-bit) by X;
 * PointInGroundLong; G_TILE's kind 0: RTS. Else its JSR's return pulled,
 * ActorSpeedYFromA5 9, on at GuardJump.
 * Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; called by JSR
 * from GuardRun. Exit: M 8-bit (no ground ahead); else never returns.
 */
void abi_GuardStepAhead(void)
{
    if (guard_step_ahead())
        actor_leave(0xA5E6A0u);
}
