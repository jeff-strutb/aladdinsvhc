/*
 * thrown_shot.c: actor kinds $10 and $14-$17 of the second pool: a shot
 * that flies its course, splashing (kind $13) where it lands, and kind
 * $10's end when it hits (kind $11). Drafted by cdraft.py; named and
 * described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"
#include "wram_video.h"
#include "wram_player.h"

void abi_ActorAllocBLong(void);
void abi_ActorEnd(void);
void abi_ActorInGround(void);
void abi_ActorMotionPreset(void);
void abi_ActorMove(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);
void abi_Spawn13AtYLong(void);

#define SHOT_16 0x16u  /* word: $C9FF */
#define SHOT_11 0x11u
#define SHOT_LAUNCH 0x40u  /* frames left before bit 7 of $08 is cleared */
#define V_1774 0x1774u  /* the high byte of the word at $1773 (the camera's x) */

/*
 * @routine 81:EABA ShotFly
 * @actor
 * WHAT IT DOES: A shot (kinds $10 and $14-$17) started new: it flies the
 * course its $07 picks until it lands in the ground, where it splashes (an
 * actor of kind $13 in its place) and ends; it ends at once when it can no
 * longer be hit, and, in levels 8 and $10, when it is above the level's
 * top, in level $0E, when it is at the top and the camera is left of x
 * $0700.
 * HOW: Position fractions cleared, $09 = $50, animation 7, $2C = 4,
 * ActorMotionPreset by $07; fractions cleared again, $16 = $C9FF, $11 and
 * $18 cleared, bit 7 of $08 set, $40 = 2. Each frame (BRK first): the
 * level tests (LEVEL 8 or $10: y's high byte negative; $0E: V_1774 below 7
 * and y's high byte 0) and bit 6 of $09 clear end it (ActorEnd); else
 * AnimStep, ActorMove; after 2 frames bit 7 of $08 cleared; ActorInGround
 * with C set and bit 6 of V_0012 clear: Spawn13AtYLong at its own record
 * (Y = D), then ActorEnd.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; reached by
 * the actor loop's jump for kinds $10, $14-$17 in state $0C (through the
 * stub at $83:920E). Exit: never returns; ends in ActorEnd.
 */
void abi_ShotFly(void)
{
    stz8(ea_dp(ACTOR_X));  /* 81EABA stz */
    stz8(ea_dp(ACTOR_Y));  /* 81EABC stz */
    lda8_v(0x50);  /* 81EABE lda */
    sta8(ea_dp(ACTOR_09));  /* 81EAC0 sta */
    lda8_v(0x07);  /* 81EAC2 lda */
    abi_AnimStart();  /* 81EAC4 jsl */
    lda8_v(0x04);  /* 81EAC8 lda */
    sta8(ea_dp(ACTOR_2C));  /* 81EACA sta */
    ldy8(ea_dp(ACTOR_07));  /* 81EACC ldy */
    abi_ActorMotionPreset();  /* 81EACE jsl */
    stz8(ea_dp(ACTOR_X));  /* 81EAD2 stz */
    stz8(ea_dp(ACTOR_Y));  /* 81EAD4 stz */
    op_rep(0x20);  /* 81EAD6 rep */
    lda16_v(0xC9FF);  /* 81EAD8 lda */
    sta16(ea_dp(SHOT_16));  /* 81EADB sta */
    op_sep(0x20);  /* 81EADD sep */
    stz8(ea_dp(SHOT_11));  /* 81EADF stz */
    stz8(ea_dp(ACTOR_18));  /* 81EAE1 stz */
    lda8_v(0x80);  /* 81EAE3 lda */
    tsb8(ea_dp(ACTOR_08));  /* 81EAE5 tsb */
    lda8_v(0x02);  /* 81EAE7 lda */
    sta8(ea_dp(SHOT_LAUNCH));  /* 81EAE9 sta */
L_81EAEB:
    actor_yield(0x81EAEBu);  /* 81EAEB brk */
    lda8(ea_abs(LEVEL));  /* 81EAED lda */
    cmp8_v(0x08);  /* 81EAF0 cmp */
    if (IF_Z) goto L_81EB09;  /* 81EAF2 beq */
    cmp8_v(0x10);  /* 81EAF4 cmp */
    if (IF_Z) goto L_81EB09;  /* 81EAF6 beq */
    cmp8_v(0x0E);  /* 81EAF8 cmp */
    if (!IF_Z) goto L_81EB0D;  /* 81EAFA bne */
    lda8(ea_abs(V_1774));  /* 81EAFC lda */
    cmp8_v(0x07);  /* 81EAFF cmp */
    if (IF_C) goto L_81EB0D;  /* 81EB01 bcs */
    lda8(ea_dp(ACTOR_Y + 2));  /* 81EB03 lda */
    if (IF_Z) goto L_81EB76;  /* 81EB05 beq */
    goto L_81EB0D;  /* 81EB07 bra */
L_81EB09:
    lda8(ea_dp(ACTOR_Y + 2));  /* 81EB09 lda */
    if (IF_N) goto L_81EB76;  /* 81EB0B bmi */
L_81EB0D:
    lda8(ea_dp(ACTOR_09));  /* 81EB0D lda */
    and8_v(0x40);  /* 81EB0F and */
    if (IF_Z) goto L_81EB76;  /* 81EB11 beq */
    abi_AnimStep();  /* 81EB13 jsl */
    abi_ActorMove();  /* 81EB17 jsl */
    lda8(ea_dp(SHOT_LAUNCH));  /* 81EB1B lda */
    if (IF_Z) goto L_81EB29;  /* 81EB1D beq */
    MEM8(dec, ea_dp(SHOT_LAUNCH));  /* 81EB1F dec */
    if (!IF_Z) goto L_81EAEB;  /* 81EB21 bne */
    lda8(ea_dp(ACTOR_08));  /* 81EB23 lda */
    and8_v(0x7F);  /* 81EB25 and */
    sta8(ea_dp(ACTOR_08));  /* 81EB27 sta */
L_81EB29:
    abi_ActorInGround();  /* 81EB29 jsl */
    if (!IF_C) goto L_81EAEB;  /* 81EB2D bcc */
    bit8_v(rd8(ea_abs(V_0012)));  /* 81EB2F bit */
    if (IF_V) goto L_81EAEB;  /* 81EB32 bvs */
    op_rep(0x30);  /* 81EB34 rep */
    tdc();  /* 81EB36 tdc */
    tay16();  /* 81EB37 tay */
    op_sep(0x20);  /* 81EB38 sep */
    abi_Spawn13AtYLong();  /* 81EB3A jsl */
    op_sep(0x30);  /* 81EB3E sep */
    abi_ActorEnd(); return;  /* 81EB40 jml */
L_81EB76:
    op_sep(0x30);  /* 81EB76 sep */
    abi_ActorEnd(); return;  /* 81EB78 jml */
}


/*
 * @routine 81:EB44 ShotHit
 * @actor
 * WHAT IT DOES: Actor kind $10 in state $4C (it hit something): a frame
 * later, sound 8 and an actor of kind $11 in its place from the second
 * pool, then its end.
 * HOW: BRK; SoundQueueAdd 8; bit 7 of $08 set; ActorAllocBLong; a record
 * (N clear): state $0C, kind $11, $07 0, $08 $80, its x and y pixels; then
 * (also with none) ActorEnd, M and X 8-bit.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; reached by
 * the actor loop's jump for kind $10 in state $4C and kind $B9 in state
 * $0C (through the stub at $83:9212). Exit: never returns; ends in
 * ActorEnd.
 */
void abi_ShotHit(void)
{
    actor_yield(0x81EB44u);  /* 81EB44 brk */
    lda8_v(0x08);  /* 81EB46 lda */
    abi_SoundQueueAdd();  /* 81EB48 jsl */
    lda8(ea_dp(ACTOR_08));  /* 81EB4C lda */
    ora8_v(0x80);  /* 81EB4E ora */
    sta8(ea_dp(ACTOR_08));  /* 81EB50 sta */
    abi_ActorAllocBLong();  /* 81EB52 jsl */
    if (IF_N) goto L_81EB76;  /* 81EB56 bmi */
    lda8_v(0x0C);  /* 81EB58 lda */
    sta8(ea_absx(ACTOR_STATE));  /* 81EB5A sta */
    lda8_v(0x11);  /* 81EB5D lda */
    sta8(ea_absx(ACTOR_06));  /* 81EB5F sta */
    stz8(ea_absx(ACTOR_07));  /* 81EB62 stz */
    lda8_v(0x80);  /* 81EB65 lda */
    sta8(ea_absx(ACTOR_08));  /* 81EB67 sta */
    op_rep(0x20);  /* 81EB6A rep */
    lda16(ea_dp(ACTOR_X + 1));  /* 81EB6C lda */
    sta16(ea_absx(ACTOR_X + 1));  /* 81EB6E sta */
    lda16(ea_dp(ACTOR_Y + 1));  /* 81EB71 lda */
    sta16(ea_absx(ACTOR_Y + 1));  /* 81EB73 sta */
L_81EB76:
    op_sep(0x30);  /* 81EB76 sep */
    abi_ActorEnd(); return;  /* 81EB78 jml */
}
