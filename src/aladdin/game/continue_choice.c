/*
 * continue_choice.c: actor kind $26 on the continue screen: the two places
 * Aladdin can step down into (yes: continue; no) and the one that watches
 * him. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"
#include "wram_state.h"

void abi_ActorEndPlaced(void);
void abi_ActorZoneHasTestPointLong(void);
void abi_AnimStart(void);
void abi_PlayerSpotBelowActor(void);

#define CONTINUE_YES 0x0076u  /* set when Aladdin takes the continue (the screen then counts a continue off) */
#define CONTINUE_NO 0x0077u  /* set when he turns it down */
#define V_0380 0x0380u  /* copied each frame to the watcher's $46 */
#define PLAYER_RESUME 0x08DEu  /* the player's resume address (its record's $03, word) */
#define PLAYER_X 0x08E7u  /* the player's x pixels (its record's $0C) */
#define CHOICE_TAKEN 0x44u  /* set once Aladdin has stepped into it */
#define CHOICE_SINK 0x40u  /* frames left of its sinking */
#define WATCHER 0x42u  /* set: it is the watcher */
#define WATCHER_ANIM 0x45u  /* the animation it shows */
#define WATCHER_46 0x46u  /* V_0380, copied each frame */

/*
 * @routine 83:9D62 ContinueChoiceActor
 * @actor
 * WHAT IT DOES: An actor of the continue screen; none when no continues
 * are left. $07 low four bits zero: a place Aladdin can step down into
 * ($07 zero: the yes, facing right; else the no, facing left): when he is
 * in its zone it takes him (his code goes on at $D9C5, set below it),
 * sinks 16 pixels with him, and marks the choice. Otherwise the watcher:
 * its animation by the choice made, or by where Aladdin stands (1 between
 * x 80 and 175, 2 left of it, 4 right; 3 once yes, 5 once no).
 * HOW: $08 bit 1 and $09 bits $90 set; facing, flip and $42 cleared.
 * CONTINUES zero: ActorEndPlaced. Place: animation 0, $44 cleared; each
 * frame until ActorZoneHasTestPointLong (zone $84) clears C; PLAYER_RESUME
 * = $D9C5, $44 up, PlayerSpotBelowActor 5; 16 frames of its y pixels up by
 * one; CONTINUE_YES (or, $07 nonzero, CONTINUE_NO) up; then BRK for good.
 * Watcher: $45 cleared, $42 up; each frame a new animation (A) is started
 * when not $45, $46 = V_0380, BRK, then A from the choice or the player's
 * x.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $26 in state $0C. Exit: never returns;
 * with no continues ends in ActorEndPlaced.
 */
void abi_ContinueChoiceActor(void)
{
    lda8(ea_dp(ACTOR_08));  /* 839D62 lda */
    ora8_v(0x02);  /* 839D64 ora */
    sta8(ea_dp(ACTOR_08));  /* 839D66 sta */
    lda8(ea_dp(ACTOR_09));  /* 839D68 lda */
    ora8_v(0x90);  /* 839D6A ora */
    sta8(ea_dp(ACTOR_09));  /* 839D6C sta */
    stz8(ea_dp(ACTOR_FACING));  /* 839D6E stz */
    stz8(ea_dp(ACTOR_FLIP));  /* 839D70 stz */
    stz8(ea_dp(WATCHER));  /* 839D72 stz */
    lda8(ea_abs(CONTINUES));  /* 839D74 lda */
    if (!IF_Z) goto L_839D7C;  /* 839D77 bne */
    abi_ActorEndPlaced(); return;  /* 839D79 jmp */
L_839D7C:
    lda8(ea_dp(ACTOR_07));  /* 839D7C lda */
    and8_v(0x0F);  /* 839D7E and */
    if (!IF_Z) goto L_839DCC;  /* 839D80 bne */
    lda8_v(0x00);  /* 839D82 lda */
    abi_AnimStart();  /* 839D84 jsl */
    stz8(ea_dp(CHOICE_TAKEN));  /* 839D88 stz */
    lda8(ea_dp(ACTOR_07));  /* 839D8A lda */
    if (IF_Z) goto L_839D94;  /* 839D8C beq */
    lda8_v(0x01);  /* 839D8E lda */
    sta8(ea_dp(ACTOR_FACING));  /* 839D90 sta */
    sta8(ea_dp(ACTOR_FLIP));  /* 839D92 sta */
L_839D94:
    actor_yield(0x839D94u);  /* 839D94 brk */
    ldy8_v(0x84);  /* 839D96 ldy */
    abi_ActorZoneHasTestPointLong();  /* 839D98 jsl */
    if (IF_C) goto L_839D94;  /* 839D9C bcs */
    op_rep(0x20);  /* 839D9E rep */
    lda16_v(0xD9C5);  /* 839DA0 lda */
    sta16(ea_abs(PLAYER_RESUME));  /* 839DA3 sta */
    op_sep(0x20);  /* 839DA6 sep */
    MEM8(inc, ea_dp(CHOICE_TAKEN));  /* 839DA8 inc */
    lda8_v(0x05);  /* 839DAA lda */
    abi_PlayerSpotBelowActor();  /* 839DAC jsl */
    lda8_v(0x10);  /* 839DB0 lda */
    sta8(ea_dp(CHOICE_SINK));  /* 839DB2 sta */
L_839DB4:
    actor_yield(0x839DB4u);  /* 839DB4 brk */
    MEM8(inc, ea_dp(ACTOR_Y + 1));  /* 839DB6 inc */
    MEM8(dec, ea_dp(CHOICE_SINK));  /* 839DB8 dec */
    if (!IF_Z) goto L_839DB4;  /* 839DBA bne */
    lda8(ea_dp(ACTOR_07));  /* 839DBC lda */
    if (!IF_Z) goto L_839DC5;  /* 839DBE bne */
    MEM8(inc, ea_abs(CONTINUE_YES));  /* 839DC0 inc */
    goto L_839DC8;  /* 839DC3 bra */
L_839DC5:
    MEM8(inc, ea_abs(CONTINUE_NO));  /* 839DC5 inc */
L_839DC8:
    actor_yield(0x839DC8u);  /* 839DC8 brk */
    goto L_839DC8;  /* 839DCA bra */
L_839DCC:
    stz8(ea_dp(WATCHER_ANIM));  /* 839DCC stz */
    MEM8(inc, ea_dp(WATCHER));  /* 839DCE inc */
L_839DD0:
    cmp8_v(rd8(ea_dp(WATCHER_ANIM)));  /* 839DD0 cmp */
    if (IF_Z) goto L_839DDA;  /* 839DD2 beq */
    sta8(ea_dp(WATCHER_ANIM));  /* 839DD4 sta */
    abi_AnimStart();  /* 839DD6 jsl */
L_839DDA:
    lda8(ea_abs(V_0380));  /* 839DDA lda */
    sta8(ea_dp(WATCHER_46));  /* 839DDD sta */
    actor_yield(0x839DDFu);  /* 839DDF brk */
    lda8(ea_abs(CONTINUE_YES));  /* 839DE1 lda */
    if (!IF_Z) goto L_839E02;  /* 839DE4 bne */
    lda8(ea_abs(CONTINUE_NO));  /* 839DE6 lda */
    if (!IF_Z) goto L_839E06;  /* 839DE9 bne */
    lda8(ea_abs(PLAYER_X));  /* 839DEB lda */
    cmp8_v(0x50);  /* 839DEE cmp */
    if (!IF_C) goto L_839DFA;  /* 839DF0 bcc */
    cmp8_v(0xB0);  /* 839DF2 cmp */
    if (IF_C) goto L_839DFE;  /* 839DF4 bcs */
    lda8_v(0x01);  /* 839DF6 lda */
    goto L_839DD0;  /* 839DF8 bra */
L_839DFA:
    lda8_v(0x02);  /* 839DFA lda */
    goto L_839DD0;  /* 839DFC bra */
L_839DFE:
    lda8_v(0x04);  /* 839DFE lda */
    goto L_839DD0;  /* 839E00 bra */
L_839E02:
    lda8_v(0x03);  /* 839E02 lda */
    goto L_839DD0;  /* 839E04 bra */
L_839E06:
    lda8_v(0x05);  /* 839E06 lda */
    goto L_839DD0;  /* 839E08 bra */
}
