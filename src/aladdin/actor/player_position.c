/*
 * player_position.c: Aladdin's position as actors set and test it.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "actor.h"

/* The player's x and y pixels; a point an actor sets ($145D, $145F). */
#define PLAYER_X (PLAYER + ACTOR_X + 1)
#define PLAYER_Y (PLAYER + ACTOR_Y + 1)
#define POINT_145D 0x00145Du
#define POINT_145F 0x00145Fu

#define ACTOR_X_PIXEL (ACTOR_X + 1)
#define ACTOR_Y_PIXEL (ACTOR_Y + 1)

static uint32_t dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

/* REP #$21; LDA dp X; STA x_to; LDA dp Y; CLC; ADC #12; STA y_to; SEP #$20. */
static void actor_pos_to(uint32_t x_to, uint32_t y_to)
{
    op_rep(P_M | P_C);
    op_lda16(dp(ACTOR_X_PIXEL));
    wr16(x_to, cpu.a);
    op_lda16(dp(ACTOR_Y_PIXEL));
    op_add16(12);
    wr16(y_to, cpu.a);
    op_sep(P_M);
}

/*
 * @routine 83:C8DD PlayerToActor
 * WHAT IT DOES: Puts Aladdin at the actor's position, 12 pixels below
 * its Y, as an actor that carries him does.
 * HOW: The actor's x and y pixels to the player's, 12 added to Y.
 * Entry: M 8-bit; D = the record; DB a bank whose $0000-$1FFF is work
 * RAM. Exit: A = the new Y (16 bits); N, V, Z, C from the addition; M
 * 8-bit. X, Y and the other flags unchanged.
 */
void abi_PlayerToActor(void)
{
    actor_pos_to(PLAYER_X, PLAYER_Y);
}

/*
 * @routine 83:C8F0 Point145DToActor
 * WHAT IT DOES: Sets the point at $145D to the actor's position, 12
 * pixels below its Y.
 * HOW: The actor's x and y pixels to the point's, 12 added to Y.
 * Entry: M 8-bit; D = the record; DB a bank whose $0000-$1FFF is work
 * RAM. Exit: A = the new Y (16 bits); N, V, Z, C from the addition; M
 * 8-bit. X, Y and the other flags unchanged.
 */
void abi_Point145DToActor(void)
{
    actor_pos_to(POINT_145D, POINT_145F);
}

/*
 * @routine 83:8865 ActorInReachOfPlayer
 * WHAT IT DOES: Tells whether Aladdin is within reach of the actor: a
 * box 26 pixels wide and 31 high placed by his position.
 * HOW: Unsigned compares, so a difference below 0 counts as far: player
 * X plus 18 (8 when $08F0 is set) less the actor's X must be below 26,
 * then player Y plus 2 less the actor's Y below 31.
 * Entry: M 8-bit, X 8-bit; D = the record; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSL. Exit: C clear when near. A = the last
 * difference (16 bits); N, Z and C from its compare; V from the last
 * subtraction; X = the byte at $08F0; M 8-bit. Y and the other flags
 * unchanged.
 */
void abi_ActorInReachOfPlayer(void)
{
    uint8_t x;

    op_rep(P_M | P_C);
    cpu.a = 0x0012;
    x = rd8(PLAYER + ACTOR_FLIP);
    cpu.x = x;
    if (x != 0)
        cpu.a = 0x0008;
    cpu.a = op_adc16(cpu.a, rd16(PLAYER_X), &cpu.p);
    op_sub16(rd16(dp(ACTOR_X_PIXEL)));
    op_cmp16(26);
    if (!(cpu.p & P_C)) {
        cpu.a = op_adc16(rd16(PLAYER_Y), 2, &cpu.p);
        op_sub16(rd16(dp(ACTOR_Y_PIXEL)));
        op_cmp16(31);
    }
    op_sep(P_M);
}
