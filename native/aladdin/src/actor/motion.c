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
