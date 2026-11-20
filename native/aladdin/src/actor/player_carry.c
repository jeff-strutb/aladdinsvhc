/*
 * player_carry.c: actors and the player beside them: carried along while
 * he rides one (as it steps), and faced. An actor that carries him keeps
 * in its own bytes whether he rides it this frame ($43) and its place as
 * it was then ($48-$49 x, $4A-$4B y, pixels).
 */
#include "actor/actor_a.h"
#include "player/player.h"

static uint16_t own_word(const Actor *a, int at)
{
    return (uint16_t)(a->own.bytes[at] | a->own.bytes[at + 1] << 8);
}

static void own_word_set(Actor *a, int at, uint16_t v)
{
    a->own.bytes[at] = (uint8_t)v;
    a->own.bytes[at + 1] = (uint8_t)(v >> 8);
}

/*
 * @routine 83:FBDA ActorCarryPlayer
 * WHAT IT DOES: Carries the player along with the actor he rides (its $43
 * set): he moves, and his place as the frame began with him, by how far
 * the actor moved since its place was kept.
 * Also 83:FBD6 ActorCarryPlayerLong, for callers in other banks.
 */
void ActorCarryPlayer(Actor *a)
{
    Actor *p = &g.actors[0];
    uint16_t dx, dy;

    if (!a->own.bytes[OWN_43])
        return;
    dx = (uint16_t)(pixels(a->x) - own_word(a, OWN_48));
    p->x = with_pixels(p->x, (uint16_t)(pixels(p->x) + dx));
    g.frame_start_x = with_pixels(g.frame_start_x, (uint16_t)(pixels(g.frame_start_x) + dx));
    dy = (uint16_t)(pixels(a->y) - own_word(a, OWN_4A));
    p->y = with_pixels(p->y, (uint16_t)(pixels(p->y) + dy));
    g.frame_start_y = with_pixels(g.frame_start_y, (uint16_t)(pixels(g.frame_start_y) + dy));
}

/*
 * @routine 83:FC96 ActorFacePlayer
 * WHAT IT DOES: Turns the actor to face the player: facing 1 (left) when
 * it is at or right of him, 0 when left of him. (Its flip is left as it
 * was.)
 */
void ActorFacePlayer(Actor *a)
{
    a->facing = pixels(a->x) >= pixels(g.actors[0].x);
}

/*
 * @routine 83:FB89 ActorTestCarries
 * WHAT IT DOES: Tells, by test `test`, whether the player rides the actor
 * this frame (its $43: 1 when he does), keeps its place for carrying
 * him, and plays its animation on. Test 0 is ActorTestSolid (he stands on
 * it as on a solid actor), 2 ActorTestLand (he lands on it); both by the
 * box its own $4C names.
 * Also 83:FB85 ActorTestCarriesLong, for callers in other banks;
 * 83:FBA6 ActorTestByX, the test by `test` (the table at $83:FBA9);
 * 83:FBAD ActorTestSolid, ActorSolidForPlayer; 83:FBB1 ActorTestLand,
 * PlayerLandOnActor.
 */
void ActorTestCarries(Actor *a, uint8_t test)
{
    uint8_t box = a->own.bytes[OWN_4C];
    bool rides = test == 0 ? ActorSolidForPlayer(a, box) == ACTOR_SOLID_ON_TOP : PlayerLandOnActor(a, box) != 0;

    a->own.bytes[OWN_43] = rides;
    own_word_set(a, OWN_48, pixels(a->x));
    own_word_set(a, OWN_4A, pixels(a->y));
    AnimStep(a);
}

/*
 * @routine 83:C1EC ActorStepCarry
 * WHAT IT DOES: Moves the actor a step along its facing (vector set $0E)
 * and carries the player riding it along.
 */
void ActorStepCarry(Actor *a)
{
    ActorMoveByVector(a, 0x0E);
    ActorCarryPlayer(a);
}

/*
 * @routine 83:C1F6 ActorStepToTargetCarry
 * WHAT IT DOES: Moves the actor a step toward its target (the pixel words
 * of its speed_x and speed_y): across and up or down by vector set 8 on each
 * axis it has not reached; then carries the player riding it along.
 */
void ActorStepToTargetCarry(Actor *a)
{
    if (pixels(a->x) != pixels((uint32_t)a->speed_x))
        ActorMoveByVectorX(a, 0x08);
    if (pixels(a->y) != pixels((uint32_t)a->speed_y))
        ActorMoveByVectorY(a, 0x08);
    ActorCarryPlayer(a);
}

/*
 * @routine 83:FB57 ActorLiftPlayerStep
 * WHAT IT DOES: A step of an actor that lifts the player: at its +$44 = 0,
 * when ActorGrabTest finds him he goes to hang from it (his code to
 * PLAYER_AT_GRABBED_HANG, his place kept above it by `offset`) and +$44
 * counts on; negative, +$44 counts on; positive, he is kept on it while
 * g.player_no_actor_landing is set, else +$44 becomes $F0.
 */
void ActorLiftPlayerStep(Actor *a, uint8_t offset)
{
    int8_t *step = (int8_t *)&a->own.bytes[OWN_44];

    if (*step == 0) {
        if (!ActorGrabTest(a))
            return;
        g.actors[0].resume = PLAYER_AT_GRABBED_HANG;
        PlayerSpotAboveActor(a, offset);
        (*step)++;
    } else if (*step < 0) {
        (*step)++;
    } else if ((uint8_t)g.player_no_actor_landing) {
        PlayerToActorAbove(a, offset);
    } else {
        *step = (int8_t)0xF0;
    }
}
