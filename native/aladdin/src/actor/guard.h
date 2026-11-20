/*
 * guard.h: what the files of actor kind $08, the guard (guard_start.c,
 * guard_pieces.c, guard_chase.c, guard_states.c), share and nothing else
 * reads: its own bytes, its resume points and its code words.
 */
#ifndef ALADDIN_GUARD_H
#define ALADDIN_GUARD_H

#include "actor/actor_own.h"
#include "map/map.h"

/* its own bytes */
#define G_CLEARED_40 OWN_40     /* +$40: cleared at its start (read by nothing found) */
#define G_MOVING OWN_41         /* +$41: counted up while a move is under way, cleared when it ends; nonzero, it does not chase */
#define G_LANDING OWN_42        /* +$42: frames left of a walk or a settle */
#define G_IN_AIR OWN_44         /* +$44: set for the frame it leaves the ground; nonzero, it may jump down to Aladdin */
#define G_GONE OWN_45           /* +$45: counted up when it runs off for good */
#define G_BACK_OFF OWN_46       /* +$46: nonzero, a wall it cannot climb turns it back */
#define G_SHOWN OWN_49          /* +$49: nonzero (started by $07 12), it never gives up the chase */
#define G_STATE OWN_4D          /* +$4D: its wait routine's state */
#define G_RESTART_AT OWN_4E     /* +$4E: the escort's restart point, a resume point of its code (the original's code word) */

/* its resume points: the entries its code words name (the original's
   writes of its resume address, $03) and the places inside its moves */
enum {
    G_WALK_IN = ACTOR_RESUME_OWN,   /* its start's 40-frame walk */
    G_STOP,                     /* A5:E5B6 GuardStop's entry (its code word): it yields first */
    G_STOP_FRAME,               /* each frame of it */
    G_RUN,                      /* A5:E5E8 GuardRun's entry */
    G_RUN_ON,                   /* each frame of the run */
    G_RUN_EDGE,                 /* off an edge: after its first frame in the air */
    G_RUN_DOWN_SPAWN,           /* GuardRunDown: its children thrown */
    G_RUN_DOWN_WAIT,            /* waiting for their report */
    G_JUMP,                     /* A5:E6A0 GuardJump's entry */
    G_JUMP_AIR,                 /* each frame in the air */
    G_SETTLE,                   /* landed: each frame of its settle */
    G_CLIMB,                    /* each frame up a wall */
    G_GIVE_UP,                  /* A5:E794 GuardGiveUp's entry */
    G_GIVE_UP_HIDDEN,           /* while something holds it back */
    G_GIVE_UP_BACK,             /* put at Aladdin: until it can be hit again */
    G_LAND_WAIT,                /* GuardLand: after 32 frames of pose 7 */
    G_WALK,                     /* A5:E807 GuardWalk's entry */
    G_STILL,                    /* for good */
    G_CHASE_POSE,               /* A5:E812 GuardChasePose's entry */
    G_CHASE_POSE_ON,            /* after 96 frames of pose $14 */
    G_NOT_CONVERTED,            /* a start its $07 names that is other code (reported) */
    G_RESTART,                  /* A5:E554 GuardRestart: its fields cleared, on by its $07 (kind $69 sends the escort here) */

    /* the escort's moves (guard_escort.c): from G_ESCORT_FIRST */
    G_ESCORT_STAY18,            /* A5:EF0C EscortStaySpot18's entry */
    G_ESCORT_STAYING,           /* for good */
    G_ESCORT_SPOT0,             /* A5:EF15 EscortWaitSpot0's entry */
    G_ESCORT_SPOT4,             /* A5:EF2B EscortWaitSpot4's */
    G_ESCORT_SPOT0C,            /* A5:EF97 EscortWaitSpot0C's */
    G_ESCORT_SPOT10,            /* A5:EFAE EscortWaitSpot10's */
    G_ESCORT_SPOT10_AGAIN,      /* A5:EFC5: its restart point, at spot $14 */
    G_ESCORT_SPOT10_NEAR,       /* each frame until Aladdin is near it there */
    G_ESCORT_FOLLOW,            /* A5:EF3F EscortFollow: each frame until Aladdin is near */
    G_ESCORT_WALK,              /* each frame of its walk after him */
    G_ESCORT_FALL               /* each frame of its fall off a ledge */
};
#define G_ESCORT_FIRST G_ESCORT_STAY18

int Kind08Start(Actor *a);              /* guard_start.c: 83:9205 Kind08Enter, A5:E515 GuardStart, A5:E554 GuardRestart */
int GuardPieces(Actor *a);              /* guard_pieces.c: every resume point from G_WALK_IN */
int GuardEscort(Actor *a);              /* guard_escort.c: the escort's moves, from G_ESCORT_FIRST */
int guard_stop(Actor *a);               /* on at GuardStop */
void guard_chase(Actor *a);             /* guard_chase.c: A5:F0F4 GuardChase */
void guard_chase_speed_up(Actor *a);    /* A5:F127 GuardChaseSpeedUp */
void guard_chase_close(Actor *a);       /* A5:F172 GuardChaseClose */

#endif
