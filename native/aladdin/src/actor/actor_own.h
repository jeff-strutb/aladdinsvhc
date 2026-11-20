/*
 * actor_own.h: what the actor files share and the rest of the game does
 * not: each kind's view of its own bytes, the 24-bit motion arithmetic, the
 * kinds' code by number, and the calls into areas whose headers do not have
 * them yet.
 */
#ifndef ALADDIN_ACTOR_OWN_H
#define ALADDIN_ACTOR_OWN_H

#include <stdint.h>

#include "actor/actor.h"
#include "system/task.h"
#include "system/unconverted.h"
#include "video/video.h"           /* TileSlotClaim, TileSlotFill (wanted there), PaletteLoad */     /* SoundQueueAdd (audio's, not converted yet) */

/* ---- each kind's own bytes (Actor.own.bytes) ----

   Where each of the record's bytes is in own.bytes: the one place the
   layout is written (it follows state.h's comment). A change to own's
   layout changes this list, and every view below follows. */
enum OwnPlace {
    OWN_0A, OWN_11, OWN_13, OWN_16, OWN_17, OWN_18, OWN_29, OWN_2A, OWN_35, OWN_36,
    OWN_3A, OWN_3B, OWN_3C, OWN_3D, OWN_3E, OWN_3F, OWN_40, OWN_41, OWN_42, OWN_43,
    OWN_44, OWN_45, OWN_46, OWN_47, OWN_48, OWN_49, OWN_4C, OWN_4D, OWN_4E, OWN_4F,
    OWN_4A, OWN_4B,         /* the same bytes as Actor.stream: a kind uses one or the other */
    OWN_BYTES
};

_Static_assert(OWN_BYTES == sizeof(((Actor *)0)->own.bytes), "OwnPlace follows own.bytes");

/* the story's figures (kinds $36, $95, $96): their counts */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: frames left of its walk or step */
    uint8_t step;               /* +$41: kind $95: part 8's step; the hops left */
    uint8_t between[OWN_4C - OWN_42];
    uint8_t hop;                /* +$4C: kind $95: the hops' set, then its offset (set * 2) */
} FigureOwn;

_Static_assert(sizeof(FigureOwn) <= OWN_BYTES, "a view fits the own bytes");

static inline FigureOwn *figure(Actor *a) { return (FigureOwn *)a->own.bytes; }

/* ActorBobY's byte, +$49: which way the bob goes (0 down) */
typedef struct {
    uint8_t before[OWN_49];
    uint8_t rising;             /* +$49 */
} BobOwn;

static inline BobOwn *bob(Actor *a) { return (BobOwn *)a->own.bytes; }

/* ---- 24-bit motion: positions 16.8 that wrap at 24 bits, speeds 16.8
   kept sign-extended from 24 ---- */

static inline uint32_t wrap24(uint32_t v) { return v & ((1u << 24) - 1); }
static inline int32_t signed24(uint32_t v) { return (int32_t)(v << 8) >> 8; }
static inline uint16_t pixels(uint32_t v) { return (uint16_t)(v >> 8); }
static inline uint32_t with_pixels(uint32_t v, uint16_t px) { return (v & 0xFFu) | (uint32_t)px << 8; }

/* ---- the kinds' code (kinds.c) ---- */

/* One kind's code, as a session registers it in its own table (kinds.c's
   own, kinds_a.c ... kinds_e.c, kinds_player.c): each table ends with a row
   whose start and wait are both NULL. A kind is in one table only, and its
   entries are one function (the loop resumes it there whichever entry it
   started at): second and third name the start's function, or are NULL
   when the record's table has no such entry. kinds.c checks both at the
   first lookup and ends the run, naming the rows, when either is broken. */
typedef struct KindCode {
    uint8_t kind;
    ActorCode start;            /* its code: start entry (state $0C-$7F) and every resume point */
    ActorCode second;           /* start's function if it has a second entry (state $80-$BF; $83:8DFD/8EFD), else NULL */
    ActorCode third;            /* start's function if it has a third (state $C0 up; $83:90A1/91A1: kinds $20, $2D, $57, $63), else NULL */
    ActorWaitCode wait;         /* its wait routine (flags bit 7), NULL none */
} KindCode;

extern const KindCode kinds_core[], kinds_a[], kinds_b[], kinds_c[], kinds_d[], kinds_e[], kinds_player[];

/* the kind's code for a start entry (ACTOR_START, _SECOND, _THIRD) or, with
   ACTOR_RESUME_OWN, for going on where it stopped; NULL: no code for it yet
   (reported once) */
ActorCode actor_kind_code(uint8_t kind, uint8_t entry);
ActorWaitCode actor_kind_wait(uint8_t kind);                    /* NULL: none, or none yet (reported once) */

int Kind36Start(Actor *a);
int Kind95Start(Actor *a);
int SteppedShowActor(Actor *a);     /* kind $96 */
int Kind82Show(Actor *a);
int Kind4AShow(Actor *a);
int Kind85Start(Actor *a);
int AnimateWhileWaiting(Actor *a);          /* the wait routine of kinds $4A, $82, $85, $95, $96 */
int AnimateWhileWaitingIf07Bit4(Actor *a);  /* kind $36's */

/* ---- ending, spawning, motion (actor_end.c, spawn.c, motion.c) ---- */

int ActorEndPlaced(Actor *a);       /* 83:96CA: ActorFreePlaced; returns ACTOR_ENDED */
void ActorFreePlaced(Actor *a);     /* 83:96E2 */
void ActorClearFields(Actor *a);    /* 83:9768 */
void ActorTileSlotRelease(Actor *a);    /* 83:9706 */
Actor *ActorCreateChild8(Actor *spawner, uint8_t kind);    /* 81:A9B1: NULL none */
void ActorSpawnInit(Actor *spawner, Actor *child);         /* 81:AA11 */
void ActorSpawnPlace(Actor *spawner, Actor *child);        /* 81:AA81 */
void ActorTileSlotShare(Actor *spawner, Actor *child);     /* 81:A9FA */
Actor *ActorSpawn(Actor *spawner, uint8_t kind);           /* 81:A9BF: NULL none (no kind count, no set-up) */
Actor *ActorSpawnSameLook(Actor *spawner, uint8_t kind);   /* 81:A9CE */
Actor *ActorSpawnFacing(Actor *spawner, uint8_t kind, uint8_t facing, uint8_t byte07);   /* 81:AA2D */
int ActorEnd(Actor *a);             /* 83:95B8: the second pool's; returns ACTOR_ENDED */
Actor *ActorCreateChild(Actor *spawner, uint8_t kind);     /* 81:A9A6 */
int ActorFreeListSort(uint8_t passes);      /* 81:ADC0: lowest on top; -1 none or too few */
int ActorFreeListSortHigh(uint8_t passes);  /* 81:ADFC: highest on top; -1 none or too few */
int ActorFreeListTake(uint8_t count);       /* 81:AE38: the first taken's place in g.free_pool; -1 too few */

void ActorMotionPreset(Actor *a, uint8_t preset);   /* 81:A3DD */
void ActorSpeedXSet(Actor *a, uint8_t at);          /* 81:A413 */
void ActorSpeedYSet(Actor *a, uint8_t at);          /* 81:A425 */
void ActorSpeedXFrom84(Actor *a, uint8_t at);       /* 84:9ED0 (its table read in bank $80: actor_tables.c) */
void ActorSpeedYFrom84(Actor *a, uint8_t at);       /* 84:9EE0 */
void ActorAccelerateX(Actor *a);    /* 81:A5FF */
void ActorSpeedUpMove(Actor *a);    /* 81:A4AF */
void ActorMove(Actor *a);           /* 81:A55D */
void ActorMoveX(Actor *a);          /* 81:A5A7 */
void ActorMoveY(Actor *a);          /* 81:A560 */
void ActorFly(Actor *a);            /* 81:A54A */
void ActorStepYBack(Actor *a);      /* 81:A4F7 */
void ActorSpeedYSetGravity(Actor *a, uint8_t gravity, uint8_t at);  /* 81:A423 */
void ActorSpeedXClear(Actor *a);    /* 81:A437 */
void ActorStopX(Actor *a);          /* 81:A435: and its acceleration */
void ActorSpeedYClear(Actor *a);    /* 81:A440 */
void ActorStopY(Actor *a);          /* 81:A43E: and its gravity */
void ActorFall(Actor *a);           /* 81:A50B: gravity, then ActorMoveY */
void ActorDecelerateX(Actor *a);    /* 81:A63B: never below 0 */
void ActorMotionReset(Actor *a, uint8_t gravity);  /* 81:A3CE */
void ActorStepY(Actor *a);          /* 81:A4E3 */
void ActorMoveRight(Actor *a);      /* 81:A5AB: right whichever way it faces */
void ActorAccelerateY(Actor *a);    /* 81:A613 */
void ActorAccelerateXBack(Actor *a);    /* 81:A627 */
void ActorAccelerateYBack(Actor *a);    /* 81:A657 */
void ActorDecelerateY(Actor *a);    /* 81:A66B: never below 0 */
void ActorFlyAgainst(Actor *a);     /* 81:A535 */
void ActorBobY(Actor *a);           /* 81:A574: its +$49 the bob's way */
void ActorMoveByVector(Actor *a, uint8_t set);         /* 81:A854: `set` the direction set's byte offset */
void ActorMoveBackByVector(Actor *a, uint8_t set);     /* 81:A897 */
void ActorSpeedAddVector(Actor *a, uint8_t set);       /* 81:A8DA */
void ActorSpeedSetVector(Actor *a, uint8_t set);       /* 81:A91D */
void ActorMoveByVectorX(Actor *a, uint8_t set);        /* 81:A952 */
void ActorMoveByVectorY(Actor *a, uint8_t set);        /* 81:A97C */
void ActorMoveByVectorTimes(Actor *a, uint8_t set, uint8_t times);  /* 81:A74C */

#endif
