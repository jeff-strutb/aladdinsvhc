/*
 * solid_actor.c: an actor Aladdin cannot pass through: he stands on its
 * top, bumps his head on its bottom, or is pushed out of its sides.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"

#define SOLID_BOXES 0xBD66u         /* by Y: half-width, half-height (words, the DB bank) */
#define PLAYER_X_PIXELS (PLAYER + ACTOR_X + 1)
#define PLAYER_Y_PIXELS (PLAYER + ACTOR_Y + 1)
#define PLAYER_SPEED_Y (PLAYER + ACTOR_SPEED_Y) /* fraction, then whole (word) */
#define PLAYER_X_BEFORE 0x00143Bu   /* his x pixels before this frame's move */
#define PLAYER_ON_ACTOR 0x001428u   /* nonzero: he stands on an actor */
#define V_141C 0x00141Cu            /* zero: he does not bump his head */
#define HEALTH 0x000367u            /* the player's health */

/* His offset from the actor in one axis plus `half`, against `whole`:
   nonzero when within. */
static int inside(uint16_t mine, uint16_t his, uint16_t half, uint16_t whole, int plus_one)
{
    uint16_t d;

    flag_set(P_C, 1);
    d = op_sbc16(mine, his, &cpu.p);
    if (plus_one)
        d = (uint16_t)(d + 1);
    flag_set(P_C, 0);
    d = op_adc16(d, half, &cpu.p);
    cpu.a = d;
    cmp16_of(d, whole);
    return d < whole;
}

/*
 * @routine 83:865E ActorSolidForPlayer
 * WHAT IT DOES: Keeps Aladdin out of a solid actor: coming down onto its
 * top he stands on it, rising into its bottom he stops, otherwise he is
 * pushed out of the side he came from.
 * HOW: Nothing (C set, V clear) when the actor cannot be hit ($09 bit 6
 * clear) or he already stands on an actor. The box at $BD66 plus Y gives
 * half-width and half-height (scratch $000C-$0013, the height's less 8
 * from $0014, doubled at $0016). His x before the move inside the box and
 * his y inside it but above its lower part: on top (y one above the box's
 * top, unless he is rising; with no health 4 pixels lower), $1428 counted
 * up and $141C cleared (C, V clear); below its middle while rising and
 * $141C nonzero: under it, his speed cleared (C, V set); otherwise C and
 * V set. Else, his x now inside the box and his y inside its lower part:
 * his x put just outside the side he is on (C set, V clear).
 * Entry: M 8-bit, X 8-bit; Y = the box's offset; D = the actor; DB a bank
 * whose $0000-$1FFF is work RAM, the boxes' bank; called by JSR. Exit:
 * M and X 8-bit (M 8-bit, X as on entry when nothing is tested); C and V
 * as above; A, N, Z from the last test or store.
 */
void abi_ActorSolidForPlayer(void)
{
    uint32_t box = dba(SOLID_BOXES) + cpu.y;
    uint16_t half, low, ay, ax;
    uint8_t flags = rd8(dpa(ACTOR_09)), v;

    bit8_of((uint8_t)cpu.a, flags);
    if (!(flags & 0x40))
        goto none;
    a8_load(rd8(dba(PLAYER_ON_ACTOR)));
    if ((uint8_t)cpu.a)
        goto none;
    /* its box: half-width and width, half-height and height, and the
       half-height less 8 and twice it (its upper part) */
    op_rep(P_M);
    half = rd16(box);
    wr16(dba(SCRATCH), half);
    wr16(dba(SCRATCH + 2), (uint16_t)(half << 1));
    half = rd16(box + 2);
    cpu.a = half;
    pha16();
    wr16(dba(SCRATCH + 4), half);
    wr16(dba(SCRATCH + 6), (uint16_t)(half << 1));
    pla16();
    flag_set(P_C, 1);
    low = op_sbc16(half, 0x0008, &cpu.p);
    wr16(dba(SCRATCH + 8), low);
    flag_set(P_C, low & 0x8000);
    wr16(dba(SCRATCH + 10), (uint16_t)(low << 1));
    /* his x before the move inside the box, his y inside it but not its
       upper part: he came from above or below */
    if (!inside(dp16(ACTOR_X + 1), rd16(dba(PLAYER_X_BEFORE)), rd16(dba(SCRATCH)), rd16(dba(SCRATCH + 2)), 0)
        || inside(dp16(ACTOR_Y + 1), rd16(dba(PLAYER_Y_PIXELS)), rd16(dba(SCRATCH + 8)), rd16(dba(SCRATCH + 10)), 0)
        || !inside(dp16(ACTOR_Y + 1), rd16(dba(PLAYER_Y_PIXELS)), rd16(dba(SCRATCH + 4)), rd16(dba(SCRATCH + 6)), 0))
        goto sides;
    ay = dp16(ACTOR_Y + 1);
    cpu.a = ay;
    cmp16_of(ay, rd16(dba(PLAYER_Y_PIXELS)));
    if (ay >= rd16(dba(PLAYER_Y_PIXELS)))
        goto top;
    /* below its middle: he bumps his head when rising */
    v = rd8(dba(V_141C));
    x8_load(v);
    if (!v)
        goto blocked;
    v = rd8(dba(PLAYER_SPEED_Y + 2));
    x8_load(v);
    if (!(v & 0x80))
        goto blocked;
    cpu.a = op_adc16(cpu.a, rd16(dba(SCRATCH + 4)), &cpu.p);
    wr16(dba(PLAYER_Y_PIXELS), cpu.a);
    wr16(dba(PLAYER_SPEED_Y), 0);
    op_sep(0x71);
    wr8(dba(PLAYER_SPEED_Y + 2), 0);
    return;
blocked:
    op_sep(0x60);
    flag_set(P_C, 1);
    return;
top:
    /* above it: when not rising, he stands on it */
    v = rd8(dba(PLAYER_SPEED_Y + 1));
    x8_load(v);
    if (v & 0x80)
        goto blocked;
    cpu.a = op_sbc16((uint16_t)(cpu.a + 1), rd16(dba(SCRATCH + 4)), &cpu.p);
    wr16(dba(PLAYER_Y_PIXELS), cpu.a);
    op_sep(0x30);
    a8_load(rd8(dba(HEALTH)));
    if (!(uint8_t)cpu.a) {                /* dead: 4 lower */
        op_rep(P_M);
        flag_set(P_C, 0);
        cpu.a = op_adc16(rd16(dba(PLAYER_Y_PIXELS)), 0x0004, &cpu.p);
        wr16(dba(PLAYER_Y_PIXELS), cpu.a);
        op_sep(P_M);
    }
    inc8_at(dba(PLAYER_ON_ACTOR));
    wr8(dba(V_141C), 0);
    flag_set(P_C, 0);
    flag_set(P_V, 0);
    return;
sides:
    /* his x now inside the box and his y within it: pushed out sideways */
    if (inside(dp16(ACTOR_X + 1), rd16(dba(PLAYER_X_PIXELS)), rd16(dba(SCRATCH)), rd16(dba(SCRATCH + 2)), 0)
        && inside(dp16(ACTOR_Y + 1), rd16(dba(PLAYER_Y_PIXELS)), rd16(dba(SCRATCH + 4)), rd16(dba(SCRATCH + 6)), 1)) {
        ax = dp16(ACTOR_X + 1);
        cpu.a = ax;
        cmp16_of(ax, rd16(dba(PLAYER_X_PIXELS)));
        if (ax >= rd16(dba(PLAYER_X_PIXELS))) {
            cpu.a = op_sbc16(ax, rd16(dba(SCRATCH)), &cpu.p);
        } else {
            flag_set(P_C, 0);
            cpu.a = op_adc16((uint16_t)(ax + 1), rd16(dba(SCRATCH)), &cpu.p);
        }
        wr16(dba(PLAYER_X_PIXELS), cpu.a);
    }
    op_sep(0x31);
    flag_set(P_V, 0);
    return;
none:
    flag_set(P_V, 0);
    flag_set(P_C, 1);
}
