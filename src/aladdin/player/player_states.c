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

/* Standing, from its top (`throwing` 0) or from the throw (1). */
static void standing(int throwing)
{
    uint8_t f;

    for (;;) {
        if (!throwing) {
            actor_yield(0x81CCCEu);
            abi_ActorDecelerateX();
            call_frame(0xCCD6, abi_PlayerGroundSnapHurt);
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
        call_frame(0xCD0F, abi_PlayerCrouch);
        call_frame(0xCD12, abi_PlayerNoGroundFall);
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







/* Where a frame in the air goes on. */
enum air_next { AIR_NEXT_FRAME, AIR_GROUND_TEST, AIR_LAND, AIR_LEDGE, AIR_HOVER };

/* One frame in the air, from after its BRK. */
static enum air_next air_frame(void)
{
    uint8_t v;

    v = ldd(ACTOR_SPEED_Y + 2);
    if (!(v & 0x80) && ld(V_1428)) {
        wr8(dba(TILE_BITS), 0);
        return AIR_LAND;
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
                return AIR_HOVER;
        }
    }
    call_frame(0xD084, abi_PlayerAirStep);
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
        return AIR_LAND;
    v = (uint8_t)(ld(PAD1_DIRS) & PAD_DOWN);
    a8_load(v);
    if (!v && !ld(NO_LANDING)) {
        abi_GroundBehindBit3();
        return (cpu.p & P_Z) ? AIR_GROUND_TEST : AIR_LEDGE;
    }
    if (ld(NO_LANDING))
        (void)dec8_at(dba(NO_LANDING));
    return AIR_GROUND_TEST;
}

/* The air's frames: the BRK first (from_brk) or the frame's test first. */
static void in_air(int from_brk)
{
    for (;;) {
        if (from_brk)
            actor_yield(0x81D02Cu);
        from_brk = 1;
        switch (air_frame()) {
        case AIR_NEXT_FRAME:
            continue;
        case AIR_GROUND_TEST:
            /* 81:D0E0: in the ground he lands, else the next frame */
            abi_ActorOutOfGround();
            if (cpu.p & P_Z)
                continue;
            abi_PlayerLand();
            return;
        case AIR_LAND:
            abi_PlayerLand();
            return;
        case AIR_LEDGE:
            abi_PlayerLedgeGrab();
            return;
        case AIR_HOVER:
            abi_PlayerHover();
            return;
        }
    }
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
    if (ld(V_1428))
        return;
    wr8(dpa(ACTOR_18), 0);
    abi_GroundLeftLong();
    if (cpu.p & P_C)
        return;
    abi_GroundRightLong();
    if (cpu.p & P_C)
        return;
    pla8();                               /* its own return dropped */
    pla8();
    actor_leave(0x81CDD2u);               /* PlayerFall */
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
    uint8_t f;

    ld(PAD1_DIRS);
    if (!bit_a(PAD_DOWN))
        return;
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
            actor_leave(0x81DADDu);       /* PlayerKnockback */
            return;
        }
        abi_PlayerFlipByPad();
        f = (uint8_t)(rd8(dpa(ACTOR_SPEED_X)) | rd8(dpa(ACTOR_SPEED_X + 1)) | rd8(dpa(ACTOR_SPEED_X + 2)));
        a8_load(f);
        if (!f) {
            wr8(dpa(ACTOR_ACCEL_X), 0);
        } else {
            abi_ActorDecelerateX();
            call_frame(0xD3B9, abi_PlayerCrouchStep);
        }
        abi_PlayerAppleThrow();
        if (cpu.p & P_C) {
            move_start(0x15);
            a8_load(0x18);
            actor_wait(0x81D3C6u);
            move_start(0x01);
        }
        call_frame(0xD3CF, abi_PlayerCrouchJump);
        call_frame(0xD3D2, abi_PlayerNoGroundFall);
        ld(PAD1_DIRS);
        if (!bit_a(PAD_DOWN))
            break;
    }
    f = ldd(ACTOR_FLIP);
    wr8(dpa(ACTOR_FACING), f);
    actor_leave(0x81CCB6u);               /* PlayerStand */
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
    abi_ActorStepX();
    call_frame(0xD3E6, abi_PlayerOutOfWallsHurt);
    if (ld(V_1428))
        return;
    abi_ActorSnapToGround();
    ld(GROUND_UNDER);
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
    if (!ld(JUMP_PRESSED))
        return;
    if (ld(V_1446))
        return;
    pla8();                               /* its own return dropped */
    pla8();
    x8_load(0x80);
    a8_load(0x01);
    actor_leave(0x81CFE0u);               /* PlayerJump */
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
    abi_ActorGravity();
    abi_ActorStepX();
    abi_ActorMoveY();
    call_frame(0xD413, abi_PlayerLandOrHurt);
    abi_ActorOutOfWalls();
    call_frame(0xD41A, abi_PlayerGroundHurtTest);
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

