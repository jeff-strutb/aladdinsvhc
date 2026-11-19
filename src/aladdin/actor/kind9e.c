/*
 * kind9e.c: actor kind $9E, the figures of the Genie's screen (level
 * $2F) that take their animation from the screen's state.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_ActorPaletteFromRom(void);

#define G_AC 0x00ACu   /* the Genie's screen: a pick (negative: a pose by G_AF; 1, 2-3, 4 up: animation 3, 4, 5) */
#define G_AD 0x00ADu   /* nonzero: a pick's animation is 1 instead */
#define G_AF 0x00AFu   /* with G_AC negative: zero, animation 7; else 9 */
#define G_B0 0x00B0u   /* the y (word) way 2 follows, less $10 */
#define G_B5 0x00B5u   /* the screen's end: 1, animation 8; other nonzero, $0A (way 0); way 2's cue */

/*
 * @routine 84:A820 Kind9EStart
 * @actor
 * WHAT IT DOES: Actor kind $9E of the Genie's screen (level $2F), by its
 * $07. Way 0: animation 0, then each frame by the screen's state: G_B5
 * nonzero ends the wait with animation 8 (1) or $0A; else a pick in G_AC
 * (cleared once taken) starts animation 3, 4 or 5 (1 if G_AD is set), or,
 * negative, 7 or 9 by G_AF ($FF: 0). Way 2: ActorPaletteFromRom $52, $10
 * down by one; once G_B5 is nonzero, animation $0B ($0C when negative);
 * then each frame its y = G_B0 + $10 and its x $34.
 * HOW: ACTOR_08 |= 3, ACTOR_09 |= $90; JMP ($A82D,X) by $07: $A831, $A88F.
 * Way 0 as above with a BRK each frame it waits. Way 2: Y = $52,
 * ActorPaletteFromRom, DEC $10; BRK until G_B5; X = $0B, INX when
 * negative; AnimStart X; then each frame (BRK first) 16-bit (REP #$21):
 * G_B0 + $10 to $0F, $0034 to $0C.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $9E in state $0C (the table at $83:8DAB,
 * through the stub at $83:93F7). Exit: never returns.
 */
void abi_Kind9EStart(void)
{
    uint16_t to;

    lda8_v(0x03);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0x90);
    tsb8(ea_dp(ACTOR_09));
    ldx8(ea_dp(ACTOR_07));
    to = rd16(0x84A82Du + cpu.x);                 /* JMP ($A82D,X) */
    if (to == 0xA88F)
        goto follow;
    if (to != 0xA831) {
        jump_routine(0x840000u | to);
        return;
    }
    lda8_v(0x00);
anim:
    jsl_frame(0x84A833u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
check:
    lda8(ea_abs(G_B5));
    if (IF_N)
        goto end_other;
    if (!IF_Z)
        goto end_one;
wait:
    actor_yield(0x84A83Eu);
    lda8(ea_abs(G_AC));
    if (IF_Z)
        goto check;
    if (IF_N)
        goto pose;
    cmp8_v(0x02);
    if (!IF_C) {
        lda8_v(0x03);
        goto pick;
    }
    cmp8_v(0x04);
    if (!IF_C) {
        lda8_v(0x04);
        goto pick;
    }
    lda8_v(0x05);
    goto pick;
end_one:
    ACC8(dec);
    if (!IF_Z)
        goto end_other;
    lda8_v(0x08);
    jsl_frame(0x84A860u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
    goto wait;
end_other:
    lda8_v(0x0A);
    jsl_frame(0x84A868u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
    goto wait;
pose:
    ldy8_v(0x00);
    ACC8(inc);
    if (!IF_N) {
        ldy8_v(0x07);
        lda8(ea_abs(G_AF));
        if (!IF_Z)
            ldy8_v(0x09);
    }
    tya8();
    jsl_frame(0x84A87Du); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
    stz8(ea_abs(G_AC));
    goto check;
pick:
    ldy8(ea_abs(G_AD));
    if (IF_Z)
        goto anim;
    lda8_v(0x01);
    goto anim;
follow:
    ldy8_v(0x52);
    jsl_frame(0x84A891u); abi_ActorPaletteFromRom(); cpu.s = (uint16_t)(cpu.s + 3);
    MEM8(dec, ea_dp(0x10));
    do {
        actor_yield(0x84A897u);
        ldx8_v(0x0B);
        lda8(ea_abs(G_B5));
    } while (IF_Z);
    if (IF_N)
        inx8();
    txa8();
    jsl_frame(0x84A8A4u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
    for (;;) {
        actor_yield(0x84A8A8u);
        op_rep(0x21);
        lda16(ea_abs(G_B0));
        adc16_v(0x0010);
        sta16(ea_dp(ACTOR_Y + 1));
        lda16_v(0x0034);
        sta16(ea_dp(ACTOR_X + 1));
        op_sep(0x20);
    }
}
