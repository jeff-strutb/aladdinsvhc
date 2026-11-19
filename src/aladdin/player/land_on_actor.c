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
    uint16_t d, py, top;
    uint8_t v;

    bit8_of((uint8_t)cpu.a, flags);
    if (!(flags & 0x40))
        goto no;
    v = rd8(dba(V_1428));
    a8_load((uint8_t)(v | rd8(dba(V_1427))));
    if ((uint8_t)cpu.a)
        goto no;
    a8_load(rd8(dba(PLAYER + ACTOR_SPEED_Y + 2)));
    if ((uint8_t)cpu.a & 0x80)            /* rising */
        goto no;
    /* the box (offset, then width: x, then y) round the actor */
    op_rep(P_M);
    flag_set(P_C, 1);
    d = dp16(ACTOR_X + 1);
    d = op_sbc16(d, rd16(dba(PLAYER + ACTOR_X + 1)), &cpu.p);
    flag_set(P_C, 0);
    d = op_adc16(d, rd16(box), &cpu.p);
    cpu.a = d;
    cmp16_of(d, rd16(box + 2));
    if (d >= rd16(box + 2))
        goto no;
    flag_set(P_C, 1);
    d = dp16(ACTOR_Y + 1);
    d = op_sbc16(d, rd16(dba(PLAYER + ACTOR_Y + 1)), &cpu.p);
    flag_set(P_C, 0);
    d = op_adc16(d, rd16(box + 4), &cpu.p);
    cpu.a = d;
    cmp16_of(d, rd16(box + 6));
    if (d >= rd16(box + 6))
        goto no;
    inc16_at(dba(SCRATCH));
    /* his feet above its top: he stands on it */
    flag_set(P_C, 0);
    py = rd16(dba(PLAYER + ACTOR_Y + 1));
    py = op_adc16(py, rd16(box + 8), &cpu.p);
    cpu.a = py;
    top = dp16(ACTOR_Y + 1);            /* one read, as the CMP makes */
    cmp16_of(py, top);
    if (py >= top)
        goto no;
    flag_set(P_C, 1);
    d = dp16(ACTOR_Y + 1);
    d = op_sbc16(d, rd16(box + 4), &cpu.p);
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

#define ESCORT_NEAR 0x146Bu       /* bit 0: an actor near the escort is marked */
#define ESCORT_NEAR_ACTOR 0x146Cu /* the record marked (word) */

/*
 * @routine 82:8A42 ActorNearEscortMark
 * WHAT IT DOES: Marks the actor as near the escort: unless one is marked
 * already, an actor within $60 across and $30 up or down of the escort
 * ($092B) is kept in ESCORT_NEAR_ACTOR and ESCORT_NEAR bit 0 set.
 * HOW: ESCORT_NEAR nonzero: back. 16-bit: |its x pixels - the escort's|
 * below $60 and |its y pixels - the escort's| below $30: ESCORT_NEAR_ACTOR
 * = D (TDC), 8-bit, ESCORT_NEAR bit 0 set (TSB). No code calls it (no JSR, JSL, JMP or table word reaches it):
 * unit_dead_82.box calls it at its own entry.
 * Entry: M 8-bit; D = the actor; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSL. Exit: M 8-bit; A as the tests leave it.
 */
void abi_ActorNearEscortMark(void)
{
    lda8(ea_abs(ESCORT_NEAR));
    if (!IF_Z)
        return;
    op_rep(0x20);
    sec();
    lda16(ea_dp(ACTOR_X + 1));
    sbc16_v(rd16(ea_abs(ACTOR_ESCORT + ACTOR_X + 1)));
    if (!IF_C) {
        eor16_v(0xFFFF);
        ACC16(inc);
    }
    cmp16_v(0x0060);
    if (IF_C)
        goto out;
    sec();
    lda16(ea_dp(ACTOR_Y + 1));
    sbc16_v(rd16(ea_abs(ACTOR_ESCORT + ACTOR_Y + 1)));
    if (!IF_C) {
        eor16_v(0xFFFF);
        ACC16(inc);
    }
    cmp16_v(0x0030);
    if (IF_C)
        goto out;
    tdc();
    sta16(ea_abs(ESCORT_NEAR_ACTOR));
    op_sep(0x20);
    lda8_v(0x01);
    tsb8(ea_abs(ESCORT_NEAR));
out:
    op_sep(0x20);
}
