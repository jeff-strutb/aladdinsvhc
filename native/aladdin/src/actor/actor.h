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

/* wanted */

/* (native-video, for OamDrawActor 81:9309) one piece of an actor's frame:
   the 5 bytes a piece of the list its +$22 names (in the original's bank
   $7E, after the count byte) */
typedef struct SpritePiece {
    int8_t x;                   /* +0: from the actor's place */
    int8_t x_mirrored;          /* +1: the same when it faces the other way */
    int8_t y;                   /* +2 */
    uint16_t tile;              /* +3: the sprite's tile word (bit 12: 16 x 16) */
} SpritePiece;
/* its frame's pieces; returns how many (the list's count byte) */
unsigned actor_pieces(const Actor *a, const SpritePiece **pieces);

#endif
