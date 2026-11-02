/*
 * player_scene_moves.c: scripted figures of Aladdin (bank $A5: started at
 * his place, their way picked by $07 through the table at $A5:E56D): the
 * ways that run and leap, and that hop for good. Drafted by cdraft.py;
 * named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorFall(void);
void abi_ActorFly(void);
void abi_ActorMoveX(void);
void abi_ActorOutOfGround(void);
void abi_ActorSnapToGround(void);
void abi_ActorSpeedXFromA5(void);
void abi_ActorSpeedYFromA5(void);
void abi_AnimStart(void);

#define SCENE_CUE 0x146Cu  /* counted up by scripted actors as the scene goes on */
#define HOP_TIME 0x42u  /* frames left of its run */

/*
 * @routine A5:EC45 PlayerSceneLeap
 * @actor
 * WHAT IT DOES: The scripted figure's way 2: it stands $20 frames, runs
 * along the ground until it leaves it, then leaps; in the air until the
 * scene's cue, then holds a pose until the next cue, and stays.
 * HOW: Animation 0, COP $20; animation 2, ActorSpeedXFromA5 $0F; each
 * frame ActorMoveX and ActorSnapToGround while Z is clear; SCENE_CUE
 * cleared, animation $0C, gravity $38, ActorSpeedYFromA5 $12; each frame
 * ActorFly until SCENE_CUE is set; it is cleared, animation $0D; BRK until
 * SCENE_CUE is set; bit 7 of $09 cleared; COP $7F for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
 * the jump through the table at $A5:E56D for $07 = 2. Exit: never returns.
 */
void abi_PlayerSceneLeap(void)
{
    lda8_v(0x00);  /* A5EC45 lda */
    abi_AnimStart();  /* A5EC47 jsl */
    lda8_v(0x20);  /* A5EC4B lda */
    actor_wait(0xA5EC4Du);  /* A5EC4D cop */
    lda8_v(0x02);  /* A5EC4F lda */
    abi_AnimStart();  /* A5EC51 jsl */
    ldy8_v(0x0F);  /* A5EC55 ldy */
    abi_ActorSpeedXFromA5();  /* A5EC57 jsr */
L_A5EC5A:
    actor_yield(0xA5EC5Au);  /* A5EC5A brk */
    abi_ActorMoveX();  /* A5EC5C jsl */
    abi_ActorSnapToGround();  /* A5EC60 jsl */
    if (!IF_Z) goto L_A5EC5A;  /* A5EC64 bne */
    stz8(ea_abs(SCENE_CUE));  /* A5EC66 stz */
    lda8_v(0x0C);  /* A5EC69 lda */
    abi_AnimStart();  /* A5EC6B jsl */
    lda8_v(0x38);  /* A5EC6F lda */
    sta8(ea_dp(ACTOR_ACCEL_Y));  /* A5EC71 sta */
    ldy8_v(0x12);  /* A5EC73 ldy */
    abi_ActorSpeedYFromA5();  /* A5EC75 jsr */
L_A5EC78:
    actor_yield(0xA5EC78u);  /* A5EC78 brk */
    abi_ActorFly();  /* A5EC7A jsl */
    lda8(ea_abs(SCENE_CUE));  /* A5EC7E lda */
    if (IF_Z) goto L_A5EC78;  /* A5EC81 beq */
    stz8(ea_abs(SCENE_CUE));  /* A5EC83 stz */
    lda8_v(0x0D);  /* A5EC86 lda */
    abi_AnimStart();  /* A5EC88 jsl */
L_A5EC8C:
    actor_yield(0xA5EC8Cu);  /* A5EC8C brk */
    lda8(ea_abs(SCENE_CUE));  /* A5EC8E lda */
    if (IF_Z) goto L_A5EC8C;  /* A5EC91 beq */
    lda8_v(0x80);  /* A5EC93 lda */
    trb8(ea_dp(ACTOR_09));  /* A5EC95 trb */
L_A5EC97:
    lda8_v(0x7F);  /* A5EC97 lda */
    actor_wait(0xA5EC99u);  /* A5EC99 cop */
    goto L_A5EC97;  /* A5EC9B bra */
}


/*
 * @routine A5:EC9D PlayerSceneHops
 * @actor
 * WHAT IT DOES: The scripted figure's way 3: it drops to the ground facing
 * left, then for good: runs $30 frames along the ground, stands, holds a
 * pose, and jumps up and lands again.
 * HOW: $4D = 2, facing and flip 1, gravity $38, ActorSpeedYFromA5 6;
 * ActorFall each frame until ActorOutOfGround. Then for good:
 * ActorSpeedYFromA5 6, ActorSpeedXFromA5 $0C, animation 1, $30 frames of
 * ActorMoveX and ActorSnapToGround; animation 0, COP $30; animation $0A,
 * COP $10; animation 0, gravity $38, ActorSpeedYFromA5 $0C, ActorFall each
 * frame until ActorOutOfGround.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
 * the jump through the table at $A5:E56D for $07 = 3. Exit: never returns.
 */
void abi_PlayerSceneHops(void)
{
    lda8_v(0x02);  /* A5EC9D lda */
    sta8(ea_dp(ACTOR_4D));  /* A5EC9F sta */
    lda8_v(0x01);  /* A5ECA1 lda */
    sta8(ea_dp(ACTOR_FACING));  /* A5ECA3 sta */
    sta8(ea_dp(ACTOR_FLIP));  /* A5ECA5 sta */
    lda8_v(0x38);  /* A5ECA7 lda */
    sta8(ea_dp(ACTOR_ACCEL_Y));  /* A5ECA9 sta */
    ldy8_v(0x06);  /* A5ECAB ldy */
    abi_ActorSpeedYFromA5();  /* A5ECAD jsr */
L_A5ECB0:
    actor_yield(0xA5ECB0u);  /* A5ECB0 brk */
    abi_ActorFall();  /* A5ECB2 jsl */
    abi_ActorOutOfGround();  /* A5ECB6 jsl */
    if (IF_Z) goto L_A5ECB0;  /* A5ECBA beq */
L_A5ECBC:
    ldy8_v(0x06);  /* A5ECBC ldy */
    abi_ActorSpeedYFromA5();  /* A5ECBE jsr */
    ldy8_v(0x0C);  /* A5ECC1 ldy */
    abi_ActorSpeedXFromA5();  /* A5ECC3 jsr */
    lda8_v(0x01);  /* A5ECC6 lda */
    abi_AnimStart();  /* A5ECC8 jsl */
    lda8_v(0x30);  /* A5ECCC lda */
    sta8(ea_dp(HOP_TIME));  /* A5ECCE sta */
L_A5ECD0:
    actor_yield(0xA5ECD0u);  /* A5ECD0 brk */
    abi_ActorMoveX();  /* A5ECD2 jsl */
    abi_ActorSnapToGround();  /* A5ECD6 jsl */
    MEM8(dec, ea_dp(HOP_TIME));  /* A5ECDA dec */
    if (!IF_Z) goto L_A5ECD0;  /* A5ECDC bne */
    lda8_v(0x00);  /* A5ECDE lda */
    abi_AnimStart();  /* A5ECE0 jsl */
    lda8_v(0x30);  /* A5ECE4 lda */
    actor_wait(0xA5ECE6u);  /* A5ECE6 cop */
    lda8_v(0x0A);  /* A5ECE8 lda */
    abi_AnimStart();  /* A5ECEA jsl */
    lda8_v(0x10);  /* A5ECEE lda */
    actor_wait(0xA5ECF0u);  /* A5ECF0 cop */
    lda8_v(0x00);  /* A5ECF2 lda */
    abi_AnimStart();  /* A5ECF4 jsl */
    lda8_v(0x38);  /* A5ECF8 lda */
    sta8(ea_dp(ACTOR_ACCEL_Y));  /* A5ECFA sta */
    ldy8_v(0x0C);  /* A5ECFC ldy */
    abi_ActorSpeedYFromA5();  /* A5ECFE jsr */
L_A5ED01:
    actor_yield(0xA5ED01u);  /* A5ED01 brk */
    abi_ActorFall();  /* A5ED03 jsl */
    abi_ActorOutOfGround();  /* A5ED07 jsl */
    if (IF_Z) goto L_A5ED01;  /* A5ED0B beq */
    goto L_A5ECBC;  /* A5ED0D bra */
}
