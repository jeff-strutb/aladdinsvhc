/*
 * backdrop_mover_actor.c: actor kind $7C, the backdrop mover (helpers in
 * backdrop_mover.c), and kind $A3, a shot that steps out of its sender and
 * flies off. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorEndPlaced(void);
void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_AnimStart(void);
void abi_BackdropOffsetStep(void);
void abi_MoverSound(void);

#define MOVER_FRAMES 0x40u  /* word: frames left of its move ($0333) */
#define MOVER_HUM 0x42u  /* frames until its hum is queued again */
#define MOVER_43 0x43u
#define A3_ANIMS 0xD507u  /* by $07, its animation (in the bank DB names) */
#define A3_STEPS 0xD509u  /* by $07, how far across it steps out (words) */

/*
 * @routine 84:9044 BackdropMover
 * @actor
 * WHAT IT DOES: Actor kind $7C: moves the backdrop layer against the play
 * field one way for $0333 frames with a hum every $30, stops with a clunk,
 * waits, and goes back the other way, for good.
 * HOW: Position cleared, $09 = $80, $42 and $43 cleared. Each move: $40 =
 * $0333 (word); each frame (BRK first) $42 down, past zero: MoverSound $3F
 * and $42 = $30; BackdropOffsetStep until it counts $40 out (Z set). Then
 * MoverSound $0D, COP $3F, the facing flipped.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $7C in state $0C (through the stub at
 * $83:928E). Exit: never returns.
 */
void abi_BackdropMover(void)
{
    op_rep(0x20);  /* 849044 rep */
    stz16(ea_dp(ACTOR_X));  /* 849046 stz */
    stz16(ea_dp(ACTOR_X + 2));  /* 849048 stz */
    stz16(ea_dp(ACTOR_Y + 1));  /* 84904A stz */
    op_sep(0x20);  /* 84904C sep */
    lda8_v(0x80);  /* 84904E lda */
    sta8(ea_dp(ACTOR_09));  /* 849050 sta */
    stz8(ea_dp(MOVER_HUM));  /* 849052 stz */
    stz8(ea_dp(MOVER_43));  /* 849054 stz */
L_849056:
    lda8_v(0x33);  /* 849056 lda */
    sta8(ea_dp(ACTOR_40));  /* 849058 sta */
    lda8_v(0x03);  /* 84905A lda */
    sta8(ea_dp(ACTOR_41));  /* 84905C sta */
L_84905E:
    actor_yield(0x84905Eu);  /* 84905E brk */
    MEM8(dec, ea_dp(MOVER_HUM));  /* 849060 dec */
    if (!IF_N) goto L_84906D;  /* 849062 bpl */
    lda8_v(0x3F);  /* 849064 lda */
    abi_MoverSound();  /* 849066 jsr */
    lda8_v(0x30);  /* 849069 lda */
    sta8(ea_dp(MOVER_HUM));  /* 84906B sta */
L_84906D:
    abi_BackdropOffsetStep();  /* 84906D jsr */
    if (!IF_Z) goto L_84905E;  /* 849070 bne */
    lda8_v(0x0D);  /* 849072 lda */
    abi_MoverSound();  /* 849074 jsr */
    lda8_v(0x3F);  /* 849077 lda */
    actor_wait(0x849079u);  /* 849079 cop */
    lda8(ea_dp(ACTOR_FACING));  /* 84907B lda */
    eor8_v(0x01);  /* 84907D eor */
    sta8(ea_dp(ACTOR_FACING));  /* 84907F sta */
    goto L_849056;  /* 849081 bra */
}


/*
 * @routine 84:8349 KindA3Shot
 * @actor
 * WHAT IT DOES: Actor kind $A3: a shot facing right; shown, it waits 8
 * frames, steps out across by A3_STEPS, waits 8 more, then flies off
 * (motion $6C) until it can no longer be hit, and ends.
 * HOW: $08 bit 1, $09 bit 7 set, its look $0400, flip and facing 0 (the
 * look's low byte), animation A3_ANIMS by $07; COP 8; x pixels plus
 * A3_STEPS by $07; COP 8; ActorMotionPreset $6C; each frame ActorFly while
 * bit 6 of $09 is set; then ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $A3 in state $0C (through the stub at
 * $83:9423). Exit: never returns; ends in ActorEndPlaced.
 */
void abi_KindA3Shot(void)
{
    lda8(ea_dp(ACTOR_08));  /* 848349 lda */
    ora8_v(0x02);  /* 84834B ora */
    sta8(ea_dp(ACTOR_08));  /* 84834D sta */
    lda8(ea_dp(ACTOR_09));  /* 84834F lda */
    ora8_v(0x80);  /* 848351 ora */
    sta8(ea_dp(ACTOR_09));  /* 848353 sta */
    op_rep(0x20);  /* 848355 rep */
    lda16_v(0x0400);  /* 848357 lda */
    sta16(ea_dp(ACTOR_LOOK));  /* 84835A sta */
    op_sep(0x20);  /* 84835C sep */
    stz8(ea_dp(ACTOR_FLIP));  /* 84835E stz */
    ldx8(ea_dp(ACTOR_07));  /* 848360 ldx */
    sta8(ea_dp(ACTOR_FACING));  /* 848362 sta */
    lda8(ea_absx(A3_ANIMS));  /* 848364 lda */
    abi_AnimStart();  /* 848367 jsl */
    lda8_v(0x08);  /* 84836B lda */
    actor_wait(0x84836Du);  /* 84836D cop */
    lda8(ea_dp(ACTOR_07));  /* 84836F lda */
    ACC8(asl);  /* 848371 asl */
    tax8();  /* 848372 tax */
    op_rep(0x20);  /* 848373 rep */
    clc();  /* 848375 clc */
    lda16(ea_absx(A3_STEPS));  /* 848376 lda */
    adc16_v(rd16(ea_dp(ACTOR_X + 1)));  /* 848379 adc */
    sta16(ea_dp(ACTOR_X + 1));  /* 84837B sta */
    op_sep(0x20);  /* 84837D sep */
    lda8_v(0x08);  /* 84837F lda */
    actor_wait(0x848381u);  /* 848381 cop */
    ldy8_v(0x6C);  /* 848383 ldy */
    abi_ActorMotionPreset();  /* 848385 jsl */
L_848389:
    actor_yield(0x848389u);  /* 848389 brk */
    abi_ActorFly();  /* 84838B jsl */
    lda8(ea_dp(ACTOR_09));  /* 84838F lda */
    bit8_i(0x40);  /* 848391 bit */
    if (!IF_Z) goto L_848389;  /* 848393 bne */
    abi_ActorEndPlaced(); return;  /* 848395 jml */
}
