/*
 * player_states.c: Aladdin's code as he stands, walks, skids, crouches,
 * jumps and falls, lands, hangs from a ledge, hovers, and is knocked back
 * or loses a life (the record's player/player_states.c). Each state goes
 * on to the next at once, as the original's jump (PlayerGoTo); a frame's
 * wait is the original's BRK, a longer one its COP.
 *
 * Not converted, dead (the record says no input reaches them): 81:DAEB
 * PlayerKnockbackFromLedge (only the JMP at 81:DCFD names it, in code
 * nothing reaches) and 81:DBCF PlayerKnockbackNoHealth (nothing names it).
 */
#include "player/player_own.h"
#include "level/level.h"
#include "system/task.h"

#define LEVEL_SCROLL_HOLD 0x05  /* the level where pushing ground behind him holds the scroll */
#define LEVEL_SLOW_KNOCK 0x06   /* the level with its own knockback arc */

static int next_frame(Actor *a, uint16_t at)
{
    a->resume = at;
    return 1;
}

/* The ground last probed hurts him: behaviour bit 5, unless his +$08 bit
   4 (blinking) or g.player_flags bit 0 is set. */
static bool ground_hurts(const Actor *a)
{
    return (player_point.behaviour & TILE_HURTS) && !(a->anim_flags & 0x10) && !(g.player_flags & 0x01);
}

/* g.progress_sequence 4, or level $1A: the ground can push and slide him */
static bool pushing_ground(void)
{
    return g.progress_sequence == 0x04 || g.level == 0x1A;
}

/* the level-5 test: pushing ground behind him at probe $18 holds the
   scroll both ways */
static void scroll_hold_test(Actor *a)
{
    if (g.level != LEVEL_SCROLL_HOLD)
        return;
    a->probe_at = 0x18;
    if (GroundBehindBit4(a, &player_point)) {
        g.scroll_x_held = (uint16_t)((g.scroll_x_held & 0xFF00) | 0x01);
        g.scroll_y_held = (uint16_t)((g.scroll_y_held & 0xFF00) | 0x01);
    }
}

static bool speed_zero(const Actor *a)
{
    return wrap24((uint32_t)a->speed_x) == 0;
}

/*
 * @routine 81:CFE0 PlayerJump
 * WHAT IT DOES: He jumps: his top speed in the air the speed `top` (8.8),
 * the jump's rise by the low bits of his speed's pixels, the jump's sound;
 * on into the air.
 */
static int jump(Actor *a, uint16_t top)
{
    g.player_landed = 0;
    g.player_top_speed = top;
    PlayerFlipByPad(a);
    ActorMotionPresetY(a, jump_preset((uint8_t)(speed_word(a->speed_x) >> 8)));
    POSE_FRAMES(a) = 0;
    SoundQueueAdd(0x01);
    g.player_no_landing = 0x04;
    return PlayerGoTo(a, P_AIR_START);
}

/* jump pressed: at his speed */
static int jump_at_speed(Actor *a)
{
    return jump(a, speed_word(a->speed_x));
}

/* ---- tests the states call (PLAYER_RETURNED, or where they sent him) ---- */

/*
 * @routine 81:D347 PlayerNoGroundFall
 * WHAT IT DOES: When there is no ground at probe 0 to either side of his
 * feet (and he stands on no actor), he falls.
 */
int PlayerNoGroundFall(Actor *a)
{
    if (g.player_on_actor)
        return PLAYER_RETURNED;
    a->probe_at = 0;
    if (GroundLeft(a, &player_point) || GroundRight(a, &player_point))
        return PLAYER_RETURNED;
    return PlayerGoTo(a, PLAYER_AT_FALL);
}

/*
 * @routine 81:D407 PlayerAirStep
 * WHAT IT DOES: A frame's move through the air: gravity, his speeds,
 * landing on a floor, out of walls, and ground that hurts.
 */
int PlayerAirStep(Actor *a)
{
    int r;

    ActorGravity(a);
    ActorStepX(a);
    ActorMoveY(a);
    if ((r = PlayerLandOrHurt(a)))
        return r;
    out_of_walls(a);
    return PlayerGroundHurtTest(a);
}

/*
 * @routine 81:D3E1 PlayerCrouchStep
 * WHAT IT DOES: Moves him across as he slides crouched, out of walls, and
 * onto the ground (not while he stands on an actor).
 */
static int crouch_step(Actor *a)
{
    int r;

    ActorStepX(a);
    if ((r = PlayerOutOfWallsHurt(a)))
        return r;
    if (!g.player_on_actor)
        snap_to_ground(a);
    return PLAYER_RETURNED;
}

/*
 * @routine 81:D3F4 PlayerCrouchJump
 * WHAT IT DOES: The jump pressed while he crouches (and g.player_no_crouch_jump
 * clear): he jumps, at top speed $0180.
 */
static int crouch_jump(Actor *a)
{
    if (!jump_pressed() || g.player_no_crouch_jump)
        return PLAYER_RETURNED;
    return jump(a, 0x0180);
}

/*
 * @routine 81:D361 PlayerCrouch
 * WHAT IT DOES: When down is held he crouches: he slides to a stop
 * crouched, may throw (move $15, 24 frames), jump, or fall, and stands
 * again when down is let go. Ground that hurts knocks him back.
 */
int PlayerCrouch(Actor *a)
{
    if (!(pad_dirs() & PAD_DOWN))
        return PLAYER_RETURNED;
    g.player_box_wanted = 0x01;
    PlayerMoveStart(a, 0x01);
    a->flags |= 0x01;
    a->accel_x = 0x28;
    a->facing = a->flip;
    return next_frame(a, P_CROUCH_FRAME);
}

/* the crouch's frame from after its throw */
static int crouch_rest(Actor *a)
{
    int r;

    if ((r = crouch_jump(a)) || (r = PlayerNoGroundFall(a)))
        return r;
    if (pad_dirs() & PAD_DOWN)
        return next_frame(a, P_CROUCH_FRAME);
    a->facing = a->flip;
    return PlayerGoTo(a, PLAYER_AT_STAND);
}

static int crouch_frame(Actor *a)
{
    int r;

    snap_to_ground(a);
    if (ground_hurts(a)) {
        PlayerEnter8C();
        return PlayerGoTo(a, PLAYER_AT_KNOCKBACK);
    }
    PlayerFlipByPad(a);
    if (speed_zero(a)) {
        a->accel_x = 0;
    } else {
        ActorDecelerateX(a);
        if ((r = crouch_step(a)))
            return r;
    }
    if (PlayerAppleThrow(a)) {
        PlayerMoveStart(a, 0x15);
        a->resume = P_CROUCH_THROW_DONE;
        return 0x18;
    }
    return crouch_rest(a);
}

/* ---- standing ---- */

/* PlayerStand's frame from its jump test on (also PlayerThrowStanding's
   after its throw) */
static int standing_rest(Actor *a)
{
    int r;

    if (jump_pressed())
        return jump_at_speed(a);
    if (pushing_ground())
        PlayerGroundSlide(a);
    if ((r = PlayerCrouch(a)) || (r = PlayerNoGroundFall(a)))
        return r;
    PlayerFlipByPad(a);
    if (g.player_stand_held || (PadFacing() & 0x80))
        return next_frame(a, P_STAND_FRAME);
    if (a->facing != a->flip) {
        /* the pad points the other way: he turns first (4 frames) */
        PlayerMoveStart(a, 0x05);
        POSE_FRAMES(a) = 0x04;
        return next_frame(a, P_STAND_TURN);
    }
    return -1;                          /* on into the walk (walk_start) */
}

/* from standing into the walk: the dash's speed, or a slow start */
static int walk_start(Actor *a)
{
    g.player_dash = 0;
    if (dash_held()) {
        ActorSpeedXSet(a, 0x00);
        g.player_dash = 0x01;
    } else {
        ActorStopX(a);
        a->speed_x = (int32_t)(((uint32_t)a->speed_x & ~0xFFu) | 0x80);
    }
    return PlayerGoTo(a, P_WALK);
}

static int stand_rest_or_walk(Actor *a)
{
    int r = standing_rest(a);

    return r < 0 ? walk_start(a) : r;
}

/*
 * @routine 81:CCB6 PlayerStand
 * WHAT IT DOES: Aladdin stands: each frame he slows to a stop on the
 * ground, may throw, jump, slide, crouch or fall off; when the pad points
 * one way he turns to it (4 frames) and walks.
 */
static int stand(Actor *a)
{
    PlayerFlagsClear();
    a->flags &= 0xFE;
    PlayerMoveStart(a, 0x00);
    g.player_box_wanted = 0;
    g.player_dash = 0;
    snap_to_ground(a);
    return next_frame(a, P_STAND_FRAME);
}

static int stand_frame(Actor *a)
{
    int r;

    ActorDecelerateX(a);
    if ((r = PlayerGroundSnapHurt(a)))
        return r;
    if (PlayerAppleThrow(a))
        return PlayerGoTo(a, P_THROW_STANDING);
    return stand_rest_or_walk(a);
}

static int stand_turn(Actor *a)
{
    if (jump_pressed())
        return jump_at_speed(a);
    if (--POSE_FRAMES(a))
        return 1;
    return walk_start(a);
}

/*
 * @routine 81:CCDC PlayerThrowStanding
 * WHAT IT DOES: He throws standing: the throw's pose ($14) for 20 frames,
 * then on as PlayerStand after its throw test.
 */
static int throw_standing(Actor *a)
{
    PlayerMoveStart(a, 0x14);
    WAIT_FRAMES(a) = 0x14;
    return next_frame(a, P_THROW_STANDING_DONE);
}

static int throw_standing_done(Actor *a)
{
    if (--WAIT_FRAMES(a))
        return 1;
    PlayerMoveStart(a, 0x00);
    return stand_rest_or_walk(a);
}

/* ---- walking ---- */

/*
 * @routine 81:CD5E PlayerWalk
 * WHAT IT DOES: He walks and runs: each frame his speed follows the pad,
 * he moves across and stays on the ground; he falls with no ground under
 * him, jumps, skids when the pad no longer points his way, throws, or
 * crouches.
 */
static int walk(Actor *a)
{
    no_actor_landing_set(0);
    g.player_in_air = 0;
    g.camera_window = 0;
    g.player_box_wanted = 0;
    a->facing = a->flip;
    PlayerMoveStart(a, 0x02);
    return next_frame(a, P_WALK_FRAME);
}

static int walk_frame(Actor *a)
{
    int r;

    if (pushing_ground())
        PlayerGroundPush(a);
    PlayerRunMove(a);
    ActorStepX(a);
    if ((r = PlayerOutOfWallsHurt(a)) || (r = PlayerGroundSnapHurt(a)))
        return r;
    if (!player_point.kind)
        return PlayerGoTo(a, PLAYER_AT_FALL);
    PlayerAirSteer(a);
    g.player_dash = dash_held() ? 0x01 : 0;
    if (jump_pressed())
        return jump_at_speed(a);
    if (PadFacing() != a->facing) {
        g.player_skid_started = 0;
        return PlayerGoTo(a, P_SKID);
    }
    if (PlayerAppleThrow(a))
        return PlayerGoTo(a, P_THROW_STANDING);
    if ((r = PlayerCrouch(a)))
        return r;
    return 1;
}

/* ---- skidding ---- */

/* the skid's turn of the pad, then its frame's wait */
static int skid_turn(Actor *a)
{
    uint8_t f = PadFacing(), move;

    if (!PlayerFlipByPad(a)) {
        a->accel_x = 0x20;
        if (PlayerAppleThrow(a)) {
            PlayerMoveStart(a, 0x14);
            a->resume = P_SKID_THROW_DONE;
            return 0x14;
        }
    } else if ((f ^ a->facing) == 0x01) {
        /* the pad points against his way */
        PlayerSpawnEvery8(a);
        if (!g.player_skid_started) {
            g.player_skid_started++;
            SoundQueueAdd(0x0A);
            a->accel_x = 0x10;
            move = 0x12;
            if (speed_word(a->speed_x) >= 0x0180) {
                SoundQueueAdd(0x04);
                move = 0x13;
            }
            PlayerMoveStart(a, move);
            if (speed_word(a->speed_x) >= 0x0180) {
                a->flags &= 0xFE;
                g.player_box_wanted = 0;
            }
        }
    }
    return next_frame(a, P_SKID_FRAME);
}

/*
 * @routine 81:D27F PlayerSkid
 * WHAT IT DOES: He skids as the pad turns him while he runs: the skid's
 * sound and pose (a fast run's own, from $0180), puffs every eighth frame,
 * slowing until he stops and stands; he walks on the new way, falls, jumps
 * or throws.
 */
static int skid(Actor *a)
{
    a->accel_x = 0x10;
    return skid_turn(a);
}

static int skid_frame(Actor *a)
{
    uint8_t f;
    bool below;
    int r;

    ActorStepX(a);
    if (!out_of_walls(a)) {
        if (!g.player_on_actor && !snap_to_ground(a))
            return PlayerGoTo(a, PLAYER_AT_FALL);
        if (jump_pressed())
            return jump(a, 0x0180);
        f = PadFacing();
        if (!(f & 0x80) && f == a->facing) {
            a->flip = f;
            return PlayerGoTo(a, P_WALK);
        }
        if (!g.player_skid_started)
            PlayerRunMove(a);
        below = signed24((uint32_t)a->speed_x - a->accel_x) < 0;
        ActorDecelerateX(a);
        if (!below && !speed_zero(a)) {
            if ((r = PlayerCrouch(a)))
                return r;
            return skid_turn(a);
        }
    }
    ActorStopX(a);
    return PlayerGoTo(a, PLAYER_AT_STAND);
}

/* ---- in the air ---- */

/*
 * @routine 81:CDD2 PlayerFall
 * WHAT IT DOES: He starts to fall: the fall's speed and pose, his speed
 * kept as his top speed, and on into the air.
 */
static int fall(Actor *a)
{
    ActorMotionPresetY(a, 0x02);
    PlayerMoveStart(a, 0x0E);
    POSE_FRAMES(a) = 0x03;
    PlayerTopSpeedHold(a);
    return PlayerGoTo(a, P_AIR_START);
}

/*
 * @routine 81:D005 PlayerAirStart
 * WHAT IT DOES: He leaves the ground: his speeds held to the air's.
 */
static int air_start(Actor *a)
{
    g.player_ground_pushed = 0;
    no_actor_landing_set(0);
    PlayerTopSpeedCap();
    PlayerRunRatesSet();
    return PlayerGoTo(a, P_AIR_TIMED);
}

static int air_frame(Actor *a);

/*
 * @routine 81:D011 PlayerAirTimed
 * WHAT IT DOES: The air's settings (the camera's window $0A, box 3, in
 * the air, his frames in it from 0), his height noted, and his first frame
 * in the air now.
 */
static int air_timed(Actor *a)
{
    g.camera_window = 0x0A;
    g.player_box_wanted = 0x03;
    g.player_in_air++;
    g.player_air_frames = 0;
    a->flags &= 0xFE;
    PlayerGroundYSave(a);
    return air_frame(a);
}

/*
 * @routine 81:D02C PlayerAir
 * WHAT IT DOES: A frame in the air: his jump's pose or a throw, the hover,
 * his move, his gravity (heavier rising with the jump let go), his speed
 * by the pad; falling, a ledge to catch, or the ground to land on.
 */
static int air_frame(Actor *a)
{
    int r;

    if (a->speed_y >= 0 && g.player_on_actor) {
        player_point.behaviour = 0;
        return PlayerGoTo(a, P_LAND);
    }
    if (!(g.player_air_frames & 0x80))
        g.player_air_frames++;
    if (jump_pressed())
        g.player_air_frames = 0;
    if (g.player_air_throw) {
        if (!--g.player_air_throw) {
            POSE_FRAMES(a) = 0x08;
            PlayerJumpPoseStep(a);
        }
    } else if (PlayerAppleThrow(a)) {
        PlayerMoveStart(a, 0x16);
        g.player_air_throw = 0x15;
    } else {
        PlayerJumpPoseStep(a);
    }
    if (g.hover_on && hover_held() && a->speed_y >= 0)
        return PlayerGoTo(a, P_HOVER);
    if ((r = PlayerAirStep(a)))
        return r;
    PlayerFlipByPad(a);
    a->accel_y = 0x38;
    if (!jump_held() && a->speed_y < 0 && !g.player_landed)
        a->accel_y = 0x80;
    PlayerRunSpeed(a);
    if (a->speed_y < 0)
        return next_frame(a, P_AIR);
    scroll_hold_test(a);
    if (g.player_on_actor)
        return PlayerGoTo(a, P_LAND);
    if (!(pad_dirs() & PAD_DOWN) && !g.player_no_landing) {
        if (GroundBehindBit3(a, &player_point))
            return PlayerGoTo(a, P_LEDGE_GRAB);
    } else if (g.player_no_landing) {
        g.player_no_landing--;
    }
    if (ActorOutOfGround(a))
        return PlayerGoTo(a, P_LAND);
    return next_frame(a, P_AIR);
}

/*
 * @routine 81:D0E0 PlayerAirLand
 * WHAT IT DOES: In the ground he lands; else his next frame in the air.
 */
static int air_land(Actor *a)
{
    if (ActorOutOfGround(a))
        return PlayerGoTo(a, P_LAND);
    return next_frame(a, P_AIR);
}

/*
 * @routine 81:D0E9 PlayerLand
 * WHAT IT DOES: He lands: ground that hurts knocks him back; else the
 * landing's sound, and after a fall of more than 128 pixels a hard landing
 * (18 frames) and he stands; else he walks on (a dash's speed $0200, a
 * run's at most $0180) or stands.
 */
static int land(Actor *a)
{
    uint16_t w;

    snap_to_ground(a);
    g.player_no_landing = 0;
    g.player_air_throw = 0;
    g.player_in_air = 0;
    g.player_landed = 0;
    if (ground_hurts(a)) {
        PlayerEnter8C();
        return PlayerGoTo(a, PLAYER_AT_KNOCKBACK);
    }
    SoundQueueAdd(0x10);
    if ((uint16_t)(g.player_ground_y + 0x0080) < y_pixels(a)) {
        PlayerMoveStart(a, 0x0F);
        LAND_FRAMES(a) = 0x12;
        return next_frame(a, P_LAND_HARD_DONE);
    }
    if (!(PadFacing() & 0x80)) {
        w = speed_word(a->speed_x);
        if (dash_held())
            w = 0x0200;
        else if (0x0180 < w)
            w = 0x0180;
        a->speed_x = with_speed_word(a->speed_x, w);
        return PlayerGoTo(a, P_WALK);
    }
    ActorSpeedXClear(a);
    return PlayerGoTo(a, PLAYER_AT_STAND);
}

static int land_hard_done(Actor *a)
{
    if (--LAND_FRAMES(a))
        return 1;
    ActorSpeedXClear(a);
    return PlayerGoTo(a, PLAYER_AT_STAND);
}

/* ---- ledges ---- */

/*
 * @routine 81:D470 PlayerLedgeGrab
 * WHAT IT DOES: He catches the ledge his fall passed: set at its corner
 * (the tile's mirror and his flip must differ, else on in the air), his
 * speeds stopped, the grab's sound and pose; on to hang.
 */
static int ledge_grab(Actor *a)
{
    uint16_t corner = player_point.x & 0xFFF0;

    if (!player_point.mirrored) {
        if (a->flip)
            return PlayerGoTo(a, P_AIR_LAND);
        set_x_pixels(a, (uint16_t)(corner + 0x0001));
    } else {
        if (!a->flip)
            return PlayerGoTo(a, P_AIR_LAND);
        set_x_pixels(a, (uint16_t)(corner + 0x000F));
    }
    set_y_pixels(a, (uint16_t)((player_point.y & 0xFFF0) + 0x0001));
    g.player_landed = 0;
    g.player_air_throw = 0;
    no_actor_landing_set((uint8_t)(no_actor_landing() + 1));
    PlayerFlagsClear();
    ActorStopX(a);
    ActorStopY(a);
    SoundQueueAdd(0x06);
    PlayerMoveStart(a, 0x06);
    g.player_box_wanted = 0x04;
    return PlayerGoTo(a, P_HANG);
}

/*
 * @routine 81:D4D1 PlayerHang
 * WHAT IT DOES: He hangs from a ledge: up climbs onto it (move 7, 32
 * frames, then up 24 pixels and across 8, standing); down lets go (18
 * pixels lower, falling, no hold for 16 frames).
 */
static int hang_frame(Actor *a)
{
    if (pad_dirs() & PAD_UP) {
        PlayerMoveStart(a, 0x07);
        g.player_box_wanted = 0x05;
        a->resume = P_HANG_CLIMBED;
        return 0x20;
    }
    if (pad_dirs() & PAD_DOWN) {
        a->facing = a->flip;
        g.player_no_landing = 0x10;
        set_y_pixels(a, (uint16_t)(y_pixels(a) + 0x0012));
        no_actor_landing_set(0);
        return PlayerGoTo(a, PLAYER_AT_FALL);
    }
    return 1;
}

static int hang_climbed(Actor *a)
{
    no_actor_landing_set(0);
    a->facing = a->flip;
    a->speed_x = 8 << 8;                /* 8 pixels across */
    a->speed_y = -(24 << 8);            /* 24 up */
    ActorMove(a);
    ActorSpeedXClear(a);
    ActorSpeedYClear(a);
    return PlayerGoTo(a, PLAYER_AT_STAND);
}

/* ---- the hover (g.hover_on) ---- */

static int hover_land(Actor *a)
{
    g.player_box_wanted = 0x03;
    a->y &= ~0xFFu;
    a->speed_y = 0;
    return PlayerGoTo(a, P_LAND);
}

static int hover_frame(Actor *a)
{
    int r;

    ActorStepX(a);
    ActorMoveY(a);
    if (!(y_pixels(a) & 0x8000)) {
        out_of_walls(a);
        if ((r = PlayerGroundHurtJump(a)))
            return r;
    }
    PlayerFlipByPad(a);
    if (g.player_on_actor)
        return hover_land(a);
    if (!hover_held()) {
        a->accel_y = 0x36;
        POSE_FRAMES(a) = 0x01;
        POSE_TIMER(a) = 0x01;
        return PlayerGoTo(a, P_AIR_START);
    }
    scroll_hold_test(a);
    PlayerRunSpeed(a);
    if ((r = PlayerOutOfWallsHurt(a)))
        return r;
    if (ActorOutOfGround(a))
        return hover_land(a);
    if (g.player_no_landing)
        g.player_no_landing--;
    else if (GroundBehindBit3(a, &player_point))
        return PlayerGoTo(a, P_LEDGE_GRAB);
    return next_frame(a, P_HOVER_FRAME);
}

/*
 * @routine 81:D86A PlayerHover
 * WHAT IT DOES: He hovers while the fourth action is held: his own pose and
 * a slow fall, across at up to $0200; let go, he falls; he can land, catch
 * a ledge or meet ground that hurts.
 */
static int hover(Actor *a)
{
    g.player_box_wanted = 0x12;
    g.player_hurt_count = 0;
    PlayerMoveStart(a, 0x0C);
    ActorSpeedYSet(a, 0x18);
    g.player_top_speed = 0x0200;
    return hover_frame(a);
}

/* ---- the knockback and the life's loss ---- */

static int life_lost(Actor *a)
{
    return next_frame(a, P_LOSS_FIRST);
}

/* he fell out of the map, or his knockback's end found no health: the life
   is lost */
static int fallen(Actor *a)
{
    g.health = 0;
    a->flags &= 0x7F;
    a->anim_flags |= 0x80;
    g.player_fallen_out = 0;
    g.scroll_x_held = (uint16_t)((g.scroll_x_held & 0xFF00) | (uint8_t)(g.scroll_x_held + 1));
    g.scroll_y_held = (uint16_t)((g.scroll_y_held & 0xFF00) | (uint8_t)(g.scroll_y_held + 1));
    g.play_starts++;
    g.camera_window = -1;
    a->anim_flags &= 0xF7;
    return life_lost(a);
}

static int knockback_end(Actor *a)
{
    g.player_flags = (uint8_t)((g.player_flags & 0xFD) | 0x04);
    if (!g.health) {
        g.play_starts++;
        g.player_box_wanted = 0x06;
        a->anim_flags |= 0x80;
        PlayerMoveStart(a, 0x08);
        return life_lost(a);
    }
    if (!g.player_on_actor)
        set_y_pixels(a, (uint16_t)(y_pixels(a) - 0x000D));
    g.player_hurt_time = 0x7E;
    a->anim_flags |= 0x10;
    g.player_hurt_count = 0;
    a->flags |= 0x80;
    PlayerFlagsClear();
    if (!(g.player_hurt_frames & 0x80))
        return PlayerGoTo(a, PLAYER_AT_STAND);
    POSE_FRAMES(a) = 0x01;
    POSE_TIMER(a) = 0x01;
    return PlayerGoTo(a, P_AIR_START);
}

/*
 * @routine 81:DADD PlayerKnockback
 * WHAT IT DOES: He is hit: knocked back the way he faced, the hurt's pose
 * and sound, for $18 frames or until he lands, then he stands (blinking,
 * $7E frames unhurt) or falls on. With no health left, or fallen out of the
 * map, he loses a life: the song, a fall of $B0 frames, the screen fades,
 * and the level starts again, or the continue screen with no lives left.
 */
static int knockback(Actor *a)
{
    uint8_t preset = 0x05;

    if (!g.health)
        g.player_no_landing = 0xFF;
    g.player_ground_pushed = 0;
    no_actor_landing_set(0);
    if (g.player_fallen_out)
        return fallen(a);
    if (g.level == LEVEL_SLOW_KNOCK)
        preset = 0x56;
    ActorMotionPreset(a, preset);
    SoundQueueAdd(0x02);
    g.player_box_wanted = 0x06;
    PlayerBoxSet(a, 0x06);
    PlayerMoveStart(a, 0x17);
    a->accel_y = 0x30;
    a->facing ^= 0x01;
    g.player_hurt_frames = 0x18;
    g.player_in_air = 0;
    return next_frame(a, P_KNOCKBACK_FRAME);
}

static int knockback_frame(Actor *a)
{
    int r;

    ActorFallCap(a);
    ActorFly(a);
    if ((r = PlayerLandOrHurt(a)))
        return r;
    out_of_walls(a);
    a->facing ^= 0x01;
    out_of_walls(a);
    a->facing ^= 0x01;
    if (g.health && (--g.player_hurt_frames & 0x80))
        return knockback_end(a);
    if (a->speed_y < 0)
        return 1;
    if (!PlayerFallenTest())
        return fallen(a);
    scroll_hold_test(a);
    if (g.player_on_actor || ActorOutOfGround(a))
        return knockback_end(a);
    return 1;
}

/* a frame of the life's loss: out of the ground, and flying when he is not
   in it */
static void loss_frame(Actor *a)
{
    if (!ActorOutOfGround(a) && !g.player_on_actor)
        ActorFly(a);
}

static int loss_first(Actor *a)
{
    SongPlay(0x2A);
    g.view_edge_rule = 0x04;
    g.player_no_landing = 0xFF;
    g.scroll_x_held = (uint16_t)((g.scroll_x_held & 0xFF00) | 0x01);
    ActorMotionPresetY(a, 0x02);
    POSE_FRAMES(a) = 0xB0;
    return next_frame(a, P_LOSS_FALL);
}

static int loss_fall(Actor *a)
{
    loss_frame(a);
    if (--POSE_FRAMES(a))
        return 1;
    TaskCreate(1, 0x04, TASK_ENTRY_BRIGHTNESS_DOWN);
    return next_frame(a, P_LOSS_WAIT);
}

static int loss_wait(Actor *a)
{
    loss_frame(a);
    if (task_busy(1))
        return 1;
    g.player_hurt_count = 0;
    Level035CSet();
    g.lives--;
    TaskExecRunning(g.lives ? TASK_ENTRY_LEVEL_PLAY_START : TASK_ENTRY_CONTINUE_SCREEN);
    return 1;
}

/* ---- this file's resume points ---- */

int player_states_code(Actor *a)
{
    switch (a->resume) {
    case PLAYER_AT_KNOCKBACK:       return knockback(a);
    case PLAYER_AT_STAND:           return stand(a);
    case PLAYER_AT_FALL:            return fall(a);
    case P_STAND_FRAME:             return stand_frame(a);
    case P_STAND_TURN:              return stand_turn(a);
    case P_THROW_STANDING:          return throw_standing(a);
    case P_THROW_STANDING_DONE:     return throw_standing_done(a);
    case P_WALK:                    return walk(a);
    case P_WALK_FRAME:              return walk_frame(a);
    case P_AIR_START:               return air_start(a);
    case P_AIR_TIMED:               return air_timed(a);
    case P_AIR:                     return air_frame(a);
    case P_AIR_LAND:                return air_land(a);
    case P_LAND:                    return land(a);
    case P_LAND_HARD_DONE:          return land_hard_done(a);
    case P_SKID:                    return skid(a);
    case P_SKID_FRAME:              return skid_frame(a);
    case P_SKID_THROW_DONE:
        PlayerMoveStart(a, 0x00);
        ActorStopX(a);
        return next_frame(a, P_SKID_FRAME);
    case P_CROUCH_FRAME:            return crouch_frame(a);
    case P_CROUCH_THROW_DONE:
        PlayerMoveStart(a, 0x01);
        return crouch_rest(a);
    case P_LEDGE_GRAB:              return ledge_grab(a);
    case P_HANG:                    return next_frame(a, P_HANG_FRAME);
    case P_HANG_FRAME:              return hang_frame(a);
    case P_HANG_CLIMBED:            return hang_climbed(a);
    case P_HOVER:                   return hover(a);
    case P_HOVER_FRAME:             return hover_frame(a);
    case P_KNOCKBACK_FRAME:         return knockback_frame(a);
    case P_LOSS_FIRST:              return loss_first(a);
    case P_LOSS_FALL:               return loss_fall(a);
    case P_LOSS_WAIT:               return loss_wait(a);
    default:                        return player_unconverted(a, "a resume point of player_states.c");
    }
}
