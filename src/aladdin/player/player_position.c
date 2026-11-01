/*
 * player_position.c: Aladdin's position as objects set and test it.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "object.h"
#include "wram_player.h"

/* REP #$21; LDA dp X; STA x_to; LDA dp Y; CLC; ADC #12; STA y_to; SEP #$20. */
static void object_pos_to(uint32_t x_to, uint32_t y_to)
{
    op_rep(P_M | P_C);
    op_lda16(dp(OBJ_0C));
    wr16(x_to, cpu.a);
    op_lda16(dp(OBJ_0F));
    op_add16(12);
    wr16(y_to, cpu.a);
    op_sep(P_M);
}

/*
 * @routine 83:C8DD PlayerToObject
 * WHAT IT DOES: Puts Aladdin at the object's position, 12 pixels below
 * its Y, as an object that carries him does.
 * HOW: The record's X and Y words to the player's, 12 added to Y.
 * Entry: M 8-bit; D = the record; DB a bank whose $0000-$1FFF is work
 * RAM. Exit: A = the new Y (16 bits); N, V, Z, C from the addition; M
 * 8-bit. X, Y and the other flags unchanged.
 */
void abi_PlayerToObject(void)
{
    object_pos_to(PLAYER_X, PLAYER_Y);
}

/*
 * @routine 83:C8F0 Point145DToObject
 * WHAT IT DOES: Sets the point at $145D to the object's position, 12
 * pixels below its Y.
 * HOW: The record's X and Y words to the point's, 12 added to Y.
 * Entry: M 8-bit; D = the record; DB a bank whose $0000-$1FFF is work
 * RAM. Exit: A = the new Y (16 bits); N, V, Z, C from the addition; M
 * 8-bit. X, Y and the other flags unchanged.
 */
void abi_Point145DToObject(void)
{
    object_pos_to(POINT_145D, POINT_145F);
}

/*
 * @routine 83:8865 ObjectNearPlayer
 * WHAT IT DOES: Tells whether Aladdin is within reach of the object: a
 * box 26 pixels wide and 31 high placed by his position.
 * HOW: Unsigned compares, so a difference below 0 counts as far: player
 * X plus 18 (8 when $08F0 is set) less the object's X must be below 26,
 * then player Y plus 2 less the object's Y below 31.
 * Entry: M 8-bit, X 8-bit; D = the record; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSL. Exit: C clear when near. A = the last
 * difference (16 bits); N, Z and C from its compare; V from the last
 * subtraction; X = the byte at $08F0; M 8-bit. Y and the other flags
 * unchanged.
 */
void abi_ObjectNearPlayer(void)
{
    uint8_t x;

    op_rep(P_M | P_C);
    cpu.a = 0x0012;
    x = rd8(PLAYER_08F0);
    cpu.x = x;
    if (x != 0)
        cpu.a = 0x0008;
    cpu.a = op_adc16(cpu.a, rd16(PLAYER_X), &cpu.p);
    op_sub16(rd16(dp(OBJ_0C)));
    op_cmp16(26);
    if (!(cpu.p & P_C)) {
        cpu.a = op_adc16(rd16(PLAYER_Y), 2, &cpu.p);
        op_sub16(rd16(dp(OBJ_0F)));
        op_cmp16(31);
    }
    op_sep(P_M);
}
