/*
 * player_held.c: Aladdin's code while something holds him or throws
 * him: held by an actor (and how he lets go or jumps off), his bounce off
 * an actor he lands on, sprung or flung up, and taken to hang. Each is
 * actor code (it yields by BRK, docs/aladdin/ACTORS.md) whose first piece
 * starts where the actor that set his resume address sends him, and goes
 * on to another of his states by a jump. Written instruction for
 * instruction from the assembly (player_held.asm), every JSR and JSL with
 * its return frame.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "wram_video.h"
#include "wram_player.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorAllocBLong(void);
void abi_ActorFlyAgainst(void);
void abi_ActorMotionPreset(void);
void abi_ActorOutOfGround(void);
void abi_ActorSpawnPlace(void);
void abi_ActorSpeedXClear(void);
void abi_ActorSpeedYClear(void);
void abi_ActorSpeedYSetGravity(void);
void abi_ActorStopX(void);
void abi_ActorStopY(void);
void abi_GroundBehindBit3(void);
void abi_GroundBehindBit4(void);
void abi_PlayerAir(void);
void abi_PlayerAirStep(void);
void abi_PlayerAirTimed(void);
void abi_PlayerAppleThrow(void);
void abi_PlayerBoxUpdate(void);
void abi_PlayerFall(void);
void abi_PlayerFlagsClear(void);
void abi_PlayerFlipByPad(void);
void abi_PlayerGroundYSave(void);
void abi_PlayerHang(void);
void abi_PlayerHover(void);
void abi_PlayerJumpPoseStep(void);
void abi_PlayerLand(void);
void abi_PlayerLedgeGrab(void);
void abi_PlayerMoveStart(void);
void abi_PlayerOutOfWallsHurt(void);
void abi_PlayerPosRestore(void);
void abi_PlayerRunRatesSet(void);
void abi_PlayerRunSpeed(void);
void abi_PlayerSpeedLeast(void);
void abi_PlayerTopSpeedCap(void);
void abi_SoundQueueAdd(void);
void abi_PlayerHeldLetGo(void);
void abi_PlayerHeldJumpOff(void);

/* Pad 1 and the game's actions (system/controls.c) */
#define PAD1_DIRS 0x0380u           /* pad 1's high byte: B Y Select Start Up Down Left Right */
#define PAD_DOWN 0x04
#define JUMP_HELD 0x0383u           /* nonzero while the jump's button is held */
#define HOVER_HELD 0x0386u          /* nonzero while the fourth action's button is held */
#define JUMP_PRESSED 0x0387u        /* nonzero when the jump's button was just pressed */

/* The player's work RAM */
#define TILE_BITS 0x0012u           /* the last probed tile's behaviour (bit 5: it hurts) */
#define HOVER_ON 0x036Eu            /* nonzero: the fourth action makes him hover */
#define V_141B 0x141Bu
#define V_141E 0x141Eu              /* the top speed's high byte (PLAYER_TOP_SPEED + 1) */
#define V_1427 0x1427u              /* nonzero: no landing on actors */
#define V_142E 0x142Eu              /* counted up while the continue screen shows */
#define AIR_THROW 0x144Bu           /* frames left of a throw in the air */
#define PLAYER_LANDED 0x144Eu       /* 1: he has landed on an actor */
#define BOUNCE_FRAMES 0x144Fu       /* frames left of a part of the bounce */
#define BOUNCE_ROW 0x1450u          /* the bounce's row of vertical speeds (0 the lowest) */
#define AIR_FRAMES 0x1451u          /* frames in the air, counted up to $80 */
#define HOLD_LET_GO 0x1458u         /* nonzero: what holds him lets him go */
#define NO_LANDING 0x145Au          /* nonzero: frames with no hold on a ledge; negative: none */
#define COPIER_HELD 0x1461u         /* nonzero: a copier's RAM was found; he stays held */
#define V_1A10 0x1A10u
#define V_1A12 0x1A12u
#define BOUNCE_SUM 0x002Cu          /* the bounce's row plus 4, kept for its times 3 */

/* Record fields of the player's code */
#define FRAMES_40 ACTOR_40          /* frames left of a pose (and the jump's pose step) */
#define FRAMES_47 ACTOR_47          /* frames left of the bounce's first part */

/* Tables in the data bank ($80), by the holder's animation frame (ACTOR_FRAME) */
#define HELD_PRESETS 0xC734u        /* the motion preset he jumps off with */
#define HELD_SPAWN_AT 0xC748u       /* where his throw's apple starts */
#define HELD_BOXES 0xC75Cu          /* his box wanted while held */
#define BOUNCE_ROWS 0xC770u         /* the bounce's row by frames in the air or left */

/* The check a copier's cartridge RAM answers and a retail cartridge does not */
#define COPIER_CHECK 0x701234u
#define COPIER_MARK 0x5A

#define HOLD_KIND 0x12              /* the kind of the actor started beside him while held */
#define STATE_START 0x0C

/* A JSL at `site`: its return address on the stack while the callee runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 81:D59A PlayerHeldLetGo
 * @actor
 * WHAT IT DOES: Aladdin lets go of what held him: he drops from it with
 * his air's settings, the hold's sound and pose, and goes on in the air.
 * HOW: PLAYER_BOX_WANTED 3, ActorSpeedXClear, ActorSpeedYClear; gravity
 * $38; top speed $0200 (16-bit); ACTOR_SPAWN_AT from HELD_SPAWN_AT by
 * ACTOR_FRAME; ActorSpawnPlace with X = the player (16-bit X);
 * PlayerMoveStart $0E; SoundQueueAdd 1; NO_LANDING 4, V_1427 0, FRAMES_40
 * 3; on at PlayerAirTimed.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * a jump from PlayerHeld (81:D76B). Exit: never returns.
 */
void abi_PlayerHeldLetGo(void)
{
    lda8_v(0x03);
    sta8(ea_abs(PLAYER_BOX_WANTED));
    jsl_call(0x81D59Fu, abi_ActorSpeedXClear);
    jsl_call(0x81D5A3u, abi_ActorSpeedYClear);
    lda8_v(0x38);
    sta8(ea_dp(ACTOR_ACCEL_Y));
    op_rep(P_M);
    lda16_v(0x0200);
    sta16(ea_abs(PLAYER_TOP_SPEED));
    op_sep(P_M);
    ldx8(ea_dp(ACTOR_FRAME));
    lda8(ea_absx(HELD_SPAWN_AT));
    sta8(ea_dp(ACTOR_SPAWN_AT));
    op_rep(P_X);
    ldx16_v(PLAYER);
    jsl_call(0x81D5C1u, abi_ActorSpawnPlace);
    op_sep(P_X);
    lda8_v(0x0E);
    call_frame(0xD5CB, abi_PlayerMoveStart);
    lda8_v(0x01);
    jsl_call(0x81D5CEu, abi_SoundQueueAdd);
    lda8_v(0x04);
    sta8(ea_abs(NO_LANDING));
    stz8(ea_abs(V_1427));
    lda8_v(0x03);
    sta8(ea_dp(FRAMES_40));
    abi_PlayerAirTimed();
}

/*
 * @routine 81:D5EC PlayerHeldJumpOff
 * @actor
 * WHAT IT DOES: Aladdin jumps off what held him, at the speed its frame
 * gives, then flies in the air a frame at a time as in PlayerAir until he
 * lands, catches a ledge or hovers.
 * HOW: V_1427 0; ActorMotionPreset with Y = HELD_PRESETS by A (the
 * holder's frame); PlayerGroundYSave; NO_LANDING $10; ACTOR_SPAWN_AT from
 * HELD_SPAWN_AT by ACTOR_FRAME; top speed $0200 (16-bit X); ActorSpawnPlace
 * with X = the player; SoundQueueAdd 1; V_1A08 $0A, PLAYER_BOX_WANTED 3,
 * V_141C up, AIR_FRAMES 0, ACTOR_09 bit 0 cleared, FRAMES_40 6. Each frame
 * (BRK first from the second): V_1428: TILE_BITS 0, on at PlayerLand.
 * AIR_FRAMES up while under $80, 0 with JUMP_PRESSED. AIR_THROW counting
 * down: at 0 FRAMES_40 2 and PlayerJumpPoseStep; else PlayerAppleThrow: a
 * throw starts move $16 and AIR_THROW $16, else PlayerJumpPoseStep.
 * PlayerAirStep, PlayerFlipByPad. HOVER_ON, HOVER_HELD and falling: on at
 * PlayerHover. Gravity $38, or $80 rising with the jump not held and not
 * PLAYER_LANDED; PlayerRunSpeed; NO_LANDING down. Rising: the next frame.
 * Level 5: GroundBehindBit4 at probe $18 sets V_1A10 and V_1A12. Not down
 * on the pad and NO_LANDING 0: GroundBehindBit3 nonzero, on at
 * PlayerLedgeGrab. NO_LANDING down again; ActorOutOfGround nonzero: on at
 * PlayerLand; else the next frame.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; A = the
 * holder's frame (4 to $0C); reached by a jump from PlayerHeld
 * (81:D779). Exit: never returns.
 */
void abi_PlayerHeldJumpOff(void)
{
    stz8(ea_abs(V_1427));
    tax8();
    ldy8(ea_absx(HELD_PRESETS));
    jsl_call(0x81D5F3u, abi_ActorMotionPreset);
    call_frame(0xD5F9, abi_PlayerGroundYSave);
    lda8_v(0x10);
    sta8(ea_abs(NO_LANDING));
    ldx8(ea_dp(ACTOR_FRAME));
    lda8(ea_absx(HELD_SPAWN_AT));
    sta8(ea_dp(ACTOR_SPAWN_AT));
    op_rep(P_X);
    ldx16_v(0x0200);
    stx16(ea_abs(PLAYER_TOP_SPEED));
    ldx16_v(PLAYER);
    jsl_call(0x81D611u, abi_ActorSpawnPlace);
    op_sep(P_X);
    lda8_v(0x01);
    jsl_call(0x81D619u, abi_SoundQueueAdd);
    lda8_v(0x0A);
    sta8(ea_abs(V_1A08));
    lda8_v(0x03);
    sta8(ea_abs(PLAYER_BOX_WANTED));
    MEM8(inc, ea_abs(V_141C));
    stz8(ea_abs(AIR_FRAMES));
    lda8(ea_dp(ACTOR_09));
    and8_v(0xFE);
    sta8(ea_dp(ACTOR_09));
    lda8_v(0x06);
    sta8(ea_dp(FRAMES_40));
    for (int first = 1;; first = 0) {
        if (!first)
            actor_yield(0x81D639u);
        lda8(ea_abs(V_1428));
        if (!IF_Z) {
            stz8(ea_abs(TILE_BITS));
            abi_PlayerLand();
            return;
        }
        lda8(ea_abs(AIR_FRAMES));
        if (!IF_N)
            MEM8(inc, ea_abs(AIR_FRAMES));
        lda8(ea_abs(JUMP_PRESSED));
        if (!IF_Z)
            stz8(ea_abs(AIR_FRAMES));
        lda8(ea_abs(AIR_THROW));
        if (!IF_Z) {
            MEM8(dec, ea_abs(AIR_THROW));
            if (IF_Z) {
                lda8_v(0x02);
                sta8(ea_dp(FRAMES_40));
                call_frame(0xD679, abi_PlayerJumpPoseStep);
            }
        } else {
            call_frame(0xD668, abi_PlayerAppleThrow);
            if (IF_C) {
                lda8_v(0x16);
                call_frame(0xD66F, abi_PlayerMoveStart);
                lda8_v(0x16);
                sta8(ea_abs(AIR_THROW));
            } else {
                call_frame(0xD679, abi_PlayerJumpPoseStep);
            }
        }
        call_frame(0xD67C, abi_PlayerAirStep);
        call_frame(0xD67F, abi_PlayerFlipByPad);
        lda8(ea_abs(HOVER_ON));
        if (!IF_Z) {
            lda8(ea_abs(HOVER_HELD));
            if (!IF_Z) {
                lda8(ea_dp(ACTOR_SPEED_Y + 2));
                if (!IF_N) {
                    abi_PlayerHover();
                    return;
                }
            }
        }
        lda8_v(0x38);
        sta8(ea_dp(ACTOR_ACCEL_Y));
        lda8(ea_abs(JUMP_HELD));
        if (IF_Z) {
            lda8(ea_dp(ACTOR_SPEED_Y + 2));
            if (IF_N) {
                lda8(ea_abs(PLAYER_LANDED));
                if (IF_Z) {
                    lda8_v(0x80);
                    sta8(ea_dp(ACTOR_ACCEL_Y));
                }
            }
        }
        call_frame(0xD6A9, abi_PlayerRunSpeed);
        lda8(ea_abs(NO_LANDING));
        if (!IF_Z)
            MEM8(dec, ea_abs(NO_LANDING));
        lda8(ea_dp(ACTOR_SPEED_Y + 2));
        if (IF_N)
            continue;
        lda8(ea_abs(LEVEL));
        cmp8_v(0x05);
        if (IF_Z) {
            lda8_v(0x18);
            sta8(ea_dp(ACTOR_18));
            jsl_call(0x81D6C1u, abi_GroundBehindBit4);
            if (!IF_Z) {
                lda8_v(0x01);
                sta8(ea_abs(V_1A10));
                sta8(ea_abs(V_1A12));
            }
        }
        lda8(ea_abs(PAD1_DIRS));
        and8_v(PAD_DOWN);
        if (IF_Z) {
            lda8(ea_abs(NO_LANDING));
            if (IF_Z) {
                jsl_call(0x81D6DBu, abi_GroundBehindBit3);
                if (!IF_Z) {
                    abi_PlayerLedgeGrab();
                    return;
                }
            }
        }
        lda8(ea_abs(NO_LANDING));
        if (!IF_Z)
            MEM8(dec, ea_abs(NO_LANDING));
        jsl_call(0x81D6ECu, abi_ActorOutOfGround);
        if (!IF_Z) {
            abi_PlayerLand();
            return;
        }
    }
}

/*
 * @routine 81:D6F8 PlayerHeld
 * @actor
 * WHAT IT DOES: Aladdin held by an actor (it set his code here): his
 * box follows its frame until it lets him go or he lets go (down on the
 * pad) or jumps off (the jump, while its frame is 4 to $0C). On a copier
 * (its cartridge RAM answers the check) he is held for good.
 * HOW: ACTOR_FACING from ACTOR_FLIP; PlayerPosRestore; SoundQueueAdd 6;
 * V_141C, PLAYER_LANDED, AIR_THROW, HOLD_LET_GO and V_141B 0, V_1427 up;
 * PlayerFlagsClear; PlayerMoveStart $10; COPIER_HELD 0; COPIER_MARK
 * stored at COPIER_CHECK and compared: equal, COPIER_HELD up.
 * ActorAllocBLong: a record (N clear, X 16-bit): its state $0C and kind
 * HOLD_KIND, X 8-bit. Each frame (BRK first): PLAYER_BOX_WANTED from
 * HELD_BOXES by ACTOR_FRAME; PlayerBoxUpdate; PlayerOutOfWallsHurt.
 * COPIER_HELD: the next frame. HOLD_LET_GO or down on the pad: on at
 * PlayerHeldLetGo. JUMP_PRESSED with ACTOR_FRAME 4 to $0C: on at
 * PlayerHeldJumpOff with A = the frame. Else the next frame.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * the actor loop's RTI where a holder (kinds $6C, $79, $7D, $87 and the
 * grabber) set his resume address. Exit: never returns.
 */
void abi_PlayerHeld(void)
{
    lda8(ea_dp(ACTOR_FLIP));
    sta8(ea_dp(ACTOR_FACING));
    call_frame(0xD6FE, abi_PlayerPosRestore);
    lda8_v(0x06);
    jsl_call(0x81D701u, abi_SoundQueueAdd);
    stz8(ea_abs(V_141C));
    stz8(ea_abs(PLAYER_LANDED));
    stz8(ea_abs(AIR_THROW));
    stz8(ea_abs(HOLD_LET_GO));
    stz8(ea_abs(V_141B));
    MEM8(inc, ea_abs(V_1427));
    call_frame(0xD719, abi_PlayerFlagsClear);
    lda8_v(0x10);
    call_frame(0xD71E, abi_PlayerMoveStart);
    stz8(ea_abs(COPIER_HELD));
    lda8_v(COPIER_MARK);
    sta8(ea_long(COPIER_CHECK));
    cmp8_v(rd8(ea_long(COPIER_CHECK)));
    if (IF_Z)
        MEM8(inc, ea_abs(COPIER_HELD));
    jsl_call(0x81D731u, abi_ActorAllocBLong);
    if (!IF_N) {
        lda8_v(STATE_START);
        sta8(ea_absx(ACTOR_STATE));
        lda8_v(HOLD_KIND);
        sta8(ea_absx(ACTOR_06));
        op_sep(P_X);
    }
    for (;;) {
        actor_yield(0x81D743u);
        ldx8(ea_dp(ACTOR_FRAME));
        lda8(ea_absx(HELD_BOXES));
        sta8(ea_abs(PLAYER_BOX_WANTED));
        call_frame(0xD74F, abi_PlayerBoxUpdate);
        call_frame(0xD752, abi_PlayerOutOfWallsHurt);
        lda8(ea_abs(COPIER_HELD));
        if (!IF_Z)
            continue;
        lda8(ea_abs(HOLD_LET_GO));
        if (!IF_Z) {
            abi_PlayerHeldLetGo();
            return;
        }
        lda8(ea_abs(PAD1_DIRS));
        and8_v(PAD_DOWN);
        if (!IF_Z) {
            abi_PlayerHeldLetGo();
            return;
        }
        lda8(ea_abs(JUMP_PRESSED));
        if (IF_Z)
            continue;
        lda8(ea_dp(ACTOR_FRAME));
        sec();
        cmp8_v(0x04);
        if (!IF_C)
            continue;
        cmp8_v(0x0D);
        if (IF_C)
            continue;
        abi_PlayerHeldJumpOff();
        return;
    }
}

/* A frame of the bounce's rise (BRK at brk_at): PlayerAirStep,
   PlayerFlipByPad, PlayerRunSpeed (their JSRs at ret, ret + 3, ret + 6). */
static void bounce_frame(uint32_t brk_at, uint16_t ret)
{
    actor_yield(brk_at);
    call_frame(ret, abi_PlayerAirStep);
    call_frame((uint16_t)(ret + 3), abi_PlayerFlipByPad);
    call_frame((uint16_t)(ret + 6), abi_PlayerRunSpeed);
}

/*
 * @routine 81:D77C PlayerBounce
 * @actor
 * WHAT IT DOES: Aladdin lands on an actor's top and bounces off it: a
 * short squat, then up at a speed by how long he fell (with the jump
 * held) or by when the jump is pressed in the squat, through the
 * bounce's two poses, and on in the air.
 * HOW: V_141B 0; PlayerGroundYSave; PLAYER_LANDED 1; V_1427 and AIR_THROW
 * 0; PlayerFlipByPad, PlayerTopSpeedCap, PlayerRunRatesSet; V_1A08 0;
 * PLAYER_BOX_WANTED $10; PlayerMoveStart 9; BOUNCE_FRAMES 9, BOUNCE_ROW
 * 0, player_slowed 0. JUMP_HELD: BOUNCE_ROW from BOUNCE_ROWS by
 * AIR_FRAMES (8 at most), FRAMES_47 $0A frames (BRK each). Else 9 frames
 * (BRK each; JUMP_PRESSED: BOUNCE_ROW from BOUNCE_ROWS by BOUNCE_FRAMES)
 * and PlayerSpeedLeast. Then V_1A08 $0A; ActorSpeedYSetGravity with
 * gravity $38 and Y = (BOUNCE_ROW + 4) * 3 (BOUNCE_SUM kept); BOUNCE_ROW
 * 0: top speed $0270 (byte by byte). PlayerMoveStart $0A and 7 frames,
 * PlayerMoveStart $0B and 7 frames, each frame (BRK first)
 * PlayerAirStep, PlayerFlipByPad, PlayerRunSpeed. player_slowed 0,
 * FRAMES_40 and ACTOR_41 1, PLAYER_BOX_WANTED 3; on at PlayerAir.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * the actor loop's RTI where PlayerLandOnActor and other actors set his
 * resume address. Exit: never returns.
 */
void abi_PlayerBounce(void)
{
    stz8(ea_abs(V_141B));
    call_frame(0xD781, abi_PlayerGroundYSave);
    lda8_v(0x01);
    sta8(ea_abs(PLAYER_LANDED));
    stz8(ea_abs(V_1427));
    stz8(ea_abs(AIR_THROW));
    call_frame(0xD78F, abi_PlayerFlipByPad);
    call_frame(0xD792, abi_PlayerTopSpeedCap);
    call_frame(0xD795, abi_PlayerRunRatesSet);
    stz8(ea_abs(V_1A08));
    lda8_v(0x10);
    sta8(ea_abs(PLAYER_BOX_WANTED));
    lda8_v(0x09);
    call_frame(0xD7A2, abi_PlayerMoveStart);
    lda8_v(0x09);
    sta8(ea_abs(BOUNCE_FRAMES));
    stz8(ea_abs(BOUNCE_ROW));
    stz8(ea_abs(PLAYER_SLOWED));
    lda8(ea_abs(JUMP_HELD));
    if (!IF_Z) {
        lda8(ea_abs(AIR_FRAMES));
        tax8();
        sec();
        sbc8_v(0x08);
        if (IF_C)
            ldx8_v(0x08);
        lda8(ea_absx(BOUNCE_ROWS));
        sta8(ea_abs(BOUNCE_ROW));
        lda8_v(0x0A);
        sta8(ea_dp(FRAMES_47));
        do {
            actor_yield(0x81D7C8u);
            MEM8(dec, ea_dp(FRAMES_47));
        } while (!IF_Z);
    } else {
        do {
            actor_yield(0x81D7D0u);
            lda8(ea_abs(JUMP_PRESSED));
            if (!IF_Z) {
                ldx8(ea_abs(BOUNCE_FRAMES));
                lda8(ea_absx(BOUNCE_ROWS));
                sta8(ea_abs(BOUNCE_ROW));
            }
            MEM8(dec, ea_abs(BOUNCE_FRAMES));
        } while (!IF_Z);
        call_frame(0xD7E7, abi_PlayerSpeedLeast);
    }
    lda8_v(0x0A);
    sta8(ea_abs(V_1A08));
    lda8(ea_abs(BOUNCE_ROW));
    clc();
    adc8_v(0x04);
    sta8(ea_abs(BOUNCE_SUM));
    ACC8(asl);
    adc8_v(rd8(ea_abs(BOUNCE_SUM)));
    tay8();
    lda8_v(0x38);
    jsl_call(0x81D7FDu, abi_ActorSpeedYSetGravity);
    lda8(ea_abs(BOUNCE_ROW));
    if (IF_Z) {
        lda8_v(0x70);
        sta8(ea_abs(PLAYER_TOP_SPEED));
        lda8_v(0x02);
        sta8(ea_abs(V_141E));
    }
    lda8_v(0x0A);
    call_frame(0xD814, abi_PlayerMoveStart);
    lda8_v(0x07);
    sta8(ea_abs(BOUNCE_FRAMES));
    do {
        bounce_frame(0x81D81Au, 0xD81E);
        MEM8(dec, ea_abs(BOUNCE_FRAMES));
    } while (!IF_Z);
    lda8_v(0x0B);
    call_frame(0xD82E, abi_PlayerMoveStart);
    lda8_v(0x07);
    sta8(ea_abs(BOUNCE_FRAMES));
    do {
        bounce_frame(0x81D834u, 0xD838);
        MEM8(dec, ea_abs(BOUNCE_FRAMES));
    } while (!IF_Z);
    stz8(ea_abs(PLAYER_SLOWED));
    lda8_v(0x01);
    sta8(ea_dp(FRAMES_40));
    sta8(ea_dp(ACTOR_41));
    lda8_v(0x03);
    sta8(ea_abs(PLAYER_BOX_WANTED));
    abi_PlayerAir();
}

/* The air's frame end of PlayerSprung and PlayerFlung: HOVER_ON,
   HOVER_HELD and falling: PlayerHover (1 returned); else gravity $38, or
   $80 rising with the jump not held and not PLAYER_LANDED, and
   PlayerRunSpeed (its JSR's return address ret). */
static int air_gravity(uint16_t ret)
{
    lda8(ea_abs(HOVER_ON));
    if (!IF_Z) {
        lda8(ea_abs(HOVER_HELD));
        if (!IF_Z) {
            lda8(ea_dp(ACTOR_SPEED_Y + 2));
            if (!IF_N) {
                abi_PlayerHover();
                return 1;
            }
        }
    }
    lda8_v(0x38);
    sta8(ea_dp(ACTOR_ACCEL_Y));
    lda8(ea_abs(JUMP_HELD));
    if (IF_Z) {
        lda8(ea_dp(ACTOR_SPEED_Y + 2));
        if (IF_N) {
            lda8(ea_abs(PLAYER_LANDED));
            if (IF_Z) {
                lda8_v(0x80);
                sta8(ea_dp(ACTOR_ACCEL_Y));
            }
        }
    }
    call_frame(ret, abi_PlayerRunSpeed);
    return 0;
}

/*
 * @routine 81:D8FD PlayerSprung
 * @actor
 * WHAT IT DOES: Aladdin sprung up by a spring (kind $40): he flies up
 * against the ceiling's pull until he slows to falling, then falls with
 * the air's settings and pose until he lands (or catches a ledge or
 * hovers).
 * HOW: V_1A08 0; gravity $12; vertical speed $00 $01 $00 (byte by byte,
 * its high byte last); player_ground_y 0 (byte by byte); ActorStopX;
 * PlayerMoveStart $0F. Each frame (BRK first) ActorFlyAgainst until the
 * vertical speed's high byte is negative. Then V_141C 1, PLAYER_LANDED 0;
 * ActorSpeedYSetGravity with gravity $38 and Y = $63; PLAYER_BOX_WANTED 3;
 * PlayerMoveStart 4; FRAMES_40 7, ACTOR_41 $0E. Each frame (BRK first):
 * PlayerAirStep, PlayerFlipByPad, PlayerJumpPoseStep; HOVER_ON,
 * HOVER_HELD and falling: on at PlayerHover. Gravity $38, or $80 rising
 * with the jump not held and not PLAYER_LANDED; PlayerRunSpeed. Rising:
 * the next frame. GroundBehindBit3 nonzero: on at PlayerLedgeGrab;
 * ActorOutOfGround nonzero: on at PlayerLand; else the next frame.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * the actor loop's RTI where kind $40 set his resume address. Exit: never
 * returns.
 */
void abi_PlayerSprung(void)
{
    stz8(ea_abs(V_1A08));
    lda8_v(0x12);
    sta8(ea_dp(ACTOR_ACCEL_Y));
    lda8_v(0x01);
    sta8(ea_dp(ACTOR_SPEED_Y + 1));
    stz8(ea_dp(ACTOR_SPEED_Y));
    stz8(ea_dp(ACTOR_SPEED_Y + 2));
    stz8(ea_abs(PLAYER_GROUND_Y));
    stz8(ea_abs(PLAYER_GROUND_Y + 1));
    jsl_call(0x81D912u, abi_ActorStopX);
    lda8_v(0x0F);
    call_frame(0xD91A, abi_PlayerMoveStart);
    do {
        actor_yield(0x81D91Bu);
        jsl_call(0x81D91Du, abi_ActorFlyAgainst);
        lda8(ea_dp(ACTOR_SPEED_Y + 2));
    } while (!IF_N);
    lda8_v(0x01);
    sta8(ea_abs(V_141C));
    stz8(ea_abs(PLAYER_LANDED));
    lda8_v(0x38);
    ldy8_v(0x63);
    jsl_call(0x81D931u, abi_ActorSpeedYSetGravity);
    lda8_v(0x03);
    sta8(ea_abs(PLAYER_BOX_WANTED));
    lda8_v(0x04);
    call_frame(0xD93E, abi_PlayerMoveStart);
    lda8_v(0x07);
    sta8(ea_dp(FRAMES_40));
    lda8_v(0x0E);
    sta8(ea_dp(ACTOR_41));
    for (;;) {
        actor_yield(0x81D947u);
        call_frame(0xD94B, abi_PlayerAirStep);
        call_frame(0xD94E, abi_PlayerFlipByPad);
        call_frame(0xD951, abi_PlayerJumpPoseStep);
        if (air_gravity(0xD97B))
            return;
        lda8(ea_dp(ACTOR_SPEED_Y + 2));
        if (IF_N)
            continue;
        jsl_call(0x81D980u, abi_GroundBehindBit3);
        if (!IF_Z) {
            abi_PlayerLedgeGrab();
            return;
        }
        jsl_call(0x81D989u, abi_ActorOutOfGround);
        if (!IF_Z) {
            abi_PlayerLand();
            return;
        }
    }
}

/*
 * @routine 81:D99B PlayerGrabbedHang
 * @actor
 * WHAT IT DOES: Aladdin taken hold of by an actor he hangs from (the
 * grabbable ones): stopped where he was kept, the hang's sound and pose,
 * and he hangs from it.
 * HOW: PLAYER_LANDED and AIR_THROW 0, V_1427 up; PlayerFlagsClear;
 * ActorStopX, ActorStopY; PlayerPosRestore; SoundQueueAdd 6;
 * PlayerMoveStart 6; PLAYER_BOX_WANTED 4; on at PlayerHang.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * the actor loop's RTI where ActorHoldPlayer set his resume address.
 * Exit: never returns.
 */
void abi_PlayerGrabbedHang(void)
{
    stz8(ea_abs(PLAYER_LANDED));
    stz8(ea_abs(AIR_THROW));
    MEM8(inc, ea_abs(V_1427));
    call_frame(0xD9A6, abi_PlayerFlagsClear);
    jsl_call(0x81D9A7u, abi_ActorStopX);
    jsl_call(0x81D9ABu, abi_ActorStopY);
    call_frame(0xD9B1, abi_PlayerPosRestore);
    lda8_v(0x06);
    jsl_call(0x81D9B4u, abi_SoundQueueAdd);
    lda8_v(0x06);
    call_frame(0xD9BC, abi_PlayerMoveStart);
    lda8_v(0x04);
    sta8(ea_abs(PLAYER_BOX_WANTED));
    abi_PlayerHang();
}

/*
 * @routine 81:D9C5 PlayerSwitchHang
 * @actor
 * WHAT IT DOES: Aladdin hangs from a switch's handle (kind $3B) or is
 * held by the continue screen's choice: he hangs where he was kept until
 * V_142E is 0 and down is on the pad, then drops 18 pixels and falls.
 * HOW: PlayerPosRestore; PLAYER_LANDED and AIR_THROW 0, V_1427 up;
 * PlayerFlagsClear; ActorStopX, ActorStopY; SoundQueueAdd 6;
 * PlayerMoveStart 6; PLAYER_BOX_WANTED 4. Each frame (BRK first): V_142E
 * nonzero or down not on the pad: the next frame. Then (no script runs
 * it) PlayerGroundYSave; NO_LANDING $10; his y pixels $12 lower (16-bit);
 * V_1427 0; on at PlayerFall.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * the actor loop's RTI where kind $3B or ContinueChoice set his resume
 * address. Exit: never returns.
 */
void abi_PlayerSwitchHang(void)
{
    call_frame(0xD9C7, abi_PlayerPosRestore);
    stz8(ea_abs(PLAYER_LANDED));
    stz8(ea_abs(AIR_THROW));
    MEM8(inc, ea_abs(V_1427));
    call_frame(0xD9D3, abi_PlayerFlagsClear);
    jsl_call(0x81D9D4u, abi_ActorStopX);
    jsl_call(0x81D9D8u, abi_ActorStopY);
    lda8_v(0x06);
    jsl_call(0x81D9DEu, abi_SoundQueueAdd);
    lda8_v(0x06);
    call_frame(0xD9E6, abi_PlayerMoveStart);
    lda8_v(0x04);
    sta8(ea_abs(PLAYER_BOX_WANTED));
    for (;;) {
        actor_yield(0x81D9ECu);
        lda8(ea_abs(V_142E));
        if (!IF_Z)
            continue;
        lda8(ea_abs(PAD1_DIRS));
        and8_v(PAD_DOWN);
        if (!IF_Z)
            break;
    }
    call_frame(0xD9FC, abi_PlayerGroundYSave);
    lda8_v(0x10);
    sta8(ea_abs(NO_LANDING));
    op_rep(P_M);
    lda16(ea_dp(ACTOR_Y + 1));
    clc();
    adc16_v(0x0012);
    sta16(ea_dp(ACTOR_Y + 1));
    op_sep(P_M);
    stz8(ea_abs(V_1427));
    abi_PlayerFall();
}

/*
 * @routine 81:DA14 PlayerFlung
 * @actor
 * WHAT IT DOES: Aladdin flung up by an actor he landed on (kind $7B): he
 * flies up against the ceiling's pull until he slows to falling, rests 8
 * frames, then is in the air as in PlayerAir until he lands, catches a
 * ledge or hovers.
 * HOW: V_1A08 0; gravity $10; vertical speed $00 $01 $00 (byte by byte);
 * ActorStopX; PlayerMoveStart $18. Each frame (BRK first) ActorFlyAgainst
 * until the vertical speed's high byte is negative. Then V_141C 1,
 * PLAYER_LANDED 0; ActorSpeedYSetGravity with gravity $38 and Y = $66;
 * COP 8; FRAMES_40 0; PlayerMoveStart 4; V_1A08 $0A; PlayerGroundYSave.
 * Each frame (BRK first): AIR_THROW counting down: at 0 FRAMES_40 8 and
 * ACTOR_41 1, PlayerJumpPoseStep; else PlayerAppleThrow: a throw starts
 * move $16 and AIR_THROW $14, else PlayerJumpPoseStep. PlayerAirStep,
 * PlayerFlipByPad; HOVER_ON, HOVER_HELD and falling: on at PlayerHover.
 * Gravity $38, or $80 rising with the jump not held and not
 * PLAYER_LANDED; PlayerRunSpeed. Rising: the next frame. GroundBehindBit3
 * nonzero: on at PlayerLedgeGrab. V_1428, or ActorOutOfGround nonzero: on
 * at PlayerLand; else the next frame.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * the actor loop's RTI where kind $7B set his resume address. Exit: never
 * returns.
 */
void abi_PlayerFlung(void)
{
    stz8(ea_abs(V_1A08));
    lda8_v(0x10);
    sta8(ea_dp(ACTOR_ACCEL_Y));
    lda8_v(0x01);
    sta8(ea_dp(ACTOR_SPEED_Y + 1));
    stz8(ea_dp(ACTOR_SPEED_Y));
    stz8(ea_dp(ACTOR_SPEED_Y + 2));
    jsl_call(0x81DA23u, abi_ActorStopX);
    lda8_v(0x18);
    call_frame(0xDA2B, abi_PlayerMoveStart);
    do {
        actor_yield(0x81DA2Cu);
        jsl_call(0x81DA2Eu, abi_ActorFlyAgainst);
        lda8(ea_dp(ACTOR_SPEED_Y + 2));
    } while (!IF_N);
    lda8_v(0x01);
    sta8(ea_abs(V_141C));
    stz8(ea_abs(PLAYER_LANDED));
    lda8_v(0x38);
    ldy8_v(0x66);
    jsl_call(0x81DA42u, abi_ActorSpeedYSetGravity);
    lda8_v(0x08);
    actor_wait(0x81DA48u);
    stz8(ea_dp(FRAMES_40));
    lda8_v(0x04);
    call_frame(0xDA50, abi_PlayerMoveStart);
    lda8_v(0x0A);
    sta8(ea_abs(V_1A08));
    call_frame(0xDA58, abi_PlayerGroundYSave);
    for (;;) {
        actor_yield(0x81DA59u);
        lda8(ea_abs(AIR_THROW));
        if (!IF_Z) {
            MEM8(dec, ea_abs(AIR_THROW));
            if (IF_Z) {
                lda8_v(0x08);
                sta8(ea_dp(FRAMES_40));
                lda8_v(0x01);
                sta8(ea_dp(ACTOR_41));
                call_frame(0xDA82, abi_PlayerJumpPoseStep);
            }
        } else {
            call_frame(0xDA71, abi_PlayerAppleThrow);
            if (IF_C) {
                lda8_v(0x16);
                call_frame(0xDA78, abi_PlayerMoveStart);
                lda8_v(0x14);
                sta8(ea_abs(AIR_THROW));
            } else {
                call_frame(0xDA82, abi_PlayerJumpPoseStep);
            }
        }
        call_frame(0xDA85, abi_PlayerAirStep);
        call_frame(0xDA88, abi_PlayerFlipByPad);
        if (air_gravity(0xDAB2))
            return;
        lda8(ea_dp(ACTOR_SPEED_Y + 2));
        if (IF_N)
            continue;
        jsl_call(0x81DAB7u, abi_GroundBehindBit3);
        if (!IF_Z) {
            abi_PlayerLedgeGrab();
            return;
        }
        lda8(ea_abs(V_1428));
        if (!IF_Z)
            break;
        jsl_call(0x81DAC5u, abi_ActorOutOfGround);
        if (!IF_Z)
            break;
    }
    abi_PlayerLand();
}
