/*
 * at_player.c: actor kind $12, kept at Aladdin's place until the box his
 * code wants is one of three.
 */
#include "actor/actor_own.h"
#include "data/kinds_b_tables.h"

/* its own byte +$11 (set to $FF at its start) */
#define K12_11 OWN_11

enum { KEEP = ACTOR_RESUME_OWN };

/*
 * @routine 81:EC02 ActorAtPlayer
 * WHAT IT DOES: Puts the actor at Aladdin's place (pixels) and moves it by
 * the motion preset his frame (+$2F) picks from the table at $80:CA03.
 * Only kind $12 calls it.
 */
static void ActorAtPlayer(Actor *a)
{
    a->x = with_pixels(a->x, pixels(g.actors[0].x));
    a->y = with_pixels(a->y, pixels(g.actors[0].y));
    ActorMotionPreset(a, at_player_motion(g.actors[0].frame));
    ActorMove(a);
}

/*
 * @routine 81:EBDE Kind12AtPlayer
 * WHAT IT DOES: Actor kind $12 (through 83:921A Kind12Start): at Aladdin's
 * place (ActorAtPlayer), +$11 $FF, facing as his flip; each frame at his
 * place again, until the box his code wants (g.player_box_wanted) is 3, 6
 * or $12; then it ends (ActorEnd). Its second entry (83:921E, a JML to
 * 81:EC1F, the JML to ActorEnd) is the same end.
 */
int Kind12AtPlayer(Actor *a)
{
    uint8_t box;

    if (a->resume == ACTOR_START_SECOND)
        return ActorEnd(a);
    if (a->resume == ACTOR_START) {
        ActorAtPlayer(a);
        a->own.bytes[K12_11] = 0xFF;
        a->facing = g.actors[0].flip;
        a->flip = g.actors[0].flip;
        ActorAtPlayer(a);
        a->resume = KEEP;
        return 1;
    }
    box = g.player_box_wanted;
    if (box == 0x03 || box == 0x06 || box == 0x12)
        return ActorEnd(a);
    ActorAtPlayer(a);
    return 1;
}
