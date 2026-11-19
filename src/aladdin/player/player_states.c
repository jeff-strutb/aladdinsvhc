/*
 * player_states.c: Aladdin's code: the states he goes between as he
 * stands, walks, skids, crouches, jumps and falls, lands, hangs, hovers
 * and is knocked back or loses a life. Each state is actor code (it
 * yields by BRK, docs/aladdin/ACTORS.md) and goes on to the next by a
 * jump; the tests it calls by JSR that end in a jump to another state
 * drop their own return address first.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "wram_video.h"
#include "wram_state.h"
#include "wram_player.h"
#include "actor.h"

void abi_ActorDecelerateX(void);
void abi_ActorFallCap(void);
void abi_ActorFly(void);
void abi_ActorGravity(void);
void abi_ActorMotionPreset(void);
void abi_ActorMotionPresetY(void);
void abi_ActorMove(void);
void abi_ActorMoveY(void);
void abi_ActorOutOfGround(void);
void abi_ActorOutOfWalls(void);
void abi_ActorSnapToGround(void);
void abi_ActorSpeedXClear(void);
void abi_ActorSpeedXSet(void);
void abi_ActorSpeedYClear(void);
void abi_ActorSpeedYSet(void);
void abi_ActorSpeedYSetGravity(void);
void abi_ActorStepX(void);
void abi_ActorStopX(void);
void abi_ActorStopY(void);
void abi_GroundBehindBit3(void);
void abi_GroundBehindBit4(void);
void abi_GroundLeftLong(void);
void abi_GroundRightLong(void);
void abi_Level035CSet(void);
void abi_PadFacing(void);
void abi_PlayerAirSteer(void);
void abi_PlayerAppleThrow(void);
void abi_PlayerBoxSet(void);
void abi_PlayerEnter8CLong(void);
void abi_PlayerFallenTest(void);
void abi_PlayerFlagsClear(void);
void abi_PlayerFlipByPad(void);
void abi_PlayerGroundPush(void);
void abi_PlayerGroundSlide(void);
void abi_PlayerGroundYSave(void);
void abi_PlayerJumpPoseStep(void);
void abi_PlayerMoveStart(void);
void abi_PlayerMoveStartAlso(void);
void abi_PlayerRunMove(void);
void abi_PlayerRunRatesSet(void);
void abi_PlayerRunSpeed(void);
void abi_PlayerSpawnEvery8(void);
void abi_PlayerTopSpeedCap(void);
void abi_PlayerTopSpeedHold(void);
void abi_SongPlay(void);
void abi_SoundQueueAdd(void);
void abi_TaskCreate(void);
void abi_TaskExec(void);
void abi_PlayerGroundSnapHurt(void);
void abi_PlayerOutOfWallsHurt(void);
void abi_PlayerGroundHurtTest(void);
void abi_PlayerGroundHurtJump(void);
void abi_PlayerLandOrHurt(void);

/* hurt_ground.c's tests' work (0: returned; else where his code goes on) */
uint32_t ground_snap_hurt(void);
uint32_t out_of_walls_hurt(void);
uint32_t ground_hurt_test(void);
uint32_t ground_hurt_jump(void);
uint32_t land_or_hurt(void);

void abi_PlayerStand(void);
void abi_PlayerThrowStanding(void);
void abi_PlayerWalk(void);
void abi_PlayerFall(void);
void abi_PlayerJump(void);
void abi_PlayerAirStart(void);
void abi_PlayerAirTimed(void);
void abi_PlayerAir(void);
void abi_PlayerAirLand(void);
void abi_PlayerLand(void);
void abi_PlayerSkid(void);
void abi_PlayerNoGroundFall(void);
void abi_PlayerCrouch(void);
void abi_PlayerCrouchStep(void);
void abi_PlayerCrouchJump(void);
void abi_PlayerAirStep(void);
void abi_PlayerLedgeGrab(void);
void abi_PlayerHang(void);
void abi_PlayerHover(void);
void abi_PlayerKnockback(void);

/* Pad 1 and the game's actions (system/controls.c) */
#define PAD1_DIRS 0x0380u           /* pad 1's high byte: B Y Select Start Up Down Left Right */
#define PAD_UP 0x08
#define PAD_DOWN 0x04
#define JUMP_HELD 0x0383u           /* nonzero while the jump's button is held */
#define DASH_HELD 0x0384u           /* nonzero while the dash's button is held */
#define HOVER_HELD 0x0386u          /* nonzero while the fourth action's button is held */
#define JUMP_PRESSED 0x0387u        /* nonzero when the jump's button was just pressed */

/* The player's work RAM */
#define TILE_BITS 0x0012u           /* the last probed tile's behaviour (bit 5: it hurts) */
#define TILE_HURTS 0x20
#define GROUND_UNDER 0x0013u        /* zero: no ground under him (after PlayerGroundSnapHurt) */
#define PLAYER_FLAGS 0x0347u        /* bit 0: no hurt from the ground; bit 1 set in state $8C, cleared here */
#define V_0360 0x0360u              /* nonzero: the level's opening walk is skipped */
#define LIVES 0x0364u               /* lives left (the status line shows it, less 1) */
#define HOVER_ON 0x036Eu            /* nonzero: the fourth action makes him hover */
#define SCENE_WAIT 0x0087u          /* the knockback's end waits for this to be 0 */
#define V_03F1 0x03F1u
#define V_03F2 0x03F2u
#define SCENE_ON 0x03FAu            /* counted up as a life is lost */
#define V_141B 0x141Bu
#define V_141E 0x141Eu              /* the top speed's high byte (PLAYER_TOP_SPEED + 1) */
#define SKID_STARTED 0x1422u        /* nonzero once the skid's sound and pose have started */
#define FALLEN_OUT 0x1424u          /* nonzero: he fell out of the map */
#define V_1427 0x1427u              /* nonzero: no landing on actors */
#define GROUND_PUSHED 0x1429u       /* 2 while he stands on ground that pushes, else 0 */
#define STAND_HELD 0x142Bu          /* nonzero: standing, he does not turn or walk */
#define HURT_FRAMES 0x142Fu         /* frames the knockback lasts while he has health */
#define V_1446 0x1446u              /* nonzero: no jump from a crouch */
#define AIR_THROW 0x144Bu           /* frames left of a throw in the air */
#define PLAYER_LANDED 0x144Eu       /* 1: he has landed on an actor */
#define AIR_FRAMES 0x1451u          /* frames in the air, counted up to $80 */
#define NO_LANDING 0x145Au          /* nonzero: frames with no hold on a ledge; negative: none */
#define VIEW_EDGE_RULE 0x145Bu      /* the rule the view's edges follow */
#define V_1A10 0x1A10u
#define V_1A12 0x1A12u

/* Record fields of the player's code */
#define FRAMES_40 ACTOR_40          /* frames left of a pose (and the jump's pose step) */
#define FRAMES_47 ACTOR_47          /* frames left of a landing's pose */
#define FRAMES_4A ACTOR_STREAM      /* frames left of the standing throw */

/* The probe point and tile the ground tests leave (map/collision.c) */
#define POINT_X 0x000Cu             /* the probe point's x (word) */
#define POINT_Y 0x000Eu             /* its y (word) */
#define TILE_MIRRORED 0x001Au       /* nonzero: the probed tile is mirrored */

/* Tables in the data bank ($80) */
#define JUMP_ROWS 0xC730u           /* the jump's vertical speed entry by the run speed's low bits */

/* An 8-bit load of a byte of work RAM, or of the record. */
static uint8_t ld(uint16_t addr)
{
    uint8_t v = rd8(dba(addr));

    a8_load(v);
    return v;
}

static uint8_t ldd(uint8_t off)
{
    uint8_t v = rd8(dpa(off));

    a8_load(v);
    return v;
}

/* LDA #n, STA: the byte n stored. */
static void put(uint16_t addr, uint8_t n)
{
    a8_load(n);
    wr8(dba(addr), n);
}

static void putd(uint8_t off, uint8_t n)
{
    a8_load(n);
    wr8(dpa(off), n);
}

/* LDA, AND #m, STA of a byte of the record. */
static void and_d(uint8_t off, uint8_t m)
{
    uint8_t v = (uint8_t)(rd8(dpa(off)) & m);

    a8_load(v);
    wr8(dpa(off), v);
}

/* LDA, ORA #m, STA. */
static void or_d(uint8_t off, uint8_t m)
{
    uint8_t v = (uint8_t)(rd8(dpa(off)) | m);

    a8_load(v);
    wr8(dpa(off), v);
}

/* BIT #m on A: Z from A AND m. */
static int bit_a(uint8_t m)
{
    flag_set(P_Z, !((uint8_t)cpu.a & m));
    return ((uint8_t)cpu.a & m) != 0;
}

static void move_start(uint8_t move)
{
    a8_load(move);
    abi_PlayerMoveStart();
}

static void sound(uint8_t n)
{
    a8_load(n);
    abi_SoundQueueAdd();
}

/* The level is $1A, or LEVEL_035C is 4: the ground can push and slide. */
static int pushing_ground(void)
{
    uint8_t v = ld(LEVEL_035C);

    cmp8_of(v, 0x04);
    if (v == 0x04)
        return 1;
    v = ld(LEVEL);
    cmp8_of(v, 0x1A);
    return v == 0x1A;
}

/* The jump pressed: on into the jump at his speed (X its fraction, A its
   pixels). */
static int jump_pressed(void)
{
    return ld(JUMP_PRESSED) != 0;
}

static void jump_at_speed(void)
{
    x8_load(rd8(dpa(ACTOR_SPEED_X)));
    a8_load(rd8(dpa(ACTOR_SPEED_X + 1)));
    abi_PlayerJump();
}

/* The ground last probed hurts him: TILE_BITS bit 5, unless ACTOR_08 bit
   4 or PLAYER_FLAGS bit 0 is set (each tested by BIT). */
static int ground_hurts(void)
{
    ld(TILE_BITS);
    if (!bit_a(TILE_HURTS))
        return 0;
    ldd(ACTOR_08);
    if (bit_a(0x10))
        return 0;
    ld(PLAYER_FLAGS);
    return !bit_a(0x01);
}

/* TaskCreate at `site` and TaskExec at `site`, their argument bytes after
   the JSL. */
static void task_create(uint32_t site)
{
    jsl_frame(site);
    abi_TaskCreate();
    cpu.unwound = 0;
}

static void task_exec(uint32_t site)
{
    jsl_frame(site);
    abi_TaskExec();
}

/* A JSR to a test that can drop its return address and go on at another
   state: the return address pushed as the JSR pushes it (`ret`, the JSR's
   last byte); 0 when the test returned (the address pulled again), else
   where his code goes on, the addresses it dropped already pulled. The
   caller goes on there in its own C (go_on): in the original that is a jump
   within the same actor code, so the actor's piece goes on; only the
   test's own call ends at the jump. */
static uint32_t jsr_test(uint16_t ret, uint32_t (*test)(void))
{
    uint32_t at;

    push16(ret);
    at = test();
    if (!at)
        cpu.s = (uint16_t)(cpu.s + 2);
    return at;
}

/* On at the state at `at`, as the jump there. */
static void go_on(uint32_t at)
{
    switch (at) {
    case 0x81CCB6u: abi_PlayerStand(); return;
    case 0x81CDD2u: abi_PlayerFall(); return;
    case 0x81CFE0u: abi_PlayerJump(); return;
    case 0x81D0E9u: abi_PlayerLand(); return;
    case 0x81D470u: abi_PlayerLedgeGrab(); return;
    case 0x81D86Au: abi_PlayerHover(); return;
    case 0x81DADDu: abi_PlayerKnockback(); return;
    default: jump_routine(at); return;
    }
}

static uint32_t no_ground_fall(void);
static uint32_t crouch(void);
static uint32_t crouch_step(void);
static uint32_t crouch_jump(void);
static uint32_t air_step(void);

/* Standing, from its top (`throwing` 0) or from the throw (1). */
static void standing(int throwing)
{
    uint8_t f;
    uint32_t at;

    for (;;) {
        if (!throwing) {
            actor_yield(0x81CCCEu);
            abi_ActorDecelerateX();
            if ((at = jsr_test(0xCCD6, ground_snap_hurt))) {
                go_on(at);
                return;
            }
            abi_PlayerAppleThrow();
            throwing = (cpu.p & P_C) != 0;
        }
        if (throwing) {
            move_start(0x14);
            putd(FRAMES_4A, 0x14);
            do
                actor_yield(0x81CCE5u);
            while (dec8_at(dpa(FRAMES_4A)));
            move_start(0x00);
            throwing = 0;
        }
        if (jump_pressed()) {
            jump_at_speed();
            return;
        }
        if (pushing_ground())
            abi_PlayerGroundSlide();
        if ((at = jsr_test(0xCD0F, crouch))) {
            go_on(at);
            return;
        }
        if ((at = jsr_test(0xCD12, no_ground_fall))) {
            go_on(at);
            return;
        }
        abi_PlayerFlipByPad();
        if (ld(STAND_HELD))
            continue;
        abi_PadFacing();
        if (cpu.p & P_N)
            continue;
        /* the pad points one way: turn first when he faces the other */
        f = ldd(ACTOR_FACING);
        cmp8_of(f, rd8(dpa(ACTOR_FLIP)));
        if (f != rd8(dpa(ACTOR_FLIP))) {
            move_start(0x05);
            putd(FRAMES_40, 0x04);
            do {
                actor_yield(0x81CD2Fu);
                if (jump_pressed()) {
                    jump_at_speed();
                    return;
                }
            } while (dec8_at(dpa(FRAMES_40)));
        }
        wr8(dba(PLAYER_DASH), 0);
        if (ld(DASH_HELD)) {
            y8_load(0x00);
            abi_ActorSpeedXSet();
            put(PLAYER_DASH, 0x01);
        } else {
            abi_ActorStopX();
            putd(ACTOR_SPEED_X, 0x80);
        }
        abi_PlayerWalk();
        return;
    }
}

/*
 * @routine 81:CCB6 PlayerStand
 * @actor
 * WHAT IT DOES: Aladdin standing: each frame he slows to a stop on the
 * ground, may throw, jump, slide, crouch or fall off; when the pad points
 * one way he turns to it (4 frames) and walks.
 * HOW: PlayerFlagsClear, ACTOR_09 bit 0 cleared, PlayerMoveStart 0,
 * PLAYER_BOX_WANTED and PLAYER_DASH 0, ActorSnapToGround. Each frame (BRK
 * first): ActorDecelerateX, PlayerGroundSnapHurt, PlayerAppleThrow; a
 * throw (C set): on as PlayerThrowStanding. JUMP_PRESSED: on to
 * PlayerJump with his speed (X, A). On pushing ground, PlayerGroundSlide;
 * PlayerCrouch and PlayerNoGroundFall (each can go on elsewhere),
 * PlayerFlipByPad. STAND_HELD set, or PadFacing negative: the next frame.
 * Facing other than ACTOR_FLIP: PlayerMoveStart 5 and 4 frames (BRK
 * first, JUMP_PRESSED jumps). Then PLAYER_DASH: DASH_HELD set, speed
 * entry 0 (ActorSpeedXSet) and PLAYER_DASH 1; else ActorStopX and speed
 * fraction $80; on at PlayerWalk.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * a jump from the player's other states. Exit: never returns.
 */
void abi_PlayerStand(void)
{
    call_frame(0xCCB8, abi_PlayerFlagsClear);
    and_d(ACTOR_09, 0xFE);
    move_start(0x00);
    wr8(dba(PLAYER_BOX_WANTED), 0);
    wr8(dba(PLAYER_DASH), 0);
    jsl_frame(0x81CCCAu);
    abi_ActorSnapToGround();
    cpu.s = (uint16_t)(cpu.s + 3);
    standing(0);
}

/*
 * @routine 81:CCDC PlayerThrowStanding
 * @actor
 * WHAT IT DOES: Aladdin throws standing: the throw's pose ($14) for 20
 * frames, then he stands on as PlayerStand does after its throw test.
 * HOW: PlayerMoveStart $14, FRAMES_4A $14 frames (BRK each), PlayerMoveStart
 * 0; then PlayerStand's frame from its jump test on.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * a jump from PlayerWalk (81:CDC9), and from PlayerStand's own throw
 * test. Exit: never returns.
 */
void abi_PlayerThrowStanding(void)
{
    standing(1);
}

/*
 * @routine 81:CD5E PlayerWalk
 * @actor
 * WHAT IT DOES: Aladdin walking and running: each frame his speed follows
 * the pad, he moves across and stays on the ground; he falls when there
 * is none, jumps, skids when the pad turns, throws, or crouches.
 * HOW: V_1427, V_141C, V_1A08 and PLAYER_BOX_WANTED 0; facing from
 * ACTOR_FLIP; X = 0, PlayerMoveStart 2. Each frame (BRK first): on
 * pushing ground PlayerGroundPush; PlayerRunMove, ActorStepX,
 * PlayerOutOfWallsHurt, PlayerGroundSnapHurt; GROUND_UNDER 0: on at
 * PlayerFall. PlayerAirSteer; PLAYER_DASH from DASH_HELD; JUMP_PRESSED:
 * PlayerJump at his speed. PadFacing other than his facing: SKID_STARTED
 * 0, on at PlayerSkid. PlayerAppleThrow C set: PlayerThrowStanding.
 * PlayerCrouch, and the next frame.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * a jump from PlayerStand, PlayerSkid and PlayerLand. Exit: never
 * returns.
 */
void abi_PlayerWalk(void)
{
    uint8_t f;
    uint32_t at;

    wr8(dba(V_1427), 0);
    wr8(dba(V_141C), 0);
    wr8(dba(V_1A08), 0);
    put(PLAYER_BOX_WANTED, 0x00);
    f = ldd(ACTOR_FLIP);
    wr8(dpa(ACTOR_FACING), f);
    x8_load(0x00);
    move_start(0x02);
    for (;;) {
        actor_yield(0x81CD77u);
        if (pushing_ground())
            call_frame(0xCD89, abi_PlayerGroundPush);
        call_frame(0xCD8C, abi_PlayerRunMove);
        call_frame(0xCD8F, abi_ActorStepX);
        if ((at = jsr_test(0xCD92, out_of_walls_hurt))) {
            go_on(at);
            return;
        }
        if ((at = jsr_test(0xCD95, ground_snap_hurt))) {
            go_on(at);
            return;
        }
        if (!ld(GROUND_UNDER)) {
            abi_PlayerFall();
            return;
        }
        call_frame(0xCD9D, abi_PlayerAirSteer);
        wr8(dba(PLAYER_DASH), 0);
        if (ld(DASH_HELD))
            put(PLAYER_DASH, 0x01);
        if (jump_pressed()) {
            jump_at_speed();
            return;
        }
        call_frame(0xCDB9, abi_PadFacing);
        f = rd8(dpa(ACTOR_FACING));
        cmp8_of((uint8_t)cpu.a, f);
        if ((uint8_t)cpu.a != f) {
            wr8(dba(SKID_STARTED), 0);
            abi_PlayerSkid();
            return;
        }
        call_frame(0xCDC6, abi_PlayerAppleThrow);
        if (cpu.p & P_C) {
            abi_PlayerThrowStanding();
            return;
        }
        if ((at = jsr_test(0xCDCE, crouch))) {
            go_on(at);
            return;
        }
    }
}

/*
 * @routine 81:CDD2 PlayerFall
 * @actor
 * WHAT IT DOES: Aladdin starts to fall: the fall's speeds and pose, and
 * on into the air.
 * HOW: Y = 2, ActorMotionPresetY; PlayerMoveStart $0E; FRAMES_40 (the
 * pose step) 3; PlayerTopSpeedHold; on at PlayerAirStart.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * a jump from the player's states and the tests that find no ground.
 * Exit: never returns.
 */
void abi_PlayerFall(void)
{
    y8_load(0x02);
    jsl_frame(0x81CDD4u);
    abi_ActorMotionPresetY();
    cpu.s = (uint16_t)(cpu.s + 3);
    move_start(0x0E);
    putd(FRAMES_40, 0x03);
    call_frame(0xCDE3, abi_PlayerTopSpeedHold);
    abi_PlayerAirStart();
}

/*
 * @routine 81:CFE0 PlayerJump
 * @actor
 * WHAT IT DOES: Aladdin jumps: his top speed in the air is the speed he
 * had, the jump's rise is by how fast he ran, with the jump's sound; on
 * into the air.
 * HOW: PLAYER_LANDED 0; X to PLAYER_TOP_SPEED, A to V_141E;
 * PlayerFlipByPad; Y from JUMP_ROWS by the speed's pixels' low two bits,
 * ActorMotionPresetY; FRAMES_40 (the pose step) 0; SoundQueueAdd 1;
 * NO_LANDING 4; on at PlayerAirStart.
 * Entry: M 8-bit, X 8-bit; X = the speed's fraction, A = its pixels; D =
 * the player's record; DB = $80; reached by a jump from the player's
 * states. Exit: never returns.
 */
void abi_PlayerJump(void)
{
    uint8_t i;

    wr8(dba(PLAYER_LANDED), 0);
    wr8(dba(PLAYER_TOP_SPEED), (uint8_t)cpu.x);
    wr8(dba(V_141E), (uint8_t)cpu.a);
    call_frame(0xCFEB, abi_PlayerFlipByPad);
    i = (uint8_t)(ldd(ACTOR_SPEED_X + 1) & 0x03);
    a8_load(i);
    x8_load(i);
    y8_load(rd8(dba(JUMP_ROWS + i)));
    jsl_frame(0x81CFF4u);
    abi_ActorMotionPresetY();
    cpu.s = (uint16_t)(cpu.s + 3);
    wr8(dpa(FRAMES_40), 0);
    sound(0x01);
    put(NO_LANDING, 0x04);
    abi_PlayerAirStart();
}

/*
 * @routine 81:D005 PlayerAirStart
 * @actor
 * WHAT IT DOES: Aladdin leaves the ground: his speeds held to the air's,
 * and on into the air.
 * HOW: GROUND_PUSHED and V_1427 0; PlayerTopSpeedCap, PlayerRunRatesSet;
 * on at PlayerAirTimed.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * a jump from PlayerFall, PlayerKnockback and PlayerHover, and from
 * PlayerJump's end. Exit: never returns.
 */
void abi_PlayerAirStart(void)
{
    wr8(dba(GROUND_PUSHED), 0);
    wr8(dba(V_1427), 0);
    call_frame(0xD00D, abi_PlayerTopSpeedCap);
    call_frame(0xD010, abi_PlayerRunRatesSet);
    abi_PlayerAirTimed();
}

/* Where a frame in the air goes on: the next frame, the ground test
   (81:D0E0), or another state's address. */
#define AIR_NEXT_FRAME 0u
#define AIR_GROUND_TEST 1u

/* One frame in the air, from after its BRK. */
static uint32_t air_frame(void)
{
    uint8_t v;
    uint32_t at;

    v = ldd(ACTOR_SPEED_Y + 2);
    if (!(v & 0x80) && ld(V_1428)) {
        wr8(dba(TILE_BITS), 0);
        return 0x81D0E9u;               /* PlayerLand */
    }
    v = ld(AIR_FRAMES);
    if (!(v & 0x80))
        (void)inc8_at(dba(AIR_FRAMES));
    if (ld(JUMP_PRESSED))
        wr8(dba(AIR_FRAMES), 0);
    if (ld(AIR_THROW)) {
        if (!dec8_at(dba(AIR_THROW))) {
            putd(FRAMES_40, 0x08);
            abi_PlayerJumpPoseStep();
        }
    } else {
        abi_PlayerAppleThrow();
        if (cpu.p & P_C) {
            move_start(0x16);
            put(AIR_THROW, 0x15);
        } else {
            abi_PlayerJumpPoseStep();
        }
    }
    if (ld(HOVER_ON)) {
        /* no script runs this path */
        if (ld(HOVER_HELD)) {
            v = ldd(ACTOR_SPEED_Y + 2);
            if (!(v & 0x80))
                return 0x81D86Au;       /* PlayerHover */
        }
    }
    if ((at = jsr_test(0xD084, air_step)))
        return at;
    abi_PlayerFlipByPad();
    /* gravity: light while rising with the jump held */
    putd(ACTOR_ACCEL_Y, 0x38);
    if (!ld(JUMP_HELD)) {
        v = ldd(ACTOR_SPEED_Y + 2);
        if ((v & 0x80) && !ld(PLAYER_LANDED))
            putd(ACTOR_ACCEL_Y, 0x80);
    }
    abi_PlayerRunSpeed();
    v = ldd(ACTOR_SPEED_Y + 2);
    if (v & 0x80)
        return AIR_NEXT_FRAME;
    /* falling */
    v = ld(LEVEL);
    cmp8_of(v, 0x05);
    if (v == 0x05) {
        putd(ACTOR_18, 0x18);
        abi_GroundBehindBit4();
        if (!(cpu.p & P_Z)) {
            put(V_1A10, 0x01);
            wr8(dba(V_1A12), 0x01);
        }
    }
    if (ld(V_1428))
        return 0x81D0E9u;               /* PlayerLand */
    v = (uint8_t)(ld(PAD1_DIRS) & PAD_DOWN);
    a8_load(v);
    if (!v && !ld(NO_LANDING)) {
        abi_GroundBehindBit3();
        return (cpu.p & P_Z) ? AIR_GROUND_TEST : 0x81D470u;    /* or PlayerLedgeGrab */
    }
    if (ld(NO_LANDING))
        (void)dec8_at(dba(NO_LANDING));
    return AIR_GROUND_TEST;
}

/* The air's frames: the BRK first (from_brk) or the frame's test first. */
static void in_air(int from_brk)
{
    uint32_t at;

    for (;;) {
        if (from_brk)
            actor_yield(0x81D02Cu);
        from_brk = 1;
        at = air_frame();
        if (at == AIR_NEXT_FRAME)
            continue;
        if (at == AIR_GROUND_TEST) {
            /* 81:D0E0: in the ground he lands, else the next frame */
            abi_ActorOutOfGround();
            if (cpu.p & P_Z)
                continue;
            abi_PlayerLand();
            return;
        }
        go_on(at);
        return;
    }
}

/*
 * @routine 81:D011 PlayerAirTimed
 * @actor
 * WHAT IT DOES: Aladdin in the air from the ground: the air's settings,
 * and his first frame in the air now.
 * HOW: V_1A08 $0A, PLAYER_BOX_WANTED 3, V_141C up, AIR_FRAMES 0, ACTOR_09
 * bit 0 cleared, PlayerGroundYSave; then PlayerAir's frame with no BRK
 * first.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached
 * from PlayerAirStart's end, and by a jump from the hang's climb
 * (81:D5DE). Exit: never returns.
 */
void abi_PlayerAirTimed(void)
{
    put(V_1A08, 0x0A);
    put(PLAYER_BOX_WANTED, 0x03);
    (void)inc8_at(dba(V_141C));
    wr8(dba(AIR_FRAMES), 0);
    and_d(ACTOR_09, 0xFE);
    call_frame(0xD029, abi_PlayerGroundYSave);
    in_air(0);
}

/*
 * @routine 81:D02C PlayerAir
 * @actor
 * WHAT IT DOES: Aladdin in the air, a frame at a time: his jump's pose,
 * a throw, his fall and its gravity (lighter while the jump is held
 * rising), and a ledge to catch, until he lands.
 * HOW: Each frame (BRK first). Falling with V_1428 set: TILE_BITS 0, on
 * at PlayerLand. AIR_FRAMES up while under $80 (0 with JUMP_PRESSED).
 * AIR_THROW counting down: at 0 FRAMES_40 (the pose step) 8 and
 * PlayerJumpPoseStep; else PlayerAppleThrow: a throw starts move $16 and
 * AIR_THROW $15, else PlayerJumpPoseStep. With HOVER_ON (no script runs
 * it): HOVER_HELD while falling, on at PlayerHover. PlayerAirStep,
 * PlayerFlipByPad; ACTOR_ACCEL_Y $38, or $80 rising with the jump not
 * held and not PLAYER_LANDED; PlayerRunSpeed. Rising: the next frame.
 * Level 5: GroundBehindBit4 at probe $18 sets V_1A10 and V_1A12 to 1.
 * V_1428: PlayerLand. Not down on the pad and NO_LANDING 0:
 * GroundBehindBit3 nonzero catches a ledge (PlayerLedgeGrab); else
 * NO_LANDING down. Then the ground test of PlayerAirLand.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * a jump from PlayerAirLand, the hang's climb and PlayerAirTimed's own
 * frames. Exit: never returns.
 */
void abi_PlayerAir(void)
{
    in_air(1);
}

/*
 * @routine 81:D0E0 PlayerAirLand
 * @actor
 * WHAT IT DOES: Aladdin in the air: when he is in the ground he lands,
 * else his next frame in the air.
 * HOW: ActorOutOfGround; Z clear: on at PlayerLand; else at PlayerAir.
 * Entry: M 8-bit, X 8-bit (M 16-bit from PlayerLedgeGrab's paths no
 * script runs); D = the player's record; DB = $80; reached at the end of
 * PlayerAir's frame and by a jump from PlayerLedgeGrab. Exit: never
 * returns.
 */
void abi_PlayerAirLand(void)
{
    jsl_frame(0x81D0E0u);
    abi_ActorOutOfGround();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (!(cpu.p & P_Z)) {
        abi_PlayerLand();
        return;
    }
    in_air(1);
}

/*
 * @routine 81:D0E9 PlayerLand
 * @actor
 * WHAT IT DOES: Aladdin lands: on ground that hurts him he is knocked
 * back; else the landing's sound, and after a fall of more than 128
 * pixels a hard landing (18 frames) and he stands; else he walks on at
 * his speed (a run's at least $0180, a dash's $0200) or stands.
 * HOW: ActorSnapToGround; NO_LANDING, AIR_THROW, V_141C and PLAYER_LANDED
 * 0. ground_hurts: PlayerEnter8CLong, on at PlayerKnockback. SoundQueueAdd
 * $10. (16-bit) PLAYER_GROUND_Y + $80 below his y: PlayerMoveStart $0F,
 * FRAMES_47 $12 frames (BRK each), ActorSpeedXClear, PlayerStand. Else
 * PadFacing negative: ActorSpeedXClear, PlayerStand. DASH_HELD: speed
 * $0200; else his speed held to $0180 at most; on at PlayerWalk.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * a jump from the air's states. Exit: never returns.
 */
void abi_PlayerLand(void)
{
    uint16_t v;

    jsl_frame(0x81D0E9u);
    abi_ActorSnapToGround();
    cpu.s = (uint16_t)(cpu.s + 3);
    wr8(dba(NO_LANDING), 0);
    wr8(dba(AIR_THROW), 0);
    wr8(dba(V_141C), 0);
    wr8(dba(PLAYER_LANDED), 0);
    if (ground_hurts()) {
        jsl_frame(0x81D10Du);
        abi_PlayerEnter8CLong();
        cpu.s = (uint16_t)(cpu.s + 3);
        abi_PlayerKnockback();
        return;
    }
    sound(0x10);
    op_rep(P_M);
    a16_load(rd16(dba(PLAYER_GROUND_Y)));
    flag_set(P_C, 0);
    cpu.a = op_adc16(cpu.a, 0x0080, &cpu.p);
    v = cpu.a;
    cmp16_of(v, dp16(ACTOR_Y + 1));
    if (v < dp16(ACTOR_Y + 1)) {
        op_sep(P_M);
        move_start(0x0F);
        putd(FRAMES_47, 0x12);
        do
            actor_yield(0x81D157u);
        while (dec8_at(dpa(FRAMES_47)));
    } else {
        op_sep(P_M);
        call_frame(0xD12D, abi_PadFacing);
        if (!(cpu.p & P_N)) {
            if (ld(DASH_HELD)) {
                op_rep(P_M);
                a16_load(0x0200);
                dp16_set(ACTOR_SPEED_X, 0x0200);
            } else {
                op_rep(P_M);
                a16_load(0x0180);
                cmp16_of(0x0180, dp16(ACTOR_SPEED_X));
                if (0x0180 < dp16(ACTOR_SPEED_X))
                    dp16_set(ACTOR_SPEED_X, 0x0180);
            }
            op_sep(P_M);
            abi_PlayerWalk();
            return;
        }
    }
    jsl_frame(0x81D15Du);
    abi_ActorSpeedXClear();
    cpu.s = (uint16_t)(cpu.s + 3);
    abi_PlayerStand();
}

/*
 * @routine 81:D27F PlayerSkid
 * @actor
 * WHAT IT DOES: Aladdin skids as the pad turns him while he runs: the
 * skid's sound and pose (a fast run's own, from $0180), slowing until he
 * stops and stands, walks on the new way, falls, jumps or throws.
 * HOW: ACTOR_ACCEL_X $10. Each turn: PlayerFlipByPad; the pad pointing
 * the other way (its facing XOR his, less 1, zero): PlayerSpawnEvery8,
 * and once (SKID_STARTED) SoundQueueAdd $0A, ACTOR_ACCEL_X $10,
 * PlayerMoveStart $12 (from speed $0180: SoundQueueAdd 4 and $13; from
 * there also ACTOR_09 bit 0 and PLAYER_BOX_WANTED cleared). No direction:
 * ACTOR_ACCEL_X $20; PlayerAppleThrow's throw: move $14 for $14 frames
 * (COP), move 0, ActorStopX. Then a frame (BRK first): ActorStepX;
 * ActorOutOfWalls C set: ActorStopX, PlayerStand. Not V_1428 and no
 * ground (ActorSnapToGround Z): PlayerFall. JUMP_PRESSED: PlayerJump
 * with X $80, A 1. PadFacing his facing: it to ACTOR_FLIP, PlayerWalk.
 * PlayerRunMove until SKID_STARTED; ActorDecelerateX negative or the
 * speed's word 0: ActorStopX, PlayerStand. PlayerCrouch, and again.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * a jump from PlayerWalk (81:CDC1). Exit: never returns.
 */
void abi_PlayerSkid(void)
{
    uint8_t v;
    uint16_t s;
    uint32_t at;

    putd(ACTOR_ACCEL_X, 0x10);
    for (;;) {
        call_frame(0xD285, abi_PlayerFlipByPad);
        if (cpu.p & P_N) {
            putd(ACTOR_ACCEL_X, 0x20);
            call_frame(0xD2D8, abi_PlayerAppleThrow);
            if (cpu.p & P_C) {
                move_start(0x14);
                a8_load(0x14);
                actor_wait(0x81D2E2u);
                move_start(0x00);
                abi_ActorStopX();
            }
        } else {
            v = (uint8_t)(((uint8_t)cpu.a ^ rd8(dpa(ACTOR_FACING))) - 1);
            a8_load(v);
            if (!v) {
                call_frame(0xD28F, abi_PlayerSpawnEvery8);
                if (!ld(SKID_STARTED)) {
                    (void)inc8_at(dba(SKID_STARTED));
                    sound(0x0A);
                    putd(ACTOR_ACCEL_X, 0x10);
                    a8_load(0x12);
                    op_rep(P_X);
                    s = dp16(ACTOR_SPEED_X);
                    cpu.x = s;
                    cmp16_of(s, 0x0180);
                    if (s >= 0x0180) {
                        sound(0x04);
                        a8_load(0x13);
                    }
                    op_sep(P_X);
                    abi_PlayerMoveStart();
                    op_rep(P_M);
                    s = dp16(ACTOR_SPEED_X);
                    a16_load(s);
                    cmp16_of(s, 0x0180);
                    op_sep(P_M);
                    if (s >= 0x0180) {
                        and_d(ACTOR_09, 0xFE);
                        put(PLAYER_BOX_WANTED, 0x00);
                    }
                }
            }
        }
        actor_yield(0x81D2EDu);
        call_frame(0xD2F1, abi_ActorStepX);
        jsl_frame(0x81D2F2u);
        abi_ActorOutOfWalls();
        cpu.s = (uint16_t)(cpu.s + 3);
        if (!(cpu.p & P_C)) {
            if (!ld(V_1428)) {
                jsl_frame(0x81D2FDu);
                abi_ActorSnapToGround();
                cpu.s = (uint16_t)(cpu.s + 3);
                if (cpu.p & P_Z) {
                    abi_PlayerFall();
                    return;
                }
            }
            if (ld(JUMP_PRESSED)) {
                x8_load(0x80);
                a8_load(0x01);
                abi_PlayerJump();
                return;
            }
            call_frame(0xD314, abi_PadFacing);
            if (!(cpu.p & P_N)) {
                v = (uint8_t)cpu.a;
                cmp8_of(v, rd8(dpa(ACTOR_FACING)));
                if (v == rd8(dpa(ACTOR_FACING))) {
                    wr8(dpa(ACTOR_FLIP), v);
                    abi_PlayerWalk();
                    return;
                }
            }
            if (!ld(SKID_STARTED))
                call_frame(0xD327, abi_PlayerRunMove);
            jsl_frame(0x81D328u);
            abi_ActorDecelerateX();
            cpu.s = (uint16_t)(cpu.s + 3);
            if (!(cpu.p & P_N)) {
                op_rep(P_M);
                s = dp16(ACTOR_SPEED_X);
                s = (uint16_t)(s | rd16(dpa(ACTOR_SPEED_X + 1)));
                a16_load(s);
                if (s) {
                    op_sep(P_M);
                    if ((at = jsr_test(0xD343, crouch))) {
                        go_on(at);
                        return;
                    }
                    continue;
                }
            }
        }
        op_sep(P_M);
        abi_ActorStopX();
        abi_PlayerStand();
        return;
    }
}

/* PlayerNoGroundFall's work: 0 when it returns, else PlayerFall's
   address, its return address pulled */
static uint32_t no_ground_fall(void)
{
    if (ld(V_1428))
        return 0;
    wr8(dpa(ACTOR_18), 0);
    abi_GroundLeftLong();
    if (cpu.p & P_C)
        return 0;
    abi_GroundRightLong();
    if (cpu.p & P_C)
        return 0;
    pla8();                               /* its own return dropped */
    pla8();
    return 0x81CDD2u;                     /* PlayerFall */
}

/*
 * @routine 81:D347 PlayerNoGroundFall
 * WHAT IT DOES: When there is no ground to either side of Aladdin's feet,
 * he falls (unless V_1428 is set).
 * HOW: V_1428 set: back. ACTOR_18 0; GroundLeftLong C set, or
 * GroundRightLong C set: back. Else its return address dropped (PLA
 * PLA) and on at PlayerFall.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from the player's states at their level (81:CD10, 81:D3D0). Exit:
 * A, N, Z and C as the last test left them. No ground: never returns.
 */
void abi_PlayerNoGroundFall(void)
{
    uint32_t at = no_ground_fall();

    if (at)
        actor_leave(at);
}

/* PlayerCrouch's work: 0 when down is not held, else where his code goes
   on at the end (PlayerStand, or where a test it calls sends him), its
   return address pulled at the start */
static uint32_t crouch(void)
{
    uint8_t f;
    uint32_t at;

    ld(PAD1_DIRS);
    if (!bit_a(PAD_DOWN))
        return 0;
    pla8();                               /* its own return dropped: on at the actor's level */
    pla8();
    put(PLAYER_BOX_WANTED, 0x01);
    move_start(0x01);
    a8_load(0x01);
    or_d(ACTOR_09, 0x01);
    putd(ACTOR_ACCEL_X, 0x28);
    f = ldd(ACTOR_FLIP);
    wr8(dpa(ACTOR_FACING), f);
    for (;;) {
        actor_yield(0x81D382u);
        abi_ActorSnapToGround();
        if (ground_hurts()) {
            abi_PlayerEnter8CLong();
            return 0x81DADDu;       /* PlayerKnockback */
        }
        abi_PlayerFlipByPad();
        f = rd8(dpa(ACTOR_SPEED_X));
        f = (uint8_t)(f | rd8(dpa(ACTOR_SPEED_X + 1)));
        f = (uint8_t)(f | rd8(dpa(ACTOR_SPEED_X + 2)));
        a8_load(f);
        if (!f) {
            wr8(dpa(ACTOR_ACCEL_X), 0);
        } else {
            abi_ActorDecelerateX();
            if ((at = jsr_test(0xD3B9, crouch_step)))
                return at;
        }
        abi_PlayerAppleThrow();
        if (cpu.p & P_C) {
            move_start(0x15);
            a8_load(0x18);
            actor_wait(0x81D3C6u);
            move_start(0x01);
        }
        if ((at = jsr_test(0xD3CF, crouch_jump)))
            return at;
        if ((at = jsr_test(0xD3D2, no_ground_fall)))
            return at;
        ld(PAD1_DIRS);
        if (!bit_a(PAD_DOWN))
            break;
    }
    f = ldd(ACTOR_FLIP);
    wr8(dpa(ACTOR_FACING), f);
    return 0x81CCB6u;               /* PlayerStand */
}

/* PlayerCrouchStep's work: 0 when it returns, else where
   PlayerOutOfWallsHurt sends him */
static uint32_t crouch_step(void)
{
    uint32_t at;

    abi_ActorStepX();
    if ((at = jsr_test(0xD3E6, out_of_walls_hurt)))
        return at;
    if (ld(V_1428))
        return 0;
    abi_ActorSnapToGround();
    ld(GROUND_UNDER);
    return 0;
}

/*
 * @routine 81:D3E1 PlayerCrouchStep
 * WHAT IT DOES: Moves Aladdin across by his speed as he slides crouched,
 * out of walls, and down onto the ground.
 * HOW: ActorStepX, PlayerOutOfWallsHurt; V_1428 set: back. Else
 * ActorSnapToGround and GROUND_UNDER loaded.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from PlayerCrouch (81:D3B7). Exit: A low = V_1428 or GROUND_UNDER,
 * N and Z from it; X and Y as the calls left them.
 */
void abi_PlayerCrouchStep(void)
{
    uint32_t at = crouch_step();

    if (at)
        actor_leave(at);
}

/* PlayerCrouchJump's work: 0 when it returns, else PlayerJump's address
   (X $80, A 1), its return address pulled */
static uint32_t crouch_jump(void)
{
    if (!ld(JUMP_PRESSED))
        return 0;
    if (ld(V_1446))
        return 0;
    pla8();                               /* its own return dropped */
    pla8();
    x8_load(0x80);
    a8_load(0x01);
    return 0x81CFE0u;               /* PlayerJump */
}

/*
 * @routine 81:D3F4 PlayerCrouchJump
 * WHAT IT DOES: When the jump is pressed while Aladdin crouches (and
 * V_1446 allows it), he jumps.
 * HOW: JUMP_PRESSED 0 or V_1446 set: back (by the RTS that ends
 * PlayerCrouchStep). Else its return address dropped (PLA PLA), X =
 * $80, A = 1, on at PlayerJump.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from PlayerCrouch at the actor's level (81:D3CD). Exit: A low as
 * loaded, N and Z from it. Jump: never returns.
 */
void abi_PlayerCrouchJump(void)
{
    uint32_t at = crouch_jump();

    if (at)
        actor_leave(at);
}

/* PlayerAirStep's work: 0 when it returns, else where a test it calls
   sends him, the return addresses they drop pulled */
static uint32_t air_step(void)
{
    uint32_t at;

    abi_ActorGravity();
    abi_ActorStepX();
    abi_ActorMoveY();
    if ((at = jsr_test(0xD413, land_or_hurt)))
        return at;
    abi_ActorOutOfWalls();
    return jsr_test(0xD41A, ground_hurt_test);
}

/*
 * @routine 81:D407 PlayerAirStep
 * WHAT IT DOES: Moves Aladdin through the air for a frame: gravity, his
 * speeds, landing on a floor, out of walls, and ground that hurts.
 * HOW: ActorGravity, ActorStepX, ActorMoveY, PlayerLandOrHurt,
 * ActorOutOfWalls, PlayerGroundHurtTest (which can drop this call's
 * return address with its own).
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from the player's air states at their level (81:D082 and others).
 * Exit: as PlayerGroundHurtTest. Hurt: never returns.
 */
void abi_PlayerAirStep(void)
{
    uint32_t at = air_step();

    if (at)
        actor_leave(at);
}

/*
 * @routine 81:D470 PlayerLedgeGrab
 * @actor
 * WHAT IT DOES: Aladdin catches a ledge as he falls past it: set at the
 * ledge's corner, his speeds stopped, the grab's sound and pose; on to
 * hang from it.
 * HOW: (16-bit A) the ledge tile's mirror flag (TILE_MIRRORED, X) and his
 * ACTOR_FLIP must differ (else, paths no script runs, on at
 * PlayerAirLand); his x pixels the probe's x to its 16 plus 1 (or plus
 * $0F, mirrored), his y pixels the probe's y to its 16 plus 1. PLAYER_LANDED
 * and AIR_THROW 0, V_1427 up, PlayerFlagsClear, ActorStopX, ActorStopY,
 * SoundQueueAdd 6, PlayerMoveStart 6, PLAYER_BOX_WANTED 4; on at
 * PlayerHang.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * a jump from PlayerAir (81:D0D5) and PlayerHover. Exit: never returns.
 */
void abi_PlayerLedgeGrab(void)
{
    uint16_t v;
    uint8_t m;

    op_rep(P_M);
    m = rd8(dba(TILE_MIRRORED));
    x8_load(m);
    if (!m) {
        x8_load(rd8(dpa(ACTOR_FLIP)));
        if (cpu.x) {
            abi_PlayerAirLand();
            return;
        }
        v = (uint16_t)(rd16(dba(POINT_X)) & 0xFFF0);
        a16_load(v);
        flag_set(P_C, 0);
        cpu.a = op_adc16(v, 0x0001, &cpu.p);
    } else {
        x8_load(rd8(dpa(ACTOR_FLIP)));
        if (!cpu.x) {
            abi_PlayerAirLand();
            return;
        }
        v = (uint16_t)(rd16(dba(POINT_X)) & 0xFFF0);
        a16_load(v);
        flag_set(P_C, 0);
        cpu.a = op_adc16(v, 0x000F, &cpu.p);
    }
    dp16_set(ACTOR_X + 1, cpu.a);
    v = (uint16_t)(rd16(dba(POINT_Y)) & 0xFFF0);
    a16_load(v);
    flag_set(P_C, 0);
    cpu.a = op_adc16(v, 0x0001, &cpu.p);
    dp16_set(ACTOR_Y + 1, cpu.a);
    op_sep(P_M);
    wr8(dba(PLAYER_LANDED), 0);
    wr8(dba(AIR_THROW), 0);
    (void)inc8_at(dba(V_1427));
    call_frame(0xD4B8, abi_PlayerFlagsClear);
    jsl_frame(0x81D4B9u);
    abi_ActorStopX();
    cpu.s = (uint16_t)(cpu.s + 3);
    jsl_frame(0x81D4BDu);
    abi_ActorStopY();
    cpu.s = (uint16_t)(cpu.s + 3);
    sound(0x06);
    move_start(0x06);
    put(PLAYER_BOX_WANTED, 0x04);
    abi_PlayerHang();
}

/*
 * @routine 81:D4D1 PlayerHang
 * @actor
 * WHAT IT DOES: Aladdin hangs from a ledge: up climbs onto it (move 7,
 * 32 frames, then up 24 pixels and across 8, standing); down lets go (18
 * pixels lower, falling, with no hold for 16 frames).
 * HOW: Each frame (BRK first): PAD1_DIRS bit 3 (up): PlayerMoveStart 7,
 * PLAYER_BOX_WANTED 5, COP $20; V_1427 0, facing from ACTOR_FLIP; speed
 * $000800 across and $FFE800 down, ActorMove once, both speeds cleared;
 * on at PlayerStand. Bit 2 (down): facing from ACTOR_FLIP, NO_LANDING
 * $10, y $12 lower (16-bit), V_1427 0, on at PlayerFall.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached
 * from PlayerLedgeGrab's end and by a jump from 81:D9C2. Exit: never
 * returns.
 */
void abi_PlayerHang(void)
{
    uint8_t v;

    for (;;) {
        actor_yield(0x81D4D1u);
        v = (uint8_t)(rd8(dba(PAD1_DIRS)) & PAD_UP);
        a8_load(v);
        if (v)
            break;
        v = (uint8_t)(rd8(dba(PAD1_DIRS)) & PAD_DOWN);
        a8_load(v);
        if (v) {
            v = ldd(ACTOR_FLIP);
            wr8(dpa(ACTOR_FACING), v);
            put(NO_LANDING, 0x10);
            op_rep(P_M);
            a16_load(dp16(ACTOR_Y + 1));
            flag_set(P_C, 0);
            cpu.a = op_adc16(cpu.a, 0x0012, &cpu.p);
            dp16_set(ACTOR_Y + 1, cpu.a);
            op_sep(P_M);
            wr8(dba(V_1427), 0);
            abi_PlayerFall();
            return;
        }
    }
    move_start(0x07);
    put(PLAYER_BOX_WANTED, 0x05);
    a8_load(0x20);
    actor_wait(0x81D50Au);
    wr8(dba(V_1427), 0);
    v = ldd(ACTOR_FLIP);
    wr8(dpa(ACTOR_FACING), v);
    putd(ACTOR_SPEED_X + 2, 0x00);
    putd(ACTOR_SPEED_Y + 2, 0xFF);
    op_rep(P_M);
    a16_load(0x0800);
    dp16_set(ACTOR_SPEED_X, 0x0800);
    a16_load(0xE800);
    dp16_set(ACTOR_SPEED_Y, 0xE800);
    op_sep(P_M);
    jsl_frame(0x81D529u);
    abi_ActorMove();
    cpu.s = (uint16_t)(cpu.s + 3);
    jsl_frame(0x81D52Du);
    abi_ActorSpeedXClear();
    cpu.s = (uint16_t)(cpu.s + 3);
    jsl_frame(0x81D531u);
    abi_ActorSpeedYClear();
    cpu.s = (uint16_t)(cpu.s + 3);
    abi_PlayerStand();
}

/*
 * @routine 81:D86A PlayerHover
 * @actor
 * WHAT IT DOES: Aladdin hovers while the fourth action is held (with
 * HOVER_ON): his own pose and a slow fall at speed $0200 across; let go,
 * he falls; he can land, catch a ledge or meet ground that hurts. No
 * script runs it.
 * HOW: PLAYER_BOX_WANTED $12, V_141B 0, PlayerMoveStart $0C; Y = $18,
 * ActorSpeedYSet; top speed $0200. Each frame (from the second, BRK
 * first): ActorStepX, ActorMoveY; his y pixels' high byte not negative:
 * ActorOutOfWalls and PlayerGroundHurtJump. PlayerFlipByPad. V_1428:
 * the landing (below). HOVER_HELD 0: gravity $36, the pose step 1 with 1
 * frame, on at PlayerAirStart. Level 5: GroundBehindBit4 at probe $18
 * sets V_1A10 and V_1A12. PlayerRunSpeed, PlayerOutOfWallsHurt;
 * ActorOutOfGround Z clear: the landing. NO_LANDING: down by 1. Else
 * GroundBehindBit3 Z clear: PlayerLedgeGrab. The landing:
 * PLAYER_BOX_WANTED 3, y fraction and the vertical speed cleared, on at
 * PlayerLand.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * a jump from the player's air states with HOVER_ON (81:D07F and three
 * more). Exit: never returns.
 */
void abi_PlayerHover(void)
{
    uint8_t v;
    uint32_t at;

    put(PLAYER_BOX_WANTED, 0x12);
    wr8(dba(V_141B), 0);
    move_start(0x0C);
    y8_load(0x18);
    jsl_frame(0x81D879u);
    abi_ActorSpeedYSet();
    cpu.s = (uint16_t)(cpu.s + 3);
    put(V_141E, 0x02);
    wr8(dba(PLAYER_TOP_SPEED), 0);
    for (int first = 1;; first = 0) {
        if (!first)
            actor_yield(0x81D887u);
        call_frame(0xD88B, abi_ActorStepX);
        jsl_frame(0x81D88Cu);
        abi_ActorMoveY();
        cpu.s = (uint16_t)(cpu.s + 3);
        v = ldd(ACTOR_Y + 2);
        if (!(v & 0x80)) {
            jsl_frame(0x81D894u);
            abi_ActorOutOfWalls();
            cpu.s = (uint16_t)(cpu.s + 3);
            if ((at = jsr_test(0xD89A, ground_hurt_jump))) {
                go_on(at);
                return;
            }
        }
        call_frame(0xD89D, abi_PlayerFlipByPad);
        if (ld(V_1428))
            break;
        if (!ld(HOVER_HELD)) {
            putd(ACTOR_ACCEL_Y, 0x36);
            putd(FRAMES_40, 0x01);
            wr8(dpa(ACTOR_41), 0x01);
            abi_PlayerAirStart();
            return;
        }
        v = ld(LEVEL);
        cmp8_of(v, 0x05);
        if (v == 0x05) {
            putd(ACTOR_18, 0x18);
            jsl_frame(0x81D8C0u);
            abi_GroundBehindBit4();
            cpu.s = (uint16_t)(cpu.s + 3);
            if (!(cpu.p & P_Z)) {
                put(V_1A10, 0x01);
                wr8(dba(V_1A12), 0x01);
            }
        }
        call_frame(0xD8D0, abi_PlayerRunSpeed);
        if ((at = jsr_test(0xD8D3, out_of_walls_hurt))) {
            go_on(at);
            return;
        }
        jsl_frame(0x81D8D4u);
        abi_ActorOutOfGround();
        cpu.s = (uint16_t)(cpu.s + 3);
        if (!(cpu.p & P_Z))
            break;
        if (ld(NO_LANDING)) {
            (void)dec8_at(dba(NO_LANDING));
            continue;
        }
        jsl_frame(0x81D8DFu);
        abi_GroundBehindBit3();
        cpu.s = (uint16_t)(cpu.s + 3);
        if (!(cpu.p & P_Z)) {
            abi_PlayerLedgeGrab();
            return;
        }
    }
    put(PLAYER_BOX_WANTED, 0x03);
    wr8(dpa(ACTOR_Y), 0);
    wr8(dpa(ACTOR_SPEED_Y), 0);
    wr8(dpa(ACTOR_SPEED_Y + 1), 0);
    wr8(dpa(ACTOR_SPEED_Y + 2), 0);
    abi_PlayerLand();
}

/* Each frame of the knockback's end (BRK at brk_at): out of the ground,
   and flying when it is not. */
static void ending_frame(uint32_t brk_at)
{
    actor_yield(brk_at);
    abi_ActorOutOfGround();
    if (!(cpu.p & P_Z))
        return;
    if (ld(V_1428))
        return;
    abi_ActorFly();
}

/*
 * @routine 81:DADD PlayerKnockback
 * @actor
 * WHAT IT DOES: Aladdin is hit: knocked back the way he faced, hurt's
 * pose and sound, for $18 frames or until he lands, then he stands
 * (blinking) or falls on. With no health left, or fallen out of the map,
 * he loses a life: the song, and once the scene is over the stage starts
 * again (or ends with no lives).
 * HOW: HEALTH 0: NO_LANDING $FF. Y = 5 ($56 on level 6), unless
 * FALLEN_OUT (then straight to the life's loss); GROUND_PUSHED and V_1427
 * 0; ActorMotionPreset; SoundQueueAdd 2; PLAYER_BOX_WANTED 6,
 * PlayerBoxSet; PlayerMoveStartAlso $17; gravity $30; facing turned
 * round for the tests; HURT_FRAMES $18, V_141C 0. Each frame (BRK first):
 * ActorFallCap, ActorFly, PlayerLandOrHurt, ActorOutOfWalls on both sides;
 * with health HURT_FRAMES down, negative: the end. Rising: again.
 * PlayerFallenTest C clear: the life's loss. Level 5: GroundBehindBit4 at
 * probe $18 sets V_1A10, V_1A12 (no script runs it). V_1428, or
 * ActorOutOfGround Z clear: the end. The end: PLAYER_FLAGS bit 1 off and
 * bit 2 on; no health: the life's loss with move 8. Else (16-bit) y less
 * 13 unless V_1428, PLAYER_HURT_TIME $7E, ACTOR_08 bit 4 and ACTOR_09
 * bit 7 set, V_141B 0, PlayerFlagsClear; HURT_FRAMES not negative:
 * PlayerStand; else the pose step 1 with 1 frame, PlayerAirStart. Fallen
 * out: HEALTH 0, ACTOR_09 bit 7 off, ACTOR_08 bit 7 on and bit 3 off,
 * FALLEN_OUT 0, V_1A10, V_1A12 and SCENE_ON up, V_1A08 $FF. The life's
 * loss: a frame, SongPlay $2A, VIEW_EDGE_RULE 4, NO_LANDING $FF, V_1A10
 * 1, Y = 2 ActorMotionPresetY, $B0 frames (out of the ground, flying);
 * TaskCreate; frames until SCENE_WAIT is 0; V_141B 0, Level035CSet, LIVES
 * down; TaskExec to one of two entries by whether any are left.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * the actor loop's jump (through the stub at $83:91FD), and
 * by a jump from the player's states when ground hurts him. Exit: never
 * returns.
 */
static void knockback(int entry);

void abi_PlayerKnockback(void)
{
    knockback(0);
}

/*
 * @routine 81:DAEB PlayerKnockbackFromLedge
 * @actor
 * WHAT IT DOES: PlayerKnockback with its motion preset 2 (as from a
 * ledge). No input reaches it: only the JMP at 81:DCFD names it, in code
 * ($81:DCE4-DCFF) nothing reaches; cov_knockback_entries.box runs it from
 * his record with its resume address set to it.
 * HOW: Y = 2; then as PlayerKnockback from its start.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * the actor loop's RTI at a resume address set to it. Exit: never
 * returns.
 */
void abi_PlayerKnockbackFromLedge(void)
{
    knockback(1);
}

/*
 * @routine 81:DBCF PlayerKnockbackNoHealth
 * @actor
 * WHAT IT DOES: Aladdin knocked back with no health left: his life is
 * lost (PlayerKnockback's own path there). No input reaches this entry:
 * it is a JMP right after PlayerKnockback's JMP to PlayerAirStart, and
 * nothing in the ROM names $81:DBCF. cov_knockback_entries.box runs it
 * from his record with its resume address set to it.
 * HOW: JMP to the life's loss as PlayerKnockback takes it with no health.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * the actor loop's RTI at a resume address set to it. Exit: never
 * returns.
 */
void abi_PlayerKnockbackNoHealth(void)
{
    knockback(2);
}

/* PlayerKnockback's body, entered at its start (0), at $81:DAEB (1) or
   at $81:DBCF (2). */
static void knockback(int entry)
{
    uint8_t v;
    uint32_t at;
    int lost = 0, fallen = 0;

    if (entry == 2)
        goto no_health;
    if (entry == 1) {
        y8_load(0x02);
        goto start;
    }
    if (!ld(HEALTH))
        put(NO_LANDING, 0xFF);
    y8_load(0x05);
start:
    wr8(dba(GROUND_PUSHED), 0);
    wr8(dba(V_1427), 0);
    if (ld(FALLEN_OUT)) {
        fallen = 1;
    } else {
        v = ld(LEVEL);
        flag_set(P_C, 1);
        cmp8_of(v, 0x06);
        if (v == 0x06)
            y8_load(0x56);
        jsl_frame(0x81DB05u);
        abi_ActorMotionPreset();
        cpu.s = (uint16_t)(cpu.s + 3);
        sound(0x02);
        x8_load(0x06);
        wr8(dba(PLAYER_BOX_WANTED), 0x06);
        call_frame(0xDB16, abi_PlayerBoxSet);
        a8_load(0x17);
        abi_PlayerMoveStartAlso();
        putd(ACTOR_ACCEL_Y, 0x30);
        v = (uint8_t)(rd8(dpa(ACTOR_FACING)) ^ 0x01);
        a8_load(v);
        wr8(dpa(ACTOR_FACING), v);
        put(HURT_FRAMES, 0x18);
        wr8(dba(V_141C), 0);
        for (;;) {
            actor_yield(0x81DB2Eu);
            call_frame(0xDB32, abi_ActorFallCap);
            abi_ActorFly();
            if ((at = jsr_test(0xDB39, land_or_hurt))) {
                go_on(at);
                return;
            }
            abi_ActorOutOfWalls();
            v = (uint8_t)(rd8(dpa(ACTOR_FACING)) ^ 0x01);
            a8_load(v);
            wr8(dpa(ACTOR_FACING), v);
            abi_ActorOutOfWalls();
            v = (uint8_t)(rd8(dpa(ACTOR_FACING)) ^ 0x01);
            a8_load(v);
            wr8(dpa(ACTOR_FACING), v);
            if (ld(HEALTH) && (dec8_at(dba(HURT_FRAMES)) & 0x80))
                break;
            if (ldd(ACTOR_SPEED_Y + 2) & 0x80)
                continue;
            call_frame(0xDB5E, abi_PlayerFallenTest);
            if (!(cpu.p & P_C)) {
                fallen = 1;
                break;
            }
            v = ld(LEVEL);
            cmp8_of(v, 0x05);
            if (v == 0x05) {
                /* no script runs this path */
                putd(ACTOR_18, 0x18);
                jsl_frame(0x81DB6Cu);
                abi_GroundBehindBit4();
                cpu.s = (uint16_t)(cpu.s + 3);
                if (!(cpu.p & P_Z)) {
                    put(V_1A10, 0x01);
                    wr8(dba(V_1A12), 0x01);
                }
            }
            if (ld(V_1428))
                break;
            abi_ActorOutOfGround();
            if (!(cpu.p & P_Z))
                break;
        }
    }
    if (fallen) {
        op_sep(P_M | P_X);
        wr8(dba(HEALTH), 0);
        and_d(ACTOR_09, 0x7F);
        a8_load(0x80);
        tsb_at(dpa(ACTOR_08), 0x80);
        wr8(dba(FALLEN_OUT), 0);
        (void)inc8_at(dba(V_1A10));
        (void)inc8_at(dba(V_1A12));
        (void)inc8_at(dba(SCENE_ON));
        put(V_1A08, 0xFF);
        and_d(ACTOR_08, 0xF7);
        lost = 1;
    } else {
        op_sep(P_M);
        v = (uint8_t)((rd8(dba(PLAYER_FLAGS)) & 0xFD) | 0x04);
        a8_load(v);
        wr8(dba(PLAYER_FLAGS), v);
        if (!ld(HEALTH)) {
no_health:
            (void)inc8_at(dba(SCENE_ON));
            x8_load(0x06);
            wr8(dba(PLAYER_BOX_WANTED), 0x06);
            a8_load(0x80);
            tsb_at(dpa(ACTOR_08), 0x80);
            a8_load(0x08);
            abi_PlayerMoveStartAlso();
            lost = 1;
        }
    }
    if (!lost) {
        if (!ld(V_1428)) {
            op_rep(P_M);
            a16_load(dp16(ACTOR_Y + 1));
            flag_set(P_C, 1);
            cpu.a = op_sbc16(cpu.a, 0x000D, &cpu.p);
            dp16_set(ACTOR_Y + 1, cpu.a);
            op_sep(P_M);
        }
        put(PLAYER_HURT_TIME, 0x7E);
        or_d(ACTOR_08, 0x10);
        wr8(dba(V_141B), 0);
        or_d(ACTOR_09, 0x80);
        call_frame(0xDBBD, abi_PlayerFlagsClear);
        if (!(ld(HURT_FRAMES) & 0x80)) {
            abi_PlayerStand();
            return;
        }
        putd(FRAMES_40, 0x01);
        wr8(dpa(ACTOR_41), 0x01);
        abi_PlayerAirStart();
        return;
    }
    /* the life is lost */
    actor_yield(0x81DC0Bu);
    a8_load(0x2A);
    jsl_frame(0x81DC0Fu);
    abi_SongPlay();
    cpu.s = (uint16_t)(cpu.s + 3);
    put(VIEW_EDGE_RULE, 0x04);
    put(NO_LANDING, 0xFF);
    put(V_1A10, 0x01);
    y8_load(0x02);
    jsl_frame(0x81DC24u);
    abi_ActorMotionPresetY();
    cpu.s = (uint16_t)(cpu.s + 3);
    putd(FRAMES_40, 0xB0);
    do
        ending_frame(0x81DC2Cu);
    while (dec8_at(dpa(FRAMES_40)));
    task_create(0x81DC41u);
    do
        ending_frame(0x81DC4Au);
    while (ld(SCENE_WAIT));
    wr8(dba(V_141B), 0);
    jsl_frame(0x81DC63u);
    abi_Level035CSet();
    cpu.s = (uint16_t)(cpu.s + 3);
    (void)dec8_at(dba(LIVES));
    if (ld(LIVES))
        task_exec(0x81DC6Fu);
    else
        task_exec(0x81DC76u);
}

/*
 * @routine 81:D361 PlayerCrouch
 * WHAT IT DOES: When down is held, Aladdin crouches: he slides to a stop
 * crouched, may throw (move $15, 24 frames), jump, or fall, and stands
 * again when down is let go. Ground that hurts knocks him back.
 * HOW: PAD1_DIRS bit 2 clear: back. Else its return address dropped (PLA
 * PLA): PLAYER_BOX_WANTED 1, PlayerMoveStart 1, ACTOR_09 bit 0 set,
 * ACTOR_ACCEL_X $28, facing from ACTOR_FLIP. Each frame (BRK first):
 * ActorSnapToGround; ground_hurts: PlayerEnter8CLong, on at
 * PlayerKnockback. PlayerFlipByPad; speed 0: ACTOR_ACCEL_X 0; else
 * ActorDecelerateX and PlayerCrouchStep. PlayerAppleThrow's throw: move
 * $15, COP $18, move 1. PlayerCrouchJump, PlayerNoGroundFall. Down still
 * held: again; else facing from ACTOR_FLIP and on at PlayerStand.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from the player's states at their level (81:CD0D, 81:CDCC,
 * 81:D341). Exit: down not held: A low = PAD1_DIRS, Z clear of bit 2.
 * Else never returns.
 */
void abi_PlayerCrouch(void)
{
    uint32_t at = crouch();

    if (at)
        actor_leave(at);
}
