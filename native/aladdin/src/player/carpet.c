/*
 * carpet.c: Aladdin on the magic carpet (the record's player/carpet_ride.c
 * and carpet.c): the ride's start (it flies in from the left), its frames
 * (the pad's eight directions push its speeds, it tilts as it climbs or
 * dives, and with no direction it drifts back to cruising and levels off),
 * a wall that knocks him off, and the crash that loses a life. Two rides:
 * kind 1's and the other's (the "Alt" routines, with their own rates and
 * limits).
 *
 * The steering direction is his record's +$14 (his facing's byte): 0
 * right, clockwise to 7 (2 down, 4 left, 6 up); 8 slow down across, 9
 * speed up, 10 neither; negative none.
 */
#include "player/player_own.h"
#include "system/task.h"

#define DIVING(a) ((a)->own.bytes[OWN_41])     /* +$41: nonzero, tilted to dive (move 3) */
#define CLIMBING(a) ((a)->own.bytes[OWN_42])   /* +$42: nonzero, tilted to climb (move 1) */
#define TILTED(a) ((a)->own.bytes[OWN_43])     /* +$43: CarpetTilt's answer */
#define SPED_UP(a) ((a)->own.bytes[OWN_44])    /* +$44: nonzero, it has sped up to cruise (direction 9) */
#define SLOWED(a) ((a)->own.bytes[OWN_45])     /* +$45: nonzero, it has slowed to cruise (direction 8) */

#define DIR_SLOWER 0x08
#define DIR_FASTER 0x09
#define DIR_NEITHER 0x0A
#define LEVEL_TALK 0x07         /* the levels where it flies in and the ride starts at once */
#define LEVEL_TALK_2 0x17

static bool first_ride(const Actor *a)
{
    return a->kind == 0x01;
}

/* a 24-bit speed up by a rate, or down by its low 7 bits when bit 7 is set */
static int32_t speed_step(int32_t speed, uint8_t rate)
{
    if (rate & 0x80)
        return signed24((uint32_t)speed - (rate & 0x7F));
    return signed24((uint32_t)speed + rate);
}

/*
 * @routine 81:E511 CarpetCruiseSetAlt
 * WHAT IT DOES: The speed across at the other ride's cruise, 1.5 pixels a
 * frame.
 */
static void cruise_set_alt(Actor *a)
{
    a->speed_x = 0x0180;
}

/*
 * @routine 81:E503 CarpetCruiseSet
 * WHAT IT DOES: The speed across at cruise: 3 pixels a frame on kind 1's
 * ride, 1.5 on the other.
 */
static void cruise_set(Actor *a)
{
    a->speed_x = first_ride(a) ? 0x0300 : 0x0180;
}

/*
 * @routine 81:E555 CarpetLevelOut
 * WHAT IT DOES: Levels the carpet off when it is tilted: out of a dive
 * (move 4) or a climb (move 2). Returns 0 when it was level, else $FF
 * (what the original's move start leaves).
 */
static uint8_t level_out(Actor *a)
{
    if (DIVING(a)) {
        DIVING(a) = 0;
        PlayerMoveStart(a, 0x04);
        return 0xFF;
    }
    if (CLIMBING(a)) {
        CLIMBING(a) = 0;
        PlayerMoveStart(a, 0x02);
        return 0xFF;
    }
    return 0;
}

/*
 * @routine 81:E56D CarpetTilt
 * WHAT IT DOES: Tilts the carpet the way it is steered: diving (move 3)
 * steered down, climbing (move 1) steered up; level steering levels it
 * off. Returns 0 only when it was level and stays level.
 */
static uint8_t tilt(Actor *a)
{
    uint8_t t = carpet_tilt(a->facing);

    if (!t)
        return level_out(a);
    if (!(t & 0x80)) {
        if (!DIVING(a)) {
            DIVING(a)++;
            CLIMBING(a) = 0;
            PlayerMoveStart(a, 0x03);
            return 0xFF;
        }
    } else if (!CLIMBING(a)) {
        CLIMBING(a)++;
        DIVING(a) = 0;
        PlayerMoveStart(a, 0x01);
    }
    return 0x01;
}

/*
 * @routine 81:E5A4 CarpetSteerX
 * WHAT IT DOES: The speed across pushed by the direction's rate (none: not).
 * Also 81:E597 CarpetSteerXAlt, the other ride's rates.
 */
static void steer_x(Actor *a)
{
    if (a->facing & 0x80)
        return;
    a->speed_x = speed_step(a->speed_x, carpet_rate_x(!first_ride(a), a->facing));
}

/*
 * @routine 81:E664 CarpetSteadyY
 * WHAT IT DOES: Steadies it up and down: steered against its vertical
 * speed (2 down while it rises faster than 8/256, 6 up while it falls at
 * 7/256 or more); between, stopped, and no direction.
 */
static void steady_y(Actor *a)
{
    uint16_t w = speed_word(a->speed_y);

    if (w & 0x8000) {
        if (w < 0xFFF8) {
            a->facing = 0x02;
            return;
        }
    } else if (w >= 0x0007) {
        a->facing = 0x06;
        return;
    }
    a->speed_y = 0;
    a->facing = 0xFF;
}

/*
 * @routine 81:E5F0 CarpetSteerY
 * WHAT IT DOES: The vertical speed pushed by the direction's rate (none:
 * not); a rate of 0 steadies it first, then the same rate. Also 81:E5DB
 * CarpetSteerYAlt, the other ride's rates.
 */
static void steer_y(Actor *a)
{
    uint8_t dir = a->facing, rate;

    if (dir & 0x80)
        return;
    rate = carpet_rate_y(!first_ride(a), dir);
    if (!rate)
        steady_y(a);
    a->speed_y = speed_step(a->speed_y, rate);
}

/*
 * @routine 81:E643 CarpetCruiseX
 * WHAT IT DOES: With no direction held, steers toward the cruising speed
 * across: below it (3 pixels a frame) faster (9, marked sped up), else
 * slower (8, marked slowed). Also 81:E62F CarpetCruiseXAlt: the other
 * ride's, at $0181, and once it has done both, neither (10).
 */
static void cruise_x(Actor *a)
{
    uint16_t top = 0x0300;

    if (!first_ride(a)) {
        if (SPED_UP(a) && SLOWED(a)) {
            a->facing = DIR_NEITHER;
            return;
        }
        top = 0x0181;
    }
    if (speed_word(a->speed_x) >= top) {
        a->facing = DIR_SLOWER;
        SLOWED(a) = DIR_SLOWER;
    } else {
        a->facing = DIR_FASTER;
        SPED_UP(a) = DIR_FASTER;
    }
}

/*
 * @routine 81:E6AE CarpetSpeedXLimit
 * WHAT IT DOES: The speed across kept between $0020 and $0480 ($0380 on
 * the other ride, 81:E68F CarpetSpeedXLimitAlt).
 */
static void speed_x_limit(Actor *a)
{
    uint16_t top = first_ride(a) ? 0x0480 : 0x0380, w = speed_word(a->speed_x);

    if (w >= top)
        a->speed_x = with_speed_word(a->speed_x, top);
    else if (w < 0x0020)
        a->speed_x = with_speed_word(a->speed_x, 0x0020);
}

/*
 * @routine 81:E6CD CarpetSpeedYLimit
 * WHAT IT DOES: The vertical speed kept from -$01C0 up to $01E0 down on
 * kind 1's ride, -$0240 to $0280 on the other.
 */
static void speed_y_limit(Actor *a)
{
    uint16_t top = first_ride(a) ? 0x01E0 : 0x0280, least = first_ride(a) ? 0xFE40 : 0xFDC0;
    uint16_t w = speed_word(a->speed_y);

    if (w & 0x8000) {
        if (w < least)
            a->speed_y = with_speed_word(a->speed_y, least);
    } else if (w >= top) {
        a->speed_y = with_speed_word(a->speed_y, top);
    }
}

/* a frame's end: steadied and steered up and down, the direction 0, moved */
static void frame_end(Actor *a)
{
    steady_y(a);
    steer_y(a);
    a->facing = 0;
    ActorMove(a);
}

/*
 * @routine 81:E3F5 PlayerCarpetStart
 * WHAT IT DOES: The ride's start (with +$07 2 the ending's ride instead):
 * the rider's look and box, the text blink's task on level 7; it flies in
 * from the left (127 frames on kind 1's ride, $35 on the other) and, but on
 * levels 7 and $17, drifts at cruise until the view holds still and starts
 * the text blink's task; then the ride.
 */
static int carpet_start(Actor *a)
{
    if (a->spawn_byte == 0x02)
        return PlayerGoTo(a, PLAYER_AT_ENDING_RIDE);
    a->flags = 0xD0;
    a->own.bytes[OWN_0A] = 0x40;
    a->own.bytes[OWN_29] = 0x17;        /* his picture list's word, $8917 */
    a->own.bytes[OWN_2A] = 0x89;
    CLIMBING(a) = 0;
    TILTED(a) = 0;
    a->redraw = 0xFF;
    g.player_box_wanted = 0x11;
    PlayerBoxSet(a, 0x11);
    if (g.level == LEVEL_TALK)
        TaskCreate(2, 0, TASK_ENTRY_TEXT_BLINK);
    g.player_flags &= 0x7F;
    g.sub_layers |= 0x10;
    SPED_UP(a) = 0;
    SLOWED(a) = 0;
    a->facing = 0;
    a->flip = 0;
    ActorStopY(a);
    if (first_ride(a)) {
        a->speed_x = 0x03E0;
        WAIT_FRAMES(a) = 0x7F;
    } else {
        a->speed_x = 0x0300;
        WAIT_FRAMES(a) = 0x35;
    }
    PlayerMoveStart(a, 0x00);
    a->resume = P_CARPET_FLY_IN;
    return 1;
}

static int fly_in(Actor *a)
{
    ActorMoveX(a);
    if (--WAIT_FRAMES(a))
        return 1;
    a->speed_x = (int32_t)((uint32_t)a->speed_x & ~0xFFu);
    if (g.level != LEVEL_TALK && g.level != LEVEL_TALK_2) {
        cruise_set_alt(a);
        a->resume = P_CARPET_DRIFT;
        return 1;
    }
    a->resume = P_CARPET;
    return 1;
}

static int drift(Actor *a)
{
    ActorMoveX(a);
    if ((uint8_t)g.scroll_y_held)
        return 1;
    TaskCreate(2, 0, TASK_ENTRY_TEXT_BLINK);
    a->resume = P_CARPET;
    return 1;
}

/*
 * @routine 81:E713 PlayerCarpetWallHurt
 * WHAT IT DOES: The carpet out of a wall in front; when the tile last
 * probed hurts (and he is not hit already nor blinking), he is hit and
 * the carpet crashes. True then.
 */
static bool wall_hurt(Actor *a)
{
    ActorOutOfWalls(a, &player_point);
    if ((a->anim_flags & 0x90) || !(player_point.behaviour & TILE_HURTS))
        return false;
    PlayerEnter8C();
    return true;
}

/*
 * @routine 81:E51C PlayerCarpetSteer
 * WHAT IT DOES: A frame steered: the carpet tilts, its speed across follows
 * the direction within its limits, and when it tilted its speed up and
 * down too; then the next frame.
 */
static int steer(Actor *a)
{
    SPED_UP(a) = 0;
    SLOWED(a) = 0;
    TILTED(a) = tilt(a);
    steer_x(a);
    speed_x_limit(a);
    if (!TILTED(a)) {
        frame_end(a);
    } else {
        steer_y(a);
        speed_y_limit(a);
        a->facing = 0;
        ActorMove(a);
    }
    a->resume = P_CARPET;
    return 1;
}

/*
 * @routine 81:E49C PlayerCarpet
 * WHAT IT DOES: A frame of the ride: the rider's animation, a wall that
 * hurts (kind 1's ride, x under $1E00), and the pad: a direction steers;
 * none levels the carpet off and drifts it back to cruising.
 */
static int carpet_frame(Actor *a)
{
    uint8_t dir;

    AnimStep(a);
    if (first_ride(a) && (x_pixels(a) >> 8) < 0x1E && wall_hurt(a))
        return PlayerGoTo(a, P_CARPET_CRASH);
    dir = carpet_direction(pad_dirs() & 0x0F);
    if (!(dir & 0x80)) {
        a->facing = dir;
        return steer(a);
    }
    level_out(a);
    cruise_x(a);
    if (a->facing == DIR_NEITHER)
        cruise_set(a);
    else
        steer_x(a);
    frame_end(a);
    return 1;
}

/*
 * @routine 81:E7E9 PlayerCarpetCrash
 * WHAT IT DOES: The carpet crashes (kind 1's second entry): the view holds
 * still, he loses his health with the song and the hurt sound and falls
 * away (move 5) until he is out of the map; once the screen has faded the
 * level starts again, or the continue screen with no lives left.
 */
static int crash(Actor *a)
{
    g.scroll_kind = 0;
    g.view_edge_rule = 0x04;
    g.scroll_x_held = (uint16_t)((g.scroll_x_held & 0xFF00) | 0x01);
    g.scroll_y_held = (uint16_t)((g.scroll_y_held & 0xFF00) | 0x01);
    a->facing = 0;
    a->flip = 0;
    g.health = 0;
    g.play_starts++;
    SongPlay(0x2A);
    SoundQueueAdd(0x02);
    ActorMotionPreset(a, 0x52);
    PlayerMoveStart(a, 0x05);
    a->resume = P_CARPET_CRASH_FALL;
    return 1;
}

static int crash_fall(Actor *a)
{
    ActorFly(a);
    if (PlayerFallenTest())
        return 1;
    TaskCreate(1, 0x04, TASK_ENTRY_BRIGHTNESS_DOWN);
    a->resume = P_CARPET_CRASH_WAIT;
    return 1;
}

static int crash_wait(Actor *a)
{
    (void)a;
    if (task_busy(1))
        return 1;
    g.lives--;
    TaskExecRunning(g.lives ? TASK_ENTRY_LEVEL_PLAY_START : TASK_ENTRY_CONTINUE_SCREEN);
    return 1;
}

int player_carpet_code(Actor *a)
{
    switch (a->resume) {
    case P_CARPET_START:        return carpet_start(a);
    case P_CARPET_FLY_IN:       return fly_in(a);
    case P_CARPET_DRIFT:        return drift(a);
    case P_CARPET:              return carpet_frame(a);
    case P_CARPET_CRASH:        return crash(a);
    case P_CARPET_CRASH_FALL:   return crash_fall(a);
    case P_CARPET_CRASH_WAIT:   return crash_wait(a);
    default:                    return player_unconverted(a, "a resume point of carpet.c");
    }
}
