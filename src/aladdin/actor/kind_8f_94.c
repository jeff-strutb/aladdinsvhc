/*
 * kind_8f_94.c: actor kinds $8F (a figure that crosses the screen and
 * back, a song playing, its x kept for other code) and $94 (a spawner that
 * starts one more of its kind at each step of a count). Drafted by
 * cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorEndPlaced(void);
void abi_ActorMoveX(void);
void abi_ActorPictureSend(void);
void abi_ActorSpawn(void);
void abi_ActorSpeedXSet(void);
void abi_AnimStart(void);
void abi_SongPlay(void);

#define V_00A5 0x00A5u  /* the walker's x pixels (word), kept for other code */
#define V_00AD 0x00ADu  /* a count the scene steps; each new value, one more spark */
#define WALK_TIME 0x40u  /* frames left of its crossing */
#define SPARK_STEP 0x40u  /* the spawner: the count it last acted on */
#define SPARK_X 0x41u  /* the spawner: the next one's x pixels */
#define SPARK_Y 0x42u  /* the spawner: the next one's y pixels */
#define KIND8F_29 0x29u  /* word: $890B */
#define SPARK_PLACES 0xD87Cu  /* by count, x and y pixels (bytes), in the bank DB names */

/*
 * @routine 83:FA4F Kind8FWalk
 * @actor
 * WHAT IT DOES: Actor kind $8F: starts off the screen's left edge, waits,
 * plays song $27, then walks right across the screen for $56 frames, comes
 * back left $80 lower for as long, and ends; its x is kept in V_00A5 all
 * the while.
 * HOW: $09 bits $90 set, $29 = $890B, x = V_00A5 = -32; COP $3A; SongPlay
 * $27; COP 6; animation 0, ActorPictureSend, ActorSpeedXSet $5D. $56
 * frames: ActorMoveX, its x pixels to V_00A5. Facing right then: y pixels'
 * low byte = $80, facing and flip 1, again; else ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $8F in state $0C (through the stub at
 * $83:9355). Exit: never returns; ends in ActorEndPlaced.
 */
void abi_Kind8FWalk(void)
{
    lda8_v(0x00);  /* 83FA4F lda */
    tsb8(ea_dp(ACTOR_08));  /* 83FA51 tsb */
    lda8_v(0x90);  /* 83FA53 lda */
    tsb8(ea_dp(ACTOR_09));  /* 83FA55 tsb */
    op_rep(0x20);  /* 83FA57 rep */
    lda16_v(0x890B);  /* 83FA59 lda */
    sta16(ea_dp(KIND8F_29));  /* 83FA5C sta */
    lda16_v(0xFFE0);  /* 83FA5E lda */
    sta16(ea_dp(ACTOR_X + 1));  /* 83FA61 sta */
    sta16(ea_abs(V_00A5));  /* 83FA63 sta */
    op_sep(0x20);  /* 83FA66 sep */
    lda8_v(0x3A);  /* 83FA68 lda */
    actor_wait(0x83FA6Au);  /* 83FA6A cop */
    lda8_v(0x27);  /* 83FA6C lda */
    abi_SongPlay();  /* 83FA6E jsl */
    lda8_v(0x06);  /* 83FA72 lda */
    actor_wait(0x83FA74u);  /* 83FA74 cop */
    lda8_v(0x00);  /* 83FA76 lda */
    abi_AnimStart();  /* 83FA78 jsl */
    abi_ActorPictureSend();  /* 83FA7C jsl */
    ldy8_v(0x5D);  /* 83FA80 ldy */
    abi_ActorSpeedXSet();  /* 83FA82 jsl */
L_83FA86:
    lda8_v(0x56);  /* 83FA86 lda */
    sta8(ea_dp(ACTOR_40));  /* 83FA88 sta */
L_83FA8A:
    actor_yield(0x83FA8Au);  /* 83FA8A brk */
    abi_ActorMoveX();  /* 83FA8C jsl */
    lda8(ea_dp(ACTOR_X + 1));  /* 83FA90 lda */
    sta8(ea_abs(V_00A5));  /* 83FA92 sta */
    lda8(ea_dp(ACTOR_X + 2));  /* 83FA95 lda */
    sta8(ea_abs(V_00A5 + 1));  /* 83FA97 sta */
    MEM8(dec, ea_dp(ACTOR_40));  /* 83FA9A dec */
    if (!IF_Z) goto L_83FA8A;  /* 83FA9C bne */
    lda8(ea_dp(ACTOR_FACING));  /* 83FA9E lda */
    if (!IF_Z) goto L_83FAB7;  /* 83FAA0 bne */
    lda8_v(0x80);  /* 83FAA2 lda */
    sta8(ea_dp(ACTOR_Y + 1));  /* 83FAA4 sta */
    lda8_v(0x01);  /* 83FAA6 lda */
    sta8(ea_dp(ACTOR_FACING));  /* 83FAA8 sta */
    sta8(ea_dp(ACTOR_FLIP));  /* 83FAAA sta */
    goto L_83FA86;  /* 83FAAC bra */
L_83FAB7:
    abi_ActorEndPlaced(); return;  /* 83FAB7 jml */
}


/*
 * @routine 83:FABB Kind94Spark
 * @actor
 * WHAT IT DOES: Actor kind $94. With $07 zero, one spark: shown $20 frames
 * with animation 0, then ended. Else the spawner: each frame V_00AD has a
 * new value, it starts a spark (its own kind, $07 0) at the place
 * SPARK_PLACES gives for it, with its own look.
 * HOW: Spark: $08 bit 0 and $09 bits $90 set, the high bytes of x and y
 * cleared, animation 0, COP $20, ActorEndPlaced. Spawner: $40 cleared;
 * each frame (BRK first) V_00AD compared with $40; new: kept, SPARK_PLACES
 * at twice it to $41 and $42; ActorSpawn of its kind; a record (N clear):
 * its $2B and $2C, x and y pixels' low bytes from $41 and $42, $07 0.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $94 in state $0C (through the stub at
 * $83:9396). Exit: never returns; a spark ends in ActorEndPlaced.
 */
void abi_Kind94Spark(void)
{
    lda8(ea_dp(ACTOR_07));  /* 83FABB lda */
    if (!IF_Z) goto L_83FAD9;  /* 83FABD bne */
    lda8_v(0x01);  /* 83FABF lda */
    tsb8(ea_dp(ACTOR_08));  /* 83FAC1 tsb */
    lda8_v(0x90);  /* 83FAC3 lda */
    tsb8(ea_dp(ACTOR_09));  /* 83FAC5 tsb */
    stz8(ea_dp(ACTOR_X + 2));  /* 83FAC7 stz */
    stz8(ea_dp(ACTOR_Y + 2));  /* 83FAC9 stz */
    lda8_v(0x00);  /* 83FACB lda */
    abi_AnimStart();  /* 83FACD jsl */
    lda8_v(0x20);  /* 83FAD1 lda */
    actor_wait(0x83FAD3u);  /* 83FAD3 cop */
    abi_ActorEndPlaced(); return;  /* 83FAD5 jml */
L_83FAD9:
    stz8(ea_dp(ACTOR_40));  /* 83FAD9 stz */
L_83FADB:
    actor_yield(0x83FADBu);  /* 83FADB brk */
    lda8(ea_abs(V_00AD));  /* 83FADD lda */
    cmp8_v(rd8(ea_dp(ACTOR_40)));  /* 83FAE0 cmp */
    if (IF_Z) goto L_83FADB;  /* 83FAE2 beq */
    sta8(ea_dp(ACTOR_40));  /* 83FAE4 sta */
    ACC8(asl);  /* 83FAE6 asl */
    tax8();  /* 83FAE7 tax */
    lda8(ea_absx(SPARK_PLACES));  /* 83FAE8 lda */
    sta8(ea_dp(ACTOR_41));  /* 83FAEB sta */
    lda8(ea_absx(SPARK_PLACES + 1));  /* 83FAED lda */
    sta8(ea_dp(ACTOR_42));  /* 83FAF0 sta */
    lda8(ea_dp(ACTOR_06));  /* 83FAF2 lda */
    abi_ActorSpawn();  /* 83FAF4 jsl */
    if (IF_N) goto L_83FB11;  /* 83FAF8 bmi */
    lda8(ea_dp(ACTOR_LOOK));  /* 83FAFA lda */
    sta8(ea_absx(ACTOR_LOOK));  /* 83FAFC sta */
    lda8(ea_dp(ACTOR_2C));  /* 83FAFF lda */
    sta8(ea_absx(ACTOR_2C));  /* 83FB01 sta */
    lda8(ea_dp(ACTOR_41));  /* 83FB04 lda */
    sta8(ea_absx(ACTOR_X + 1));  /* 83FB06 sta */
    lda8(ea_dp(ACTOR_42));  /* 83FB09 lda */
    sta8(ea_absx(ACTOR_Y + 1));  /* 83FB0B sta */
    stz8(ea_absx(ACTOR_07));  /* 83FB0E stz */
L_83FB11:
    op_sep(0x10);  /* 83FB11 sep */
    goto L_83FADB;  /* 83FB13 bra */
}
