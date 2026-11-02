/*
 * land_on_actor.c: Aladdin landing on top of an actor he falls onto: he
 * stands on it. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
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
    bit8_v(rd8(ea_dp(ACTOR_09)));  /* 8289E3 bit */
    if (!IF_V) goto L_828A3F;  /* 8289E5 bvc */
    lda8(ea_abs(V_1428));  /* 8289E7 lda */
    ora8_v(rd8(ea_abs(V_1427)));  /* 8289EA ora */
    if (!IF_Z) goto L_828A3F;  /* 8289ED bne */
    lda8(ea_abs(PLAYER + ACTOR_SPEED_Y + 2));  /* 8289EF lda */
    if (IF_N) goto L_828A3F;  /* 8289F2 bmi */
    op_rep(0x20);  /* 8289F4 rep */
    sec();  /* 8289F6 sec */
    lda16(ea_dp(ACTOR_X + 1));  /* 8289F7 lda */
    sbc16_v(rd16(ea_abs(PLAYER + ACTOR_X + 1)));  /* 8289F9 sbc */
    clc();  /* 8289FC clc */
    adc16_v(rd16(ea_absy(LAND_BOXES)));  /* 8289FD adc */
    cmp16_v(rd16(ea_absy(LAND_BOXES + 2)));  /* 828A00 cmp */
    if (IF_C) goto L_828A3F;  /* 828A03 bcs */
    sec();  /* 828A05 sec */
    lda16(ea_dp(ACTOR_Y + 1));  /* 828A06 lda */
    sbc16_v(rd16(ea_abs(PLAYER + ACTOR_Y + 1)));  /* 828A08 sbc */
    clc();  /* 828A0B clc */
    adc16_v(rd16(ea_absy(LAND_BOXES + 4)));  /* 828A0C adc */
    cmp16_v(rd16(ea_absy(LAND_BOXES + 6)));  /* 828A0F cmp */
    if (IF_C) goto L_828A3F;  /* 828A12 bcs */
    MEM16(inc, ea_abs(SCRATCH));  /* 828A14 inc */
    clc();  /* 828A17 clc */
    lda16(ea_abs(PLAYER + ACTOR_Y + 1));  /* 828A18 lda */
    adc16_v(rd16(ea_absy(LAND_BOXES + 8)));  /* 828A1B adc */
    cmp16_v(rd16(ea_dp(ACTOR_Y + 1)));  /* 828A1E cmp */
    if (IF_C) goto L_828A3F;  /* 828A20 bcs */
    sec();  /* 828A22 sec */
    lda16(ea_dp(ACTOR_Y + 1));  /* 828A23 lda */
    sbc16_v(rd16(ea_absy(LAND_BOXES + 4)));  /* 828A25 sbc */
    ACC16(inc);  /* 828A28 inc */
    sta16(ea_abs(PLAYER + ACTOR_Y + 1));  /* 828A29 sta */
    op_sep(0x30);  /* 828A2C sep */
    stz8(ea_abs(PLAYER + ACTOR_SPEED_Y));  /* 828A2E stz */
    stz8(ea_abs(PLAYER + ACTOR_SPEED_Y + 1));  /* 828A31 stz */
    stz8(ea_abs(PLAYER + ACTOR_SPEED_Y + 2));  /* 828A34 stz */
    MEM8(inc, ea_abs(V_1428));  /* 828A37 inc */
    stz8(ea_abs(V_141C));  /* 828A3A stz */
    clc();  /* 828A3D clc */
    return;  /* 828A3E rtl */
L_828A3F:
    op_sep(0x31);  /* 828A3F sep */
    return;  /* 828A41 rtl */
}
