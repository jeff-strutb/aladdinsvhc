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
