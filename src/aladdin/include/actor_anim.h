/*
 * actor_anim.h: actors' animations (actor/anim.c): the fields of an
 * actor's record they use, and the tables a level loads. The same names
 * as asm/aladdin/include/actor_anim.inc, in upper case.
 */
#ifndef ALADDIN_ACTOR_ANIM_H
#define ALADDIN_ACTOR_ANIM_H

/* Record fields (offsets in the actor's record, its direct page; the
   others in actor.h). ACTOR_06 picks its animation set too, and bit 3 of
   ACTOR_08 says it is animated. */
#define ACTOR_PICTURE_GFX 0x22u   /* the frame's picture: its graphics (word, from the set's base) */
#define ACTOR_ANIM_TABLE 0x24u    /* its set's animation table, in the animation data (word) */
#define ACTOR_FRAME_TIME 0x26u    /* frames the frame has left */
#define ACTOR_PICTURE 0x27u       /* the frame's picture; at a start, the animation's number */
#define ACTOR_REDRAW 0x28u        /* $FF: its picture changed */
#define ACTOR_ANIM 0x2Du          /* the animation's offset in its set's table (its number times 2) */
#define ACTOR_FRAME 0x2Fu         /* the frame in the animation */
#define ACTOR_LOOP_BACK 0x30u     /* frames the animation steps back at its end */
#define ACTOR_GFX_BASE 0x31u      /* its set's graphics, in the picture list (word) */

/* Tables the level loads */
#define ANIM_SET_OF_KIND 0x7FE000u /* per kind, its animation set (byte) */
#define ANIM_SET_GFX 0x7FE100u     /* per set, its graphics base (word) */
#define ANIM_SET_TABLE 0x7FE300u   /* per set, its animation table (word) */
#define ANIM_DATA 0x7EB000u        /* animation tables, then each animation's frames: word per frame, time then picture; time 0 ends */
#define PICTURE_LIST 0x7E2000u     /* per picture, its graphics' offset */

/* Actors' ends (actor/actor_end.c) */
#define ACTOR_0A 0x0Au
#define ACTOR_13 0x13u
#define ACTOR_2C 0x2Cu
#define ACTOR_33 0x33u
#define ACTOR_38 0x38u
#define KIND_COUNT 0x7EFF00u      /* by ACTOR_06, how many such actors are live */
#define ACTOR_POOL2 0x0015DAu     /* the second pool's free records (words), a stack */
#define ACTOR_POOL2_TOP 0x0015E6u /* byte offset of its top entry */
/* Animated-tile slots (video/tile_slots.c): by slot 0-3, the map cell
   that claimed it; by cell, its attribute word and how many more slots it
   holds after its first (0-2) */
#define TILE_SLOT_CELL 0x0015EDu
#define CELL_ATTRS 0x7FE700u
#define CELL_EXTRA_SLOTS 0x7FE900u

#endif
