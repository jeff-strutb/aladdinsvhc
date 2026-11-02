/*
 * land_on_actor.c: Aladdin landing on top of an actor he falls onto: he
 * stands on it. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"
#include "wram_player.h"


#define V_1427 0x1427u  /* nonzero: no landing on actors */
#define LAND_BOXES 0xBE92u  /* bank $82, by Y: x offset, width, y offset, height, then how far below his feet it may be (words) */

/*
 * @routine 82:89E3 PlayerLandOnActor
 * WHAT IT DOES: Lands Aladdin on the actor when he is falling (or still)
 * onto its top, inside its landing box Y: his feet are put on it, his fall
 * stops and he is marked standing on an actor. C clear when he landed, set
 * when not.
 * HOW: The actor can be hit (bit 6 of $09), V_1428 and V_1427 both zero
 * and his speed y not negative, else out. Its x less his plus the box's x
 * offset below its width, and its y less his plus the y offset below its
 * height, else out; $000C up by one (a box matched); his y plus the box's
 * fifth word below its y, else out; then his y = its y less the y offset
 * plus 1, his speed y cleared, V_1428 up, V_141C cleared, C clear. Out: M
 * and X 8-bit, C set.
 * Entry: M 8-bit, X 16-bit; Y = the box's offset; D = the actor's record;
 * DB = $82; reached by a jump (JML at $83:FBB1) from a routine called by
 * JSL. Exit: by RTL; M and X 8-bit; C as above.
 */
void abi_PlayerLandOnActor(void)
{
    uint32_t box = dba(LAND_BOXES) + cpu.y;
    uint8_t flags = rd8(dpa(ACTOR_09));
    uint16_t d, py;

    bit8_of((uint8_t)cpu.a, flags);
    if (!(flags & 0x40))
        goto no;
    a8_load((uint8_t)(rd8(dba(V_1428)) | rd8(dba(V_1427))));
    if ((uint8_t)cpu.a)
        goto no;
    a8_load(rd8(dba(PLAYER + ACTOR_SPEED_Y + 2)));
    if ((uint8_t)cpu.a & 0x80)            /* rising */
        goto no;
    /* the box (offset, then width: x, then y) round the actor */
    op_rep(P_M);
    flag_set(P_C, 1);
    d = op_sbc16(dp16(ACTOR_X + 1), rd16(dba(PLAYER + ACTOR_X + 1)), &cpu.p);
    flag_set(P_C, 0);
    d = op_adc16(d, rd16(box), &cpu.p);
    cpu.a = d;
    cmp16_of(d, rd16(box + 2));
    if (d >= rd16(box + 2))
        goto no;
    flag_set(P_C, 1);
    d = op_sbc16(dp16(ACTOR_Y + 1), rd16(dba(PLAYER + ACTOR_Y + 1)), &cpu.p);
    flag_set(P_C, 0);
    d = op_adc16(d, rd16(box + 4), &cpu.p);
    cpu.a = d;
    cmp16_of(d, rd16(box + 6));
    if (d >= rd16(box + 6))
        goto no;
    inc16_at(dba(SCRATCH));
    /* his feet above its top: he stands on it */
    flag_set(P_C, 0);
    py = op_adc16(rd16(dba(PLAYER + ACTOR_Y + 1)), rd16(box + 8), &cpu.p);
    cpu.a = py;
    cmp16_of(py, dp16(ACTOR_Y + 1));
    if (py >= dp16(ACTOR_Y + 1))
        goto no;
    flag_set(P_C, 1);
    d = op_sbc16(dp16(ACTOR_Y + 1), rd16(box + 4), &cpu.p);
    a16_load((uint16_t)(d + 1));
    wr16(dba(PLAYER + ACTOR_Y + 1), cpu.a);
    op_sep(0x30);
    wr8(dba(PLAYER + ACTOR_SPEED_Y), 0);
    wr8(dba(PLAYER + ACTOR_SPEED_Y + 1), 0);
    wr8(dba(PLAYER + ACTOR_SPEED_Y + 2), 0);
    inc8_at(dba(V_1428));
    wr8(dba(V_141C), 0);
    flag_set(P_C, 0);
    return;
no:
    op_sep(0x31);
}
