/*
 * actor.h: the actors (docs/aladdin/ACTORS.md; plan section 3, "Actors'
 * yields"): the loop, the kinds' code, the pool, spawning, placement and
 * animation, as the rest of the game calls them.
 *
 * The records are g.actors (system/state.h): actors[0] the player's, then
 * 35 more; actors[1..7] run even while g.pause_block holds the pool's 28
 * (actors[8..35]) still.
 *
 * A kind's code is one function of its record. The loop calls it when the
 * actor starts or when its wait runs out; it switches on a->resume, does
 * one frame's work and returns:
 *   - a wait in frames, 1 to 256 (the original's BRK is 1, its COP A is A,
 *     A = 0 being 256), after setting a->resume to where it goes on; or
 *   - ACTOR_ENDED when it ended the actor (its record already cleared and
 *     freed by the ending routine it called).
 * The loop starts an actor by setting a->resume to ACTOR_START,
 * ACTOR_START_SECOND or ACTOR_START_THIRD by its state ($0C-$7F, $80-$BF,
 * $C0 up) before the first call; a kind's own resume points are numbered
 * from ACTOR_RESUME_OWN. Kind $08's rewrite of its resume address to
 * another entry is an assignment of a->resume.
 *
 * A kind's wait routine (a->flags bit 7: run each frame it waits) is a
 * second function, returning ACTOR_GOES_ON, or ACTOR_ENDED when it ended
 * the actor.
 */
#ifndef ALADDIN_ACTOR_H
#define ALADDIN_ACTOR_H

#include <stdbool.h>
#include <stdint.h>

#include "system/state.h"

/* ---- the kinds' code ---- */

#define ACTOR_ENDED 0           /* a kind's code or wait routine ended its actor */
#define ACTOR_GOES_ON 1         /* a wait routine's actor waits on */

enum {
    ACTOR_START = 0,            /* state $0C-$7F: the kind's start entry */
    ACTOR_START_SECOND = 1,     /* state $80-$BF: its second entry */
    ACTOR_START_THIRD = 2,      /* state $C0 up: its third */
    ACTOR_RESUME_OWN = 3        /* a kind's own resume points from here */
};

typedef int (*ActorCode)(Actor *a);     /* frames to wait (1-256), or ACTOR_ENDED */
typedef int (*ActorWaitCode)(Actor *a); /* ACTOR_GOES_ON or ACTOR_ENDED */

/* record states (a->state) */
#define ACTOR_STATE_FREE 0x00
#define ACTOR_STATE_WAITING 0x01
#define ACTOR_STATE_RUNNING 0x08
#define ACTOR_STATE_NEW 0x0C    /* starts at its next turn, at ACTOR_START */

/* ---- the loop (called from FrameStep, 81:8311) ---- */

void PlayerLoop(void);          /* 83:9475: actors[0]'s turn */
void ActorLoop(void);           /* 83:94A4: actors[1..35]'s turns, in order */

/* hands the record to `code` at `resume` (it owns the record's resume
   point from now on) and runs it; returns its wait or ACTOR_ENDED */
int ActorRunAs(Actor *a, ActorCode code, uint16_t resume);

/* ---- the pool ---- */

void ActorRecordsInit(void);    /* 81:AADD: every record empty, the free lists full (a level's start) */
void ActorsFreeAll(void);       /* 81:9D82: the pool's 28 records back on the free list */
Actor *ActorAlloc(void);        /* 81:AB3F: a free record of the pool, or NULL */
Actor *ActorAllocB(void);       /* 81:EA41: a free record of the second pool (actors[3..7]), or NULL */

/* ---- creating and spawning ---- */

Actor *ActorCreate(uint8_t kind);   /* 82:AA2B: an actor of the kind, its set-up run; NULL: none free, or its set-up failed */
/* 80:A4DC: a new actor of the kind at (x, y) pixels with `byte07` its +$07, started at its next turn; NULL: none */
Actor *ActorSpawnAt(uint8_t kind, uint8_t byte07, uint16_t x, uint16_t y);

/* ---- ending (called from a kind's code, which then returns ACTOR_ENDED) ---- */

void ActorClear(Actor *a);      /* 83:975B: free, still, drawn no more, one fewer of its kind live */
void ActorFree(Actor *a);       /* 83:95E5: ActorClear, and back on the second pool's free list */

/* ---- placement ---- */

void PlacementsStart(void);     /* 82:A72C: the level's placements indexed and those in reach created (level load) */
void EscortStart(void);         /* 81:85A1: the level's escort (actors[1]) started beside the player */
void PlacementsScroll(void);    /* 82:A79C: the placements the camera's move brings into reach created */
void ActorsInView(void);        /* 81:9198: which actors can be hit this frame, and the list drawn */

/* ---- animation ---- */

void AnimSetsLoad(void);        /* 81:8F47: the level's animation sets loaded */
void AnimSetsFill(uint8_t set); /* 81:9D4C: every kind's animation set = `set` ($FF none) */
void AnimStart(Actor *a, uint8_t anim);     /* 81:90C2: animation `anim` from its first frame */
void AnimResume(Actor *a, uint8_t anim);    /* 81:90BE: animation `anim` at the frame it is on */
void AnimStep(Actor *a);        /* 81:912B: one frame of time on */

/* ---- the shared routines the kinds call (actors A: the record's actor_*.c,
   hit_box.c, player_*.c, shot_hit.c and the rest that are not kind*.c) ----

   How they answer:
   - A routine that may end its actor returns ACTOR_ENDED when it did (its
     record freed; the caller returns ACTOR_ENDED at once), else
     ACTOR_GOES_ON. A wait routine passes it on as its own answer.
   - A test the original answers in the carry is a bool, true for the case
     its comment names.
   - `at` is a table entry's byte offset, as the original's X or Y holds it.
   - A routine that waits inside it (the original's helpers that keep their
     caller's return at +$4E and yield, and those a kind jumps into for
     good) is called again at the same resume point of its caller each
     time the actor goes on, until it returns ACTOR_RETURNS (or
     ACTOR_RETURNS_C). Meanwhile it returns a wait (1-256), or
     ACTOR_ENDED. Where it is inside it is a->call_at (0: not inside it). */

#define ACTOR_RETURNS (-1)      /* back to its caller (the original's C clear where it says) */
#define ACTOR_RETURNS_C (-2)    /* back to its caller with the original's C set */

/* Own bytes the shared routines use (actor_own.h's OwnPlace): OWN_11, hits
   it takes (a stomp takes one); OWN_35-OWN_36, where ActorStompedByPlayer
   keeps a->resume (low byte first). */

/* range of the camera's point (actor_range.c, wait_step.c) */
bool ActorInRange(const Actor *a, uint8_t at);      /* 83:962C: true in range of the point by box `at` (8 bytes a box) */
int ActorEndOutOfRange(Actor *a, uint8_t at);       /* 83:96BE, 83:9659 Long: ended when out of range */
bool ActorFreeOutOfRange(Actor *a, uint8_t at);     /* 83:966C, 83:9668 Long: true when out of range and freed (ActorFreePlaced; its code goes on) */
int ActorEndIfLeftBehind(Actor *a);                 /* 83:9738: ended once the camera has left it behind */
int ActorEndFarWithChildren(Actor *a);              /* 83:C4AB */

/* where it is against the player or a point (player_near.c, player_relation.c, player_carry.c) */
uint8_t ActorSideOfPlayer(const Actor *a);          /* 81:AB85: the facing that looks at him: 0 right, 1 left */
uint8_t ActorPlayerBelow(const Actor *a);           /* 81:AB62: 1 when he is level with or below it */
bool ActorNearPlayer(const Actor *a, uint8_t at);   /* 81:ACDE: he is inside box `at` round it (4 bytes a box) */
bool ActorNearPlayerX(const Actor *a, uint8_t at);  /* 81:AD68: within span `at` across (2 bytes a span) */
bool ActorNearPlayerY(const Actor *a, uint8_t at);  /* 81:ADA4: within span `at` up or down */
bool ActorNearPoint(const Actor *a, uint8_t at, uint16_t x, uint16_t y);   /* 81:AD17: it is inside box `at` round (x, y) */
uint8_t ActorDirectionToPlayer(const Actor *a);     /* 81:ABEA: a direction of the table at $81:BBE3 */
uint8_t ActorDirectionToPoint(const Actor *a, uint16_t x, uint16_t y);     /* 81:ABFB */
uint8_t ActorAimAt(const Actor *a, uint16_t x, uint16_t y);                /* 81:AC68: a direction of the table at $80:BC03 */
void ActorFacePlayer(Actor *a);                     /* 83:FC96 */
bool PlayerInFrontBox(const Actor *a, uint8_t at);  /* 81:AEFD: he is inside box `at` in front of it (6 bytes a box) */

/* touches: the test box, zones, the player's (hit_box.c, actor_overlap.c, player_touch.c, solid_actor.c) */
bool ActorHitsTestBox(const Actor *a);              /* 83:8312, 83:8300 Long: it can be hit and overlaps the test box */
bool ActorHitsTestBoxIfActive(const Actor *a);      /* 83:830E: the same, its +$08 bit 7 clear */
bool ActorHitsTestBoxIfPlayerFree(const Actor *a);  /* 83:8304, 83:82F4 Long: the same, while Aladdin can be hit */
bool ActorHurtsPlayerOnTouch(Actor *a);             /* 83:80A5, 83:80A1 Long: true when he was hurt */
bool ActorStompOrHurt(Actor *a);                    /* 83:83C9, 83:83C5 Long: true when he was hurt (false: stomped, or no touch) */
bool ActorBouncesPlayer(Actor *a);                  /* 83:840F: true when he bounced off it */
bool ActorStompOrHurt16(Actor *a);                  /* 83:83AD: as ActorStompOrHurt, above by a margin of 16 */
void ActorStompedByPlayer(Actor *a);                /* 83:8362, 83:835E Long */
uint8_t ActorBouncesOrHurtsPlayer(Actor *a);        /* 83:844A, 83:8446 Long: 2 he was hurt, 1 he stands on it, 0 neither */
bool PlayerLandsOnActor(Actor *a);                  /* 83:848D: true when he landed on it */
void PlayerPushAside(const Actor *a);               /* 83:80C3, 83:80B5 Long: he is put beside it */
bool ActorZoneHasTestBox(const Actor *a, uint8_t zone_at);     /* 83:8752 */
bool ActorZoneHasTestPoint(const Actor *a, uint8_t zone_at);   /* 83:87EC, 83:87E8 Long */
bool ActorZoneHurtsPlayer(Actor *a, uint8_t zone_at);          /* 83:8743: true when he was hurt */
Actor *ActorTouchesSecondPool(const Actor *a);      /* 83:8578: the second pool's record that overlaps it, or NULL */
typedef enum {
    ACTOR_SOLID_BESIDE,         /* not tested, missed, or pushed out of its side (the original's C set, V clear) */
    ACTOR_SOLID_ON_TOP,         /* he stands on it (C, V clear) */
    ACTOR_SOLID_UNDER           /* he is under it or inside (C, V set) */
} ActorSolid;
ActorSolid ActorSolidForPlayer(Actor *a, uint8_t at);   /* 83:865E, 83:8657 Long: box `at` of the boxes at $80:BD66 */
void ActorCarryPlayer(Actor *a);                    /* 83:FBDA */
void ActorTestCarries(Actor *a, uint8_t test);      /* 83:FB89, 83:FB85 Long: test 0 solid, 2 land, by the box at its +$4C; $43 1 when he rides it; its place kept at $48, $4A; its animation on */

/* shots from the second pool (shot_hit.c): the shot that reached it, or NULL */
Actor *ActorDownedByShot(Actor *a);                 /* 83:80FC */
Actor *ActorHitByShotCC(Actor *a);                  /* 83:8135 */
Actor *ActorTurnsShotBack(Actor *a);                /* 83:819A, 83:8196 Long */
Actor *ActorEndsShot(Actor *a);                     /* 83:81CF */
Actor *ActorDownedByHarmlessShot(Actor *a);         /* 83:8208 */
Actor *ActorStopsShot(Actor *a);                    /* 83:8171, 83:816D Long */
Actor *ActorTakesShot(Actor *a);                    /* 83:81E8: a shot that hits (a harmless one passes) */

/* the record's state and fields (actor_fields.c, actor_state_8c.c, priority_clear.c) */
void ActorStateSet8C(Actor *a);                     /* 83:8267: state $8C, +$08 bit 7 set, +$09 bit 7 clear */
void ActorStateSet8CSelf(Actor *a);                 /* 83:8254: the same */
void ActorStateSetCC(Actor *a);                     /* 83:824B: state $CC */
void ActorPriorityClear(Actor *a);                  /* 84:81B2 */
void ActorFallSpeedCap(Actor *a);                   /* 83:B61A: falling, at most 6 pixels a frame */
void ActorSpawnPair(Actor *a);                      /* 83:D332: two children like it at spawn points $46 and $48 */
void ActorSpawnRing(Actor *a);                      /* 83:D0A3: up to five kind $34 children in a ring round it */
void ActorFlagOnce(Actor *a);                       /* 83:CF10: drops its children once (its cases 83:CF31-CF67) */
void Spawn11AtY(const Actor *at);                   /* 83:827D: a kind $11 started at its place, when the second pool has a record */
void Spawn13AtY(const Actor *at);                   /* 83:82B5: kind $13 */

/* routines that wait inside them (point_moves.c, knocked_away.c) */
int ActorHopFromTable(Actor *a, uint8_t at);        /* 83:FD2C: hop `at` of the table at $83:D9BF, back on the ground */
int ActorSpawnLookWait(Actor *a);                   /* 83:FC0D */
int ActorRiseWhenInReach(Actor *a, uint8_t at);     /* 83:FCA5: ACTOR_RETURNS once risen; ACTOR_RETURNS_C out of reach */
int ActorKnockedAway(Actor *a, uint8_t anim, uint8_t sound, uint8_t preset);    /* 83:9532: never returns; ends it */
int ActorKnockedAwayResumed(Actor *a, uint8_t anim, uint8_t frame, uint8_t preset);   /* 83:9589: never returns; ends it */
int ActorKnockedAwayResumedB(Actor *a, uint8_t anim, uint8_t frame, uint8_t preset);  /* 83:959F: the same */

/* ---- for video (OamDrawActor, 81:9309) ---- */

/* one piece of an actor's frame: 5 bytes of the list its picture_gfx names */
typedef struct SpritePiece {
    int8_t x;                   /* +0: from the actor's place */
    int8_t x_mirrored;          /* +1: the same when it faces the other way */
    int8_t y;                   /* +2 */
    uint16_t tile;              /* +3: the sprite's tile word (bit 12: 16 x 16) */
} SpritePiece;
/* its frame's pieces, valid until the next call; returns how many (the list's count byte) */
unsigned actor_pieces(const Actor *a, const SpritePiece **pieces);

/* wanted */

/* (n5-kinds-d, for kind $5D; actor_moves.c) */
void ActorKeepSpot(Actor *a);           /* 81:9D56: its x and y pixels kept (its +$4C, +$4E), for PlayerMoveWithActor */

/* (kinds B; their stand-ins in actor/wanted_b.c) */
void ActorFallAgainst(Actor *a);        /* 81:A520 (motion.c): gravity taken from the vertical speed, then ActorMoveY. NOTES-core
                                           lists it dead, but JSL $81:A520 is at 83:BDA1, 83:F5C6, 83:F731 and 84:AAAF */
void ActorMotionPresetY(Actor *a, uint8_t preset);  /* 81:A3EC (motion.c, inside ActorMotionPreset): the preset's vertical speed and gravity only */
void ActorHoldPlayer(Actor *a);         /* 83:BAF3, 83:BAEF Long (player_grab.c): its own +$13 the offset he is held at */
void ActorYByFrame47(Actor *a);         /* 83:BDE9 (actor_y_steps.c): its height follows its frame (the frame kept at +$47) */
void ActorYByFrame(Actor *a);           /* 83:F031 (actor_steps.c): its y by its frame's step; the frame kept at +$49 */
int ActorAnimFreeFar(Actor *a);         /* 84:832B (actor_84_misc.c): ACTOR_ENDED when freed out of range (box $58), else ACTOR_GOES_ON */

/* (n5-kinds-c, for kinds $2B, $2F, $4F) */
uint8_t ActorMapAhead5Down12(Actor *a);             /* 83:B5F6: the class of the map's block 5 ahead of it and 12 below */
void ActorVariantChildStep(Actor *a);               /* 83:D078 */
void ActorSpeedXSetAccel(Actor *a, uint8_t accel, uint8_t at);    /* 81:A411: acceleration `accel`, speed entry `at` */
void ActorMoveAlong(Actor *a, uint16_t direction, uint8_t speed);  /* 81:A73D: along direction `direction` (0-$1FF) of the direction table, at `speed` */

/* (n5-kinds-e, for kind $31; point_moves.c: kind $31 is their only caller) */
int ActorGoToPoint(Actor *a);       /* 83:FC5D: waits inside (ACTOR_RETURNS once within box $20 of the point): a step a frame
                                       toward the point at its +$48, +$4A (pixels) by direction set +$4C, facing the way it goes */
bool ActorProbeAheadInGround(const Actor *a, uint16_t ahead, uint16_t up);  /* 83:FD6D: true when the point `ahead` in front
                                       (by its facing) and `up` above, or `up` above its own x, is in the ground */

/* (player/) */
/* 81:A9E9: a child of `kind` at the spawner's spawn point, sharing its animated-tile slot, started at its next turn; NULL none free */
Actor *ActorSpawnShared(Actor *spawner, uint8_t kind);
/* (n5-kinds-e, for kind $3B; player_carry.c) */
void ActorStepCarry(Actor *a);              /* 83:C1EC: a step along its facing (vector set $0E), carrying the player riding it */
void ActorStepToTargetCarry(Actor *a);      /* 83:C1F6: a step (vector set 8) on each axis toward its target, the pixel words
                                               of its speed_x and speed_y (the original's $1A, $1E), carrying the player */
void ActorLiftPlayerStep(Actor *a, uint8_t offset);  /* 83:FB57: by its +$44: 0 and ActorGrabTest finds him, his code to his
                                               lift (PLAYER_AT_*) and he is placed on it, +$44 up; negative, up; positive, kept on it
                                               while g.player_no_actor_landing, else +$44 = $F0 */
/* (n5-kinds-e, for kinds $54 and $55; spawn_group.c: their only caller) */
void ActorSpawn5E(Actor *a);                /* 83:D7C9: a child of kind $5E at spawn point $4E (ActorCreateChild8) */
void ActorSpawnCopy(Actor *a);              /* 83:D7D4: a child of its own kind and look at spawn point 0, +$07 bit 3 set for it */
#endif
