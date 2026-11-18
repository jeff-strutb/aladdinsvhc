/*
 * kind3f.c: actor kind $3F, a platform that crumbles a while after
 * Aladdin first rides it. Its wait is Kind3FWait ($83:B1E8), which tests
 * the ride while K3F_CARRYING is nonzero.
 */
#include "machine.h"
#include "native.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorEndPlaced(void);
void abi_AnimStart(void);
void abi_SoundQueueAdd(void);

#define K3F_CARRYING 0x42u        /* nonzero: its wait tests the ride; frames left once ridden */
#define K3F_CARRIES 0x43u         /* nonzero: Aladdin rides it this frame (its wait's ActorTestCarries) */
#define K3F_FRAMES 0x40u          /* frames left of its crumbling */
#define K3F_TEST_BOX 0x4Cu        /* the ride test's box */
#define K3F_TEST_BOXES 0xD299u    /* K3F_TEST_BOX by $07 (bytes, bank DB) */

/* LDA #n or a computed A, JSL at `site`: its return address on the stack
   while the callee runs, as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:B19A Kind3FStart
 * @actor
 * WHAT IT DOES: Actor kind $3F, a platform: it waits until Aladdin rides
 * it, then $30 frames on it crumbles (its animation $07 * 2 + 1, sound
 * $2C) and ends $38 frames later.
 * HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set; facing and flip 0;
 * K3F_CARRIES 0; K3F_CARRYING = $30; K3F_TEST_BOX from K3F_TEST_BOXES by
 * $07; AnimStart $07 * 2. Each frame (BRK) until K3F_CARRIES is set;
 * then each frame K3F_CARRYING down to 0 (the wait stops testing);
 * AnimStart $07 * 2 + 1; SoundQueueAdd $2C; K3F_FRAMES = $38 frames;
 * K3F_CARRIES 0; then JML ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $3F in its start state (the table at
 * $83:8CAB). Exit: never returns.
 */
void abi_Kind3FStart(void)
{
    lda8(ea_dp(ACTOR_08));  /* 83B19A lda */
    ora8_v(0x02);  /* 83B19C ora */
    sta8(ea_dp(ACTOR_08));  /* 83B19E sta */
    lda8(ea_dp(ACTOR_09));  /* 83B1A0 lda */
    ora8_v(0x90);  /* 83B1A2 ora */
    sta8(ea_dp(ACTOR_09));  /* 83B1A4 sta */
    stz8(ea_dp(ACTOR_FACING));  /* 83B1A6 stz */
    stz8(ea_dp(ACTOR_FLIP));  /* 83B1A8 stz */
    stz8(ea_dp(K3F_CARRIES));  /* 83B1AA stz */
    lda8_v(0x30);  /* 83B1AC lda */
    sta8(ea_dp(K3F_CARRYING));  /* 83B1AE sta */
    ldx8(ea_dp(ACTOR_07));  /* 83B1B0 ldx */
    lda8(ea_absx(K3F_TEST_BOXES));  /* 83B1B2 lda */
    sta8(ea_dp(K3F_TEST_BOX));  /* 83B1B5 sta */
    lda8(ea_dp(ACTOR_07));  /* 83B1B7 lda */
    ACC8(asl);  /* 83B1B9 asl */
    jsl_call(0x83B1BAu, abi_AnimStart);  /* 83B1BA jsl */
    do {
        actor_yield(0x83B1BEu);  /* 83B1BE brk */
        lda8(ea_dp(K3F_CARRIES));  /* 83B1C0 lda */
    } while (IF_Z);  /* 83B1C2 beq */
    do {
        actor_yield(0x83B1C4u);  /* 83B1C4 brk */
        MEM8(dec, ea_dp(K3F_CARRYING));  /* 83B1C6 dec */
    } while (!IF_Z);  /* 83B1C8 bne */
    lda8(ea_dp(ACTOR_07));  /* 83B1CA lda */
    ACC8(asl);  /* 83B1CC asl */
    ACC8(inc);  /* 83B1CD inc */
    jsl_call(0x83B1CEu, abi_AnimStart);  /* 83B1CE jsl */
    lda8_v(0x2C);  /* 83B1D2 lda */
    jsl_call(0x83B1D4u, abi_SoundQueueAdd);  /* 83B1D4 jsl */
    lda8_v(0x38);  /* 83B1D8 lda */
    sta8(ea_dp(K3F_FRAMES));  /* 83B1DA sta */
    stz8(ea_dp(K3F_CARRIES));  /* 83B1DC stz */
    do {
        actor_yield(0x83B1DEu);  /* 83B1DE brk */
        MEM8(dec, ea_dp(K3F_FRAMES));  /* 83B1E0 dec */
    } while (!IF_Z);  /* 83B1E2 bne */
    abi_ActorEndPlaced();  /* 83B1E4 jml */
}
