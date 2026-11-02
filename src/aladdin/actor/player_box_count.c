/*
 * player_box_count.c: counts, for the actor whose code is at $83:C3E4,
 * whether the player is within its box on each axis.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

#define PLAYER_X_PIXELS (PLAYER + ACTOR_X + 1)
#define PLAYER_Y_PIXELS (PLAYER + ACTOR_Y + 1)
#define BOX_HALF_W 0x0010          /* the box: 16 pixels each side in x */
#define BOX_HALF_H 0x003C          /* 60 pixels each side in y */

/*
 * @routine 83:C47E ActorPlayerBoxCount
 * WHAT IT DOES: Counts the player's nearness to the actor on each axis:
 * the word at $45 steps up when the actor is within 16 pixels of the
 * player in x (-16 to 15), the word at $47 when within 60 in y (-60 to
 * 59).
 * HOW: For each axis, the actor's pixels less the player's, plus the half
 * span, compared unsigned with the whole span; INC of the word when
 * below.
 * Entry: M 16-bit; D = the actor; DB a bank whose $0000-$1FFF is work
 * RAM. Exit: A = the y difference plus 60; C from its compare with 120;
 * V from adding the 60; N and Z from the compare, or from the INC of $47
 * when it counted; M 16-bit; X, Y and the other flags unchanged.
 */
void abi_ActorPlayerBoxCount(void)
{
    sec();
    lda16(ea_dp(ACTOR_X + 1));
    sbc16_v(rd16(ea_abs(PLAYER_X_PIXELS)));
    clc();
    adc16_v(BOX_HALF_W);
    cmp16_v(2 * BOX_HALF_W);
    if (!IF_C)
        MEM16(inc, ea_dp(ACTOR_45));
    sec();
    lda16(ea_dp(ACTOR_Y + 1));
    sbc16_v(rd16(ea_abs(PLAYER_Y_PIXELS)));
    clc();
    adc16_v(BOX_HALF_H);
    cmp16_v(2 * BOX_HALF_H);
    if (!IF_C)
        MEM16(inc, ea_dp(ACTOR_47));
}
