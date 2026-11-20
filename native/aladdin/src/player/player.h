/*
 * player.h: Aladdin, as the rest of the game calls him (the record's
 * player/). He is actor record 0, g.actors[0], run by PlayerLoop
 * (actor/actor.h) as any actor: his code is one function of the record
 * that switches on its resume point (plan, "Actors' yields").
 *
 * His code. Kinds $00-$03 and $07 are his (the record's tables at
 * $83:8CAB, $83:8DFD, $83:8F4F): their start entry is PlayerStart, their
 * second entry (state $80-$BF, as PlayerEnter8C sets) PlayerKnockback, or
 * PlayerCarpetCrash for kind $01 (the carpet), and their wait routine
 * PlayerFrameWork. The actor core calls PlayerCode and PlayerWait for
 * those kinds.
 *
 * Sending him to a state. Other areas' code sends his code to one of his
 * states, as the original writes the state's address into his record's
 * resume address ($08DE): it sets g.actors[0].resume to a PLAYER_AT_*
 * value, and he goes on there when his wait runs out. Code that runs on
 * his record and jumps to one of his states (the scenes' code in game/)
 * calls PlayerGoTo, which runs that state's code at once, as the jump.
 */
#ifndef ALADDIN_PLAYER_H
#define ALADDIN_PLAYER_H

#include <stdint.h>

#include "actor/actor.h"

/* ---- his code (kinds $00-$03, $07) ---- */

int PlayerCode(Actor *a);       /* frames to wait (1-256); his record never ends */
int Kind60Start(Actor *a);      /* 83:E480 GroundPushMark: kind $60, the mark at his feet on ground that pushes him (ground_mark.c) */
int PlayerWait(Actor *a);       /* 83:91F8 ActorKind00Wait: PlayerFrameWork; returns ACTOR_GOES_ON */
void PlayerFrameWork(Actor *a); /* 81:E864: his work each frame he waits (animation, hurt time, picture, box) */

/* where other code sends him (g.actors[0].resume) */
#define PLAYER_BLOCK(n) ((uint16_t)((n) << 8))   /* each of player/'s files numbers its resume points in its own block */
enum {
    PLAYER_AT_START = ACTOR_START,                  /* 81:CB3B PlayerStart */
    PLAYER_AT_KNOCKBACK = ACTOR_START_SECOND,       /* 81:DADD PlayerKnockback (kind $01: 81:E7E9 PlayerCarpetCrash) */

    PLAYER_AT_STAND = PLAYER_BLOCK(2),              /* 81:CCB6 PlayerStand */
    PLAYER_AT_FALL,                                 /* 81:CDD2 PlayerFall */

    PLAYER_AT_HELD = PLAYER_BLOCK(3),               /* 81:D6F8 PlayerHeld: an actor holds him */
    PLAYER_AT_BOUNCE,                               /* 81:D77C PlayerBounce: a pad bounces him */
    PLAYER_AT_SPRUNG,                               /* 81:D8FD PlayerSprung: a spring throws him up */
    PLAYER_AT_CARRIED,                              /* 81:D992 PlayerCarried: kind $2C carries him */
    PLAYER_AT_GRABBED_HANG,                         /* 81:D99B PlayerGrabbedHang: he hangs from what lifts him */
    PLAYER_AT_SWITCH_HANG,                          /* 81:D9C5 PlayerSwitchHang */
    PLAYER_AT_FLUNG,                                /* 81:DA14 PlayerFlung */

    PLAYER_AT_ESCORT_EXIT = PLAYER_BLOCK(4),        /* 81:E730 PlayerEscortExit: the level's exit with its escort */
    PLAYER_AT_STAGE_END,                            /* 81:E7B1 PlayerStageEnd */
    PLAYER_AT_SPOT_EXIT,                            /* 81:CBF9 PlayerSpotExit: the level's exit at a spot */
    PLAYER_AT_SCENE_FIGURE,                         /* 81:CC31 PlayerSceneFigure */

    PLAYER_AT_SCENE_CUED = PLAYER_BLOCK(5),         /* 81:E154 PlayerSceneCued: a scene's actor has him */
    PLAYER_AT_START_DROPPED,                        /* 81:DDFF PlayerStartDropped ($07 5) */
    PLAYER_AT_START_NEW_GAME,                       /* 81:DE32 PlayerStartNewGame ($07 6) */
    PLAYER_AT_SCENE_WALK_ON,                        /* 81:E042 PlayerSceneWalkOn ($07 10) */
    PLAYER_AT_SCENE_WALK_RIGHT,                     /* 81:E126 PlayerSceneWalkRight ($07 11) */

    PLAYER_AT_SCENE_LAND = PLAYER_BLOCK(6),         /* 81:DFBB PlayerSceneLand ($07 9; filed in the record's game/scene_figures.c) */
    PLAYER_AT_SCENE_STAND,                          /* 81:E23E PlayerSceneStand ($07 15) */
    PLAYER_AT_ENDING_RIDE,                          /* 81:E27A PlayerEndingRide ($07 14) */
};

/* a jump to one of his states from code running on his record: his code
   goes on at `at` now; returns as PlayerCode */
int PlayerGoTo(Actor *a, uint16_t at);

/* ---- his moves (the animation each picks) ---- */

void PlayerMoveStart(Actor *a, uint8_t move);   /* 81:DCA2: g.player_move = move, its animation from its first frame */
void PlayerMoveChange(Actor *a, uint8_t move);  /* 81:DC9D: the same, unless he is in that move already */

/* ---- his hurt (state $8C: the knockback, on his next turn) ---- */

void PlayerEnter8C(void);                       /* 83:8008 (83:8004 Long): hit, one health less */
/* 83:8024 (83:8000 Long): hit by `by`, turned away from it, unless he is
   already hit (g.actors[0].anim_flags bit 7); returns 1 when he was hit */
int PlayerEnter8CFacing(const Actor *by);
void PlayerEnter8CFrom1(void);                  /* 83:8056 (83:8052 Long): fallen out: health 1 first, so it ends at 0 */

/* ---- his place against actors ---- */

/* 82:89E3: lands him on `a` when he falls (or is still) onto its top, by
   the landing box `box` (an offset into the boxes, as the original's Y);
   returns 1 when he landed */
int PlayerLandOnActor(const Actor *a, uint16_t box);
void PlayerMoveWithActor(Actor *a);             /* 81:9D63: moved as far as `a` moved since its own bytes kept its place */
void PlayerToActorAbove(const Actor *a, uint8_t offset);   /* 82:BB3A: at offset `offset` above `a`, along his facing */
void PlayerToActorBelow(const Actor *a, uint8_t offset);   /* 82:BB26: below */
void PlayerSpotAboveActor(const Actor *a, uint8_t offset); /* 82:BB73: where ToActorAbove would put him, in g.player_saved_x, _y; he stays */
void PlayerSpotBelowActor(const Actor *a, uint8_t offset); /* 82:BB9C */

/* ---- on screen ---- */

void PlayerKeepOnScreenX(Actor *a);             /* 81:E990: x 16 to 240 pixels from the play field's left */
void PlayerKeepOnScreen(Actor *a);              /* 81:E9AD: and y 24 to 216 from its top */

#endif
