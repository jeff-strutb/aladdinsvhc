/*
 * kind6a.c: actor kind $6A, which stage a3 s1 places as spawners: a
 * spawner ($07 0-3) makes a child of its own kind with $07 4 more, again
 * and again after a pause by its $07; a child ($07 4-7) moves one of four
 * ways for good. Its wait routine is Kind6AWait: its animation, Aladdin
 * hurt by touching it, ended once out of range (box $18). Its second
 * entry (83:EC1C Kind6ASecond) ends it.
 */
#include <stdio.h>

#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "data/kinds_d_tables.h"

enum {
    SPAWN = ACTOR_RESUME_OWN,   /* the spawner: each pause's end */
    WAY4,                       /* a child's way, each frame */
    WAY5,
    WAY6,
    WAY7
};

/*
 * @routine 83:EB9C Kind6ASpawnChild
 * WHAT IT DOES: A kind $6A spawner makes a child like itself with $07 4
 * more, at its spawn point 0, and counts it for its kind.
 */
static void spawn_child(Actor *a)
{
    uint8_t kept = a->spawn_byte;

    a->spawn_at = 0;
    a->spawn_byte = (uint8_t)(kept + 4);
    if (ActorSpawnSameLook(a, a->kind))
        g.kind_count[a->kind]++;
    a->spawn_byte = kept;
}

/*
 * @routine 83:EB5E Kind6AStart
 * WHAT IT DOES: Actor kind $6A: by its $07, a spawner (0-3) that makes a
 * child of its own kind with $07 4 more and waits its pause ($80:D65B by
 * $07), again and again; or a child (4-7), shown in animation 0 facing
 * right, that moves one of four ways for good: 83:EBC1 Kind6AWay4 (thrown
 * up with gravity $0C, then slowed and moved up and down), 83:EBD5
 * Kind6AWay5 (speed entry $36 across gaining 6, $60 up or down, each frame
 * faster), 83:EBEB Kind6AWay6 (entries $39 and $60, moved each frame) and
 * 83:EBFF Kind6AWay7 (entry $3C across gaining 8, $60 with gravity 8, each
 * frame 8 faster across and flown against its pull: 81:A498
 * ActorSpeedXUpFlyAgainst, which only it calls). Also 83:EC1C
 * Kind6ASecond: its second entry ends it.
 */
int Kind6AStart(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        if (a->spawn_byte >= 0x04) {
            a->anim_flags |= 0x02;
            a->flip = 0;
            a->flags |= 0x90;
            AnimStart(a, 0);
        }
        switch (a->spawn_byte) {
        case 0: case 1: case 2: case 3:
            spawn_child(a);
            a->resume = SPAWN;
            return k6a_pause(a->spawn_byte);
        case 4:
            ActorSpeedYSetGravity(a, 0x0C, 0x00);
            a->resume = WAY4;
            return 1;
        case 5:
            ActorSpeedXSetAccel(a, 0x06, 0x36);
            ActorSpeedYSet(a, 0x60);
            a->resume = WAY5;
            return 1;
        case 6:
            ActorSpeedXSet(a, 0x39);
            ActorSpeedYSet(a, 0x60);
            a->resume = WAY6;
            return 1;
        case 7:
            ActorSpeedXSetAccel(a, 0x08, 0x3C);
            ActorSpeedYSetGravity(a, 0x08, 0x60);
            a->resume = WAY7;
            return 1;
        default: {
            static int told;

            /* past its table of eight ways: the original jumps where the
               next words point */
            if (!told++)
                fprintf(stderr, "Kind6AStart: $07 %u (past its table) not converted\n", a->spawn_byte);
            a->resume = WAY4 + 4;
            return 1;
        }
        }
    case SPAWN:
        spawn_child(a);
        return k6a_pause(a->spawn_byte);
    case WAY4:
        ActorAccelerateYBack(a);
        ActorMoveY(a);
        return 1;
    case WAY5:
        ActorSpeedUpMove(a);
        return 1;
    case WAY6:
        ActorMove(a);
        return 1;
    case WAY7:
        a->speed_x = signed24(wrap24((uint32_t)a->speed_x + 8));
        ActorFlyAgainst(a);
        return 1;
    case ACTOR_START_SECOND:
        return ActorEndPlaced(a);
    default:
        return 1;
    }
}

/*
 * @routine 83:EC20 Kind6AWait
 * WHAT IT DOES: Kind $6A's wait routine: its animation steps; Aladdin
 * touching it is hurt; it ends once out of range (box $18).
 */
int Kind6AWait(Actor *a)
{
    AnimStep(a);
    (void)ActorHurtsPlayerOnTouch(a);
    return ActorEndOutOfRange(a, 0x18);
}
