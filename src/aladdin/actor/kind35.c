/*
 * kind35.c: actor kind $35, a gem: touched, it counts on the HUD, plays
 * its sound and leaves sparkles. Its wait is ActorWaitAnimEnd10
 * (wait_step.c).
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "wram_state.h"
#include "actor.h"

void abi_ActorEndPlaced(void);
void abi_ActorHitsTestBoxLong(void);
void abi_ActorSpawn(void);
void abi_AnimStart(void);
void abi_SoundQueueAdd(void);

#define GEMS 0x00036Bu             /* the HUD's second counter (gems): two decimal digits */
#define GEM_SETS_FOUND 0x000373u   /* a bit per set gem found (word) */
#define GEM_SETS 0x000375u         /* set gems found */
#define V_0364 0x000364u           /* counted up to 100 by a tenth gem at full health marks */
#define SET_BITS 0xD1B6u           /* in bank $80: each set gem's bit, by the low digit of $07 (words) */
#define TEST_BOX_X 0x001484u       /* the touch test box's x and y (words) */
#define TEST_BOX_Y 0x001486u
#define PLACED_FLAGS 0x7EC600u     /* a byte per placement: bit 7 set, never placed again */
#define KIND35_TIMES 0x40u         /* sparkles left to leave; a sparkle's frames */

/*
 * @routine 83:BBF2 Kind35Start
 * @actor
 * WHAT IT DOES: Actor kind $35, a gem: when Aladdin (with health) touches
 * it, it counts on the HUD's gem counter (three for a set gem, by bit 4
 * of $07, kept as found in GEM_SETS_FOUND), every tenth refills his
 * health or adds a mark (no script has that), plays its sound, is never
 * placed again, and leaves three sparkles (kind $27) where he touched it.
 * HOW: ACTOR_08 = 2, ACTOR_09 bits $90 set, flip 0, its look $04E0 at
 * $2B (level $035C 6) or $0440. A set gem (bit 4) not found yet and not
 * stage 2: AnimStart $1D; else $07 cleared, AnimStart $1C. Each frame
 * (BRK) until ActorHitsTestBoxLong clears C with health nonzero. A set
 * gem: (not stage 2) GEM_SETS up and its bit in GEM_SETS_FOUND (by $07,
 * from SET_BITS), GEMS up by 2. GEMS up; its low digit at $0A: less $0A
 * plus $10, and at $A0 in the high digit (no script) the low digit kept
 * alone, health marks below $0A: one more and health = them; at $0A:
 * health = $0A, V_0364 up to 100; sound 9. Else sound $31 ($32 with level
 * $035C 6). PLACED_FLAGS by its $34 = $80; ACTOR_08 bit 3 and $21
 * cleared; three times: its x and y = the test box's, ActorSpawn kind $27
 * (a child: $07 = $10, its look, $40 = $10), COP 7. Then ActorEndPlaced
 * (by the JML at $83:BD1E).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $35 in its start state (the table at
 * $83:8CAB). Exit: never returns.
 */
void abi_Kind35Start(void)
{
    lda8_v(0x02);  /* 83BBF2 lda */
    sta8(ea_dp(ACTOR_08));  /* 83BBF4 sta */
    lda8_v(0x90);  /* 83BBF6 lda */
    tsb8(ea_dp(ACTOR_09));  /* 83BBF8 tsb */
    stz8(ea_dp(ACTOR_FLIP));  /* 83BBFA stz */
    op_rep(0x20);  /* 83BBFC rep */
    lda16_v(0x04E0);  /* 83BBFE lda */
    ldx8(ea_abs(LEVEL_035C));  /* 83BC01 ldx */
    cpx8_v(0x06);  /* 83BC04 cpx */
    if (IF_Z) goto L_83BC0B;  /* 83BC06 beq */
    lda16_v(0x0440);  /* 83BC08 lda */
L_83BC0B:
    sta16(ea_dp(ACTOR_LOOK));  /* 83BC0B sta */
    op_sep(0x20);  /* 83BC0D sep */
    lda8(ea_dp(ACTOR_07));  /* 83BC0F lda */
    bit8_i(0x10);  /* 83BC11 bit */
    if (IF_Z) goto L_83BC31;  /* 83BC13 beq */
    lda8(ea_abs(V_033F));  /* 83BC15 lda */
    cmp8_v(0x02);  /* 83BC18 cmp */
    if (IF_Z) goto L_83BC2D;  /* 83BC1A beq */
    lda8(ea_dp(ACTOR_07));  /* 83BC1C lda */
    and8_v(0x0F);  /* 83BC1E and */
    tay8();  /* 83BC20 tay */
    op_rep(0x20);  /* 83BC21 rep */
    lda16(ea_absy(SET_BITS));  /* 83BC23 lda */
    and16_v(rd16(ea_abs(GEM_SETS_FOUND)));  /* 83BC26 and */
    op_sep(0x20);  /* 83BC29 sep */
    if (!IF_Z) goto L_83BC31;  /* 83BC2B bne */
L_83BC2D:
    lda8_v(0x1D);  /* 83BC2D lda */
    goto L_83BC35;  /* 83BC2F bra */
L_83BC31:
    stz8(ea_dp(ACTOR_07));  /* 83BC31 stz */
    lda8_v(0x1C);  /* 83BC33 lda */
L_83BC35:
    jsl_frame(0x83BC35u);  /* 83BC35 jsl */
    abi_AnimStart();
    cpu.s = (uint16_t)(cpu.s + 3);
L_83BC39:
    actor_yield(0x83BC39u);  /* 83BC39 brk */
    jsl_frame(0x83BC3Bu);  /* 83BC3B jsl */
    abi_ActorHitsTestBoxLong();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (IF_C) goto L_83BC39;  /* 83BC3F bcs */
    lda8(ea_abs(HEALTH));  /* 83BC41 lda */
    if (IF_Z) goto L_83BC39;  /* 83BC44 beq */
    lda8(ea_dp(ACTOR_07));  /* 83BC46 lda */
    and8_v(0x10);  /* 83BC48 and */
    if (IF_Z) goto L_83BC6B;  /* 83BC4A beq */
    lda8(ea_abs(V_033F));  /* 83BC4C lda */
    cmp8_v(0x02);  /* 83BC4F cmp */
    if (IF_Z) goto L_83BC65;  /* 83BC51 beq */
    MEM8(inc, ea_abs(GEM_SETS));  /* 83BC53 inc */
    lda8(ea_dp(ACTOR_07));  /* 83BC56 lda */
    and8_v(0x0F);  /* 83BC58 and */
    tay8();  /* 83BC5A tay */
    op_rep(0x20);  /* 83BC5B rep */
    lda16(ea_absy(SET_BITS));  /* 83BC5D lda */
    tsb16(ea_abs(GEM_SETS_FOUND));  /* 83BC60 tsb */
    op_sep(0x20);  /* 83BC63 sep */
L_83BC65:
    MEM8(inc, ea_abs(GEMS));  /* 83BC65 inc */
    MEM8(inc, ea_abs(GEMS));  /* 83BC68 inc */
L_83BC6B:
    MEM8(inc, ea_abs(GEMS));  /* 83BC6B inc */
    lda8(ea_abs(GEMS));  /* 83BC6E lda */
    and8_v(0x0F);  /* 83BC71 and */
    cmp8_v(0x0A);  /* 83BC73 cmp */
    if (IF_N) goto L_83BCB4;  /* 83BC75 bmi */
    lda8(ea_abs(GEMS));  /* 83BC77 lda */
    sec();  /* 83BC7A sec */
    sbc8_v(0x0A);  /* 83BC7B sbc */
    clc();  /* 83BC7D clc */
    adc8_v(0x10);  /* 83BC7E adc */
    sta8(ea_abs(GEMS));  /* 83BC80 sta */
    and8_v(0xF0);  /* 83BC83 and */
    cmp8_v(0xA0);  /* 83BC85 cmp */
    if (!IF_Z) goto L_83BCB4;  /* 83BC87 bne */
    lda8(ea_abs(HEALTH_MARKS));  /* 83BC89 lda */
    cmp8_v(0x0A);  /* 83BC8C cmp */
    if (!IF_Z) goto L_83BCA1;  /* 83BC8E bne */
    lda8(ea_abs(V_0364));  /* 83BC90 lda */
    cmp8_v(0x64);  /* 83BC93 cmp */
    if (IF_Z) goto L_83BC9A;  /* 83BC95 beq */
    MEM8(inc, ea_abs(V_0364));  /* 83BC97 inc */
L_83BC9A:
    lda8_v(0x0A);  /* 83BC9A lda */
    sta8(ea_abs(HEALTH));  /* 83BC9C sta */
    goto L_83BCA8;  /* 83BC9F bra */
L_83BCA1:
    ACC8(inc);  /* 83BCA1 inc */
    sta8(ea_abs(HEALTH));  /* 83BCA2 sta */
    sta8(ea_abs(HEALTH_MARKS));  /* 83BCA5 sta */
L_83BCA8:
    lda8(ea_abs(GEMS));  /* 83BCA8 lda */
    and8_v(0x0F);  /* 83BCAB and */
    sta8(ea_abs(GEMS));  /* 83BCAD sta */
    lda8_v(0x09);  /* 83BCB0 lda */
    goto L_83BCBF;  /* 83BCB2 bra */
L_83BCB4:
    lda8_v(0x31);  /* 83BCB4 lda */
    ldx8(ea_abs(LEVEL_035C));  /* 83BCB6 ldx */
    cpx8_v(0x06);  /* 83BCB9 cpx */
    if (!IF_Z) goto L_83BCBF;  /* 83BCBB bne */
    lda8_v(0x32);  /* 83BCBD lda */
L_83BCBF:
    jsl_frame(0x83BCBFu);  /* 83BCBF jsl */
    abi_SoundQueueAdd();
    cpu.s = (uint16_t)(cpu.s + 3);
    ldx8(ea_dp(ACTOR_34));  /* 83BCC3 ldx */
    lda8_v(0x80);  /* 83BCC5 lda */
    sta8(ea_longx(PLACED_FLAGS));  /* 83BCC7 sta */
    lda8_v(0x08);  /* 83BCCB lda */
    trb8(ea_dp(ACTOR_08));  /* 83BCCD trb */
    stz8(ea_dp(ACTOR_SPAWN_AT));  /* 83BCCF stz */
    lda8_v(0x03);  /* 83BCD1 lda */
    sta8(ea_dp(KIND35_TIMES));  /* 83BCD3 sta */
L_83BCD5:
    op_rep(0x20);  /* 83BCD5 rep */
    lda16(ea_abs(TEST_BOX_X));  /* 83BCD7 lda */
    sta16(ea_dp(ACTOR_X + 1));  /* 83BCDA sta */
    lda16(ea_abs(TEST_BOX_Y));  /* 83BCDC lda */
    sta16(ea_dp(ACTOR_Y + 1));  /* 83BCDF sta */
    op_sep(0x20);  /* 83BCE1 sep */
    lda8_v(0x27);  /* 83BCE3 lda */
    jsl_frame(0x83BCE5u);  /* 83BCE5 jsl */
    abi_ActorSpawn();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (IF_N) goto L_83BCFF;  /* 83BCE9 bmi */
    lda8_v(0x10);  /* 83BCEB lda */
    sta8(ea_absx(ACTOR_07));  /* 83BCED sta */
    lda8(ea_dp(ACTOR_LOOK));  /* 83BCF0 lda */
    sta8(ea_absx(ACTOR_LOOK));  /* 83BCF2 sta */
    lda8(ea_dp(ACTOR_LOOK + 1));  /* 83BCF5 lda */
    sta8(ea_absx(ACTOR_LOOK + 1));  /* 83BCF7 sta */
    lda8_v(0x10);  /* 83BCFA lda */
    sta8(ea_absx(KIND35_TIMES));  /* 83BCFC sta */
L_83BCFF:
    op_sep(0x10);  /* 83BCFF sep */
    lda8_v(0x07);  /* 83BD01 lda */
    actor_wait(0x83BD03u);  /* 83BD03 cop */
    MEM8(dec, ea_dp(KIND35_TIMES));  /* 83BD05 dec */
    if (!IF_Z) goto L_83BCD5;  /* 83BD07 bne */
    abi_ActorEndPlaced();  /* 83BD09 bra, 83BD1E jml */
}

