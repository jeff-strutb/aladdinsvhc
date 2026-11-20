/*
 * player_own.h: what player/'s files share and the rest of the game does
 * not: his view of his record's own bytes, the resume points of his code
 * past the entries player.h names, and the routines of one file another
 * calls.
 *
 * How his code goes. Each state is the code from one of the record's
 * entries; it does its frame's work and returns the frames to wait, after
 * setting a->resume to where it goes on. A state that goes on at another
 * state at once (the original's jump) returns what that one returns:
 * `return PlayerGoTo(a, PLAYER_AT_...)`. A test the original calls by JSR
 * that may drop its return and jump to another state returns 0 when it
 * returned, else what the state it went to returned; its caller then
 * returns that at once (`if ((r = test(a))) return r;`).
 */
#ifndef ALADDIN_PLAYER_OWN_H
#define ALADDIN_PLAYER_OWN_H

#include <stdbool.h>
#include <stdint.h>

#include "player/player.h"
#include "actor/actor.h"
#include "actor/actor_own.h"
#include "audio/sound.h"
#include "data/player_tables.h"
#include "map/map.h"

#define PLAYER_RETURNED 0       /* a test returned to its caller: his code goes on there */

/* ---- his record's own bytes (actor_own.h's OwnPlace) ---- */

#define POSE_FRAMES(a) ((a)->own.bytes[OWN_40])     /* +$40: frames left of a pose; the jump's pose step */
#define POSE_TIMER(a) ((a)->own.bytes[OWN_41])      /* +$41: frames left of a timed pose step */
#define TOP_Y_LOW(a) ((a)->own.bytes[OWN_42])       /* +$42-$43: his y pixels as the jump's rise ended */
#define TOP_Y_HIGH(a) ((a)->own.bytes[OWN_43])
#define LAND_FRAMES(a) ((a)->own.bytes[OWN_47])     /* +$47: frames left of a landing's pose */
#define WAIT_FRAMES(a) ((a)->own.bytes[OWN_4A])     /* +$4A: frames left of the walk in or the standing throw */

/* $1427, a byte: video/ keeps it as the low byte of a word (g.player_no_actor_landing) */
static inline uint8_t no_actor_landing(void) { return (uint8_t)g.player_no_actor_landing; }
static inline void no_actor_landing_set(uint8_t v) { g.player_no_actor_landing = (uint16_t)((g.player_no_actor_landing & 0xFF00) | v); }

static inline uint16_t top_y(Actor *a) { return (uint16_t)(TOP_Y_LOW(a) | TOP_Y_HIGH(a) << 8); }

/* ---- 24-bit motion ---- */

/* a speed's low word (its fraction and its pixels' low byte), as the
   original's 16-bit reads and writes of it, its top byte kept */
static inline uint16_t speed_word(int32_t s) { return (uint16_t)s; }
static inline int32_t with_speed_word(int32_t s, uint16_t w) { return signed24((((uint32_t)s >> 16 & 0xFFu) << 16) | w); }
static inline uint16_t x_pixels(const Actor *a) { return pixels(a->x); }
static inline uint16_t y_pixels(const Actor *a) { return pixels(a->y); }
static inline void set_x_pixels(Actor *a, uint16_t px) { a->x = with_pixels(a->x, px); }
static inline void set_y_pixels(Actor *a, uint16_t px) { a->y = with_pixels(a->y, px); }

/* ---- the pad (system/'s g.pad1_held, g.action_*) ---- */

#define PAD_UP 0x08             /* in pad 1's high byte */
#define PAD_DOWN 0x04
static inline uint8_t pad_dirs(void) { return (uint8_t)(g.pad1_held >> 8); }
static inline bool jump_held(void) { return g.action_held[0] != 0; }
static inline bool dash_held(void) { return g.action_held[1] != 0; }
static inline bool hover_held(void) { return g.action_held[3] != 0; }
static inline bool jump_pressed(void) { return g.action_pressed[0] != 0; }
static inline bool throw_pressed(void) { return g.action_pressed[2] != 0; }

/* ---- what the map's tests he calls last found: the original reads it in
   its scratch ($0C-$1E) after them, sometimes in a later state of the same
   frame (native: player/'s own copy) ---- */
extern MapPoint player_point;

#define TILE_HURTS 0x20         /* behaviour bit 5 */

/* the map's moves whose findings his code reads after them */
static inline int snap_to_ground(Actor *a) { return ActorSnapToGround(a, &player_point); }
static inline int out_of_walls(Actor *a) { return ActorOutOfWalls(a, &player_point); }
static inline int settle_on_floor(Actor *a) { return ActorSettleOnFloor(a, &player_point); }

/* ---- resume points past player.h's ---- */
enum {
    /* player_starts.c */
    P_WALK_IN_FRAME = PLAYER_BLOCK(1),
    P_START_TASKS,
    P_START_UNCONVERTED,

    /* held.c (its entries in player.h from PLAYER_BLOCK(3)) */
    P_HELD_FRAME = PLAYER_BLOCK(3) + 0x10,
    P_HELD_LET_GO,
    P_HELD_JUMP_OFF,
    P_JUMP_OFF_FRAME,
    P_BOUNCE_SQUAT_HELD,
    P_BOUNCE_SQUAT,
    P_BOUNCE_RISE_1,
    P_BOUNCE_RISE_2,
    P_SPRUNG_UP,
    P_SPRUNG_AIR,
    P_CARRIED_FRAME,
    P_SWITCH_HANG_FRAME,
    P_FLUNG_UP,
    P_FLUNG_REST,
    P_FLUNG_AIR,

    /* carpet.c */
    P_CARPET_START = PLAYER_BLOCK(7),
    P_CARPET_FLY_IN,
    P_CARPET_DRIFT,
    P_CARPET,
    P_CARPET_CRASH,
    P_CARPET_CRASH_FALL,
    P_CARPET_CRASH_WAIT,

    /* player_states.c (its entries in player.h from PLAYER_BLOCK(2)) */
    P_STAND_FRAME = PLAYER_BLOCK(2) + 0x10,
    P_STAND_TURN,
    P_THROW_STANDING,
    P_THROW_STANDING_DONE,
    P_WALK,
    P_WALK_FRAME,
    P_AIR_START,
    P_AIR_TIMED,
    P_AIR,
    P_AIR_LAND,
    P_LAND,
    P_LAND_HARD_DONE,
    P_SKID,
    P_SKID_FRAME,
    P_SKID_THROW_DONE,
    P_CROUCH_FRAME,
    P_CROUCH_THROW_DONE,
    P_LEDGE_GRAB,
    P_HANG,
    P_HANG_FRAME,
    P_HANG_CLIMBED,
    P_HOVER,
    P_HOVER_FRAME,
    P_KNOCKBACK_FROM_LEDGE,
    P_KNOCKBACK_NO_HEALTH,
    P_KNOCKBACK_FRAME,
    P_LOSS_FIRST,
    P_LOSS_FALL,
    P_LOSS_WAIT,
};

/* ---- the files' code by block (player_code.c dispatches to them) ---- */

int player_starts_code(Actor *a);       /* player_starts.c */
int player_states_code(Actor *a);       /* player_states.c */
int player_held_code(Actor *a);         /* player_held.c */
int player_exit_code(Actor *a);         /* player_exit.c */
int player_scene_code(Actor *a);        /* scene_starts.c */
int player_carpet_code(Actor *a);       /* carpet_ride.c */
int player_unconverted(Actor *a, const char *name);    /* a state not converted yet: its name once, a frame's wait */

/* ---- one file's routines another calls ---- */

/* moves.c */
uint8_t PadFacing(void);                /* 81:CE7C: 0 right, 1 left, $FF neither or both */
bool PlayerFlipByPad(Actor *a);         /* 81:CE70: his flip from the pad; false when it points neither way */
uint8_t AnimOfMove(const Actor *a, uint8_t move);   /* 81:DCB1 */
void PlayerMoveKeepFrame(Actor *a, uint8_t move);   /* 81:DC7D */
void PlayerRunMove(Actor *a);           /* 81:CF1C */
void PlayerRunSpeed(Actor *a);          /* 81:D1A0 */
void PlayerAirSteer(Actor *a);          /* 81:CF2E */
void PlayerSpeedLeast(Actor *a);        /* 81:D855 */
void PlayerPosRestore(Actor *a);        /* 81:DACE */
void PlayerFlagsClear(void);            /* 81:DDB4 */
void ActorStepX(Actor *a);              /* 81:D41C */
void ActorFallCap(Actor *a);            /* 81:D448 */
void ActorGravity(Actor *a);            /* 81:D44D */
void PlayerGroundYSave(Actor *a);       /* 81:D164 */
void PlayerTopSpeedHold(Actor *a);      /* 81:D16E */
void PlayerTopSpeedCap(void);           /* 81:D179 */
void PlayerRunRatesSet(void);           /* 81:D189 */
void PlayerGroundPush(Actor *a);        /* 81:CE86 */
void PlayerGroundSlide(Actor *a);       /* 81:CEF4 */
void PlayerSpawnEvery8(Actor *a);       /* 81:DD90 */
bool PlayerAppleThrow(Actor *a);        /* 81:CDE9: true when he threw */
void PlayerJumpPoseStep(Actor *a);      /* 81:D1F5 */

/* player_box.c, player_bounds.c */
void PlayerBoxUpdate(Actor *a);         /* 81:E909 */
void PlayerBoxSet(Actor *a, uint8_t box);   /* 81:E911 */
void TestBoxPlace(const Actor *a);      /* 81:E8D5 */
bool PlayerFallenTest(void);            /* 81:E8EC: true while he is above the level's bottom (the original's C set) */
void PlayerHurtTimeOut(Actor *a);       /* 81:E966 */
void PlayerHurtCountdown(Actor *a);     /* 81:E95C */

/* hurt_ground.c: each returns PLAYER_RETURNED, or what PlayerKnockback
   (where a hurting tile sends him) returned */
int PlayerGroundSnapHurt(Actor *a);     /* 81:DCBC */
int PlayerOutOfWallsHurt(Actor *a);     /* 81:DD01 */
int PlayerGroundHurtTest(Actor *a);     /* 81:DD1E */
int PlayerGroundHurtJump(Actor *a);     /* 81:DD38 */
int PlayerLandOrHurt(Actor *a);         /* 81:DD50 */

/* player_states.c's tests */
int PlayerCrouch(Actor *a);             /* 81:D361: PLAYER_RETURNED when down is not held */
int PlayerNoGroundFall(Actor *a);       /* 81:D347 */
int PlayerAirStep(Actor *a);            /* 81:D407 */

#endif
