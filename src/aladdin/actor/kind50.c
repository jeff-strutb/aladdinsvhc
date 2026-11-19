/*
 * kind50.c: actor kind $50 of level 0, which rises out of the ground in
 * steps, waits facing Aladdin, sinks back and waits again, for good. Its
 * wait is Kind50Wait (kind_waits_rest.c). Named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorSideOfPlayer(void);
void abi_AnimStart(void);
void abi_Kind50Down8(void);
void abi_Kind50Face(void);
void abi_Kind50Still(void);
void abi_Kind50Up8(void);
void abi_RandomFromList(void);
void abi_RandomNext(void);
void abi_SoundQueueAdd(void);
void abi_ActorMotionPreset(void);
void abi_AnimStep(void);
void abi_ActorFly(void);
void abi_ActorEndOutOfRange(void);

#define K50_FRAMES 0x40u        /* frames left of its wait below or on top */
#define K50_TOP 0x41u           /* frames it waits each time ($20 or $40) */
#define K50_STEP 0x42u          /* frames left of a step */
#define K50_POSE 0x43u          /* its animation: 6 down, 0 up */
#define K50_DOWNED 0x45u        /* counted up by Kind50Wait when a shot downs it */
#define K50_LOW 0x44u           /* its second state: 7 once its pose came to the lowest of its fall */
#define K50_STOMP_SOUND 0x0Fu   /* the sound when it is downed by a stomp, not a shot */
#define K50_FALL 0x2Fu          /* ActorMotionPreset's preset for its fall away */

/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:D109 Kind50Start
 * @actor
 * WHAT IT DOES: Actor kind $50 started: hidden in the ground, it waits a
 * random time, rises in 6 steps, waits on top, sinks in 6 steps, waits,
 * and again, always facing Aladdin.
 * HOW: K50_DOWNED = 0, $08 = 3, $09 |= $90, Kind50Face; pose 6
 * (AnimStart); K50_FRAMES = RandomFromList 4; K50_TOP = $40 when
 * RandomNext is negative, else $20. Below: each frame (BRK first)
 * Kind50Face for K50_FRAMES + 1 frames; K50_FRAMES = K50_TOP. Up: every 7
 * frames (BRK) pose one less (AnimStart) and Kind50Up8, down to pose 0.
 * On top: each frame (BRK) Kind50Face for K50_FRAMES frames; Kind50Still.
 * Down: every 7 frames pose one more (AnimStart) and Kind50Down8, up to
 * pose 6; K50_FRAMES = K50_TOP, Kind50Still, and below again.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for state $0C. Exit: never returns.
 */
void abi_Kind50Start(void)
{
    stz8(ea_dp(K50_DOWNED));  /* 83D109 stz */
    lda8_v(0x03);  /* 83D10B lda */
    sta8(ea_dp(ACTOR_08));  /* 83D10D sta */
    lda8_v(0x90);  /* 83D10F lda */
    tsb8(ea_dp(ACTOR_09));  /* 83D111 tsb */
    call_frame(0xD115, abi_Kind50Face);  /* 83D113 jsr */
    lda8_v(0x06);  /* 83D116 lda */
    sta8(ea_dp(K50_POSE));  /* 83D118 sta */
    jsl_frame(0x83D11Au); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D11A jsl */
    ldx8_v(0x04);  /* 83D11E ldx */
    jsl_frame(0x83D120u); abi_RandomFromList(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D120 jsl */
    sta8(ea_dp(K50_FRAMES));  /* 83D124 sta */
    jsl_frame(0x83D126u); abi_RandomNext(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D126 jsl */
    if (IF_N) goto L_83D130;  /* 83D12A bmi */
    lda8_v(0x20);  /* 83D12C lda */
    goto L_83D132;  /* 83D12E bra */
L_83D130:
    lda8_v(0x40);  /* 83D130 lda */
L_83D132:
    sta8(ea_dp(K50_TOP));  /* 83D132 sta */
L_83D134:
    actor_yield(0x83D134u);  /* 83D134 brk */
    call_frame(0xD138, abi_Kind50Face);  /* 83D136 jsr */
    MEM8(dec, ea_dp(K50_FRAMES));  /* 83D139 dec */
    if (!IF_N) goto L_83D134;  /* 83D13B bpl */
    lda8(ea_dp(K50_TOP));  /* 83D13D lda */
    sta8(ea_dp(K50_FRAMES));  /* 83D13F sta */
L_83D141:
    lda8_v(0x07);  /* 83D141 lda */
    sta8(ea_dp(K50_STEP));  /* 83D143 sta */
L_83D145:
    actor_yield(0x83D145u);  /* 83D145 brk */
    MEM8(dec, ea_dp(K50_STEP));  /* 83D147 dec */
    if (!IF_Z) goto L_83D145;  /* 83D149 bne */
    MEM8(dec, ea_dp(K50_POSE));  /* 83D14B dec */
    if (IF_N) goto L_83D15A;  /* 83D14D bmi */
    lda8(ea_dp(K50_POSE));  /* 83D14F lda */
    jsl_frame(0x83D151u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D151 jsl */
    call_frame(0xD157, abi_Kind50Up8);  /* 83D155 jsr */
    goto L_83D141;  /* 83D158 bra */
L_83D15A:
    MEM8(inc, ea_dp(K50_POSE));  /* 83D15A inc */
L_83D15C:
    actor_yield(0x83D15Cu);  /* 83D15C brk */
    call_frame(0xD160, abi_Kind50Face);  /* 83D15E jsr */
    MEM8(dec, ea_dp(K50_FRAMES));  /* 83D161 dec */
    if (!IF_Z) goto L_83D15C;  /* 83D163 bne */
    call_frame(0xD167, abi_Kind50Still);  /* 83D165 jsr */
L_83D168:
    lda8_v(0x07);  /* 83D168 lda */
    sta8(ea_dp(K50_STEP));  /* 83D16A sta */
L_83D16C:
    actor_yield(0x83D16Cu);  /* 83D16C brk */
    MEM8(dec, ea_dp(K50_STEP));  /* 83D16E dec */
    if (!IF_Z) goto L_83D16C;  /* 83D170 bne */
    MEM8(inc, ea_dp(K50_POSE));  /* 83D172 inc */
    lda8(ea_dp(K50_POSE));  /* 83D174 lda */
    cmp8_v(0x07);  /* 83D176 cmp */
    if (IF_Z) goto L_83D183;  /* 83D178 beq */
    jsl_frame(0x83D17Au); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D17A jsl */
    call_frame(0xD180, abi_Kind50Down8);  /* 83D17E jsr */
    goto L_83D168;  /* 83D181 bra */
L_83D183:
    MEM8(dec, ea_dp(K50_POSE));  /* 83D183 dec */
    lda8(ea_dp(K50_TOP));  /* 83D185 lda */
    sta8(ea_dp(K50_FRAMES));  /* 83D187 sta */
    call_frame(0xD18B, abi_Kind50Still);  /* 83D189 jsr */
    goto L_83D134;  /* 83D18C bra */
}

/*
 * @routine 83:D18E Kind50Still
 * WHAT IT DOES: Nothing: kind $50 calls it where it turns between rising
 * and sinking.
 * HOW: RTS.
 * Entry: called by JSR. Exit: all unchanged.
 */
void abi_Kind50Still(void)
{
}

/*
 * @routine 83:D18F Kind50Down8
 * WHAT IT DOES: Moves the actor 8 pixels down.
 * HOW: 16-bit A, carry cleared (REP #$21): y pixels + 8.
 * Entry: M 8-bit; D = the actor's record; called by JSR. Exit: M 8-bit; C
 * from the add.
 */
void abi_Kind50Down8(void)
{
    op_rep(0x21);  /* 83D18F rep */
    lda16(ea_dp(ACTOR_Y + 1));  /* 83D191 lda */
    adc16_v(0x0008);  /* 83D193 adc */
    sta16(ea_dp(ACTOR_Y + 1));  /* 83D196 sta */
    op_sep(0x20);  /* 83D198 sep */
}

/*
 * @routine 83:D19B Kind50Up8
 * WHAT IT DOES: Moves the actor 8 pixels up.
 * HOW: 16-bit A, carry cleared (REP #$21): y pixels - 7 - 1 (SBC with the
 * carry clear).
 * Entry: M 8-bit; D = the actor's record; called by JSR. Exit: M 8-bit; C
 * from the subtract.
 */
void abi_Kind50Up8(void)
{
    op_rep(0x21);  /* 83D19B rep */
    lda16(ea_dp(ACTOR_Y + 1));  /* 83D19D lda */
    sbc16_v(0x0007);  /* 83D19F sbc */
    sta16(ea_dp(ACTOR_Y + 1));  /* 83D1A2 sta */
    op_sep(0x20);  /* 83D1A4 sep */
}

/*
 * @routine 83:D1A7 Kind50Face
 * WHAT IT DOES: Turns the actor to face Aladdin.
 * HOW: ActorSideOfPlayer into its facing and its flip (the same value).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSR. Exit: as ActorSideOfPlayer.
 */
void abi_Kind50Face(void)
{
    jsl_frame(0x83D1A7u); abi_ActorSideOfPlayer(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D1A7 jsl */
    sta8(ea_dp(ACTOR_FACING));  /* 83D1AB sta */
    sta8(ea_dp(ACTOR_FLIP));  /* 83D1AD sta */
}

/*
 * @routine 83:D1C7 Kind50Second
 * @actor
 * WHAT IT DOES: Kind $50 downed (its second state, state $8C): by a stomp
 * it plays sound $0F; it shows its hit poses (7 up to 13) as it sinks a
 * step and rises again, then falls away out of view and ends there.
 * HOW: Its facing negative (set by ActorDownedByShot for a shot whose $11
 * is negative): facing and flip = its bit 0. K50_DOWNED zero:
 * SoundQueueAdd K50_STOMP_SOUND. Pose = 13 less its pose (AnimStart), COP
 * 3; K50_LOW 0; pose one less, or 7 (K50_LOW 7) where that is 6;
 * AnimStart; K50_LOW zero: Kind50Down8; COP 8. Then each step pose one
 * more; at 14: AnimStart, ActorMotionPreset K50_FALL, and each frame (BRK
 * first) AnimStep, ActorFly, ActorEndOutOfRange with box 0. Below 14:
 * AnimStart, Kind50Up8, BRK, the next step.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $50 in a state with bit 7 set (the table
 * at $83:8DFD). Exit: never returns.
 */
void abi_Kind50Second(void)
{
    lda8(ea_dp(ACTOR_FACING));
    if (IF_N) {
        and8_v(0x01);
        sta8(ea_dp(ACTOR_FACING));
        sta8(ea_dp(ACTOR_FLIP));
    }
    lda8(ea_dp(K50_DOWNED));
    if (IF_Z) {
        lda8_v(K50_STOMP_SOUND);
        jsl_call(0x83D1D7u, abi_SoundQueueAdd);
    }
    lda8_v(0x06);
    sec();
    sbc8_v(rd8(ea_dp(K50_POSE)));
    clc();
    adc8_v(0x07);
    sta8(ea_dp(K50_POSE));
    jsl_call(0x83D1E5u, abi_AnimStart);
    lda8_v(0x03);
    actor_wait(0x83D1EBu);
    stz8(ea_dp(K50_LOW));
    lda8(ea_dp(K50_POSE));
    ACC8(dec);
    cmp8_v(0x06);
    if (IF_Z) {
        lda8_v(0x07);
        sta8(ea_dp(K50_LOW));
    }
    sta8(ea_dp(K50_POSE));
    jsl_call(0x83D1FCu, abi_AnimStart);
    lda8(ea_dp(K50_LOW));
    if (IF_Z)
        call_frame(0xD206, abi_Kind50Down8);
    lda8_v(0x08);
    actor_wait(0x83D209u);
    for (;;) {
        MEM8(inc, ea_dp(K50_POSE));
        lda8(ea_dp(K50_POSE));
        cmp8_v(0x0E);
        if (IF_Z)
            break;
        jsl_call(0x83D213u, abi_AnimStart);
        call_frame(0xD219, abi_Kind50Up8);
        actor_yield(0x83D21Au);
    }
    jsl_call(0x83D21Eu, abi_AnimStart);
    ldy8_v(K50_FALL);
    jsl_call(0x83D224u, abi_ActorMotionPreset);
    for (;;) {
        actor_yield(0x83D228u);
        jsl_call(0x83D22Au, abi_AnimStep);
        jsl_call(0x83D22Eu, abi_ActorFly);
        ldx8_v(0x00);
        call_frame(0xD236, abi_ActorEndOutOfRange);
    }
}
