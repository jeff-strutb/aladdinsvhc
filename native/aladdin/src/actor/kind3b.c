/*
 * kind3b.c: actor kind $3B, by its +$07 a switch, a marker of a level
 * flag, or a platform that carries the player: across, up and down, or
 * along a track drawn in the map; and its wait.
 */
#include <stdio.h>

#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "data/kinds_e_tables.h"
#include "level/level.h"
#include "map/map.h"
#include "player/player.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: frames left; the switch: how far below its rest it is (its wait sets it) */
    uint8_t run;                /* +$41: frames of a run after the first */
    uint8_t toggled;            /* +$42: the switch: nonzero once it toggled; the marker: the flag shown */
    uint8_t b43;
    int8_t state;               /* +$44: the switch: 0 up, positive down, negative coming back up (ActorLiftPlayerStep's step) */
    uint8_t quarter;            /* +$45: the track: the quarter's high byte, then the direction after a spot */
    uint8_t turn;               /* +$46: the track: added to the direction at each step to a spot */
} Kind3BOwn;

/* past +$49, by OwnPlace (+$4A and +$4B are own's last bytes) */
#define REST_Y_LOW OWN_4A               /* +$4A: the switch: its y pixels at rest (word) */
#define REST_Y_HIGH OWN_4B
#define BOX OWN_4C                      /* +$4C: ActorTestCarries' box */

_Static_assert(sizeof(Kind3BOwn) <= OWN_BYTES, "a view fits the own bytes");

static inline Kind3BOwn *own(Actor *a) { return (Kind3BOwn *)a->own.bytes; }
static uint16_t rest_y(Actor *a) { return (uint16_t)(a->own.bytes[REST_Y_LOW] | a->own.bytes[REST_Y_HIGH] << 8); }

#define SWITCH 0x10             /* +$07 bit 4: a switch */
#define MARKER 0x08             /* +$07 bit 3: a marker */

enum {
    SWITCH_FRAME = ACTOR_RESUME_OWN,    /* the switch: each frame */
    MARKER_FRAME,                       /* the marker: each frame */
    MARKER_CLEAR,                       /* after its 16 frames on a marked flag */
    P0_GO,                              /* part 0: after its first frame */
    P0_MOVE,                            /* each frame of a run */
    P0_REST,                            /* each of its 12 frames at an end */
    P1_GO,
    P1_MOVE,
    P1_REST,
    TRACK_ON,                           /* part 2: each of 8 steps, then the next */
    TRACK_BACK,                         /* 8 steps, then back */
    TRACK_TURN,                         /* 8 steps, then a turn */
    TRACK_REST,                         /* each of 8 frames, then the next step */
    TRACK_FRAME,                        /* a frame, then the next step */
    SPOT_1,                             /* to a spot: the first of three frames of a turn */
    SPOT_2,
    SPOT_3,
    STAY,
};

/* the level flag of its +$07's low 3 bits */
static uint8_t *flag_of(Actor *a) { return &g.level_switch_flags[a->spawn_byte & 0x07]; }

/* the switch's frame (its wait has set +$40 and +$44 by then) */
static int switch_frame(Actor *a)
{
    Kind3BOwn *o = own(a);

    if (o->state == 0) {
        /* up: Aladdin, off the ground, comes into its zone $70 */
        if (g.player_in_air && ActorZoneHasTestPoint(a, 0x70)) {
            g.actors[0].resume = PLAYER_AT_SWITCH_HANG;
            PlayerSpotBelowActor(a, 0x03);
            o->state++;
        }
    } else if (o->state > 0) {
        /* down: comes back up when he no longer holds it; else, far enough
           down, toggles its flag once (unless an actor has marked it) */
        if (!(uint8_t)g.player_no_actor_landing) {
            o->state = (int8_t)0xFC;
        } else if (!o->toggled && o->frames >= 0x0F && !(*flag_of(a) & 0x80)) {
            *flag_of(a) ^= 0x01;
            o->toggled++;
        }
    } else {
        o->state++;
    }
    if (!o->frames)
        o->toggled = 0;
    return 1;
}

/* the marker's frame: a flag an actor marked (bit 7) is cleared of its high
   bits after 16 frames; else a change of the flag shows as pose 2 + flag */
static int marker_frame(Actor *a)
{
    uint8_t f = *flag_of(a);

    if (f & 0x80) {
        a->resume = MARKER_CLEAR;
        return 0x10;
    }
    if (f != own(a)->toggled) {
        own(a)->toggled = f;
        AnimStart(a, (uint8_t)(f + 2));
    }
    a->resume = MARKER_FRAME;
    return 1;
}

/* `n` frames from the next, at `at` */
static int frames_of(Actor *a, uint8_t n, int at)
{
    own(a)->frames = n;
    a->resume = (uint16_t)at;
    return 1;
}

/* the speed down turned round: its low word negated, its top byte EOR $FF
   (not a 24-bit negation when the low word is 0, as the original) */
static void speed_y_turn(Actor *a)
{
    uint32_t v = wrap24((uint32_t)a->speed_y);
    uint32_t low = (uint32_t)(-(int32_t)(v & 0xFFFF)) & 0xFFFF;
    uint32_t top = ~(v >> 16) & 0xFF;

    a->speed_y = signed24(top << 16 | low);
}

/* the target word ($1A, $1E: the speeds' pixel words) ActorStepToTargetCarry steps to */
static uint16_t target_x(const Actor *a) { return pixels((uint32_t)a->speed_x); }
static uint16_t target_y(const Actor *a) { return pixels((uint32_t)a->speed_y); }
static void set_target(Actor *a, uint16_t x, uint16_t y)
{
    a->speed_x = signed24(((uint32_t)a->speed_x & 0xFF) | (uint32_t)x << 8);
    a->speed_y = signed24(((uint32_t)a->speed_y & 0xFF) | (uint32_t)y << 8);
}

static int track_step(Actor *a);

/*
 * @routine 83:C1EC ActorStepCarry
 * WHAT IT DOES: Moves the actor a step along its facing (vector set $0E)
 * and carries Aladdin riding it along. Kind $3B is its only caller
 * (player_carry.c's record; its piece).
 */
static void step_carry(Actor *a)
{
    ActorMoveByVector(a, 0x0E);
    ActorCarryPlayer(a);
}

/*
 * @routine 83:C1F6 ActorStepToTargetCarry
 * WHAT IT DOES: Moves the actor a step toward its target (the speeds'
 * pixel words, the original's $1A and $1E) by vector set 8 on each axis it
 * has not reached, then carries Aladdin riding it along. Kind $3B is its
 * only caller (player_carry.c's record; its piece).
 */
static void step_to_target_carry(Actor *a)
{
    if (pixels(a->x) != target_x(a))
        ActorMoveByVectorX(a, 0x08);
    if (pixels(a->y) != target_y(a))
        ActorMoveByVectorY(a, 0x08);
    ActorCarryPlayer(a);
}

/* part 2's travel to a flagged spot at (x, y) (case 3) */
static int to_spot(Actor *a, uint16_t x, uint16_t y)
{
    Kind3BOwn *o = own(a);
    int flag = LevelFlagSpotFind(x, y);
    uint8_t old, at;

    set_target(a, x, y);
    if (flag < 0) {
        static int told;

        /* the original tries again in the same frame for good */
        if (!told++)
            fprintf(stderr, "Kind3BPart2: a track spot not in the level's list (the original never goes on)\n");
        a->resume = STAY;
        return 1;
    }
    old = g.level_switch_flags[flag];
    g.level_switch_flags[flag] = (uint8_t)(LevelFlagFromMask((uint8_t)flag) | 0x80);
    at = (uint8_t)(a->facing << 1 | (old & 0x01));
    o->turn = k3b_spot_turn(at);
    o->quarter = k3b_spot_dir(at);
    set_target(a, (uint16_t)(target_x(a) + k3b_ahead_x((uint8_t)(o->quarter << 2))),
               (uint16_t)(target_y(a) + k3b_ahead_y((uint8_t)(o->quarter << 2))));
    a->facing = (uint8_t)(a->facing << 2);
    return frames_of(a, 0x05, SPOT_1);
}

/*
 * @routine 83:C0D7 Kind3BPart2
 * WHAT IT DOES: Kind $3B's platform part 2: follows a track drawn in the
 * map, carrying the player: at each step it reads the map quarter ahead in
 * its direction (track_step) and by its low bits goes on 8 steps (0, 1),
 * goes on and turns back (4, 5), goes on and turns by the quarter's high
 * bits (2), waits a frame (6, 7), or at a flagged spot (3) marks the level
 * flag there and travels to the spot's target in 5 turns of three frames,
 * turning as it goes. It starts facing Aladdin, its direction doubled.
 */
static int track_step(Actor *a)
{
    uint8_t at = (uint8_t)(a->facing << 2);
    uint16_t x = (uint16_t)(k3b_ahead_x(at) + pixels(a->x));
    uint16_t y = (uint16_t)(k3b_ahead_y(at) + pixels(a->y));
    uint16_t q = MapQuarterAt(x, y);

    own(a)->quarter = (uint8_t)(q >> 8);
    switch (q & 0x07) {
    case 0:
    case 1:
        return frames_of(a, 8, TRACK_ON);
    case 4:
    case 5:
        return frames_of(a, 8, TRACK_BACK);
    case 2:
        return frames_of(a, 8, TRACK_TURN);
    case 3:
        return to_spot(a, x, y);
    default:                            /* 6, 7 */
        a->resume = TRACK_FRAME;
        return 1;
    }
}

/* one of part 2's 8 steps; nonzero when they are done */
static int track_8(Actor *a)
{
    step_carry(a);
    return --own(a)->frames == 0;
}

/*
 * @routine 83:BF83 Kind3BStart
 * WHAT IT DOES: Actor kind $3B, by its +$07: with bit 4 a switch Aladdin
 * works by coming into its zone (his code sent to PlayerSwitchHang and he
 * is placed under it; the level flag of its +$07's low bits toggled when it
 * is down far enough); with bit 3 a marker that shows that level flag
 * (pose 2 plus the flag) and clears its high bits 16 frames after an actor
 * marks it; else a platform he rides (pose 4), one of three parts by its
 * +$07's low bits: Kind3BPart0, Kind3BPart1, Kind3BPart2. Its start sets
 * +$08 bit 1, its wait routine on, its facing and +$44 0. Its second entry
 * (83:C2A5) ends it.
 */
int Kind3BStart(Actor *a)
{
    Kind3BOwn *o = own(a);

    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        a->facing = 0;
        a->flip = 0;
        o->state = 0;
        if (a->spawn_byte & SWITCH) {
            AnimStart(a, 0x00);
            a->own.bytes[REST_Y_LOW] = (uint8_t)pixels(a->y);
            a->own.bytes[REST_Y_HIGH] = (uint8_t)(pixels(a->y) >> 8);
            o->state = 0;
            o->frames = 0;
            o->toggled = 0;
            a->resume = SWITCH_FRAME;
            return 1;
        }
        if (a->spawn_byte & MARKER) {
            o->toggled = 0xFF;
            a->anim_flags |= 0x03;
            return marker_frame(a);
        }
        AnimStart(a, 0x04);
        switch (a->spawn_byte & 0x03) {
        case 0:
            a->resume = P0_GO;
            return 1;
        case 1:
            a->resume = P1_GO;
            return 1;
        case 2:
            ActorFacePlayer(a);
            a->facing = (uint8_t)(a->facing << 1);
            return track_step(a);
        default: {
            static int told;

            if (!told++)
                fprintf(stderr, "Kind3BStart: platform part 3 (past its table) not converted\n");
            a->resume = STAY;
            return 1;
        }
        }
    case ACTOR_START_SECOND:
        return ActorEndPlaced(a);
    case SWITCH_FRAME:
        return switch_frame(a);
    case MARKER_FRAME:
        return marker_frame(a);
    case MARKER_CLEAR:
        *flag_of(a) &= 0x0F;
        a->resume = MARKER_FRAME;
        return 1;
    /*
     * @routine 83:C05E Kind3BPart0
     * WHAT IT DOES: Kind $3B's platform part 0: after a frame it moves
     * away from Aladdin's facing (speed $0C) for 32 frames carrying him,
     * rests 12 frames, turns, and again for 64 frames, back and forth for
     * good.
     */
    case P0_GO:
        a->facing = (uint8_t)(g.actors[0].facing ^ 0x01);
        o->run = 0x40;
        ActorSpeedXSet(a, 0x0C);
        return frames_of(a, 0x20, P0_MOVE);
    case P0_MOVE:
        ActorMoveX(a);
        ActorCarryPlayer(a);
        if (--o->frames)
            return 1;
        a->facing ^= 0x01;
        return frames_of(a, 0x0C, P0_REST);
    case P0_REST:
        if (--o->frames)
            return 1;
        return frames_of(a, o->run, P0_MOVE);
    /*
     * @routine 83:C098 Kind3BPart1
     * WHAT IT DOES: Kind $3B's platform part 1: after a frame it moves up
     * and down (speed $1E), carrying Aladdin: 32 frames one way first, then
     * 64 each way, resting 12 frames at each end.
     */
    case P1_GO:
        o->run = 0x40;
        ActorSpeedYSet(a, 0x1E);
        return frames_of(a, 0x20, P1_MOVE);
    case P1_MOVE:
        ActorMoveY(a);
        ActorCarryPlayer(a);
        if (--o->frames)
            return 1;
        speed_y_turn(a);
        return frames_of(a, 0x0C, P1_REST);
    case P1_REST:
        if (--o->frames)
            return 1;
        return frames_of(a, o->run, P1_MOVE);
    case TRACK_ON:
        if (!track_8(a))
            return 1;
        return track_step(a);
    case TRACK_BACK:
        if (!track_8(a))
            return 1;
        a->facing ^= 0x02;
        return frames_of(a, 8, TRACK_REST);
    case TRACK_TURN: {
        uint8_t at;

        if (!track_8(a))
            return 1;
        /* the quarter's bits 7 and 6, then the direction's bit 0 */
        at = (uint8_t)((o->quarter >> 7) << 2 | ((o->quarter >> 6) & 1) << 1 | (a->facing & 1));
        o->quarter = (uint8_t)(o->quarter << 2);
        a->facing = k3b_turn(at);
        return frames_of(a, 8, TRACK_REST);
    }
    case TRACK_REST:
        if (--o->frames)
            return 1;
        return track_step(a);
    case TRACK_FRAME:
        return track_step(a);
    case SPOT_1:
    case SPOT_2:
        step_to_target_carry(a);
        a->resume++;
        return 1;
    case SPOT_3:
        step_to_target_carry(a);
        a->facing = (uint8_t)((a->facing + o->turn) & 0x0F);
        if (--o->frames) {
            a->resume = SPOT_1;
            return 1;
        }
        a->x &= ~0xFFu;
        a->y &= ~0xFFu;
        a->facing = o->quarter;
        return track_step(a);
    default:                            /* STAY */
        return 1;
    }
}

/*
 * @routine 83:C21C Kind3BWait
 * WHAT IT DOES: Kind $3B's wait, by its +$07: bit 4 (the switch), how far
 * below its rest it is into +$40 (above it: put back at it, 0); while it
 * lifts Aladdin (+$44 positive), below $10 it goes down (speed $5D) and
 * keeps him under it, else while below its rest it rises (speed $5A); then
 * its animation, and it is freed once out of range (box $18). Bit 3 (the
 * marker): its animation; freed out of range, it sets its level flag from
 * the level's mask. Else (the platforms): it lifts him when he lands on it
 * (box $1E, test 2); part 2 stays while another actor shares its tile slot;
 * freed out of range.
 */
int Kind3BWait(Actor *a)
{
    Kind3BOwn *o = own(a);

    if (a->spawn_byte & SWITCH) {
        uint16_t y = pixels(a->y);

        if (y < rest_y(a)) {
            a->y = with_pixels(a->y, rest_y(a));
            o->frames = 0;
        } else {
            o->frames = (uint8_t)(y - rest_y(a));
        }
        if (o->state > 0) {
            if (o->frames < 0x10) {
                ActorSpeedYSet(a, 0x5D);
                ActorMoveY(a);
                PlayerToActorBelow(a, 0x03);
            }
        } else if (o->frames) {
            ActorSpeedYSet(a, 0x5A);
            ActorMoveY(a);
        }
        AnimStep(a);
        return ActorFreeOutOfRange(a, 0x18) ? ACTOR_ENDED : ACTOR_GOES_ON;
    }
    if (a->spawn_byte & MARKER) {
        AnimStep(a);
        if (!ActorFreeOutOfRange(a, 0x18))
            return ACTOR_GOES_ON;
        (void)LevelFlagFromMask((uint8_t)(a->spawn_byte & 0x07));
        return ACTOR_ENDED;
    }
    a->own.bytes[BOX] = 0x1E;
    ActorTestCarries(a, 0x02);
    ActorLiftPlayerStep(a, 0x02);
    if (a->spawn_byte == 2) {
        /* the users of its slot ($15E8 + slot); with no slot the byte
           before the four, which nothing writes: 0 */
        uint8_t users = a->tile_slot ? g.palette_slot_users[a->tile_slot - 1] : 0;

        if (users != 1)
            return ACTOR_GOES_ON;
    }
    return ActorFreeOutOfRange(a, 0x18) ? ACTOR_ENDED : ACTOR_GOES_ON;
}
