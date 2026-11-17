/*
 * kind28.c: actor kind $28, which waits for Aladdin to come near, rises
 * out of hiding, moves up, then drifts across bobbing on a wave; its
 * variant with bit 4 of $07 is instead a spawner of its own kind while he
 * is near. Its second state (downed) plays out and ends it. Its wait is
 * Kind28Wait (actor_waits_small.c).
 */
#include "machine.h"
#include "ops65.h"
#include "wram_video.h"
#include "actor.h"

void abi_ActorEndPlaced(void);
void abi_ActorFallStart(void);
void abi_ActorMotionPreset(void);
void abi_ActorMoveX(void);
void abi_ActorNearPlayer(void);
void abi_ActorRiseAfterDelay(void);
void abi_ActorSideOfPlayer(void);
void abi_ActorSpawnSameLook(void);
void abi_ActorStepY(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);

#define KIND28_RIGHT 0x46u     /* counted each frame it waits facing right (Kind28Wait) */
#define KIND28_UP 0x45u        /* set once it has risen */
#define KIND28_STEPS 0x40u     /* frames left of its rise */
#define KIND28_WAVE 0x41u      /* its place in WAVE (0 to $23) */
#define KIND28_FRAMES 0x42u    /* frames left before it ends */
#define SPAWN_WAIT 0x43u       /* the spawner variant: frames before the next (word) */
#define SPAWN_COUNTS 0x7EFF00u /* spawned actors counted by kind */
#define WAVE 0xD07Au           /* in bank $80: its y step each frame of the wave (signed bytes) */

/*
 * @routine 83:A03E Kind28Start
 * @actor
 * WHAT IT DOES: Actor kind $28: hidden until Aladdin is near, it shows
 * itself facing him, waits 32 frames, comes up 15 pixels and rises for a
 * while (1 frame, or $2A with bit 0 of $07), waits 8 frames, then falls
 * and drifts across on a wave (or, with bit 1 of $07, drifts with no wave;
 * no script has that) for good. With bit 4 of $07 it is a spawner
 * instead: while he is near, an actor of its kind every $60 frames, each
 * counted in SPAWN_COUNTS (no script has it).
 * HOW: KIND28_RIGHT cleared; bit 4 of $07: on into the spawner. ACTOR_08
 * bit 1, ACTOR_09 bit 4 set; level $0C: ACTOR_09 bits 4-5 cleared (no script).
 * KIND28_UP cleared; bit 3: AnimStart 1 at once (no script); else BRK
 * each frame until ActorNearPlayer with Y = $28, then AnimStart 1. Facing
 * from ActorSideOfPlayer (bit 2 of $07: from its x's bit 0 instead, no
 * script); ACTOR_09 bit 7 set; COP $20; y less 15 (SBC with the carry as
 * AnimStart left it); AnimStart 0; KIND28_STEPS = 1 or $2A by bit 0 of
 * $07 (shifted out); ActorMotionPreset with Y = $44; each frame (BRK)
 * ActorStepY until the steps run out; KIND28_UP up; COP 8. Bit 1 of $07
 * (shifted out): ActorFallStart, then each frame ActorMoveX and
 * ActorRiseAfterDelay. Else ActorFallStart, KIND28_WAVE = $0A, and each
 * frame (BRK) ActorMoveX, its y plus WAVE's step, KIND28_WAVE on (back to
 * 0 at $24), ActorRiseAfterDelay. The spawner: $07 bit 4 cleared, bit 3
 * set; SPAWN_WAIT and $21 cleared; each frame (BRK) while SPAWN_WAIT
 * counts down, or ActorNearPlayer with Y = $28 says he is far; then
 * ActorSpawnSameLook with its kind (a record: SPAWN_COUNTS by kind up),
 * SPAWN_WAIT = $160, and each frame it counts down (to 0: spawn again)
 * while he stays near.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $28 in its start state (the table at
 * $83:8CAB). Exit: never returns.
 */
void abi_Kind28Start(void)
{
    stz8(ea_dp(KIND28_RIGHT));  /* 83A03E stz */
    lda8(ea_dp(ACTOR_07));  /* 83A040 lda */
    and8_v(0x10);  /* 83A042 and */
    if (IF_Z) goto L_83A049;  /* 83A044 beq */
    goto L_83A110;  /* 83A046 jmp */
L_83A049:
    lda8_v(0x02);  /* 83A049 lda */
    tsb8(ea_dp(ACTOR_08));  /* 83A04B tsb */
    lda8_v(0x10);  /* 83A04D lda */
    tsb8(ea_dp(ACTOR_09));  /* 83A04F tsb */
    lda8(ea_abs(LEVEL));  /* 83A051 lda */
    cmp8_v(0x0C);  /* 83A054 cmp */
    if (!IF_Z) goto L_83A05C;  /* 83A056 bne */
    lda8_v(0x30);  /* 83A058 lda */
    trb8(ea_dp(ACTOR_09));  /* 83A05A trb */
L_83A05C:
    stz8(ea_dp(KIND28_UP));  /* 83A05C stz */
    lda8(ea_dp(ACTOR_07));  /* 83A05E lda */
    and8_v(0x08);  /* 83A060 and */
    if (IF_Z) goto L_83A06C;  /* 83A062 beq */
    lda8_v(0x01);  /* 83A064 lda */
    abi_AnimStart();  /* 83A066 jsl */
    goto L_83A07C;  /* 83A06A bra */
L_83A06C:
    actor_yield(0x83A06Cu);  /* 83A06C brk */
    ldy8_v(0x28);  /* 83A06E ldy */
    abi_ActorNearPlayer();  /* 83A070 jsl */
    if (IF_C) goto L_83A06C;  /* 83A074 bcs */
    lda8_v(0x01);  /* 83A076 lda */
    abi_AnimStart();  /* 83A078 jsl */
L_83A07C:
    lda8(ea_dp(ACTOR_07));  /* 83A07C lda */
    and8_v(0x04);  /* 83A07E and */
    if (IF_Z) goto L_83A088;  /* 83A080 beq */
    lda8(ea_dp(ACTOR_X + 1));  /* 83A082 lda */
    and8_v(0x01);  /* 83A084 and */
    goto L_83A08C;  /* 83A086 bra */
L_83A088:
    abi_ActorSideOfPlayer();  /* 83A088 jsl */
L_83A08C:
    sta8(ea_dp(ACTOR_FACING));  /* 83A08C sta */
    eor8_v(0x01);  /* 83A08E eor */
    sta8(ea_dp(ACTOR_FLIP));  /* 83A090 sta */
    lda8_v(0x80);  /* 83A092 lda */
    tsb8(ea_dp(ACTOR_09));  /* 83A094 tsb */
    lda8_v(0x20);  /* 83A096 lda */
    actor_wait(0x83A098u);  /* 83A098 cop */
    op_rep(0x20);  /* 83A09A rep */
    lda16(ea_dp(ACTOR_Y + 1));  /* 83A09C lda */
    sbc16_v(0x000F);  /* 83A09E sbc */
    sta16(ea_dp(ACTOR_Y + 1));  /* 83A0A1 sta */
    op_sep(0x20);  /* 83A0A3 sep */
    lda8_v(0x00);  /* 83A0A5 lda */
    abi_AnimStart();  /* 83A0A7 jsl */
    ldy8_v(0x01);  /* 83A0AB ldy */
    MEM8(lsr, ea_dp(ACTOR_07));  /* 83A0AD lsr */
    if (!IF_C) goto L_83A0B3;  /* 83A0AF bcc */
    ldy8_v(0x2A);  /* 83A0B1 ldy */
L_83A0B3:
    sty8(ea_dp(KIND28_STEPS));  /* 83A0B3 sty */
    ldy8_v(0x44);  /* 83A0B5 ldy */
    abi_ActorMotionPreset();  /* 83A0B7 jsl */
L_83A0BB:
    actor_yield(0x83A0BBu);  /* 83A0BB brk */
    abi_ActorStepY();  /* 83A0BD jsl */
    MEM8(dec, ea_dp(KIND28_STEPS));  /* 83A0C1 dec */
    if (!IF_Z) goto L_83A0BB;  /* 83A0C3 bne */
    MEM8(inc, ea_dp(KIND28_UP));  /* 83A0C5 inc */
    lda8_v(0x08);  /* 83A0C7 lda */
    actor_wait(0x83A0C9u);  /* 83A0C9 cop */
    MEM8(lsr, ea_dp(ACTOR_07));  /* 83A0CB lsr */
    if (!IF_C) goto L_83A0DD;  /* 83A0CD bcc */
    abi_ActorFallStart();  /* 83A0CF jsr */
L_83A0D2:
    actor_yield(0x83A0D2u);  /* 83A0D2 brk */
    abi_ActorMoveX();  /* 83A0D4 jsl */
    abi_ActorRiseAfterDelay();  /* 83A0D8 jsr */
    goto L_83A0D2;  /* 83A0DB bra */
L_83A0DD:
    abi_ActorFallStart();  /* 83A0DD jsr */
    lda8_v(0x0A);  /* 83A0E0 lda */
    sta8(ea_dp(KIND28_WAVE));  /* 83A0E2 sta */
L_83A0E4:
    actor_yield(0x83A0E4u);  /* 83A0E4 brk */
    abi_ActorMoveX();  /* 83A0E6 jsl */
    ldx8(ea_dp(KIND28_WAVE));  /* 83A0EA ldx */
    ldy8_v(0x00);  /* 83A0EC ldy */
    lda8(ea_absx(WAVE));  /* 83A0EE lda */
    if (!IF_N) goto L_83A0F5;  /* 83A0F1 bpl */
    ldy8_v(0xFF);  /* 83A0F3 ldy */
L_83A0F5:
    tax8();  /* 83A0F5 tax */
    tya8();  /* 83A0F6 tya */
    xba();  /* 83A0F7 xba */
    txa8();  /* 83A0F8 txa */
    op_rep(0x21);  /* 83A0F9 rep */
    adc16_v(rd16(ea_dp(ACTOR_Y + 1)));  /* 83A0FB adc */
    sta16(ea_dp(ACTOR_Y + 1));  /* 83A0FD sta */
    op_sep(0x20);  /* 83A0FF sep */
    MEM8(inc, ea_dp(KIND28_WAVE));  /* 83A101 inc */
    lda8(ea_dp(KIND28_WAVE));  /* 83A103 lda */
    cmp8_v(0x24);  /* 83A105 cmp */
    if (!IF_Z) goto L_83A10B;  /* 83A107 bne */
    stz8(ea_dp(KIND28_WAVE));  /* 83A109 stz */
L_83A10B:
    abi_ActorRiseAfterDelay();  /* 83A10B jsr */
    goto L_83A0E4;  /* 83A10E bra */
L_83A110:
    lda8(ea_dp(ACTOR_07));  /* 83A110 lda */
    and8_v(0xEF);  /* 83A112 and */
    ora8_v(0x08);  /* 83A114 ora */
    sta8(ea_dp(ACTOR_07));  /* 83A116 sta */
    stz8(ea_dp(SPAWN_WAIT));  /* 83A118 stz */
    stz8(ea_dp(SPAWN_WAIT + 1));  /* 83A11A stz */
    stz8(ea_dp(ACTOR_SPAWN_AT));  /* 83A11C stz */
L_83A11E:
    actor_yield(0x83A11Eu);  /* 83A11E brk */
    lda8(ea_dp(SPAWN_WAIT));  /* 83A120 lda */
    if (!IF_Z) goto L_83A128;  /* 83A122 bne */
    lda8(ea_dp(SPAWN_WAIT + 1));  /* 83A124 lda */
    if (IF_Z) goto L_83A130;  /* 83A126 beq */
L_83A128:
    op_rep(0x20);  /* 83A128 rep */
    MEM16(dec, ea_dp(SPAWN_WAIT));  /* 83A12A dec */
    op_sep(0x20);  /* 83A12C sep */
    goto L_83A11E;  /* 83A12E bra */
L_83A130:
    ldy8_v(0x28);  /* 83A130 ldy */
    abi_ActorNearPlayer();  /* 83A132 jsl */
    if (IF_C) goto L_83A11E;  /* 83A136 bcs */
L_83A138:
    lda8(ea_dp(ACTOR_06));  /* 83A138 lda */
    abi_ActorSpawnSameLook();  /* 83A13A jsl */
    if (IF_N) goto L_83A14B;  /* 83A13E bmi */
    ldx8(ea_dp(ACTOR_06));  /* 83A140 ldx */
    lda8(ea_longx(SPAWN_COUNTS));  /* 83A142 lda */
    ACC8(inc);  /* 83A146 inc */
    sta8(ea_longx(SPAWN_COUNTS));  /* 83A147 sta */
L_83A14B:
    lda8_v(0x60);  /* 83A14B lda */
    sta8(ea_dp(SPAWN_WAIT));  /* 83A14D sta */
    lda8_v(0x01);  /* 83A14F lda */
    sta8(ea_dp(SPAWN_WAIT + 1));  /* 83A151 sta */
L_83A153:
    actor_yield(0x83A153u);  /* 83A153 brk */
    op_rep(0x20);  /* 83A155 rep */
    MEM16(dec, ea_dp(SPAWN_WAIT));  /* 83A157 dec */
    op_sep(0x20);  /* 83A159 sep */
    if (IF_Z) goto L_83A138;  /* 83A15B beq */
    ldy8_v(0x28);  /* 83A15D ldy */
    abi_ActorNearPlayer();  /* 83A15F jsl */
    if (IF_C) goto L_83A11E;  /* 83A163 bcs */
    goto L_83A153;  /* 83A165 bra */
}

/*
 * @routine 83:A196 Kind28Second
 * @actor
 * WHAT IT DOES: Actor kind $28 in its second state (downed): a sound ($2C,
 * and $29 first when it never waited facing right; no script has that),
 * animation 2 for 32 frames, then its end.
 * HOW: KIND28_RIGHT zero: SoundQueueAdd $29. SoundQueueAdd $2C;
 * AnimStart 2; KIND28_FRAMES = $20; each frame (BRK) AnimStep until they
 * run out; on into ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $28 in a state with bit 7 set (the table
 * at $83:8DFD). Exit: never returns.
 */
void abi_Kind28Second(void)
{
    lda8(ea_dp(KIND28_RIGHT));  /* 83A196 lda */
    if (!IF_Z) goto L_83A1A0;  /* 83A198 bne */
    lda8_v(0x29);  /* 83A19A lda */
    abi_SoundQueueAdd();  /* 83A19C jsl */
L_83A1A0:
    lda8_v(0x2C);  /* 83A1A0 lda */
    abi_SoundQueueAdd();  /* 83A1A2 jsl */
    lda8_v(0x02);  /* 83A1A6 lda */
    abi_AnimStart();  /* 83A1A8 jsl */
    lda8_v(0x20);  /* 83A1AC lda */
    sta8(ea_dp(KIND28_FRAMES));  /* 83A1AE sta */
L_83A1B0:
    actor_yield(0x83A1B0u);  /* 83A1B0 brk */
    abi_AnimStep();  /* 83A1B2 jsl */
    MEM8(dec, ea_dp(KIND28_FRAMES));  /* 83A1B6 dec */
    if (!IF_Z) goto L_83A1B0;  /* 83A1B8 bne */
    abi_ActorEndPlaced();  /* 83A1BA jmp */
}
