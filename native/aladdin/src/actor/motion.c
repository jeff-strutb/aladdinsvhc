/*
 * motion.c: actors' speeds and moves. Positions are 16.8 and wrap at 24
 * bits; speeds are 16.8, signed from 24 bits; accelerations and gravity are
 * a fraction byte, added as a whole positive byte.
 */
#include "actor/actor_own.h"
#include "data/actor_tables.h"

/*
 * @routine 81:A3DD ActorMotionPreset
 * WHAT IT DOES: Gives the actor the speeds and gravity of motion preset
 * `preset`: a jump or a throw's arc.
 * Also 81:A3EC ActorMotionPresetY, its vertical half.
 */
void ActorMotionPreset(Actor *a, uint8_t preset)
{
    MotionPreset m = motion_preset(preset);

    a->speed_x = signed24(m.speed_x);
    a->speed_y = signed24(m.speed_y);
    a->accel_y = m.gravity;
}

/*
 * @routine 81:A413 ActorSpeedXSet
 * WHAT IT DOES: Sets the actor's horizontal speed from the speed table's
 * entry at byte offset `at`.
 * (81:A411 ActorSpeedXSetAccel sets its acceleration first: the caller
 * does so.)
 */
void ActorSpeedXSet(Actor *a, uint8_t at)
{
    a->speed_x = signed24(speed_x_entry(at));
}

/*
 * @routine 81:A425 ActorSpeedYSet
 * WHAT IT DOES: Sets the actor's vertical speed from the speed table.
 */
void ActorSpeedYSet(Actor *a, uint8_t at)
{
    a->speed_y = signed24(speed_y_entry(at));
}

/*
 * @routine 84:9ED0 ActorSpeedXFrom84
 * WHAT IT DOES: Sets the actor's horizontal speed from the story figures'
 * speed table, entry `at`.
 */
void ActorSpeedXFrom84(Actor *a, uint8_t at)
{
    a->speed_x = signed24(speed_triple(at));
}

/*
 * @routine 84:9EE0 ActorSpeedYFrom84
 * WHAT IT DOES: Sets the actor's vertical speed from the same table.
 */
void ActorSpeedYFrom84(Actor *a, uint8_t at)
{
    a->speed_y = signed24(speed_triple(at));
}

/*
 * @routine 81:A5FF ActorAccelerateX
 * WHAT IT DOES: Speeds the actor up horizontally by its acceleration.
 */
void ActorAccelerateX(Actor *a)
{
    a->speed_x = signed24((uint32_t)a->speed_x + a->accel_x);
}

/*
 * @routine 81:A560 ActorMoveY
 * WHAT IT DOES: Moves the actor by its vertical speed.
 */
void ActorMoveY(Actor *a)
{
    a->y = wrap24(a->y + (uint32_t)a->speed_y);
}

/*
 * @routine 81:A5A7 ActorMoveX
 * WHAT IT DOES: Moves the actor by its horizontal speed the way it faces:
 * facing right (0) the speed is added to its x, else taken away.
 * Also 81:A5AF ActorMoveXNear and 81:A5B3 ActorMoveRightNear.
 */
void ActorMoveX(Actor *a)
{
    if (a->facing == 0)
        a->x = wrap24(a->x + (uint32_t)a->speed_x);
    else
        a->x = wrap24(a->x - (uint32_t)a->speed_x);
}

/*
 * @routine 81:A55D ActorMove
 * WHAT IT DOES: Moves the actor by its speeds: horizontally the way it
 * faces, and vertically.
 */
void ActorMove(Actor *a)
{
    ActorMoveX(a);
    ActorMoveY(a);
}

/*
 * @routine 81:A4AF ActorSpeedUpMove
 * WHAT IT DOES: Speeds the actor up by its acceleration, then moves it.
 */
void ActorSpeedUpMove(Actor *a)
{
    ActorAccelerateX(a);
    ActorMove(a);
}

/*
 * @routine 81:A54A ActorFly
 * WHAT IT DOES: One frame of flight: gravity added to the vertical speed,
 * then the actor moves by its speeds.
 */
void ActorFly(Actor *a)
{
    a->speed_y = signed24((uint32_t)a->speed_y + a->accel_y);
    ActorMove(a);
}

/*
 * @routine 81:A4F7 ActorStepYBack
 * WHAT IT DOES: Moves the actor against its vertical speed (up when the
 * speed is down).
 */
void ActorStepYBack(Actor *a)
{
    a->y = wrap24(a->y - (uint32_t)a->speed_y);
}

/*
 * @routine 81:A423 ActorSpeedYSetGravity
 * WHAT IT DOES: Sets the actor's gravity and its vertical speed from the
 * speed table's entry at byte offset `at`.
 */
void ActorSpeedYSetGravity(Actor *a, uint8_t gravity, uint8_t at)
{
    a->accel_y = gravity;
    ActorSpeedYSet(a, at);
}

/*
 * @routine 81:A437 ActorSpeedXClear
 * WHAT IT DOES: Stops the actor's horizontal movement.
 */
void ActorSpeedXClear(Actor *a)
{
    a->speed_x = 0;
}

/*
 * @routine 81:A435 ActorStopX
 * WHAT IT DOES: Stops the actor's horizontal movement and its horizontal
 * acceleration.
 */
void ActorStopX(Actor *a)
{
    a->accel_x = 0;
    ActorSpeedXClear(a);
}

/*
 * @routine 81:A440 ActorSpeedYClear
 * WHAT IT DOES: Stops the actor's vertical movement.
 */
void ActorSpeedYClear(Actor *a)
{
    a->speed_y = 0;
}

/*
 * @routine 81:A43E ActorStopY
 * WHAT IT DOES: Stops the actor's vertical movement and takes its gravity
 * away.
 */
void ActorStopY(Actor *a)
{
    a->accel_y = 0;
    ActorSpeedYClear(a);
}

/*
 * @routine 81:A50B ActorFall
 * WHAT IT DOES: One frame of a fall with no horizontal movement: gravity
 * added to the vertical speed, then the actor moves by it.
 */
void ActorFall(Actor *a)
{
    a->speed_y = signed24((uint32_t)a->speed_y + a->accel_y);
    ActorMoveY(a);
}

/*
 * @routine 81:A63B ActorDecelerateX
 * WHAT IT DOES: Slows the actor down horizontally by its acceleration,
 * stopping it rather than turning it round: a speed that goes below 0
 * becomes 0.
 */
void ActorDecelerateX(Actor *a)
{
    a->speed_x = signed24((uint32_t)a->speed_x - a->accel_x);
    if (a->speed_x < 0)
        a->speed_x = 0;
}

/*
 * @routine 81:A3CE ActorMotionReset
 * WHAT IT DOES: Stops the actor: no speed either way, no horizontal
 * acceleration, and `gravity` its gravity.
 */
void ActorMotionReset(Actor *a, uint8_t gravity)
{
    a->speed_x = 0;
    a->accel_x = 0;
    a->speed_y = 0;
    a->accel_y = gravity;
}

/*
 * @routine 81:A4E3 ActorStepY
 * WHAT IT DOES: Moves the actor down by its vertical speed (up when the
 * speed is negative): ActorMoveY's steps, a routine of its own.
 */
void ActorStepY(Actor *a)
{
    ActorMoveY(a);
}

/*
 * @routine 81:A5AB ActorMoveRight
 * WHAT IT DOES: Moves the actor right by its horizontal speed, whichever
 * way it faces.
 */
void ActorMoveRight(Actor *a)
{
    a->x = wrap24(a->x + (uint32_t)a->speed_x);
}

/*
 * @routine 81:A613 ActorAccelerateY
 * WHAT IT DOES: Gravity pulls the actor's vertical speed down.
 */
void ActorAccelerateY(Actor *a)
{
    a->speed_y = signed24((uint32_t)a->speed_y + a->accel_y);
}

/*
 * @routine 81:A627 ActorAccelerateXBack
 * WHAT IT DOES: Takes the actor's acceleration from its horizontal speed,
 * with no stop at 0: a brake that can turn into a move backwards.
 */
void ActorAccelerateXBack(Actor *a)
{
    a->speed_x = signed24((uint32_t)a->speed_x - a->accel_x);
}

/*
 * @routine 81:A657 ActorAccelerateYBack
 * WHAT IT DOES: The actor's gravity pulls its vertical speed up, with no
 * stop at 0.
 */
void ActorAccelerateYBack(Actor *a)
{
    a->speed_y = signed24((uint32_t)a->speed_y - a->accel_y);
}

/*
 * @routine 81:A66B ActorDecelerateY
 * WHAT IT DOES: Takes the actor's gravity from its vertical speed,
 * stopping at 0: a rise that slows to a halt.
 */
void ActorDecelerateY(Actor *a)
{
    ActorAccelerateYBack(a);
    if (a->speed_y < 0)
        a->speed_y = 0;
}

/*
 * @routine 81:A535 ActorFlyAgainst
 * WHAT IT DOES: One frame of flight with the vertical pull reversed: the
 * gravity taken from the vertical speed, and the actor moves by its speeds.
 */
void ActorFlyAgainst(Actor *a)
{
    ActorAccelerateYBack(a);
    ActorMove(a);
}

/*
 * @routine 81:A574 ActorBobY
 * WHAT IT DOES: Moves the actor up and down in a bob: falling, its speed
 * gathers until it is $0081 or more down, then it rises; rising, until it is
 * more than $80 up, then it falls; then it moves by its vertical speed. Its
 * +$49 keeps which way it is going (0 falling).
 */
void ActorBobY(Actor *a)
{
    BobOwn *b = bob(a);

    if (!b->rising) {
        ActorAccelerateY(a);
        if (a->speed_y >= 0 && (uint16_t)a->speed_y >= 0x0081)
            b->rising++;
    } else {
        ActorAccelerateYBack(a);
        if (a->speed_y < 0 && (uint16_t)a->speed_y < 0xFF80)
            b->rising = 0;
    }
    ActorMoveY(a);
}

/* The vector of direction set `set` for the way the actor faces: the set's
   first, plus its facing byte. */
static uint16_t vector_of(const Actor *a, uint8_t set)
{
    return (uint16_t)(vector_set_first(set) + a->facing);
}

/*
 * @routine 81:A854 ActorMoveByVector
 * WHAT IT DOES: Moves the actor by a vector from direction set `set`, the
 * set's left vector when it faces left.
 */
void ActorMoveByVector(Actor *a, uint8_t set)
{
    uint16_t v = vector_of(a, set);

    a->x = wrap24(a->x + vector_x(v));
    a->y = wrap24(a->y + vector_y(v));
}

/*
 * @routine 81:A897 ActorMoveBackByVector
 * WHAT IT DOES: Moves the actor back by a vector from direction set `set`:
 * ActorMoveByVector the other way.
 */
void ActorMoveBackByVector(Actor *a, uint8_t set)
{
    uint16_t v = vector_of(a, set);

    a->x = wrap24(a->x - vector_x(v));
    a->y = wrap24(a->y - vector_y(v));
}

/*
 * @routine 81:A8DA ActorSpeedAddVector
 * WHAT IT DOES: Adds a vector from direction set `set` to the actor's
 * speeds.
 */
void ActorSpeedAddVector(Actor *a, uint8_t set)
{
    uint16_t v = vector_of(a, set);

    a->speed_x = signed24((uint32_t)a->speed_x + vector_x(v));
    a->speed_y = signed24((uint32_t)a->speed_y + vector_y(v));
}

/*
 * @routine 81:A91D ActorSpeedSetVector
 * WHAT IT DOES: Sets the actor's speeds to a vector from direction set
 * `set`.
 */
void ActorSpeedSetVector(Actor *a, uint8_t set)
{
    uint16_t v = vector_of(a, set);

    a->speed_x = signed24(vector_x(v));
    a->speed_y = signed24(vector_y(v));
}

/*
 * @routine 81:A952 ActorMoveByVectorX
 * WHAT IT DOES: Moves the actor across by the x part of a vector from
 * direction set `set`.
 */
void ActorMoveByVectorX(Actor *a, uint8_t set)
{
    a->x = wrap24(a->x + vector_x(vector_of(a, set)));
}

/*
 * @routine 81:A97C ActorMoveByVectorY
 * WHAT IT DOES: Moves the actor up or down by the y part of a vector from
 * direction set `set`.
 */
void ActorMoveByVectorY(Actor *a, uint8_t set)
{
    a->y = wrap24(a->y + vector_y(vector_of(a, set)));
}

/* A vector's part scaled by `times` as the original's byte multiplies keep
   it: the fraction's product's low byte; the pixels' low byte times
   `times` plus the fraction's product's high byte, kept to a byte; the high
   byte as it was. */
static uint32_t scaled(uint32_t part, uint8_t times)
{
    uint16_t low = (uint16_t)((part & 0xFF) * times);
    uint16_t mid = (uint16_t)(((part >> 8) & 0xFF) * times);

    return part >> 16 << 16 | (uint32_t)((uint8_t)(mid + (low >> 8))) << 8 | (low & 0xFF);
}

/*
 * @routine 81:A74C ActorMoveByVectorTimes
 * WHAT IT DOES: Moves the actor by a vector from direction set `set` times
 * `times`, the set's left vector when it faces left, each part scaled as
 * the original's byte multiplies keep it (scaled above).
 */
void ActorMoveByVectorTimes(Actor *a, uint8_t set, uint8_t times)
{
    uint16_t v = vector_of(a, set);

    a->x = wrap24(a->x + scaled(vector_x(v), times));
    a->y = wrap24(a->y + scaled(vector_y(v), times));
}
