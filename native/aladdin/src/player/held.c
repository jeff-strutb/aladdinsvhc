/*
 * held.c: Aladdin while something holds him or throws him (the record's
 * player/player_held.c): held by an actor and how he lets go or jumps off,
 * his bounce off an actor's top, sprung or flung up, carried, and taken to
 * hang. Each starts where the actor that set his resume point sends him.
 *
 * PlayerHeld's copier test (it writes $5A to cartridge RAM at $70:1234 and
 * holds him for good when it reads back) is dead on the retail board,
 * whose open bus never answers $5A (plan section 3, "Cartridge RAM").
 */
#include "player/player_own.h"

#define HOLD_KIND 0x12          /* the kind started beside him while he is held */

static int next_frame(Actor *a, uint16_t at)
{
    a->resume = at;
    return 1;
}

/* the air's gravity of his thrown and held states: $38, or $80 rising
   with the jump not held and not landed on an actor */
static void air_gravity(Actor *a)
{
    a->accel_y = 0x38;
    if (!jump_held() && a->speed_y < 0 && !g.player_landed)
        a->accel_y = 0x80;
}

static bool hovers(const Actor *a)
{
    return g.hover_on && hover_held() && a->speed_y >= 0;
}

/*
 * @routine 81:D6F8 PlayerHeld
 * WHAT IT DOES: He is held by an actor (it set his code here): put where
 * he was kept, the hold's sound and pose, a kind $12 started beside him;
 * each frame his box follows his frame, until what holds him lets him go
 * or he lets go (down on the pad) or jumps off (the jump, while his frame
 * is 4 to $0C).
 */
static int held(Actor *a)
{
    Actor *mark;

    a->facing = a->flip;
    PlayerPosRestore(a);
    SoundQueueAdd(0x06);
    g.player_in_air = 0;
    g.player_landed = 0;
    g.player_air_throw = 0;
    g.player_let_go = 0;
    g.player_hurt_count = 0;
    no_actor_landing_set((uint8_t)(no_actor_landing() + 1));
    PlayerFlagsClear();
    PlayerMoveStart(a, 0x10);
    mark = ActorAllocB();
    if (mark) {
        mark->state = ACTOR_STATE_NEW;
        mark->kind = HOLD_KIND;
    }
    return next_frame(a, P_HELD_FRAME);
}

static int held_frame(Actor *a)
{
    int r;

    g.player_box_wanted = held_box(a->frame);
    PlayerBoxUpdate(a);
    if ((r = PlayerOutOfWallsHurt(a)))
        return r;
    if (g.player_let_go || (pad_dirs() & PAD_DOWN))
        return PlayerGoTo(a, P_HELD_LET_GO);
    if (jump_pressed() && a->frame >= 0x04 && a->frame < 0x0D)
        return PlayerGoTo(a, P_HELD_JUMP_OFF);
    return 1;
}

/*
 * @routine 81:D59A PlayerHeldLetGo
 * WHAT IT DOES: He lets go of what held him: dropped from it at his spawn
 * point for his frame, the air's settings, the hold's sound and pose, and
 * on in the air.
 */
static int held_let_go(Actor *a)
{
    g.player_box_wanted = 0x03;
    ActorSpeedXClear(a);
    ActorSpeedYClear(a);
    a->accel_y = 0x38;
    g.player_top_speed = 0x0200;
    a->spawn_at = held_spawn_at(a->frame);
    ActorSpawnPlace(a, a);
    PlayerMoveStart(a, 0x0E);
    SoundQueueAdd(0x01);
    g.player_no_landing = 0x04;
    no_actor_landing_set(0);
    POSE_FRAMES(a) = 0x03;
    return PlayerGoTo(a, P_AIR_TIMED);
}

static int jump_off_frame(Actor *a);

/*
 * @routine 81:D5EC PlayerHeldJumpOff
 * WHAT IT DOES: He jumps off what held him at the speed his frame gives,
 * then flies a frame at a time as in PlayerAir until he lands, catches a
 * ledge or hovers.
 */
static int held_jump_off(Actor *a)
{
    no_actor_landing_set(0);
    ActorMotionPreset(a, held_preset(a->frame));
    PlayerGroundYSave(a);
    g.player_no_landing = 0x10;
    a->spawn_at = held_spawn_at(a->frame);
    g.player_top_speed = 0x0200;
    ActorSpawnPlace(a, a);
    SoundQueueAdd(0x01);
    g.camera_window = 0x0A;
    g.player_box_wanted = 0x03;
    g.player_in_air++;
    g.player_air_frames = 0;
    a->flags &= 0xFE;
    POSE_FRAMES(a) = 0x06;
    return jump_off_frame(a);
}

static int jump_off_frame(Actor *a)
{
    int r;

    if (g.player_on_actor) {
        player_point.behaviour = 0;
        return PlayerGoTo(a, P_LAND);
    }
    if (!(g.player_air_frames & 0x80))
        g.player_air_frames++;
    if (jump_pressed())
        g.player_air_frames = 0;
    if (g.player_air_throw) {
        if (!--g.player_air_throw) {
            POSE_FRAMES(a) = 0x02;
            PlayerJumpPoseStep(a);
        }
    } else if (PlayerAppleThrow(a)) {
        PlayerMoveStart(a, 0x16);
        g.player_air_throw = 0x16;
    } else {
        PlayerJumpPoseStep(a);
    }
    if ((r = PlayerAirStep(a)))
        return r;
    PlayerFlipByPad(a);
    if (hovers(a))
        return PlayerGoTo(a, P_HOVER);
    air_gravity(a);
    PlayerRunSpeed(a);
    if (g.player_no_landing)
        g.player_no_landing--;
    if (a->speed_y < 0)
        return next_frame(a, P_JUMP_OFF_FRAME);
    if (g.level == 0x05) {
        a->probe_at = 0x18;
        if (GroundBehindBit4(a, &player_point)) {
            g.scroll_x_held = (uint16_t)((g.scroll_x_held & 0xFF00) | 0x01);
            g.scroll_y_held = (uint16_t)((g.scroll_y_held & 0xFF00) | 0x01);
        }
    }
    if (!(pad_dirs() & PAD_DOWN) && !g.player_no_landing && GroundBehindBit3(a, &player_point))
        return PlayerGoTo(a, P_LEDGE_GRAB);
    if (g.player_no_landing)
        g.player_no_landing--;
    if (ActorOutOfGround(a))
        return PlayerGoTo(a, P_LAND);
    return next_frame(a, P_JUMP_OFF_FRAME);
}

/*
 * @routine 81:D77C PlayerBounce
 * WHAT IT DOES: He lands on an actor's top and bounces off it: a short
 * squat, then up at a speed by how long he fell (the jump held) or by when
 * the jump is pressed in the squat, through the bounce's two poses, and on
 * in the air.
 */
static int bounce(Actor *a)
{
    g.player_hurt_count = 0;
    PlayerGroundYSave(a);
    g.player_landed = 0x01;
    no_actor_landing_set(0);
    g.player_air_throw = 0;
    PlayerFlipByPad(a);
    PlayerTopSpeedCap();
    PlayerRunRatesSet();
    g.camera_window = 0;
    g.player_box_wanted = 0x10;
    PlayerMoveStart(a, 0x09);
    g.player_bounce_frames = 0x09;
    g.player_bounce_row = 0;
    g.player_slowed &= 0xFF00;          /* its low byte, as the original's byte store */
    if (jump_held()) {
        g.player_bounce_row = bounce_row(g.player_air_frames >= 0x08 ? 0x08 : g.player_air_frames);
        LAND_FRAMES(a) = 0x0A;
        return next_frame(a, P_BOUNCE_SQUAT_HELD);
    }
    return next_frame(a, P_BOUNCE_SQUAT);
}

/* the squat over: up at the row's speed */
static int bounce_up(Actor *a)
{
    uint8_t sum = (uint8_t)(g.player_bounce_row + 4);

    g.camera_window = 0x0A;
    ActorSpeedYSetGravity(a, 0x38, (uint8_t)(sum * 3 + (sum >> 7)));
    if (!g.player_bounce_row)
        g.player_top_speed = 0x0270;
    PlayerMoveStart(a, 0x0A);
    g.player_bounce_frames = 0x07;
    return next_frame(a, P_BOUNCE_RISE_1);
}

/* a frame of the bounce's rise; returns the state he went to, or 0 */
static int rise_frame(Actor *a)
{
    int r;

    if ((r = PlayerAirStep(a)))
        return r;
    PlayerFlipByPad(a);
    PlayerRunSpeed(a);
    return PLAYER_RETURNED;
}

static int bounce_rise(Actor *a, int second)
{
    int r;

    if ((r = rise_frame(a)))
        return r;
    if (--g.player_bounce_frames)
        return 1;
    if (!second) {
        PlayerMoveStart(a, 0x0B);
        g.player_bounce_frames = 0x07;
        return next_frame(a, P_BOUNCE_RISE_2);
    }
    g.player_slowed &= 0xFF00;
    POSE_FRAMES(a) = 0x01;
    POSE_TIMER(a) = 0x01;
    g.player_box_wanted = 0x03;
    return next_frame(a, P_AIR);
}

/*
 * @routine 81:D8FD PlayerSprung
 * WHAT IT DOES: A spring (kind $40) throws him up: he flies up against its
 * pull until he slows to falling, then falls with the air's settings and
 * the jump's poses until he lands, catches a ledge or hovers.
 */
static int sprung(Actor *a)
{
    g.camera_window = 0;
    a->accel_y = 0x12;
    a->speed_y = 1 << 8;
    g.player_ground_y = 0;
    ActorStopX(a);
    PlayerMoveStart(a, 0x0F);
    return next_frame(a, P_SPRUNG_UP);
}

static int sprung_up(Actor *a)
{
    ActorFlyAgainst(a);
    if (a->speed_y >= 0)
        return 1;
    g.player_in_air = 0x01;
    g.player_landed = 0;
    ActorSpeedYSetGravity(a, 0x38, 0x63);
    g.player_box_wanted = 0x03;
    PlayerMoveStart(a, 0x04);
    POSE_FRAMES(a) = 0x07;
    POSE_TIMER(a) = 0x0E;
    return next_frame(a, P_SPRUNG_AIR);
}

static int sprung_air(Actor *a)
{
    int r;

    if ((r = PlayerAirStep(a)))
        return r;
    PlayerFlipByPad(a);
    PlayerJumpPoseStep(a);
    if (hovers(a))
        return PlayerGoTo(a, P_HOVER);
    air_gravity(a);
    PlayerRunSpeed(a);
    if (a->speed_y < 0)
        return 1;
    if (GroundBehindBit3(a, &player_point))
        return PlayerGoTo(a, P_LEDGE_GRAB);
    if (ActorOutOfGround(a))
        return PlayerGoTo(a, P_LAND);
    return 1;
}

/*
 * @routine 81:D992 PlayerCarried
 * WHAT IT DOES: Kind $2C carries him: its pose $1A, and he waits a frame at
 * a time for good while it moves him, until it sets his code again.
 */
static int carried(Actor *a)
{
    PlayerMoveStart(a, 0x1A);
    return next_frame(a, P_CARRIED_FRAME);
}

/* the hang's start, from what took hold of him or a switch's handle */
static void hang_start(Actor *a)
{
    g.player_landed = 0;
    g.player_air_throw = 0;
    no_actor_landing_set((uint8_t)(no_actor_landing() + 1));
    PlayerFlagsClear();
    ActorStopX(a);
    ActorStopY(a);
}

/*
 * @routine 81:D99B PlayerGrabbedHang
 * WHAT IT DOES: An actor he hangs from takes hold of him: stopped where he
 * was kept, the hang's sound and pose, and he hangs from it.
 */
static int grabbed_hang(Actor *a)
{
    hang_start(a);
    PlayerPosRestore(a);
    SoundQueueAdd(0x06);
    PlayerMoveStart(a, 0x06);
    g.player_box_wanted = 0x04;
    return PlayerGoTo(a, P_HANG);
}

/*
 * @routine 81:D9C5 PlayerSwitchHang
 * WHAT IT DOES: He hangs from a switch's handle (kind $3B) or is held by
 * the continue screen's choice: where he was kept, until the continue
 * screen's count is 0 and down is on the pad; then 18 pixels lower he
 * falls.
 */
static int switch_hang(Actor *a)
{
    PlayerPosRestore(a);
    hang_start(a);
    SoundQueueAdd(0x06);
    PlayerMoveStart(a, 0x06);
    g.player_box_wanted = 0x04;
    return next_frame(a, P_SWITCH_HANG_FRAME);
}

static int switch_hang_frame(Actor *a)
{
    if (g.continue_frames || !(pad_dirs() & PAD_DOWN))
        return 1;
    PlayerGroundYSave(a);
    g.player_no_landing = 0x10;
    set_y_pixels(a, (uint16_t)(y_pixels(a) + 0x0012));
    no_actor_landing_set(0);
    return PlayerGoTo(a, PLAYER_AT_FALL);
}

/*
 * @routine 81:DA14 PlayerFlung
 * WHAT IT DOES: An actor he landed on (kind $7B) flings him up: he flies up
 * against its pull until he slows to falling, rests 8 frames, then is in
 * the air as in PlayerAir until he lands, catches a ledge or hovers.
 */
static int flung(Actor *a)
{
    g.camera_window = 0;
    a->accel_y = 0x10;
    a->speed_y = 1 << 8;
    ActorStopX(a);
    PlayerMoveStart(a, 0x18);
    return next_frame(a, P_FLUNG_UP);
}

static int flung_up(Actor *a)
{
    ActorFlyAgainst(a);
    if (a->speed_y >= 0)
        return 1;
    g.player_in_air = 0x01;
    g.player_landed = 0;
    ActorSpeedYSetGravity(a, 0x38, 0x66);
    a->resume = P_FLUNG_REST;
    return 0x08;
}

static int flung_rest(Actor *a)
{
    POSE_FRAMES(a) = 0;
    PlayerMoveStart(a, 0x04);
    g.camera_window = 0x0A;
    PlayerGroundYSave(a);
    return next_frame(a, P_FLUNG_AIR);
}

static int flung_air(Actor *a)
{
    int r;

    if (g.player_air_throw) {
        if (!--g.player_air_throw) {
            POSE_FRAMES(a) = 0x08;
            POSE_TIMER(a) = 0x01;
            PlayerJumpPoseStep(a);
        }
    } else if (PlayerAppleThrow(a)) {
        PlayerMoveStart(a, 0x16);
        g.player_air_throw = 0x14;
    } else {
        PlayerJumpPoseStep(a);
    }
    if ((r = PlayerAirStep(a)))
        return r;
    PlayerFlipByPad(a);
    if (hovers(a))
        return PlayerGoTo(a, P_HOVER);
    air_gravity(a);
    PlayerRunSpeed(a);
    if (a->speed_y < 0)
        return 1;
    if (GroundBehindBit3(a, &player_point))
        return PlayerGoTo(a, P_LEDGE_GRAB);
    if (g.player_on_actor || ActorOutOfGround(a))
        return PlayerGoTo(a, P_LAND);
    return 1;
}

int player_held_code(Actor *a)
{
    switch (a->resume) {
    case PLAYER_AT_HELD:            return held(a);
    case PLAYER_AT_BOUNCE:          return bounce(a);
    case PLAYER_AT_SPRUNG:          return sprung(a);
    case PLAYER_AT_CARRIED:         return carried(a);
    case PLAYER_AT_GRABBED_HANG:    return grabbed_hang(a);
    case PLAYER_AT_SWITCH_HANG:     return switch_hang(a);
    case PLAYER_AT_FLUNG:           return flung(a);
    case P_HELD_FRAME:              return held_frame(a);
    case P_HELD_LET_GO:             return held_let_go(a);
    case P_HELD_JUMP_OFF:           return held_jump_off(a);
    case P_JUMP_OFF_FRAME:          return jump_off_frame(a);
    case P_BOUNCE_SQUAT_HELD:
        if (--LAND_FRAMES(a))
            return 1;
        return bounce_up(a);
    case P_BOUNCE_SQUAT:
        if (jump_pressed())
            g.player_bounce_row = bounce_row(g.player_bounce_frames);
        if (--g.player_bounce_frames)
            return 1;
        PlayerSpeedLeast(a);
        return bounce_up(a);
    case P_BOUNCE_RISE_1:           return bounce_rise(a, 0);
    case P_BOUNCE_RISE_2:           return bounce_rise(a, 1);
    case P_SPRUNG_UP:               return sprung_up(a);
    case P_SPRUNG_AIR:              return sprung_air(a);
    case P_CARRIED_FRAME:           return 1;
    case P_SWITCH_HANG_FRAME:       return switch_hang_frame(a);
    case P_FLUNG_UP:                return flung_up(a);
    case P_FLUNG_REST:              return flung_rest(a);
    case P_FLUNG_AIR:               return flung_air(a);
    default:                        return player_unconverted(a, "a resume point of held.c");
    }
}
