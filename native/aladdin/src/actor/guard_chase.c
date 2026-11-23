/*
 * guard_chase.c: actor kind $08's chase: keeping pace with the player,
 * and jumping up or down to a ledge when he is well above or below
 * (GuardLedgeJump), from its wait routine. It sends the guard's code to a
 * move by its resume point (the original's code word at its +$03) and
 * marks the move it sent it to in g.screen_signal (the original's $146B).
 */
#include "actor/guard.h"
#include "actor/kinds_d.h"
#include "data/kinds_d_tables.h"
#include "video/video.h"
#include "system/runahead.h"

/* $146E: nonzero, one close step keeps pace instead of slowing (only the
   chase reads it, and nothing in the record sets it) */
static uint8_t pace_once;
RUNAHEAD_KEEP(pace_once)

static uint16_t speed16(const Actor *a) { return (uint16_t)(a->speed_x & 0xFFFF); }

/*
 * @routine A5:F284 ActorLedgeAbove
 * WHAT IT DOES: Looks for the top of the ground at a point offset from it
 * (`dy` down, `dx` along its facing), up a tile at a time: true when a
 * ground tile with an empty tile above it is found within `probes`
 * probes. `*left` gets the probes left, `p` the last point. Only the
 * guard calls it.
 */
static bool ledge_above(Actor *a, uint16_t dy, uint8_t probes, uint16_t dx, MapPoint *p, uint8_t *left)
{
    uint16_t n = probes;

    p->y = (uint16_t)(dy + pixels(a->y));
    p->x = (uint16_t)((a->facing ? (uint16_t)-dx : dx) + pixels(a->x));
    for (;;) {
        (void)PointInGround(a, p);
        if (p->behaviour & 0x07)
            break;
        if (!--n) {
            *left = 0;
            return false;
        }
        p->y = (uint16_t)(p->y - 0x10);
    }
    for (;;) {
        p->y = (uint16_t)(p->y - 0x10);
        (void)PointInGround(a, p);
        if (!(p->behaviour & 0x07)) {
            *left = (uint8_t)n;
            return true;
        }
        n = (uint16_t)((n & 0xFF00) | (uint8_t)(n - 1));
        if (!(uint8_t)n)
            break;
    }
    *left = (uint8_t)n;
    return false;
}

/*
 * @routine A5:F242 ActorProbeUpTowardPlayer
 * WHAT IT DOES: Probes up from a point offset from it toward the player
 * (`dy` down, `dx` his way), a tile at a time for `probes` probes, for a
 * tile whose behaviour has bit 3 or 6: true when one is found. `*left`
 * gets the probes left, `p` the last point. Only the guard calls it.
 */
static bool probe_up_toward_player(Actor *a, uint16_t dy, uint8_t probes, uint16_t dx, MapPoint *p, uint8_t *left)
{
    uint16_t n = probes;

    p->y = (uint16_t)(dy + pixels(a->y));
    if (pixels(g.actors[0].x) < pixels(a->x))
        dx = (uint16_t)-dx;
    p->x = (uint16_t)(dx + pixels(a->x));
    for (;;) {
        (void)PointInGround(a, p);
        if (p->behaviour & 0x48) {
            *left = (uint8_t)n;
            return true;
        }
        p->y = (uint16_t)(p->y - 0x10);
        if (!--n) {
            *left = 0;
            return false;
        }
    }
}

/*
 * @routine A5:F19D GuardLedgeJump
 * WHAT IT DOES: When the player is well above or below the guard, picks a
 * ledge toward him and starts the guard's jump to it: true when it jumps
 * (its caller, GuardChase, ends). His y + 16 less the guard's: above by
 * more than 128, or by 65-128 when a random pick (list $25) allows,
 * ActorProbeUpTowardPlayer (48 up, 5 probes, 24 across); found with more
 * than 2 probes left, or on a tile with behaviour bit 3: its code to
 * GuardJump, facing him, motion preset by the probes left ($80:D054), on a
 * bit-3 tile its speed + $80. Below with +$44 set (then cleared): by less
 * than 64 only on a random pick (list $26); ActorLedgeAbove (96 down, 5
 * probes, 32 ahead); found: its code to GuardJump, preset $83.
 */
static bool ledge_jump(Actor *a)
{
    uint16_t d = (uint16_t)(pixels(g.actors[0].y) + 0x10 - pixels(a->y));
    MapPoint p;
    uint8_t left;

    if (d & 0x8000) {
        if (d >= 0xFFC0)
            return false;
        if (d >= 0xFF80 && !(RandomFromList(0x4A >> 1) & 0xFF))
            return false;
        if (!probe_up_toward_player(a, 0xFFD0, 0x05, 0x18, &p, &left))
            return false;
        if (left <= 0x02 && !(p.behaviour & 0x08))
            return false;
        a->resume = G_JUMP;
        a->facing = a->flip = pixels(a->x) >= pixels(g.actors[0].x) ? 1 : 0;
        ActorMotionPreset(a, k08_jump_motion(left));
        if (p.behaviour & 0x08)
            a->speed_x = (int32_t)((a->speed_x & ~0xFFFF) | (uint16_t)(speed16(a) + 0x80));
        return true;
    }
    if (!a->own.bytes[G_IN_AIR])
        return false;
    a->own.bytes[G_IN_AIR] = 0;
    if (d < 0x40 && !(RandomFromList(0x4C >> 1) & 0xFF))
        return false;
    if (!ledge_above(a, 0x0060, 0x05, 0x20, &p, &left))
        return false;
    a->resume = G_JUMP;
    ActorMotionPreset(a, 0x83);
    return true;
}

/* A chase step's new speed: the run marked and its code sent to GuardRun
   (once), the speed's top byte cleared, facing toward him. */
static void store(Actor *a, uint16_t speed)
{
    a->speed_x = speed;
    if (g.screen_signal != 0x02) {
        g.screen_signal = 0x02;
        a->resume = G_RUN;
    }
    a->facing = a->flip = pixels(a->x) >= pixels(g.actors[0].x) ? 1 : 0;
}

/* Toward his speed by 8, or $C0 when he is slower than that. */
static void keep_pace(Actor *a)
{
    uint16_t pace = (uint16_t)(g.actors[0].speed_x & 0xFFFF), speed = speed16(a);

    if (0x00C0 >= pace) {
        store(a, 0x00C0);
        return;
    }
    if (speed < pace) {
        store(a, (uint16_t)(speed + 8));
        return;
    }
    speed = (uint16_t)(speed - 8);
    store(a, (speed & 0x8000) ? 0 : speed);
}

/*
 * @routine A5:F127 GuardChaseSpeedUp
 * WHAT IT DOES: GuardChase's speed-up step (also GuardRunOff's): faster by
 * 8; above $300, $300; then on as each new speed.
 */
void guard_chase_speed_up(Actor *a)
{
    uint16_t speed = speed16(a);

    store(a, 0x0300 < speed ? 0x0300 : (uint16_t)(speed + 8));
}

/*
 * @routine A5:F172 GuardChaseClose
 * WHAT IT DOES: GuardChase's close step (the player within 48; also
 * GuardHoldLeft's and GuardHoldRight's): once after a pace kept, it keeps
 * his pace; a moving guard slows by $30 (to 0); a stopped one is sent to
 * GuardStop (marked 1, once).
 */
void guard_chase_close(Actor *a)
{
    uint16_t speed = speed16(a);

    if (pace_once) {
        pace_once = 0;
        keep_pace(a);
        return;
    }
    if (!speed) {
        if (g.screen_signal != 0x01) {
            g.screen_signal = 0x01;
            a->resume = G_STOP;
        }
        return;
    }
    speed = (uint16_t)(speed - 0x30);
    store(a, (speed & 0x8000) ? 0 : speed);
}

/*
 * @routine A5:F0F4 GuardChase
 * WHAT IT DOES: The guard's chase step: GuardLedgeJump; then its speed
 * and facing by how far the player is ahead of it: within 48, the close
 * step; within 96, keeping his pace; within 368, speeding up; farther,
 * unless +$49 is set, it gives up (its code to GuardGiveUp, marked $40).
 */
void guard_chase(Actor *a)
{
    uint16_t d;

    if (ledge_jump(a))
        return;
    d = (uint16_t)(pixels(g.actors[0].x) - pixels(a->x) + 0x30);
    if (d < 0x0060) {
        guard_chase_close(a);
        return;
    }
    d = (uint16_t)(d + 0x30);
    if (d < 0x00C0) {
        keep_pace(a);
        return;
    }
    d = (uint16_t)(d + 0x0110);
    if (d >= 0x02E0 && !a->own.bytes[G_SHOWN]) {
        g.screen_signal = 0x40;
        a->resume = G_GIVE_UP;
        return;
    }
    guard_chase_speed_up(a);
}
