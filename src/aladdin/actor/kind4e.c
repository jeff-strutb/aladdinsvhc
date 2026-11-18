/*
 * kind4e.c: actor kind $4E of stage a5 s0, a thing Aladdin bounces off
 * when he comes down on it: it then plays a squash and spring back. Its
 * wait is Kind4EWait (kind_waits_rest.c). Named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "ops65.h"
#include "actor.h"

void abi_AnimStart(void);

#define K4E_STILL 0x42u         /* nonzero: Aladdin does not bounce on it (while it springs) */
#define K4E_POSE 0x41u          /* the animation of the squash or spring step */

/* From 83:CFB5: the rest, for good. */
static void kind4e_rest(void)
{
    stz8(ea_dp(K4E_STILL));  /* 83CFB5 stz */
    lda8_v(0x06);  /* 83CFB7 lda */
    jsl_frame(0x83CFB9u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83CFB9 jsl */
    for (;;)
        actor_yield(0x83CFBDu);  /* 83CFBD brk, 83CFBF bra */
}

/*
 * @routine 83:CFA9 Kind4EStart
 * @actor
 * WHAT IT DOES: Actor kind $4E started: it rests in animation 6; Aladdin
 * may bounce on it (Kind4EWait).
 * HOW: Bits 0-1 of $08 and 7 of $09 set; then the rest: K4E_STILL = 0,
 * AnimStart 6, BRK for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for state $0C. Exit: never returns.
 */
void abi_Kind4EStart(void)
{
    lda8(ea_dp(ACTOR_08));  /* 83CFA9 lda */
    ora8_v(0x03);  /* 83CFAB ora */
    sta8(ea_dp(ACTOR_08));  /* 83CFAD sta */
    lda8(ea_dp(ACTOR_09));  /* 83CFAF lda */
    ora8_v(0x80);  /* 83CFB1 ora */
    sta8(ea_dp(ACTOR_09));  /* 83CFB3 sta */
    kind4e_rest();
}

/*
 * @routine 83:CFC1 Kind4EBounced
 * @actor
 * WHAT IT DOES: Kind $4E after Aladdin bounced on it (Kind4EWait resumes
 * its code here): squashed, it springs back up, then rests again.
 * HOW: AnimStart 7, COP $10; K4E_STILL up; animations 5 down to 0, COP 8
 * after each; COP $30; animations 0 up to 5, COP 8 after each; then the
 * rest of Kind4EStart.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; the actor
 * loop resumes it here (the resume address Kind4EWait wrote). Exit:
 * never returns.
 */
void abi_Kind4EBounced(void)
{
    lda8_v(0x07);  /* 83CFC1 lda */
    jsl_frame(0x83CFC3u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83CFC3 jsl */
    lda8_v(0x10);  /* 83CFC7 lda */
    actor_wait(0x83CFC9u);  /* 83CFC9 cop */
    MEM8(inc, ea_dp(K4E_STILL));  /* 83CFCB inc */
    lda8_v(0x05);  /* 83CFCD lda */
    sta8(ea_dp(K4E_POSE));  /* 83CFCF sta */
L_83CFD1:
    lda8(ea_dp(K4E_POSE));  /* 83CFD1 lda */
    jsl_frame(0x83CFD3u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83CFD3 jsl */
    lda8_v(0x08);  /* 83CFD7 lda */
    actor_wait(0x83CFD9u);  /* 83CFD9 cop */
    MEM8(dec, ea_dp(K4E_POSE));  /* 83CFDB dec */
    if (!IF_N) goto L_83CFD1;  /* 83CFDD bpl */
    lda8_v(0x30);  /* 83CFDF lda */
    actor_wait(0x83CFE1u);  /* 83CFE1 cop */
    lda8_v(0x00);  /* 83CFE3 lda */
L_83CFE5:
    sta8(ea_dp(K4E_POSE));  /* 83CFE5 sta */
    jsl_frame(0x83CFE7u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83CFE7 jsl */
    lda8_v(0x08);  /* 83CFEB lda */
    actor_wait(0x83CFEDu);  /* 83CFED cop */
    lda8(ea_dp(K4E_POSE));  /* 83CFEF lda */
    ACC8(inc);  /* 83CFF1 inc */
    cmp8_v(0x06);  /* 83CFF2 cmp */
    if (!IF_C) goto L_83CFE5;  /* 83CFF4 bcc */
    kind4e_rest();  /* 83CFF6 bra */
}
