/*
 * solid_actor.c: an actor Aladdin cannot pass through: he stands on its
 * top, bumps his head on its bottom, or is pushed out of its sides.
 */
#include "machine.h"
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
    bit8_v(rd8(ea_dp(ACTOR_09)));
    if (!IF_V) goto none;
    lda8(ea_abs(PLAYER_ON_ACTOR));
    if (!IF_Z) goto none;
    op_rep(0x20);
    lda16(ea_absy(SOLID_BOXES));
    sta16(ea_abs(SCRATCH));
    ACC16(asl);
    sta16(ea_abs(SCRATCH + 2));
    lda16(ea_absy(SOLID_BOXES + 2));
    pha16();
    sta16(ea_abs(SCRATCH + 4));
    ACC16(asl);
    sta16(ea_abs(SCRATCH + 6));
    pla16();
    sec();
    sbc16_v(0x0008);
    sta16(ea_abs(SCRATCH + 8));
    ACC16(asl);
    sta16(ea_abs(SCRATCH + 10));
    /* his x before the move inside the box, his y inside it but above
       its lower part */
    sec();
    lda16(ea_dp(ACTOR_X + 1));
    sbc16_v(rd16(ea_abs(PLAYER_X_BEFORE)));
    clc();
    adc16_v(rd16(ea_abs(SCRATCH)));
    cmp16_v(rd16(ea_abs(SCRATCH + 2)));
    if (IF_C) goto sides;
    sec();
    lda16(ea_dp(ACTOR_Y + 1));
    sbc16_v(rd16(ea_abs(PLAYER_Y_PIXELS)));
    clc();
    adc16_v(rd16(ea_abs(SCRATCH + 8)));
    cmp16_v(rd16(ea_abs(SCRATCH + 10)));
    if (!IF_C) goto sides;
    sec();
    lda16(ea_dp(ACTOR_Y + 1));
    sbc16_v(rd16(ea_abs(PLAYER_Y_PIXELS)));
    clc();
    adc16_v(rd16(ea_abs(SCRATCH + 4)));
    cmp16_v(rd16(ea_abs(SCRATCH + 6)));
    if (IF_C) goto sides;
    lda16(ea_dp(ACTOR_Y + 1));
    cmp16_v(rd16(ea_abs(PLAYER_Y_PIXELS)));
    if (IF_C) goto top;
    /* he is below the box's middle */
    ldx8(ea_abs(V_141C));
    if (IF_Z) goto blocked;
    ldx8(ea_abs(PLAYER_SPEED_Y + 2));
    if (!IF_N) goto blocked;
    adc16_v(rd16(ea_abs(SCRATCH + 4)));
    sta16(ea_abs(PLAYER_Y_PIXELS));
    stz16(ea_abs(PLAYER_SPEED_Y));
    op_sep(0x71);
    stz8(ea_abs(PLAYER_SPEED_Y + 2));
    return;
blocked:
    op_sep(0x60);
    sec();
    return;
top:
    ldx8(ea_abs(PLAYER_SPEED_Y + 1));
    if (IF_N) goto blocked;
    ACC16(inc);
    sbc16_v(rd16(ea_abs(SCRATCH + 4)));
    sta16(ea_abs(PLAYER_Y_PIXELS));
    op_sep(0x30);
    lda8(ea_abs(HEALTH));
    if (IF_Z) {
        op_rep(0x20);
        clc();
        lda16(ea_abs(PLAYER_Y_PIXELS));
        adc16_v(0x0004);
        sta16(ea_abs(PLAYER_Y_PIXELS));
        op_sep(0x20);
    }
    MEM8(inc, ea_abs(PLAYER_ON_ACTOR));
    stz8(ea_abs(V_141C));
    clc();
    clv();
    return;
sides:
    sec();
    lda16(ea_dp(ACTOR_X + 1));
    sbc16_v(rd16(ea_abs(PLAYER_X_PIXELS)));
    clc();
    adc16_v(rd16(ea_abs(SCRATCH)));
    cmp16_v(rd16(ea_abs(SCRATCH + 2)));
    if (IF_C) goto out;
    sec();
    lda16(ea_dp(ACTOR_Y + 1));
    sbc16_v(rd16(ea_abs(PLAYER_Y_PIXELS)));
    ACC16(inc);
    clc();
    adc16_v(rd16(ea_abs(SCRATCH + 4)));
    cmp16_v(rd16(ea_abs(SCRATCH + 6)));
    if (IF_C) goto out;
    lda16(ea_dp(ACTOR_X + 1));
    cmp16_v(rd16(ea_abs(PLAYER_X_PIXELS)));
    if (IF_C) {
        sbc16_v(rd16(ea_abs(SCRATCH)));
        sta16(ea_abs(PLAYER_X_PIXELS));
    } else {
        clc();
        ACC16(inc);
        adc16_v(rd16(ea_abs(SCRATCH)));
        sta16(ea_abs(PLAYER_X_PIXELS));
    }
out:
    op_sep(0x31);
    clv();
    return;
none:
    clv();
    sec();
}
