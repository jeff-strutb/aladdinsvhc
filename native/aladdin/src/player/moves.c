/*
 * moves.c: how Aladdin moves: the pad's facing, his moves and their
 * animations, his speed across on the ground and in the air, his run
 * settings, the ground that pushes or slides him, and what his poses
 * start (the record's player/facing.c, move_anim.c, move_change.c,
 * movement.c, run_speed.c, air_steer.c, ground_push.c, ground_slide.c,
 * spawn_every8.c, apple_throw.c, jump_pose.c).
 */
#include <stdio.h>

#include "player/player_own.h"

#define FALL_SPEED_MAX 0x0580   /* the fastest an actor falls: 5.5 pixels a frame */
#define PUSH_TOP_SPEED 0x0280   /* the fastest pushing ground carries him */

/*
 * @routine 81:CE7C PadFacing
 * WHAT IT DOES: The way pad 1's Left and Right point: 0 right, 1 left, or
 * $FF when neither or both are held.
 */
uint8_t PadFacing(void)
{
    return pad_facing(pad_dirs() & 0x03);
}

/*
 * @routine 81:CE70 PlayerFlipByPad
 * WHAT IT DOES: Turns his sprite the way the pad points, when it points one
 * way. Returns false when it points neither way.
 */
bool PlayerFlipByPad(Actor *a)
{
    uint8_t f = PadFacing();

    if (f & 0x80)
        return false;
    a->flip = f;
    return true;
}

/*
 * @routine 81:DCB1 AnimOfMove
 * WHAT IT DOES: The animation an actor of its kind plays for the move.
 */
uint8_t AnimOfMove(const Actor *a, uint8_t move)
{
    return anim_of_move(a->kind, move);
}

/*
 * @routine 81:DCA2 PlayerMoveStart
 * WHAT IT DOES: Puts him into the move, its animation from its first frame.
 * Also 81:DC98 PlayerMoveStartAlso, a second entry to the same code.
 */
void PlayerMoveStart(Actor *a, uint8_t move)
{
    g.player_move = move;
    AnimStart(a, AnimOfMove(a, move));
}

/*
 * @routine 81:DC9D PlayerMoveChange
 * WHAT IT DOES: Puts him into the move, unless he is in it already.
 */
void PlayerMoveChange(Actor *a, uint8_t move)
{
    if (move != g.player_move)
        PlayerMoveStart(a, move);
}

/*
 * @routine 81:DC7D PlayerMoveKeepFrame
 * WHAT IT DOES: Puts him into the move, unless he is in it already,
 * keeping his animation's frame: the new move's animation goes on at the
 * frame the old one had reached.
 */
void PlayerMoveKeepFrame(Actor *a, uint8_t move)
{
    uint8_t frame;

    if (move == g.player_move)
        return;
    g.player_move = move;
    frame = a->frame;
    AnimResume(a, AnimOfMove(a, move));
    a->frame = frame;
}

/*
 * @routine 81:CF1C PlayerRunMove
 * WHAT IT DOES: His running move by his speed: move 2 below 2 pixels a
 * frame, move 3 at or above, keeping the animation's frame.
 */
void PlayerRunMove(Actor *a)
{
    PlayerMoveKeepFrame(a, speed_word(a->speed_x) >= 0x0200 ? 0x03 : 0x02);
}

/*
 * @routine 81:D1A0 PlayerRunSpeed
 * WHAT IT DOES: Steps his running speed by the pad: held his way, up by the
 * run rate to the top speed; no way held, down by the slowing rate to 0
 * (unless held up, g.player_slowed); held the other way, down by the
 * turning rate, and past 0 he turns round.
 */
void PlayerRunSpeed(Actor *a)
{
    uint8_t f = PadFacing();
    uint16_t w = speed_word(a->speed_x);

    if (f & 0x80) {
        if ((uint8_t)g.player_slowed || !w)
            return;
        w = (uint16_t)(w + (uint16_t)g.player_run_decel);
        a->speed_x = with_speed_word(a->speed_x, (w & 0x8000) ? 0 : w);
        return;
    }
    if (f == a->facing) {
        w = (uint16_t)(w + (uint16_t)g.player_run_accel);
        if (w >= g.player_top_speed)
            w = g.player_top_speed;
        a->speed_x = with_speed_word(a->speed_x, w);
        return;
    }
    w = (uint16_t)(w + (uint16_t)g.player_turn_decel);
    if (w & 0x8000) {
        w = (uint16_t)-w;
        a->facing ^= 0x01;
    }
    a->speed_x = with_speed_word(a->speed_x, w);
}

/*
 * @routine 81:CF2E PlayerAirSteer
 * WHAT IT DOES: Steers him across by the ground's direction bits: the
 * actor's he stands on (g.player_actor_ground, 0 read as 2) or the tile's
 * his last ground test found. Bits 0-2 at 0, 2 or 3: his speed settles to
 * the dash row's speed. Else bit 7 against his facing picks: the same way,
 * faster by the row's rate to its top; the other way, slower by its rate to
 * its least. The rows are by the low two bits and g.player_dash.
 */
void PlayerAirSteer(Actor *a)
{
    uint8_t dir, low, way, dash = g.player_dash;
    uint16_t w, limit;

    if (g.player_on_actor)
        dir = g.player_actor_ground ? g.player_actor_ground : 0x02;
    else
        dir = player_point.behaviour;
    low = dir & 0x07;
    if (low == 0 || low == 2 || low == 3) {
        limit = air_settle(dash);
        w = speed_word(a->speed_x);
        if (limit > w) {
            a->speed_x = with_speed_word(a->speed_x, limit);
            a->accel_x = 0x28;
            ActorAccelerateX(a);
        } else if (limit < w) {
            a->accel_x = 0x08;
            ActorDecelerateX(a);
            if (0x0180 >= speed_word(a->speed_x))
                a->speed_x = with_speed_word(a->speed_x, 0x0180);
        }
        return;
    }
    way = dir & 0x03;
    if ((dir >> 7) ^ a->facing) {
        a->accel_x = air_accel_toward(way, dash);
        ActorAccelerateX(a);
        limit = air_top_toward(way, dash);
        if (limit < speed_word(a->speed_x))
            a->speed_x = with_speed_word(a->speed_x, limit);
    } else {
        a->accel_x = air_accel_against(way);
        ActorDecelerateX(a);
        limit = air_least_against(way, dash);
        if (limit >= speed_word(a->speed_x))
            a->speed_x = with_speed_word(a->speed_x, limit);
    }
}

/*
 * @routine 81:D855 PlayerSpeedLeast
 * WHAT IT DOES: Keeps him moving across at three quarters of a pixel a
 * frame or more, counting the frames he had to be held up.
 */
void PlayerSpeedLeast(Actor *a)
{
    if (speed_word(a->speed_x) < 0x00C0) {
        g.player_slowed++;
        a->speed_x = with_speed_word(a->speed_x, 0x00C0);
    }
}

/*
 * @routine 81:DACE PlayerPosRestore
 * WHAT IT DOES: Puts him back at the place kept for him.
 */
void PlayerPosRestore(Actor *a)
{
    set_x_pixels(a, g.player_saved_x);
    set_y_pixels(a, g.player_saved_y);
}

/*
 * @routine 81:DDB4 PlayerFlagsClear
 * WHAT IT DOES: The camera's window and three of his marks back to 0.
 * ($144D, the fourth byte it clears, nothing reads.)
 */
void PlayerFlagsClear(void)
{
    g.camera_window = 0;
    g.player_in_air = 0;
    g.player_anim_held = 0;
}

/*
 * @routine 81:D41C ActorStepX
 * WHAT IT DOES: Moves the actor across by its speed, the way it faces.
 */
void ActorStepX(Actor *a)
{
    if (a->facing == 0)
        a->x = wrap24(a->x + (uint32_t)a->speed_x);
    else
        a->x = wrap24(a->x - (uint32_t)a->speed_x);
}

/* the fall speed held to 5.5 pixels a frame */
static void fall_cap(Actor *a)
{
    if (speed_word(a->speed_y) > FALL_SPEED_MAX)
        a->speed_y = with_speed_word(a->speed_y, FALL_SPEED_MAX);
}

/*
 * @routine 81:D448 ActorFallCap
 * WHAT IT DOES: An actor falling falls no faster than 5.5 pixels a frame.
 */
void ActorFallCap(Actor *a)
{
    if (a->speed_y >= 0)
        fall_cap(a);
}

/*
 * @routine 81:D44D ActorGravity
 * WHAT IT DOES: Gravity: its speed down grows by its gravity, no faster
 * than 5.5 pixels a frame once it falls.
 */
void ActorGravity(Actor *a)
{
    a->speed_y = signed24((uint32_t)a->speed_y + a->accel_y);
    if (a->speed_y >= 0)
        fall_cap(a);
}

/*
 * @routine 81:D164 PlayerGroundYSave
 * WHAT IT DOES: Notes the height he stands at, from which his falls are
 * measured.
 */
void PlayerGroundYSave(Actor *a)
{
    g.player_ground_y = y_pixels(a);
}

/*
 * @routine 81:D16E PlayerTopSpeedHold
 * WHAT IT DOES: His present speed becomes the fastest he may run.
 */
void PlayerTopSpeedHold(Actor *a)
{
    g.player_top_speed = speed_word(a->speed_x);
}

/*
 * @routine 81:D179 PlayerTopSpeedCap
 * WHAT IT DOES: His top running speed at least 1.5 pixels a frame: one at
 * or under it becomes it (the record's comment says "at most"; its code, CMP
 * then BCC, stores $0180 when it is not above).
 */
void PlayerTopSpeedCap(void)
{
    if (g.player_top_speed <= 0x0180)
        g.player_top_speed = 0x0180;
}

/*
 * @routine 81:D189 PlayerRunRatesSet
 * WHAT IT DOES: How fast he gains and loses speed running: $20 up, 8 down
 * with no way held, $40 down held the other way.
 */
void PlayerRunRatesSet(void)
{
    g.player_run_accel = 0x0020;
    g.player_run_decel = -0x0008;
    g.player_turn_decel = -0x0040;
}

/*
 * @routine 81:CE86 PlayerGroundPush
 * WHAT IT DOES: On ground that pushes (behaviour bit 4 behind him at probe
 * $18): marked pushed (2), and his speed $80 faster (to $0280) when the
 * push is his way, $80 slower (not below 0) when it is not; the tile's bit
 * 7 and his facing say which.
 */
void PlayerGroundPush(Actor *a)
{
    int found;
    bool faster;
    uint16_t w = speed_word(a->speed_x);

    g.player_ground_pushed = 0;
    a->probe_at = 0x18;
    found = GroundBehindBit4(a, &player_point);
    if (!found)
        return;
    g.player_ground_pushed = 0x02;
    faster = found == 2 ? a->facing == 0 : a->facing != 0;
    if (faster) {
        w = (uint16_t)(w + 0x0080);
        a->speed_x = with_speed_word(a->speed_x, w >= PUSH_TOP_SPEED ? PUSH_TOP_SPEED : w);
    } else if (w >= 0x0080) {
        a->speed_x = with_speed_word(a->speed_x, (uint16_t)(w - 0x0080));
    }
}

/*
 * @routine 81:CEF4 PlayerGroundSlide
 * WHAT IT DOES: On ground that slides him (behaviour bit 4 behind him at
 * probe $18): turned the way the tile's bit 7 says, marked sliding (1), and
 * moved along at half a pixel a frame.
 */
void PlayerGroundSlide(Actor *a)
{
    int found;

    g.player_ground_pushed = 0;
    a->probe_at = 0x18;
    found = GroundBehindBit4(a, &player_point);
    if (!found)
        return;
    a->facing = found == 2 ? 0 : 1;
    g.player_ground_pushed = 0x01;
    a->speed_x = 0x80;
    ActorStepX(a);
}

/*
 * @routine 81:DD90 PlayerSpawnEvery8
 * WHAT IT DOES: Every eighth frame (g.sprite_flicker's low three bits 0) a
 * kind $34 starts at his spawn point 2, facing his way, its +$07 1. His
 * flip is his facing while it is placed; when no record is free it stays
 * so, as the original's.
 */
void PlayerSpawnEvery8(Actor *a)
{
    uint8_t flip = a->flip;

    if (g.sprite_flicker & 0x07)
        return;
    a->spawn_at = 0x02;
    a->flip = a->facing;
    if (ActorSpawnFacing(a, 0x34, a->facing, 0x01))
        a->flip = flip;
}

/*
 * @routine 81:CDE9 PlayerAppleThrow
 * WHAT IT DOES: Throws an apple when the throw is pressed, apples are left
 * and the level is not $25: a shot (kind $10) from the second pool, at his
 * hand for his pose on the course the pose gives, facing his way; one apple
 * fewer (two decimal digits), sound 3, and his motion across stopped unless
 * he is in the air. True when he threw.
 */
bool PlayerAppleThrow(Actor *a)
{
    Actor *apple;
    uint8_t pose;

    if (!throw_pressed() || !g.apples || g.level == 0x25)
        return false;
    apple = ActorAllocB();
    if (!apple)
        return false;
    apple->state = ACTOR_STATE_NEW;
    apple->facing = a->flip;
    apple->flip = a->flip;
    pose = g.player_box_wanted & 0x03;
    if (a->flip)
        set_x_pixels(apple, (uint16_t)(x_pixels(a) + throw_x(pose)));
    else
        set_x_pixels(apple, (uint16_t)(x_pixels(a) - throw_x(pose)));
    set_y_pixels(apple, (uint16_t)(y_pixels(a) - throw_y(pose)));
    apple->kind = 0x10;
    apple->spawn_byte = throw_course(pose);
    g.apples = (g.apples & 0x0F) ? (uint8_t)(g.apples - 1) : (uint8_t)(g.apples - 7);
    SoundQueueAdd(0x03);
    if (!g.player_in_air)
        ActorStopX(a);
    return true;
}

/*
 * @routine 81:D1F5 PlayerJumpPoseStep
 * WHAT IT DOES: Steps the pose of his jump by its step (POSE_FRAMES): 0
 * move 4 for 14 frames; 1 then move $0D, the height reached kept; 2 move
 * $0E once he is lower than that; 3 holds. 4 and 5 sound 2, move 9 for 16
 * frames. 6 move $11 and back to step 1 for $12 frames; 7 move $0D after
 * them; 8 move $0E once he falls, on at step 3.
 */
void PlayerJumpPoseStep(Actor *a)
{
    uint16_t top;

    switch (POSE_FRAMES(a)) {
    case 0:
        PlayerMoveStart(a, 0x04);
        POSE_FRAMES(a)++;
        POSE_TIMER(a) = 0x0E;
        return;
    case 1:
        if (--POSE_TIMER(a))
            return;
        PlayerMoveStart(a, 0x0D);
        TOP_Y_LOW(a) = (uint8_t)y_pixels(a);
        TOP_Y_HIGH(a) = (uint8_t)(y_pixels(a) >> 8);
        POSE_FRAMES(a)++;
        return;
    case 2:
        top = top_y(a);
        if (!(top & 0x8000) && top >= y_pixels(a))
            return;
        PlayerMoveStart(a, 0x0E);
        POSE_FRAMES(a)++;
        return;
    case 3:
        return;
    case 4:
    case 5:
        SoundQueueAdd(0x02);
        PlayerMoveStart(a, 0x09);
        POSE_TIMER(a) = 0x10;
        POSE_FRAMES(a)++;
        return;
    case 6:
        PlayerMoveStart(a, 0x11);
        POSE_FRAMES(a) = 0x01;
        POSE_TIMER(a) = 0x12;
        return;
    case 7:
        if (--POSE_TIMER(a))
            return;
        PlayerMoveStart(a, 0x0D);
        POSE_FRAMES(a)++;
        return;
    case 8:
        if (a->speed_y < 0)
            return;
        PlayerMoveStart(a, 0x0E);
        POSE_FRAMES(a) = 0x03;
        return;
    default: {
        /* past the table's nine steps the original jumps where the bytes
           after it point: no play reaches it */
        static int told;

        if (!told++)
            fprintf(stderr, "PlayerJumpPoseStep: step %u past its table not converted\n", POSE_FRAMES(a));
        return;
    }
    }
}
