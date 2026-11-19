/*
 * kinda5.c: actor kind $A5 (stage a5 s2), by its $07: 0, the bomb kind
 * $98 throws after each hit, which falls to the ground, bursts and ends;
 * 1 to 3, a column of them set off from one of three places across the
 * top, a child a frame; $10 and up, one of the column's pieces, at a
 * height by its low bits, which poses and ends. Written instruction for
 * instruction from the assembly (kinda5.asm), every JSL with its return
 * frame.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorCreateChild8(void);
void abi_ActorEndPlaced(void);
void abi_ActorMoveY(void);
void abi_ActorOutOfGround(void);
void abi_ActorSpeedYSet(void);
void abi_AnimStart(void);

#define ACTOR_PROBES 0x16u      /* its probe list's address (word) */
#define KA5_PROBES 0xD984u      /* (data bank $80) the bomb's probes */
#define KA5_PLACES_X 0xD988u    /* (data bank $80) the column's x by $07 1-3, less one (words) */
#define KA5_HEIGHTS 0xD98Eu     /* (data bank $80) a piece's y pixels by its $07's low four bits */
#define KA5_PIECE 0x10          /* $07 bit 4: a piece of the column */
#define KA5_KIND 0xA5           /* the column's children are of this kind */
#define KA5_LAST 0x1C           /* the last piece's $07 plus one */
#define KA5_FALL 0x93           /* the bomb's fall: ActorSpeedYSet's entry */

/* A JSL at `site`: its return address on the stack while the callee runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/* $07 bit 4: a piece of the column. */
static void piece(void)
{
    lda8(ea_dp(ACTOR_07));
    and8_v(0x0F);
    tax8();
    and8_v(0x03);
    tay8();
    lda8(ea_absx(KA5_HEIGHTS));
    sta8(ea_dp(ACTOR_Y + 1));
    tya8();
    jsl_call(0x84ABBAu, abi_AnimStart);
    lda8_v(0x12);
    actor_wait(0x84ABC0u);
}

/* $07 1 to 3: the column's start. */
static void column(void)
{
    op_rep(P_M);
    lda16(ea_dp(ACTOR_07));
    and16_v(0x0003);
    ACC16(dec);
    ACC16(asl);
    tax16();
    lda16(ea_absx(KA5_PLACES_X));
    sta16(ea_dp(ACTOR_X + 1));
    stz16(ea_dp(ACTOR_Y + 1));
    op_sep(P_M);
    stz8(ea_dp(ACTOR_SPAWN_AT));
    stz8(ea_dp(ACTOR_07));
    lda8_v(KA5_KIND);
    jsl_call(0x84ABDEu, abi_ActorCreateChild8);
    lda8_v(KA5_PIECE);
    sta8(ea_dp(ACTOR_07));
    do {
        actor_yield(0x84ABE6u);
        stz8(ea_dp(ACTOR_SPAWN_AT));
        lda8_v(KA5_KIND);
        jsl_call(0x84ABECu, abi_ActorCreateChild8);
        MEM8(inc, ea_dp(ACTOR_07));
        lda8(ea_dp(ACTOR_07));
        cmp8_v(KA5_LAST);
    } while (!IF_Z);
}

/* $07 0: the bomb. */
static void bomb(void)
{
    op_rep(P_M);
    lda16_v(KA5_PROBES);
    sta16(ea_dp(ACTOR_PROBES));
    op_sep(P_M);
    lda8_v(0x04);
    jsl_call(0x84AC06u, abi_AnimStart);
    ldy8_v(KA5_FALL);
    jsl_call(0x84AC0Cu, abi_ActorSpeedYSet);
    do {
        actor_yield(0x84AC10u);
        jsl_call(0x84AC12u, abi_ActorMoveY);
        jsl_call(0x84AC16u, abi_ActorOutOfGround);
    } while (IF_Z);
    lda8(ea_dp(ACTOR_Y + 1));
    clc();
    adc8_v(0x08);
    sta8(ea_dp(ACTOR_Y + 1));
    lda8_v(0x05);
    jsl_call(0x84AC25u, abi_AnimStart);
    lda8_v(0x08);
    actor_wait(0x84AC2Bu);
}

/*
 * @routine 84:AB9C KindA5Start
 * @actor
 * WHAT IT DOES: Actor kind $A5, by its $07: 0, a bomb (thrown by kind $98)
 * that falls until it is in the ground, then bursts 8 pixels lower and
 * ends; 1 to 3, a column set off from x KA5_PLACES_X entry $07 - 1 at y 0,
 * one child of this kind a frame ($07 0, then $10 to $1B), then it ends;
 * $10 up, a piece of a column put at y pixels KA5_HEIGHTS entry by its
 * low four bits, posing (animation $07 AND 3) for $12 frames, then it
 * ends.
 * HOW: $08 bit 1 and $09 bits $90 set (TSB). $07 0: ACTOR_PROBES =
 * KA5_PROBES (16-bit); AnimStart 4; ActorSpeedYSet with Y = KA5_FALL;
 * each frame (BRK first) ActorMoveY, ActorOutOfGround until nonzero; y
 * pixels' low byte + 8; AnimStart 5; COP 8. $07 bit 4: X = $07 AND $0F,
 * Y = that AND 3; y pixels' low byte KA5_HEIGHTS,X; AnimStart with A = Y;
 * COP $12. Else (16-bit) x pixels = KA5_PLACES_X entry ($07 AND 3) - 1,
 * y pixels 0; ACTOR_SPAWN_AT 0, $07 0, ActorCreateChild8 with A =
 * KA5_KIND; $07 $10; each frame (BRK first): ACTOR_SPAWN_AT 0,
 * ActorCreateChild8 KA5_KIND, $07 up, until $07 = KA5_LAST. All end at
 * ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $A5 in state $0C (through the stub at
 * $83:9435). Exit: never returns.
 */
void abi_KindA5Start(void)
{
    lda8_v(0x02);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0x90);
    tsb8(ea_dp(ACTOR_09));
    lda8(ea_dp(ACTOR_07));
    if (IF_Z) {
        bomb();
    } else {
        and8_v(KA5_PIECE);
        if (!IF_Z)
            piece();
        else
            column();
    }
    abi_ActorEndPlaced();
}
