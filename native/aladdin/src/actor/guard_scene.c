/*
 * guard_scene.c: the scripted figures of Aladdin (kind $08's records
 * started with $07 1-6, bank $A5's table at A5:E56D): the figure that runs,
 * waits and leaps (way 1), leaps and stays (2), hops for good (3), drops in
 * and plays a scene by its cues (4), walks off (5), and falls and poses (6).
 * Ways 1 and 6 end by starting over as way 0 at GuardRestart. They are
 * resume points of the guard's code (guard.h); GuardScene takes them on.
 * Their scene cues are g.scripted_cue ($146C), g.scene_cue_first ($00BD),
 * g.scene_cue ($00BE) and g.scene_cue_bf ($00BF).
 *
 * Walk off (way 5) tests, as its start, that the byte a guard's start kept
 * in cartridge RAM reads back as it was stored; a retail board has none
 * there and a read gives the bus's last byte, the address's high byte $70,
 * which is what the start kept ($71 less 1): so it always walks.
 */
#include <stdio.h>

#include "actor/guard.h"
#include "actor/kinds_d.h"

#define COUNT G_LANDING         /* +$42: frames left of a run or a walk */
#define LEAPS G_SHOWN           /* +$49: counted up as the running figure rears to leap */
#define FALL_MAX 0x0580         /* the fastest it falls (A5:EAB3 ActorFlyFallCapped) */
#define MARK_X 0x0F68           /* the walk-off figure's mark */

/*
 * @routine A5:EAB3 ActorFlyFallCapped
 * WHAT IT DOES: ActorFly, with the fall held to FALL_MAX at most. Only the
 * running figure's leap calls it.
 */
static void fly_fall_capped(Actor *a)
{
    ActorFly(a);
    if (!((a->speed_y >> 16) & 0x80) && (a->speed_y & 0xFFFF) > FALL_MAX)
        a->speed_y = (a->speed_y & ~0xFFFF) | FALL_MAX;
}

/* On as way 0 at GuardRestart ($07 0). */
static int start_over(Actor *a)
{
    a->spawn_byte = 0;
    a->resume = G_RESTART;
    return Kind08Start(a);
}

/* Ways 1: the run (animation 2, speed entries $15, 6). */
static int run_begin(Actor *a)
{
    AnimStart(a, 2);
    ActorSpeedXFromA5(a, 0x15);
    ActorSpeedYFromA5(a, 0x06);
    a->resume = G_SR_RUN;
    return 1;
}

/* Way 3: a hop's start (speed entries 6, $0C, animation 1, $30 frames). */
static int hop_begin(Actor *a)
{
    ActorSpeedYFromA5(a, 0x06);
    ActorSpeedXFromA5(a, 0x0C);
    AnimStart(a, 1);
    a->own.bytes[COUNT] = 0x30;
    a->resume = G_HP_RUN;
    return 1;
}

/* Way 5's walks: animation `anim`, speed entry `speed`, on to `next`. */
static int walk_foot(Actor *a, uint8_t anim, uint8_t speed, uint16_t next)
{
    AnimStart(a, anim);
    ActorSpeedXFromA5(a, speed);
    a->resume = next;
    return 1;
}

/* A walk frame: ActorMoveX, ActorSnapToGround. */
static void walk_step(Actor *a)
{
    ActorMoveX(a);
    (void)ActorSnapToGround(a, NULL);
}

/*
 * @routine A5:E9C0 GuardSceneEnd
 * WHAT IT DOES: The end of a scene: the actor goes on as GuardStop with its
 * +$08 low bits 1, +$41 and its state (+$4D) 0.
 */
static int scene_end(Actor *a)
{
    a->anim_flags = (uint8_t)((a->anim_flags & 0x03) | 0x01);
    a->own.bytes[OWN_41] = 0;
    a->own.bytes[G_STATE] = 0;
    return guard_stop(a);
}

/*
 * @routine A5:E96C GuardSceneCharge
 * WHAT IT DOES: Scene 1: the actor charges left by motion $88 for $C0
 * frames (animation $1B), flies by $87 while it rises, flies against by
 * $88 until it falls again, flies against 256 frames more, and ends the
 * scene (GuardSceneEnd).
 */
static int scene_charge(Actor *a)
{
    a->anim_flags |= 0x03;
    a->own.bytes[OWN_41]++;
    AnimStart(a, 0x1B);
    a->facing = a->flip = 1;
    a->own.bytes[COUNT] = 0xC0;
    ActorMotionPreset(a, 0x88);
    a->resume = G_SC_MOVE;
    return 1;
}

/*
 * @routine A5:E826 GuardSceneStart
 * WHAT IT DOES: Where kind $92 sends the escort: by the marker's $07 kept
 * in its +$47: 0 the walk out (A5:E835 GuardSceneWalkOut, not converted:
 * reported), 1 the charge, 2 and 3 the end.
 */
static int scene_start(Actor *a)
{
    switch (a->own.bytes[OWN_47]) {
    case 1:
        return scene_charge(a);
    case 2:
    case 3:
        return scene_end(a);
    default: {
        static int told;

        if (!told++)
            fprintf(stderr, "GuardSceneStart: scene %u (A5:E835 GuardSceneWalkOut) not converted\n", a->own.bytes[OWN_47]);
        a->resume = G_NOT_CONVERTED;
        return 1;
    }
    }
}

/*
 * @routine A5:EB31 PlayerSceneRuns
 * WHAT IT DOES: The scripted figure's way 1: it stands (animation 0, 64
 * frames), then runs along the ground (dropping off edges as it comes to
 * them) until it is more than 192 pixels right of the player; it is put at
 * spot $1C facing left until the player is well on (x $0B00), waits 16
 * frames, runs back 72 frames, rears (animation $11, 64 frames) and jumps
 * (animation $12), waits for the player to pass, and leaps again (animation
 * $0C, fall held to $0580); on landing it starts over as way 0.
 * Also A5:EEE1 PlayerSceneFallPose (way 6: it falls in pose $1C to the
 * ground, holds 192 frames, poses $1D for 144 and 128 more, and starts
 * over as way 0), A5:EC45 PlayerSceneLeap (way 2: stands 32 frames, runs
 * along the ground until it leaves it, leaps (animation $0C, speed entry
 * $12) until the scene's cue, holds animation $0D until the next, and
 * stays, no longer hittable), A5:EC9D PlayerSceneHops (way 3: drops to the
 * ground facing left in state 2, then for good runs 48 frames, stands 48,
 * poses $0A 16 and jumps up and lands again), A5:ED0F PlayerSceneDropIn
 * (way 4: drops, is put at spot $24 and leaps in, then plays its part by
 * the cues: posed while the player is hurt, a leap and fall once cue $BD
 * comes, a wait for the player's $07 to be $0D, a short walk, a pose until
 * cue $BE bit 7, and walks off for good) and A5:EE29 PlayerSceneWalkOff
 * (way 5: falls to the ground, walks right to x $0F68, poses, walks on
 * until 24 pixels past the player, turns to face left, and once the scene
 * says so walks on for good).
 */
int GuardScene(Actor *a)
{
    MapPoint p;

    switch (a->resume) {
    case G_SCENE_RUNS:
        AnimStart(a, 0);
        a->resume = G_SR_BEGIN;
        return 0x40;
    case G_SR_BEGIN:
        return run_begin(a);
    case G_SR_RUN:
        ActorMoveX(a);
        if (!ActorSnapToGround(a, &p) || (p.behaviour & 0x20)) {
            a->accel_y = 0x38;
            ActorSpeedYFromA5(a, 0x09);
            a->resume = G_SR_DROP;
            return 1;
        }
        ActorAnimByTile(a, p.behaviour);
        if ((uint16_t)(pixels(g.actors[0].x) + 0xC0) < pixels(a->x)) {
            ActorPlaceAtSpot(a, 0x1C);
            AnimStart(a, 0);
            a->facing = a->flip = 1;
            a->resume = G_SR_PLAYER;
        }
        return 1;
    case G_SR_DROP:
        ActorAnimRiseFall(a);
        ActorFly(a);
        if (!ActorOutOfGround(a))
            return 1;
        return run_begin(a);
    case G_SR_PLAYER:
        if ((pixels(g.actors[0].x) >> 8) < 0x0B)
            return 1;
        a->resume = G_SR_BACK;
        return 0x10;
    case G_SR_BACK:
        a->facing = a->flip = 0;
        a->own.bytes[COUNT] = 0x48;
        AnimStart(a, 1);
        ActorSpeedXFromA5(a, 0x0C);
        a->resume = G_SR_BACK_RUN;
        return 1;
    case G_SR_BACK_RUN:
        walk_step(a);
        if (--a->own.bytes[COUNT])
            return 1;
        a->own.bytes[LEAPS]++;
        AnimStart(a, 0x11);
        a->resume = G_SR_REAR;
        return 0x40;
    case G_SR_REAR:
        AnimStart(a, 0x12);
        a->accel_y = 0x08;
        ActorSpeedYFromA5(a, 0x0F);
        if (!((a->speed_y >> 16) & 0x80))
            a->accel_y = 0x18;
        a->resume = G_SR_JUMP;
        return 1;
    case G_SR_JUMP:
        ActorFall(a);
        if (ActorOutOfGround(a)) {
            AnimStart(a, 0x13);
            a->resume = G_SR_PASS;
            return 1;
        }
        if (!((a->speed_y >> 16) & 0x80))
            a->accel_y = 0x18;
        return 1;
    case G_SR_PASS:
        if ((uint16_t)(pixels(a->x) + 0x20) >= pixels(g.actors[0].x))
            return 1;
        AnimStart(a, 0x0C);
        a->accel_y = 0x38;
        ActorSpeedXFromA5(a, 0x03);
        ActorSpeedYFromA5(a, 0x0C);
        a->resume = G_SR_LEAP;
        return 1;
    case G_SR_LEAP:
        ActorAnimRiseFall(a);
        fly_fall_capped(a);
        (void)ActorOutOfWall8(a, &p);
        if (!ActorOutOfGround(a))
            return 1;
        AnimStart(a, 0);
        a->resume = G_SR_END;
        return 1;
    case G_SR_END:
        return start_over(a);

    case G_SCENE_FALLPOSE:
        AnimStart(a, 0x1C);
        a->resume = G_FP_FALL;
        return 1;
    case G_FP_FALL:
        ActorFall(a);
        if (!ActorOutOfGround(a))
            return 1;
        a->resume = G_FP_POSE;
        return 0xC0;
    case G_FP_POSE:
        AnimStart(a, 0x1D);
        a->resume = G_FP_HOLD;
        return 0x90;
    case G_FP_HOLD:
        a->resume = G_FP_END;
        return 0x80;
    case G_FP_END:
        return start_over(a);

    case G_SCENE_LEAP:
        AnimStart(a, 0);
        a->resume = G_LP_BEGIN;
        return 0x20;
    case G_LP_BEGIN:
        AnimStart(a, 2);
        ActorSpeedXFromA5(a, 0x0F);
        a->resume = G_LP_RUN;
        return 1;
    case G_LP_RUN:
        ActorMoveX(a);
        if (ActorSnapToGround(a, NULL))
            return 1;
        g.scripted_cue = 0;
        AnimStart(a, 0x0C);
        a->accel_y = 0x38;
        ActorSpeedYFromA5(a, 0x12);
        a->resume = G_LP_AIR;
        return 1;
    case G_LP_AIR:
        ActorFly(a);
        if (!g.scripted_cue)
            return 1;
        g.scripted_cue = 0;
        AnimStart(a, 0x0D);
        a->resume = G_LP_POSE;
        return 1;
    case G_LP_POSE:
        if (!g.scripted_cue)
            return 1;
        a->flags &= 0x7F;
        a->resume = G_LP_STAY;
        return 0x7F;
    case G_LP_STAY:
        return 0x7F;

    case G_SCENE_HOPS:
        a->own.bytes[G_STATE] = 0x02;
        a->facing = a->flip = 1;
        a->accel_y = 0x38;
        ActorSpeedYFromA5(a, 0x06);
        a->resume = G_HP_FALL;
        return 1;
    case G_HP_FALL:
    case G_HP_JUMP:
        ActorFall(a);
        if (!ActorOutOfGround(a))
            return 1;
        return hop_begin(a);
    case G_HP_RUN:
        walk_step(a);
        if (--a->own.bytes[COUNT])
            return 1;
        AnimStart(a, 0);
        a->resume = G_HP_STAND;
        return 0x30;
    case G_HP_STAND:
        AnimStart(a, 0x0A);
        a->resume = G_HP_POSE;
        return 0x10;
    case G_HP_POSE:
        AnimStart(a, 0);
        a->accel_y = 0x38;
        ActorSpeedYFromA5(a, 0x0C);
        a->resume = G_HP_JUMP;
        return 1;

    case G_SCENE_WALKOFF:
        if (ActorOutOfGround(a)) {
            a->facing = a->flip = 0;
            return walk_foot(a, 1, 0, G_WO_MARK);
        }
        a->resume = G_WO_FALL;
        return 1;
    case G_WO_FALL:
        ActorFly(a);
        if (!ActorOutOfGround(a))
            return 1;
        a->facing = a->flip = 0;
        return walk_foot(a, 1, 0, G_WO_MARK);
    case G_WO_MARK:
        walk_step(a);
        if (pixels(a->x) < MARK_X)
            return 1;
        g.scene_cue_bf++;
        AnimStart(a, 6);
        a->resume = G_WO_POSE1;
        return 0x30;
    case G_WO_POSE1:
        AnimStart(a, 0x1E);
        a->resume = G_WO_POSE2;
        return 0x20;
    case G_WO_POSE2:
        return walk_foot(a, 2, 0x12, G_WO_PLAYER);
    case G_WO_PLAYER:
        walk_step(a);
        if ((uint16_t)(pixels(g.actors[0].x) + 0x18) >= pixels(a->x))
            return 1;
        g.scene_cue++;
        AnimStart(a, 4);
        a->flip = 1;
        a->resume = G_WO_TURN;
        return 0x80;
    case G_WO_TURN:
        a->flip = 0;
        AnimStart(a, 0);
        a->resume = G_WO_CUE;
        return 1;
    case G_WO_CUE:
        if (!g.scene_cue_first)
            return 1;
        a->resume = G_WO_AFTER;
        return 0x10;
    case G_WO_AFTER:
        return walk_foot(a, 1, 0, G_WO_FOREVER);
    case G_WO_FOREVER:
        walk_step(a);
        return 1;

    case G_SCENE_DROPIN:
        ActorFall(a);
        if (ActorOutOfGround(a)) {
            AnimStart(a, 0);
            a->resume = G_DI_SPOT;
            return 0x80;
        }
        a->resume = G_DI_FALL;
        return 1;
    case G_DI_FALL:
        ActorAnimRiseFall(a);
        ActorFall(a);
        if (!ActorOutOfGround(a))
            return 1;
        AnimStart(a, 0);
        a->resume = G_DI_SPOT;
        return 0x80;
    case G_DI_SPOT:
        ActorPlaceAtSpot(a, 0x24);
        a->facing = a->flip = 0;
        a->anim_flags = (uint8_t)((a->anim_flags & 0xFC) | 0x03);
        AnimStart(a, 0x0C);
        ActorMotionPreset(a, 0x86);
        a->resume = G_DI_LEAP;
        return 1;
    case G_DI_LEAP:
        ActorAnimRiseFall(a);
        ActorFly(a);
        if (!ActorOutOfGround(a))
            return 1;
        AnimStart(a, 0);
        a->resume = G_DI_POSE_ENTRY;
        return 0x30;
    case G_DI_POSE_ENTRY:
        AnimStart(a, 0x17);
        a->resume = G_DI_POSE;
        return 1;
    case G_DI_POSE:
        if (g.player_hurt_count) {
            AnimStart(a, 0x18);
            a->resume = G_DI_HURT;
            return 1;
        }
        a->flip = g.scripted_cue;
        if (!g.scene_cue_first)
            return 1;
        a->facing = a->flip = 0;
        ActorMotionPreset(a, 0x7B);
        a->resume = G_DI_LEAP2;
        return 1;
    case G_DI_HURT:
        if (g.player_hurt_count)
            return 1;
        AnimStart(a, 0x17);
        a->resume = G_DI_POSE;
        return 1;
    case G_DI_LEAP2:
        ActorAnimRiseFall(a);
        ActorFly(a);
        if ((a->speed_y >> 16) & 0x80)
            return 1;
        a->anim_flags = (uint8_t)((a->anim_flags & 0xFC) | 0x01);
        a->resume = G_DI_FALL2;
        return 1;
    case G_DI_FALL2:
        ActorAnimRiseFall(a);
        ActorFly(a);
        if (!ActorOutOfGround(a))
            return 1;
        AnimStart(a, 0x19);
        a->resume = G_DI_P1;
        return 0x50;
    case G_DI_P1:
        AnimStart(a, 0);
        a->resume = G_DI_WAIT07;
        return 1;
    case G_DI_WAIT07:
        if (g.actors[0].spawn_byte != 0x0D)
            return 1;
        AnimStart(a, 1);
        ActorSpeedXFromA5(a, 0);
        a->own.bytes[COUNT] = 0x40;
        a->resume = G_DI_WALK;
        return 1;
    case G_DI_WALK:
        ActorMoveX(a);
        if (--a->own.bytes[COUNT])
            return 1;
        AnimStart(a, 0);
        a->resume = G_DI_WAIT_BE;
        return 1;
    case G_DI_WAIT_BE:
        if (!(g.scene_cue & 0x80))
            return 1;
        AnimStart(a, 0x1A);
        a->resume = G_DI_P2;
        return 0x80;
    case G_DI_P2:
        ActorSpeedXFromA5(a, 0x0F);
        AnimStart(a, 2);
        a->own.bytes[COUNT] = 0x28;
        a->resume = G_DI_WALK2;
        return 1;
    case G_DI_WALK2:
        ActorMoveX(a);
        if (--a->own.bytes[COUNT])
            return 1;
        ActorSpeedXFromA5(a, 0x0C);
        AnimStart(a, 1);
        a->resume = G_DI_FOREVER;
        return 1;
    case G_DI_FOREVER:
        ActorMoveX(a);
        return 1;
    case G_SCENE_START:
        return scene_start(a);
    case G_SC_MOVE:
        ActorMove(a);
        if (--a->own.bytes[COUNT])
            return 1;
        ActorMotionPreset(a, 0x87);
        a->resume = G_SC_FLY;
        return 1;
    case G_SC_FLY:
        ActorFly(a);
        if (a->speed_y < 0)
            return 1;
        ActorMotionPreset(a, 0x88);
        a->resume = G_SC_AGAINST;
        return 1;
    case G_SC_AGAINST:
        ActorFlyAgainst(a);
        if (a->speed_y >= 0)
            return 1;
        a->own.bytes[COUNT] = 0;
        a->resume = G_SC_AGAINST_WAIT;
        return 1;
    case G_SC_AGAINST_WAIT:
        ActorFlyAgainst(a);
        if (--a->own.bytes[COUNT])
            return 1;
        return scene_end(a);
    default:
        return 1;
    }
}
