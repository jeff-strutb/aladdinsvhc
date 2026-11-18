/*
 * kind9a.c: actor kinds $9A and $99 (stage a5 s2): kind $9A, dropped by
 * kind $98, falls to the ground and bursts into 32 kind $99: two that run
 * off along the ground, the rest flashing out in a spread and gone. Their
 * waits are Kind9AWaitWork and Kind99WaitWork (kind_waits_84.c).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_SoundQueueAdd(void);
void abi_ActorMoveX(void);
void abi_ActorFly(void);
void abi_ActorOutOfGround(void);
void abi_ActorCreateChild8(void);
void abi_ActorEndPlaced(void);
void abi_Kind9ASpawn(void);

#define ACTOR_PROBES 0x16u      /* its probe list's address (word) */
#define K9A_FRAMES 0x40u        /* kind $9A: pairs of children left to make */
#define K9A_BOUNCES 0x41u       /* kind $9A: nonzero once burst: Aladdin bounces off it (Kind9AWaitWork) */
#define K9A_PROBES 0xD96Eu      /* (data bank $80) kind $9A's probes */
#define K9A_CHILD_KIND 0x99
#define K99_ANIMS 0xD90Eu       /* (data bank $80) kind $99's animation by $07 (bytes) */
#define K99_SPREAD 0xD92Eu      /* (data bank $80) kind $99's offset across by $07 (words) */

/* A JSL at `site` to fn, its return address on the stack while fn runs, as
   the original's (frame_step.c's model). */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 84:A3B7 Kind99Start
 * @actor
 * WHAT IT DOES: Actor kind $99, a piece of a kind $9A's burst, by its $07
 * (0-31, from its maker): 0 and 1 run off along the ground, right or
 * left, until they can no longer be hit; the others show for 8 frames, are
 * put across by their offset (right for even $07, left for odd), hide for
 * 16 frames and end.
 * HOW: $08 bit 1 and $09 bits $90 set; facing and flip = $07 AND 1;
 * AnimStart K99_ANIMS by $07. $07 AND $FE zero: COP 8; its speed across
 * 8 pixels; each frame (BRK) ActorMoveX while bit 6 of $09 is set; then
 * ActorEndPlaced. Else: $08 bit 3 cleared; COP 8; 16-bit, its x plus
 * (or, $07 odd, less) K99_SPREAD by $07 AND $1F; $08 bit 3 set (TSB);
 * COP $10; ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $99 in state $0C (the table at $83:8DAB,
 * through the stub at $83:93CB). Exit: never returns.
 */
void abi_Kind99Start(void)
{
    lda8(ea_dp(ACTOR_08));
    ora8_v(0x02);
    sta8(ea_dp(ACTOR_08));
    lda8(ea_dp(ACTOR_09));
    ora8_v(0x90);
    sta8(ea_dp(ACTOR_09));
    lda8(ea_dp(ACTOR_07));
    and8_v(0x01);
    sta8(ea_dp(ACTOR_FACING));
    sta8(ea_dp(ACTOR_FLIP));
    ldx8(ea_dp(ACTOR_07));
    lda8(ea_absx(K99_ANIMS));
    jsl_call(0x84A3D0u, abi_AnimStart);
    lda8(ea_dp(ACTOR_07));
    and8_v(0xFE);
    if (IF_Z) {
        lda8_v(0x08);
        actor_wait(0x84A41Eu);
        lda8_v(0x08);
        sta8(ea_dp(ACTOR_SPEED_X + 1));
        stz8(ea_dp(ACTOR_SPEED_X));
        stz8(ea_dp(ACTOR_SPEED_X + 2));
        do {
            actor_yield(0x84A428u);
            jsl_call(0x84A42Au, abi_ActorMoveX);
            lda8(ea_dp(ACTOR_09));
            and8_v(0x40);
        } while (!IF_Z);
        abi_ActorEndPlaced();
        return;
    }
    lda8(ea_dp(ACTOR_08));
    and8_v(0xF7);
    sta8(ea_dp(ACTOR_08));
    lda8_v(0x08);
    actor_wait(0x84A3E2u);
    lda8(ea_dp(ACTOR_07));
    and8_v(0x01);
    int left = !IF_Z;
    op_rep(0x20);
    lda16(ea_dp(ACTOR_07));
    and16_v(0x001F);
    ACC16(asl);
    tax8();
    lda16(ea_dp(ACTOR_X + 1));
    if (!left) {
        clc();
        adc16_v(rd16(ea_absx(K99_SPREAD)));
    } else {
        sec();
        sbc16_v(rd16(ea_absx(K99_SPREAD)));
    }
    sta16(ea_dp(ACTOR_X + 1));
    op_sep(0x20);
    lda8_v(0x08);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0x10);
    actor_wait(0x84A418u);
    abi_ActorEndPlaced();
}

/*
 * @routine 84:A446 Kind9AStart
 * @actor
 * WHAT IT DOES: Actor kind $9A, dropped by kind $98: falls until it is in
 * the ground, sets down 4 pixels lower with sound $48, and after 8 frames
 * with sound $3E bursts: two kind $99 every 2 frames, 32 in all ($07 0 to
 * 31). Then Aladdin bounces off it (K9A_BOUNCES); after $80 frames it
 * hides ($08 bit 4), after $80 more it ends.
 * HOW: K9A_BOUNCES 0; $08 bit 1 and $09 bits $90 set; 16-bit: probes
 * K9A_PROBES, the look's palette bits $0600 set, its speed across 0, down
 * $0100 (its top bytes 0); gravity $30; AnimStart $17. Each frame (BRK)
 * ActorFly and ActorOutOfGround until that raises it (Z clear). Its y
 * low byte + 4; AnimStart $18; SoundQueueAdd $48; COP 8; AnimStart $1E;
 * SoundQueueAdd $3E; K9A_FRAMES $10, $07 0; each time COP 2, Kind9ASpawn
 * twice. K9A_BOUNCES up; AnimStart $1F; COP $80; $08 bit 4 set; COP $80;
 * ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $9A in state $0C (the table at $83:8DAB,
 * through the stub at $83:93D8). Exit: never returns.
 */
void abi_Kind9AStart(void)
{
    stz8(ea_dp(K9A_BOUNCES));
    lda8(ea_dp(ACTOR_08));
    ora8_v(0x02);
    sta8(ea_dp(ACTOR_08));
    lda8(ea_dp(ACTOR_09));
    ora8_v(0x90);
    sta8(ea_dp(ACTOR_09));
    op_rep(0x20);
    lda16_v(K9A_PROBES);
    sta16(ea_dp(ACTOR_PROBES));
    lda16(ea_dp(ACTOR_LOOK));
    ora16_v(0x0600);
    sta16(ea_dp(ACTOR_LOOK));
    stz16(ea_dp(ACTOR_SPEED_X));
    lda16_v(0x0100);
    sta16(ea_dp(ACTOR_SPEED_Y));
    op_sep(0x20);
    stz8(ea_dp(ACTOR_SPEED_X + 2));
    stz8(ea_dp(ACTOR_SPEED_Y + 2));
    lda8_v(0x30);
    sta8(ea_dp(ACTOR_ACCEL_Y));
    lda8_v(0x17);
    jsl_call(0x84A475u, abi_AnimStart);
    do {
        actor_yield(0x84A479u);
        jsl_call(0x84A47Bu, abi_ActorFly);
        jsl_call(0x84A47Fu, abi_ActorOutOfGround);
    } while (IF_Z);
    lda8(ea_dp(ACTOR_Y + 1));
    clc();
    adc8_v(0x04);
    sta8(ea_dp(ACTOR_Y + 1));
    lda8_v(0x18);
    jsl_call(0x84A48Eu, abi_AnimStart);
    lda8_v(0x48);
    jsl_call(0x84A494u, abi_SoundQueueAdd);
    lda8_v(0x08);
    actor_wait(0x84A49Au);
    lda8_v(0x1E);
    jsl_call(0x84A49Eu, abi_AnimStart);
    lda8_v(0x3E);
    jsl_call(0x84A4A4u, abi_SoundQueueAdd);
    lda8_v(0x10);
    sta8(ea_dp(K9A_FRAMES));
    stz8(ea_dp(ACTOR_07));
    do {
        lda8_v(0x02);
        actor_wait(0x84A4B0u);
        call_frame(0xA4B4u, abi_Kind9ASpawn);
        call_frame(0xA4B7u, abi_Kind9ASpawn);
        MEM8(dec, ea_dp(K9A_FRAMES));
    } while (!IF_Z);
    MEM8(inc, ea_dp(K9A_BOUNCES));
    lda8_v(0x1F);
    jsl_call(0x84A4C0u, abi_AnimStart);
    lda8_v(0x80);
    actor_wait(0x84A4C6u);
    lda8(ea_dp(ACTOR_08));
    ora8_v(0x10);
    sta8(ea_dp(ACTOR_08));
    lda8_v(0x80);
    actor_wait(0x84A4D0u);
    abi_ActorEndPlaced();
}

/*
 * @routine 84:A4D4 Kind9ASpawn
 * WHAT IT DOES: Makes one kind $99 of a kind $9A's burst, with its $07,
 * then counts its $07 up for the next.
 * HOW: Spawn place 0; ActorCreateChild8 with A = $99; $07 up.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSR from Kind9AStart. Exit: M and X 8-bit; as ActorCreateChild8 leaves
 * A and the flags but N and Z from the $07 increment.
 */
void abi_Kind9ASpawn(void)
{
    stz8(ea_dp(ACTOR_SPAWN_AT));
    lda8_v(K9A_CHILD_KIND);
    jsl_call(0x84A4D8u, abi_ActorCreateChild8);
    MEM8(inc, ea_dp(ACTOR_07));
}
