/*
 * scene.c: Aladdin placed by a scene (the record's player/player_starts.c,
 * scene_starts.c and scene_steps.c, and game/scene_figures.c's three):
 * dropped in, placed for a new game, the end scene of level 5, the walk
 * that follows a landing, the scene that waits for its cue and walks off,
 * the standing scene and the ending's ride.
 *
 * PlayerSceneJumpIn ($07 8) and PlayerSceneWalkOut ($07 13) are played (level
 * 3's and level $10's scenes: found by a play by keys; the record says no level
 * plays them). Not converted (nothing found plays them): 81:DDC1
 * PlayerScenePlaceHigh, 81:DDE0 PlayerScenePlaceLow, 81:DF3A
 * PlayerSceneWaitWalk, 81:E154 PlayerSceneCued and the steps only they call
 * (81:E224, 81:E22C, 81:E235).
 */
#include "player/player_own.h"
#include "system/task.h"

#define SCENE_SLOT 2

static int next_frame(Actor *a, uint16_t at)
{
    a->resume = at;
    return 1;
}

static int wait_then(Actor *a, uint16_t at, int frames)
{
    a->resume = at;
    return frames;
}

/* he falls in the ground-less scenes: down at 3 pixels a frame */
static void fall_start(Actor *a)
{
    PlayerMoveStart(a, 0x0E);
    a->speed_y = 3 << 8;
}

/*
 * @routine 81:DDFF PlayerStartDropped
 * WHAT IT DOES: He is dropped in facing right: he falls with his jump's
 * poses until he is in the ground, lands with its sound and pose ($19) for
 * 128 frames, and stands.
 */
static int start_dropped(Actor *a)
{
    a->facing = 0;
    a->flip = 0;
    ActorMotionPreset(a, 0x4D);
    POSE_FRAMES(a) = 0;
    POSE_TIMER(a) = 0;
    return next_frame(a, P_DROPPED_FRAME);
}

static int dropped_frame(Actor *a)
{
    PlayerJumpPoseStep(a);
    ActorFly(a);
    if (a->speed_y < 0 || !ActorOutOfGround(a))
        return 1;
    SoundQueueAdd(0x10);
    PlayerMoveStart(a, 0x19);
    return wait_then(a, P_DROPPED_LANDED, 0x80);
}

/*
 * @routine 81:DE32 PlayerStartNewGame
 * WHAT IT DOES: He is placed for a new game at (128, 176) facing right:
 * 3 lives, $10 apples, full health; then his start's end.
 */
static int start_new_game(Actor *a)
{
    a->own_pictures = player_pictures();
    set_x_pixels(a, 0x0080);
    set_y_pixels(a, 0x00B0);
    g.view_edge_rule = 0x01;
    a->facing = 0;
    a->flip = 0;
    PlayerMoveStart(a, 0x00);
    a->flags = 0xD0;
    a->redraw = 0xFF;
    a->kind = 0;
    g.tasks[0].param = 0;
    g.apples = 0x10;
    g.lives = 0x03;
    g.health = g.health_marks;
    g.hud_count_2 = 0;
    g.hover_on = 0;
    return player_start_tasks(a);
}

/*
 * @routine 81:E042 PlayerSceneWalkOn
 * WHAT IT DOES: In a scene of level 5's end he drops to the ground if he is
 * above it, faces right and cues the scene, walks right to x $70, stops,
 * and cues it on twice with a pose ($1B) between; then he stays.
 */
static int walk_on(Actor *a)
{
    if (!snap_to_ground(a)) {
        fall_start(a);
        return next_frame(a, P_WALK_ON_FALL);
    }
    return PlayerGoTo(a, P_WALK_ON_CUED);
}

static int walk_on_fall(Actor *a)
{
    ActorMoveY(a);
    if (!ActorOutOfGround(a))
        return 1;
    return PlayerGoTo(a, P_WALK_ON_CUED);
}

static int walk_on_cued(Actor *a)
{
    a->facing = 0;
    a->flip = 0;
    PlayerMoveStart(a, 0x00);
    g.scene_cue_c3++;
    g.scene_exits++;
    return wait_then(a, P_WALK_ON_WALK, 0x40);
}

static int walk_on_walk(Actor *a)
{
    g.scene_cue_c4++;
    PlayerMoveStart(a, 0x02);
    a->speed_x = 1 << 8;
    return next_frame(a, P_WALK_ON_STAND);
}

static int walk_on_stand(Actor *a)
{
    ActorMoveX(a);
    if ((uint8_t)x_pixels(a) < 0x70)
        return 1;
    PlayerMoveStart(a, 0x00);
    return wait_then(a, P_WALK_ON_POSE, 0x20);
}

static int walk_on_pose(Actor *a)
{
    g.scene_cue++;
    PlayerMoveStart(a, 0x1B);
    return wait_then(a, P_WALK_ON_END, 0xA0);
}

static int walk_on_end(Actor *a)
{
    g.scene_cue_bf++;
    return wait_then(a, P_WALK_ON_STAY, 0x80);
}

/*
 * @routine 81:E126 PlayerSceneWalkRight
 * WHAT IT DOES: He walks right in a scene until x $0248, holds the
 * landing's pose ($19) for 128 frames, and stands.
 */
static int walk_right(Actor *a)
{
    a->facing = 0;
    a->flip = 0;
    PlayerMoveStart(a, 0x02);
    ActorSpeedXSet(a, 0x03);
    return next_frame(a, P_WALK_RIGHT);
}

static int walk_right_frame(Actor *a)
{
    ActorMoveX(a);
    if (x_pixels(a) <= 0x0248)
        return 1;
    PlayerMoveStart(a, 0x19);
    return wait_then(a, P_WALK_RIGHT_POSED, 0x80);
}

/*
 * @routine 81:DF6F PlayerSceneJumpIn
 * WHAT IT DOES: ($07 8: level 3's and level $10's scenes) He walks right
 * for 24 frames (move 2, speed entry 3), jumps (preset $4D) and flies
 * with his jump's poses until he falls into the ground, lands with its
 * sound and pose ($19) for 128 frames, and stands.
 */
static int jump_in(Actor *a)
{
    a->facing = 0;
    a->flip = 0;
    PlayerMoveStart(a, 0x02);
    ActorSpeedXSet(a, 0x03);
    WAIT_FRAMES(a) = 0x18;
    return next_frame(a, P_JUMP_IN_WALK);
}

static int jump_in_walk(Actor *a)
{
    ActorMoveX(a);
    if (--WAIT_FRAMES(a))
        return 1;
    ActorMotionPreset(a, 0x4D);
    POSE_FRAMES(a) = 0;
    POSE_TIMER(a) = 0;
    return next_frame(a, P_DROPPED_FRAME);
}

/*
 * @routine 81:E0AE PlayerSceneWalkOut
 * WHAT IT DOES: ($07 13: level 3's last scene) He walks right at one pixel
 * a frame to the scene's cue point (x $0650 or more by its bytes), stands
 * until the scene's cue (bit 7 of g.scene_cue), walks on for 256 frames
 * with the escort's state set to 3, then as the screen fades (sound $F6,
 * the brightness task) walks on; at the end of the fade the restart point
 * and the stage are cleared, the story's part counted up, and the level's
 * end task goes on.
 */
static int walk_out(Actor *a)
{
    a->facing = 0;
    a->flip = 0;
    g.view_edge_rule = 0x04;
    g.scene_exits++;
    PlayerMoveStart(a, 0x02);
    a->speed_x = 1 << 8;
    return next_frame(a, P_WALK_OUT_WALK);
}

static int walk_out_walk(Actor *a)
{
    ActorMoveX(a);
    if (((x_pixels(a) >> 8) & 0xFF) < 0x06 || (x_pixels(a) & 0xFF) < 0x50)
        return 1;
    PlayerMoveStart(a, 0x00);
    g.scene_cue++;
    return next_frame(a, P_WALK_OUT_CUE);
}

static int walk_out_cue(Actor *a)
{
    if (!(g.scene_cue & 0x80))
        return 1;
    PlayerMoveStart(a, 0x02);
    POSE_FRAMES(a) = 0;
    g.actors[1].own.bytes[OWN_4D] = 0x03;
    return next_frame(a, P_WALK_OUT_ON);
}

static int walk_out_on(Actor *a)
{
    ActorMoveX(a);
    if (--POSE_FRAMES(a))
        return 1;
    SoundQueueF6With1();
    TaskCreate(SCENE_SLOT, 0x04, TASK_ENTRY_BRIGHTNESS_DOWN);
    return next_frame(a, P_WALK_OUT_FADE);
}

static int walk_out_fade(Actor *a)
{
    ActorMoveX(a);
    if (task_busy(SCENE_SLOT))
        return 1;
    g.level_restart = 0;
    g.progress_stage = 0;
    g.progress_sequence++;
    TaskExecRunning(TASK_ENTRY_LEVEL_END);
    return 1;
}

/*
 * @routine 81:DFBB PlayerSceneLand
 * WHAT IT DOES: ($07 9) He faces right and, unless there is ground just
 * under him, falls 3 pixels a frame in move $0E until he is out of the
 * ground; then he stands (move 0) and waits for the scene's cue; cued, he
 * turns for $80 frames and back for $80, walks off right at 2 pixels a
 * frame for $80 frames, and the screen fades: the next part (or the Genie
 * screen's figure when kind $27's figures were touched).
 */
static int scene_land(Actor *a)
{
    a->facing = 0;
    a->flip = 0;
    if (!snap_to_ground(a)) {
        fall_start(a);
        return next_frame(a, P_LAND_FALL);
    }
    return PlayerGoTo(a, P_LAND_STAND);
}

static int land_fall(Actor *a)
{
    ActorMoveY(a);
    if (!ActorOutOfGround(a))
        return 1;
    return PlayerGoTo(a, P_LAND_STAND);
}

static int land_stand(Actor *a)
{
    g.view_edge_rule = 0x04;
    PlayerMoveStart(a, 0x00);
    return next_frame(a, P_LAND_WAIT_CUE);
}

static int land_wait_cue(Actor *a)
{
    if (!g.scene_cue)
        return 1;
    a->flip++;
    return wait_then(a, P_LAND_TURN, 0x80);
}

static int land_turn(Actor *a)
{
    a->flip = 0;
    return wait_then(a, P_LAND_BACK, 0x80);
}

static int land_back(Actor *a)
{
    g.scene_cue_first++;
    PlayerMoveStart(a, 0x03);
    a->speed_x = 2 << 8;
    POSE_FRAMES(a) = 0x80;
    return next_frame(a, P_LAND_WALK);
}

static int land_walk(Actor *a)
{
    ActorMoveX(a);
    if (--POSE_FRAMES(a))
        return 1;
    g.progress_stage++;
    g.scene_exits++;
    if (g.figures_touched)
        return PlayerGoTo(a, PLAYER_AT_SCENE_FIGURE);
    TaskCreate(SCENE_SLOT, 0x04, TASK_ENTRY_BRIGHTNESS_DOWN);
    return next_frame(a, P_LAND_FADE);
}

static int land_fade(Actor *a)
{
    ActorMoveX(a);
    if (task_busy(SCENE_SLOT))
        return 1;
    TaskExecRunning(TASK_ENTRY_NEXT_PART);
    return 1;
}

/*
 * @routine 81:E23E PlayerSceneStand
 * WHAT IT DOES: ($07 15) He is set at (128, 176) facing right for a
 * scene, standing; after $20 frames g.player_anim_held is counted up, and
 * he stays.
 */
static int scene_stand(Actor *a)
{
    a->own_pictures = player_pictures();
    set_x_pixels(a, 0x0080);
    set_y_pixels(a, 0x00B0);
    g.view_edge_rule = 0x01;
    a->facing = 0;
    a->flip = 0;
    PlayerMoveStart(a, 0x08);
    g.player_scene++;
    a->flags = 0xD0;
    a->redraw = 0xFF;
    a->kind = 0;
    g.tasks[0].param = 0;
    return wait_then(a, P_STAND_SCENE_DONE, 0x20);
}

/* a frame of the ending's ride: bobbing and moving until the play field's
   screen is `screen` */
static bool ride_at(uint8_t screen)
{
    return (uint8_t)(pixels(g.layers[0].scroll_x) >> 8) == screen;
}

/*
 * @routine 81:E27A PlayerEndingRide
 * WHAT IT DOES: ($07 14) He rides the carpet across the ending: right at 2
 * pixels a frame for $57 frames, then bobbing at 3/4 of a pixel a frame as
 * the play field scrolls: move 0 to screen 5, move 3 for $1B0 frames, move
 * 4 to screen 12, then (81:E315, which the record has not filed: read from
 * its bytes) move 1 for $149 frames bobbing up, move 2 to screen $17, move 3
 * for $1BA frames bobbing down, move 4 to screen $1E (holding the camera
 * down at screen 2 across), then move 1 accelerating until he is off the
 * screen's right, when $1B14 is counted up, he is put out of sight (x high
 * byte $22, y high byte $FE, still, his +$08 $80), and after 255 frames the
 * camera is held; he stays.
 */
static int ending_ride(Actor *a)
{
    a->flags = 0xD0;
    a->own.bytes[OWN_0A] = 0x40;
    a->own_pictures = player_carpet_pictures();
    a->redraw = 0xFF;
    a->flip = 0;
    a->speed_x = 2 << 8;
    POSE_TIMER(a) = 0;
    PlayerMoveStart(a, 0x00);
    g.player_move = 0;
    POSE_FRAMES(a) = 0x57;
    return next_frame(a, P_RIDE_FLY);
}

static int ride_fly(Actor *a)
{
    ActorMoveX(a);
    if (--POSE_FRAMES(a))
        return 1;
    a->speed_x = 0x00C0;
    a->own.bytes[OWN_49] = 1;            /* the bob going up */
    ActorSpeedYClear(a);
    a->accel_y = 1;
    POSE_TIMER(a) = 0;
    PlayerMoveChange(a, 0x00);
    return next_frame(a, P_RIDE_BOB_1);
}

static int ride_bob_1(Actor *a)
{
    ActorBobY(a);
    ActorMoveX(a);
    if (!ride_at(0x05))
        return 1;
    POSE_TIMER(a) = 1;
    PlayerMoveChange(a, 0x03);
    a->own.bytes[OWN_46] = 0xB0;        /* +$46: frames left of the third stretch (word) */
    a->own.bytes[OWN_47] = 0x01;
    return next_frame(a, P_RIDE_BOB_2);
}

static int ride_bob_2(Actor *a)
{
    uint16_t n = (uint16_t)(a->own.bytes[OWN_46] | a->own.bytes[OWN_47] << 8);

    a->own.bytes[OWN_49] = 0;
    ActorBobY(a);
    ActorMoveX(a);
    n--;
    a->own.bytes[OWN_46] = (uint8_t)n;
    a->own.bytes[OWN_47] = (uint8_t)(n >> 8);
    if (n)
        return 1;
    POSE_TIMER(a) = 0;
    PlayerMoveChange(a, 0x04);
    return next_frame(a, P_RIDE_BOB_3);
}

static int ride_bob_3(Actor *a)
{
    ActorBobY(a);
    ActorMoveX(a);
    if (!ride_at(0x0C))
        return 1;
    POSE_TIMER(a) = 0;
    PlayerMoveChange(a, 0x01);
    a->own.bytes[OWN_46] = 0x49;
    a->own.bytes[OWN_47] = 0x01;
    return next_frame(a, P_RIDE_T1);
}

static bool tail_count_done(Actor *a)
{
    uint16_t n = (uint16_t)((a->own.bytes[OWN_46] | a->own.bytes[OWN_47] << 8) - 1);

    a->own.bytes[OWN_46] = (uint8_t)n;
    a->own.bytes[OWN_47] = (uint8_t)(n >> 8);
    return n == 0;
}

static uint8_t scroll_screen_x(void) { return (uint8_t)(pixels(g.layers[0].scroll_x) >> 8); }
static uint8_t scroll_screen_y(void) { return (uint8_t)(pixels(g.layers[0].scroll_y) >> 8); }

static int ride_t1(Actor *a)
{
    a->own.bytes[OWN_49] = 1;
    ActorBobY(a);
    ActorMoveX(a);
    if (!tail_count_done(a))
        return 1;
    PlayerMoveChange(a, 0x02);
    return next_frame(a, P_RIDE_T2);
}

static int ride_t2(Actor *a)
{
    ActorBobY(a);
    ActorMoveX(a);
    if (scroll_screen_x() != 0x17)
        return 1;
    POSE_TIMER(a) = 1;
    PlayerMoveChange(a, 0x03);
    a->own.bytes[OWN_46] = 0xBA;
    a->own.bytes[OWN_47] = 0x01;
    return next_frame(a, P_RIDE_T3);
}

static int ride_t3(Actor *a)
{
    a->own.bytes[OWN_49] = 0;
    ActorBobY(a);
    ActorMoveX(a);
    if (!tail_count_done(a))
        return 1;
    POSE_TIMER(a) = 0;
    PlayerMoveChange(a, 0x04);
    return next_frame(a, P_RIDE_T4);
}

static int ride_t4(Actor *a)
{
    ActorBobY(a);
    ActorMoveX(a);
    if (scroll_screen_y() == 0x02)
        g.scroll_y_held = (uint16_t)((g.scroll_y_held & 0xFF00) | 0x01);
    if (scroll_screen_x() != 0x1E)
        return 1;
    g.scroll_y_held &= 0xFF00;
    PlayerMoveChange(a, 0x01);
    g.scroll_x_held = (uint16_t)((g.scroll_x_held & 0xFF00) | 0x01);
    g.view_edge_rule = 0x04;
    a->accel_x = 0x08;
    a->accel_y = 0x08;
    return next_frame(a, P_RIDE_T5);
}

static int ride_t5(Actor *a)
{
    a->own.bytes[OWN_49] = 1;
    ActorBobY(a);
    ActorAccelerateX(a);
    ActorMoveX(a);
    if ((uint16_t)(pixels(g.layers[0].scroll_x) + 0x0140) >= x_pixels(a))
        return 1;
    g.boss_beaten++;
    set_x_pixels(a, (uint16_t)((x_pixels(a) & 0x00FF) | 0x2200));
    set_y_pixels(a, (uint16_t)((y_pixels(a) & 0x00FF) | 0xFE00));
    ActorStopY(a);
    a->anim_flags = 0x80;
    a->resume = P_RIDE_T6;
    return 0xFF;
}

static int ride_t6(Actor *a)
{
    g.scroll_y_held = (uint16_t)((g.scroll_y_held & 0xFF00) | 0x01);
    g.scroll_x_held = (uint16_t)((g.scroll_x_held & 0xFF00) | 0x01);
    return next_frame(a, P_STAY);
}

/*
 * @routine 81:DE79 PlayerStory
 * WHAT IT DOES: ($07 7) He is in the ending story's scene: he turns this way
 * and that (move 5 for 8 frames, then move 0) as its figures come and its
 * pages of text go (the text task, slot 4, busy), starts the story's next
 * two pages (the text task 81:F344, parameters 9 and $0B), and when its
 * parts are done is shown no more (his +$09 bits 6 and 7 cleared, x's high
 * byte 1).
 */
#define TEXT_SLOT 4

static Task *story_task(void) { return &g.tasks[0]; }

static int story_start(Actor *a)
{
    a->own_pictures = player_pictures();
    g.player_box_wanted = 0;
    g.player_anim_held = 0;
    a->facing = 1;
    a->flip = 1;
    PlayerMoveStart(a, 0x00);
    a->flags = 0xD0;
    a->redraw = 0xFF;
    return next_frame(a, P_STORY_1);
}

/* a turn: move 5 for 8 frames; it ends with move 0 (the step after) */
static int story_turn(Actor *a, uint16_t then)
{
    PlayerMoveStart(a, 0x05);
    return wait_then(a, then, 0x08);
}

static int story_step(Actor *a)
{
    switch (a->resume) {
    case P_STORY_1:
        if (!g.story_in_place)
            return 1;
        return story_turn(a, P_STORY_2);
    case P_STORY_2:
        PlayerMoveStart(a, 0x00);
        a->flip = 0;
        return next_frame(a, P_STORY_3);
    case P_STORY_3:
        if (task_busy(TEXT_SLOT))
            return 1;
        return wait_then(a, P_STORY_4, 0x20);
    case P_STORY_4:
        return story_turn(a, P_STORY_5);
    case P_STORY_5:
        PlayerMoveStart(a, 0x00);
        a->flip++;
        return wait_then(a, P_STORY_6, 0x20);
    case P_STORY_6:
        TaskCreate(TEXT_SLOT, 0x09, TASK_ENTRY_STORY_TEXT1);
        return next_frame(a, P_STORY_7);
    case P_STORY_7:
        if (task_busy(TEXT_SLOT))
            return 1;
        return wait_then(a, P_STORY_8, 0x20);
    case P_STORY_8:
        return story_turn(a, P_STORY_9);
    case P_STORY_9:
        PlayerMoveStart(a, 0x00);
        a->flip = 0;
        TaskCreate(TEXT_SLOT, 0x0B, TASK_ENTRY_STORY_TEXT1);
        return next_frame(a, P_STORY_10);
    case P_STORY_10:
        if (task_busy(TEXT_SLOT))
            return 1;
        return wait_then(a, P_STORY_11, 0x10);
    case P_STORY_11:
        g.story_part1 = 0x80;
        return next_frame(a, P_STORY_12);
    case P_STORY_12:
        if (!g.story_part3_done)
            return 1;
        return story_turn(a, P_STORY_13);
    case P_STORY_13:
        PlayerMoveStart(a, 0x00);
        a->flip++;
        return next_frame(a, P_STORY_14);
    default:                        /* P_STORY_14 */
        if (story_task()->param == 0x04)
            return 1;
        a->flags &= 0x3F;
        set_x_pixels(a, (uint16_t)((x_pixels(a) & 0x00FF) | 0x0100));
        return next_frame(a, P_STAY);
    }
}

int player_scene_code(Actor *a)
{
    switch (a->resume) {
    case PLAYER_AT_START_DROPPED:   return start_dropped(a);
    case PLAYER_AT_STORY:           return story_start(a);
    case PLAYER_AT_START_NEW_GAME:  return start_new_game(a);
    case PLAYER_AT_SCENE_WALK_ON:   return walk_on(a);
    case PLAYER_AT_SCENE_WALK_RIGHT:return walk_right(a);
    case PLAYER_AT_SCENE_JUMP_IN:   return jump_in(a);
    case PLAYER_AT_SCENE_WALK_OUT:  return walk_out(a);
    case P_JUMP_IN_WALK:            return jump_in_walk(a);
    case P_WALK_OUT_WALK:           return walk_out_walk(a);
    case P_WALK_OUT_CUE:            return walk_out_cue(a);
    case P_WALK_OUT_ON:             return walk_out_on(a);
    case P_WALK_OUT_FADE:           return walk_out_fade(a);
    case PLAYER_AT_SCENE_LAND:      return scene_land(a);
    case PLAYER_AT_SCENE_STAND:     return scene_stand(a);
    case PLAYER_AT_ENDING_RIDE:     return ending_ride(a);
    case P_DROPPED_FRAME:           return dropped_frame(a);
    case P_DROPPED_LANDED:          return PlayerGoTo(a, PLAYER_AT_STAND);
    case P_WALK_ON_FALL:            return walk_on_fall(a);
    case P_WALK_ON_CUED:            return walk_on_cued(a);
    case P_WALK_ON_WALK:            return walk_on_walk(a);
    case P_WALK_ON_STAND:           return walk_on_stand(a);
    case P_WALK_ON_POSE:            return walk_on_pose(a);
    case P_WALK_ON_END:             return walk_on_end(a);
    case P_WALK_RIGHT:              return walk_right_frame(a);
    case P_WALK_RIGHT_POSED:        return PlayerGoTo(a, PLAYER_AT_STAND);
    case P_LAND_FALL:               return land_fall(a);
    case P_LAND_STAND:              return land_stand(a);
    case P_LAND_WAIT_CUE:           return land_wait_cue(a);
    case P_LAND_TURN:               return land_turn(a);
    case P_LAND_BACK:               return land_back(a);
    case P_LAND_WALK:               return land_walk(a);
    case P_LAND_FADE:               return land_fade(a);
    case P_STAND_SCENE_DONE:
        g.player_anim_held++;
        return next_frame(a, P_STAY);
    case P_STAY:                    return 1;
    case P_WALK_ON_STAY:            return 0x80;
    case P_RIDE_FLY:                return ride_fly(a);
    case P_RIDE_BOB_1:              return ride_bob_1(a);
    case P_RIDE_BOB_2:              return ride_bob_2(a);
    case P_RIDE_BOB_3:              return ride_bob_3(a);
    case P_RIDE_T1:                 return ride_t1(a);
    case P_RIDE_T2:                 return ride_t2(a);
    case P_RIDE_T3:                 return ride_t3(a);
    case P_RIDE_T4:                 return ride_t4(a);
    case P_RIDE_T5:                 return ride_t5(a);
    case P_RIDE_T6:                 return ride_t6(a);
    default:
        if (a->resume >= P_STORY_1 && a->resume <= P_STORY_14)
            return story_step(a);
        return player_unconverted(a, "a resume point of scene.c");
    }
}
