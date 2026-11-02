/*
 * continue_choice.c: actor kind $26 on the continue screen: the two places
 * Aladdin can step down into (yes: continue; no) and the one that watches
 * him. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"
#include "wram_state.h"

void abi_ActorEndPlaced(void);
void abi_ActorZoneHasTestPointLong(void);
void abi_AnimStart(void);
void abi_PlayerSpotBelowActor(void);

#define CONTINUE_YES 0x0076u  /* set when Aladdin takes the continue (the screen then counts a continue off) */
#define CONTINUE_NO 0x0077u  /* set when he turns it down */
#define V_0380 0x0380u  /* copied each frame to the watcher's $46 */
#define PLAYER_RESUME 0x08DEu  /* the player's resume address (its record's $03, word) */
#define PLAYER_X 0x08E7u  /* the player's x pixels (its record's $0C) */
#define CHOICE_TAKEN 0x44u  /* set once Aladdin has stepped into it */
#define CHOICE_SINK 0x40u  /* frames left of its sinking */
#define WATCHER 0x42u  /* set: it is the watcher */
#define WATCHER_ANIM 0x45u  /* the animation it shows */
#define WATCHER_46 0x46u  /* V_0380, copied each frame */

/*
 * @routine 83:9D62 ContinueChoiceActor
 * @actor
 * WHAT IT DOES: An actor of the continue screen; none when no continues
 * are left. $07 low four bits zero: a place Aladdin can step down into
 * ($07 zero: the yes, facing right; else the no, facing left): when he is
 * in its zone it takes him (his code goes on at $D9C5, set below it),
 * sinks 16 pixels with him, and marks the choice. Otherwise the watcher:
 * its animation by the choice made, or by where Aladdin stands (1 between
 * x 80 and 175, 2 left of it, 4 right; 3 once yes, 5 once no).
 * HOW: $08 bit 1 and $09 bits $90 set; facing, flip and $42 cleared.
 * CONTINUES zero: ActorEndPlaced. Place: animation 0, $44 cleared; each
 * frame until ActorZoneHasTestPointLong (zone $84) clears C; PLAYER_RESUME
 * = $D9C5, $44 up, PlayerSpotBelowActor 5; 16 frames of its y pixels up by
 * one; CONTINUE_YES (or, $07 nonzero, CONTINUE_NO) up; then BRK for good.
 * Watcher: $45 cleared, $42 up; each frame a new animation (A) is started
 * when not $45, $46 = V_0380, BRK, then A from the choice or the player's
 * x.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $26 in state $0C. Exit: never returns;
 * with no continues ends in ActorEndPlaced.
 */
void abi_ContinueChoiceActor(void)
{
    uint8_t variant, anim, x;

    wr8(dpa(ACTOR_08), (uint8_t)(rd8(dpa(ACTOR_08)) | 0x02));
    a8_load((uint8_t)(rd8(dpa(ACTOR_09)) | 0x90));
    wr8(dpa(ACTOR_09), (uint8_t)cpu.a);
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    wr8(dpa(WATCHER), 0);
    a8_load(rd8(dba(CONTINUES)));
    if (!(uint8_t)cpu.a) {                /* no continues left */
        abi_ActorEndPlaced();
        return;
    }
    variant = rd8(dpa(ACTOR_07));
    a8_load(variant & 0x0F);
    if (!(variant & 0x0F)) {
        /* a choice ($07 0: yes, else no): Aladdin steps into it, it sinks
           16 pixels, and the choice is made */
        a8_load(0x00);
        abi_AnimStart();
        wr8(dpa(CHOICE_TAKEN), 0);
        a8_load(rd8(dpa(ACTOR_07)));
        if ((uint8_t)cpu.a) {
            a8_load(0x01);
            wr8(dpa(ACTOR_FACING), 0x01);
            wr8(dpa(ACTOR_FLIP), 0x01);
        }
        do {
            actor_yield(0x839D94u);
            y8_load(0x84);
            abi_ActorZoneHasTestPointLong();
        } while (cpu.p & P_C);
        wr16(dba(PLAYER_RESUME), 0xD9C5);
        cpu.a = 0xD9C5;
        inc8_at(dpa(CHOICE_TAKEN));
        a8_load(0x05);
        abi_PlayerSpotBelowActor();
        a8_load(0x10);
        wr8(dpa(CHOICE_SINK), 0x10);
        do {
            actor_yield(0x839DB4u);
            inc8_at(dpa(ACTOR_Y + 1));
        } while (dec8_at(dpa(CHOICE_SINK)));
        a8_load(rd8(dpa(ACTOR_07)));
        inc8_at(dba((uint8_t)cpu.a ? CONTINUE_NO : CONTINUE_YES));
        for (;;)
            actor_yield(0x839DC8u);
    }
    /* the watcher: its animation by where Aladdin is, or the choice made */
    wr8(dpa(WATCHER_ANIM), 0);
    inc8_at(dpa(WATCHER));
    for (;;) {
        anim = (uint8_t)cpu.a;
        cmp8_of(anim, rd8(dpa(WATCHER_ANIM)));
        if (anim != rd8(dpa(WATCHER_ANIM))) {
            wr8(dpa(WATCHER_ANIM), anim);
            abi_AnimStart();
        }
        a8_load(rd8(dba(V_0380)));
        wr8(dpa(WATCHER_46), (uint8_t)cpu.a);
        actor_yield(0x839DDFu);
        a8_load(rd8(dba(CONTINUE_YES)));
        if ((uint8_t)cpu.a) {
            a8_load(0x03);
            continue;
        }
        a8_load(rd8(dba(CONTINUE_NO)));
        if ((uint8_t)cpu.a) {
            a8_load(0x05);
            continue;
        }
        x = rd8(dba(PLAYER_X));
        a8_load(x);
        cmp8_of(x, 0x50);
        if (x < 0x50) {
            a8_load(0x02);
            continue;
        }
        cmp8_of(x, 0xB0);
        a8_load(x >= 0xB0 ? 0x04 : 0x01);
    }
}
