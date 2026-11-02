/*
 * player_box_count.c: counts, for the actor whose code is at $83:C3E4,
 * whether the player is within its box on each axis.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

#define PLAYER_X_PIXELS (PLAYER + ACTOR_X + 1)
#define PLAYER_Y_PIXELS (PLAYER + ACTOR_Y + 1)
#define BOX_HALF_W 0x0010          /* the box: 16 pixels each side in x */
#define BOX_HALF_H 0x003C          /* 60 pixels each side in y */

/* Counts the word at `count` up when `mine` less `his` lies within
   `half` either side; A and the flags as the test leaves them. */
static void count_within(uint16_t mine, uint16_t his, uint16_t half, uint16_t count)
{
    uint16_t d;

    flag_set(P_C, 1);
    d = op_sbc16(mine, his, &cpu.p);
    flag_set(P_C, 0);
    d = op_adc16(d, half, &cpu.p);
    cpu.a = d;
    cmp16_of(d, (uint16_t)(2 * half));
    if (d < 2 * half) {
        uint16_t n = (uint16_t)(dp16(count) + 1);
        dp16_set(count, n);
        nz16_of(n);
    }
}

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
    count_within(dp16(ACTOR_X + 1), rd16(dba(PLAYER_X_PIXELS)), BOX_HALF_W, ACTOR_45);
    count_within(dp16(ACTOR_Y + 1), rd16(dba(PLAYER_Y_PIXELS)), BOX_HALF_H, ACTOR_47);
}
