/*
 * still_actors.c: actors that only stand, drift or fly off: kinds $7D
 * (keeps its spot from a table by its height), $7A (flies its course for
 * good), $4A (its own palette, an animation, nothing more), and the fly-
 * off end of the actor at $83:C763. Drafted by cdraft.py; named and
 * described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorEndPlaced(void);
void abi_ActorFly(void);
void abi_ActorKeepSpot(void);
void abi_ActorMotionPreset(void);
void abi_AnimStart(void);
void abi_PaletteLoad(void);

#define KIND7D_16 0x16u  /* word: $D820 */
#define KIND7D_SPOTS 0xD7D2u  /* by its y pixels' low six bits, two bytes kept in $40 and $41 (in the bank DB names) */
#define FLYOFF_LEFT 0x41u  /* frames left of its flight */

/*
 * @routine 84:90CE Kind7DKeepSpot
 * @actor
 * WHAT IT DOES: Actor kind $7D: takes two bytes from KIND7D_SPOTS by the
 * low six bits of its y (kept as its $07), then each frame keeps its place
 * in $4C and $4E (ActorKeepSpot), for good.
 * HOW: $09 = $80; $16 = $D820 (two 8-bit stores); $07 = y pixels and $3F;
 * KIND7D_SPOTS at twice it to $40 and $41; each frame (BRK first)
 * ActorKeepSpot.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $7D in state $0C (through the stub at
 * $83:929B). Exit: never returns.
 */
void abi_Kind7DKeepSpot(void)
{
    lda8_v(0x80);  /* 8490CE lda */
    sta8(ea_dp(ACTOR_09));  /* 8490D0 sta */
    lda8_v(0x20);  /* 8490D2 lda */
    sta8(ea_dp(KIND7D_16));  /* 8490D4 sta */
    lda8_v(0xD8);  /* 8490D6 lda */
    sta8(ea_dp(KIND7D_16 + 1));  /* 8490D8 sta */
    lda8(ea_dp(ACTOR_Y + 1));  /* 8490DA lda */
    and8_v(0x3F);  /* 8490DC and */
    sta8(ea_dp(ACTOR_07));  /* 8490DE sta */
    ACC8(asl);  /* 8490E0 asl */
    tax8();  /* 8490E1 tax */
    lda8(ea_absx(KIND7D_SPOTS));  /* 8490E2 lda */
    sta8(ea_dp(ACTOR_40));  /* 8490E5 sta */
    lda8(ea_absx(KIND7D_SPOTS + 1));  /* 8490E7 lda */
    sta8(ea_dp(FLYOFF_LEFT));  /* 8490EA sta */
L_8490EC:
    actor_yield(0x8490ECu);  /* 8490EC brk */
    abi_ActorKeepSpot();  /* 8490EE jsl */
    goto L_8490EC;  /* 8490F2 bra */
}


/*
 * @routine 83:C925 ActorFlyOffEnd
 * @actor
 * WHAT IT DOES: The end of the actor at $83:C763: it can no longer be hit,
 * flies off on motion $2B for $18 frames with animation 3, and ends.
 * HOW: Bit 7 of $09 cleared; A = $80 shifted right, C clear, so Y = $2A +
 * 1; ActorMotionPreset; AnimStart 3; $41 = $18; each frame ActorFly (BRK
 * first) until $41 is counted out; then ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by a
 * jump from that actor's code (83:C773). Exit: never returns; ends in
 * ActorEndPlaced.
 */
void abi_ActorFlyOffEnd(void)
{
    lda8_v(0x80);  /* 83C925 lda */
    trb8(ea_dp(ACTOR_09));  /* 83C927 trb */
    ACC8(lsr);  /* 83C929 lsr */
    ldy8_v(0x2A);  /* 83C92A ldy */
    if (IF_C) goto L_83C92F;  /* 83C92C bcs */
    iny8();  /* 83C92E iny */
L_83C92F:
    abi_ActorMotionPreset();  /* 83C92F jsl */
    lda8_v(0x03);  /* 83C933 lda */
    abi_AnimStart();  /* 83C935 jsl */
    lda8_v(0x18);  /* 83C939 lda */
    sta8(ea_dp(FLYOFF_LEFT));  /* 83C93B sta */
L_83C93D:
    actor_yield(0x83C93Du);  /* 83C93D brk */
    abi_ActorFly();  /* 83C93F jsl */
    MEM8(dec, ea_dp(FLYOFF_LEFT));  /* 83C943 dec */
    if (!IF_Z) goto L_83C93D;  /* 83C945 bne */
    abi_ActorEndPlaced(); return;  /* 83C947 jmp */
}


/*
 * @routine 83:F757 Kind7AFly
 * @actor
 * WHAT IT DOES: Actor kind $7A: facing right, shown with animation 0, it
 * flies its course (motion $53) for good.
 * HOW: Facing and flip 0; $08 bit 1 and $09 bit 4 set; AnimStart 0;
 * ActorMotionPreset $53; each frame (BRK first) ActorFly.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $7A in state $0C. Exit: never returns.
 */
void abi_Kind7AFly(void)
{
    stz8(ea_dp(ACTOR_FACING));  /* 83F757 stz */
    stz8(ea_dp(ACTOR_FLIP));  /* 83F759 stz */
    lda8(ea_dp(ACTOR_08));  /* 83F75B lda */
    ora8_v(0x02);  /* 83F75D ora */
    sta8(ea_dp(ACTOR_08));  /* 83F75F sta */
    lda8(ea_dp(ACTOR_09));  /* 83F761 lda */
    ora8_v(0x10);  /* 83F763 ora */
    sta8(ea_dp(ACTOR_09));  /* 83F765 sta */
    lda8_v(0x00);  /* 83F767 lda */
    abi_AnimStart();  /* 83F769 jsl */
    ldy8_v(0x53);  /* 83F76D ldy */
    abi_ActorMotionPreset();  /* 83F76F jsl */
L_83F773:
    actor_yield(0x83F773u);  /* 83F773 brk */
    abi_ActorFly();  /* 83F775 jsl */
    goto L_83F773;  /* 83F779 bra */
}


/*
 * @routine 83:B496 Kind4AShow
 * @actor
 * WHAT IT DOES: Actor kind $4A: facing right, with its look $0600, loads
 * palette set $0A, shows animation 6 and stays.
 * HOW: $09 bits $90 set; facing and flip 0; its look $0600; PaletteLoad
 * $0A; AnimStart 6; BRK for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $4A in state $0C. Exit: never returns.
 */
void abi_Kind4AShow(void)
{
    lda8(ea_dp(ACTOR_09));  /* 83B496 lda */
    ora8_v(0x90);  /* 83B498 ora */
    sta8(ea_dp(ACTOR_09));  /* 83B49A sta */
    stz8(ea_dp(ACTOR_FACING));  /* 83B49C stz */
    stz8(ea_dp(ACTOR_FLIP));  /* 83B49E stz */
    op_rep(0x20);  /* 83B4A0 rep */
    lda16_v(0x0600);  /* 83B4A2 lda */
    sta16(ea_dp(ACTOR_LOOK));  /* 83B4A5 sta */
    op_sep(0x20);  /* 83B4A7 sep */
    lda8_v(0x0A);  /* 83B4A9 lda */
    abi_PaletteLoad();  /* 83B4AB jsl */
    lda8_v(0x06);  /* 83B4AF lda */
    abi_AnimStart();  /* 83B4B1 jsl */
L_83B4B5:
    actor_yield(0x83B4B5u);  /* 83B4B5 brk */
    goto L_83B4B5;  /* 83B4B7 bra */
}
