/*
 * kind_7f.c: actor kind $7F: a backdrop figure that drifts across for
 * good, slower the higher its $07. Drafted by cdraft.py; named and
 * described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorMoveX(void);
void abi_AnimStart(void);

#define KIND7F_16 0x16u  /* word: $D832 */
#define KIND7F_SLOWER 0x40u  /* $07 times 16 */

/*
 * @routine 84:934B Kind7FDrift
 * @actor
 * WHAT IT DOES: Actor kind $7F: shown with the animation $07 picks (bits
 * from $07 also in $08), facing right, it drifts across at $70 less 16
 * times $07 (a fraction of a pixel a frame) for good.
 * HOW: $08 |= 2 | $07; $09 bits $90 set; $16 = $D832; AnimStart $07;
 * facing and flip 0; $40 = $07 * 16; its speed's fraction = $70 - $40;
 * each frame (BRK first) ActorMoveX.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $7F in state $0C (through the stub at
 * $83:92B5). Exit: never returns.
 */
void abi_Kind7FDrift(void)
{
    lda8(ea_dp(ACTOR_08));  /* 84934B lda */
    ora8_v(0x02);  /* 84934D ora */
    ora8_v(rd8(ea_dp(ACTOR_07)));  /* 84934F ora */
    sta8(ea_dp(ACTOR_08));  /* 849351 sta */
    lda8(ea_dp(ACTOR_09));  /* 849353 lda */
    ora8_v(0x90);  /* 849355 ora */
    sta8(ea_dp(ACTOR_09));  /* 849357 sta */
    op_rep(0x20);  /* 849359 rep */
    lda16_v(0xD832);  /* 84935B lda */
    sta16(ea_dp(KIND7F_16));  /* 84935E sta */
    op_sep(0x20);  /* 849360 sep */
    lda8(ea_dp(ACTOR_07));  /* 849362 lda */
    abi_AnimStart();  /* 849364 jsl */
    stz8(ea_dp(ACTOR_FACING));  /* 849368 stz */
    stz8(ea_dp(ACTOR_FLIP));  /* 84936A stz */
    lda8(ea_dp(ACTOR_07));  /* 84936C lda */
    ACC8(asl);  /* 84936E asl */
    ACC8(asl);  /* 84936F asl */
    ACC8(asl);  /* 849370 asl */
    ACC8(asl);  /* 849371 asl */
    sta8(ea_dp(KIND7F_SLOWER));  /* 849372 sta */
    lda8_v(0x70);  /* 849374 lda */
    sec();  /* 849376 sec */
    sbc8_v(rd8(ea_dp(KIND7F_SLOWER)));  /* 849377 sbc */
    sta8(ea_dp(ACTOR_SPEED_X));  /* 849379 sta */
L_84937B:
    actor_yield(0x84937Bu);  /* 84937B brk */
    abi_ActorMoveX();  /* 84937D jsl */
    goto L_84937B;  /* 849381 bra */
}
