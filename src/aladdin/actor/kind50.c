/*
 * kind50.c: actor kind $50 of level 0, which rises out of the ground in
 * steps, waits facing Aladdin, sinks back and waits again, for good. Its
 * wait is Kind50Wait (kind_waits_rest.c). Named and described by hand.
 */
#include "machine.h"
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

#define K50_FRAMES 0x40u        /* frames left of its wait below or on top */
#define K50_TOP 0x41u           /* frames it waits each time ($20 or $40) */
#define K50_STEP 0x42u          /* frames left of a step */
#define K50_POSE 0x43u          /* its animation: 6 down, 0 up */
#define K50_DOWNED 0x45u        /* counted up by Kind50Wait when a shot downs it */

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
    abi_Kind50Face();  /* 83D113 jsr */
    lda8_v(0x06);  /* 83D116 lda */
    sta8(ea_dp(K50_POSE));  /* 83D118 sta */
    abi_AnimStart();  /* 83D11A jsl */
    ldx8_v(0x04);  /* 83D11E ldx */
    abi_RandomFromList();  /* 83D120 jsl */
    sta8(ea_dp(K50_FRAMES));  /* 83D124 sta */
    abi_RandomNext();  /* 83D126 jsl */
    if (IF_N) goto L_83D130;  /* 83D12A bmi */
    lda8_v(0x20);  /* 83D12C lda */
    goto L_83D132;  /* 83D12E bra */
L_83D130:
    lda8_v(0x40);  /* 83D130 lda */
L_83D132:
    sta8(ea_dp(K50_TOP));  /* 83D132 sta */
L_83D134:
    actor_yield(0x83D134u);  /* 83D134 brk */
    abi_Kind50Face();  /* 83D136 jsr */
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
    abi_AnimStart();  /* 83D151 jsl */
    abi_Kind50Up8();  /* 83D155 jsr */
    goto L_83D141;  /* 83D158 bra */
L_83D15A:
    MEM8(inc, ea_dp(K50_POSE));  /* 83D15A inc */
L_83D15C:
    actor_yield(0x83D15Cu);  /* 83D15C brk */
    abi_Kind50Face();  /* 83D15E jsr */
    MEM8(dec, ea_dp(K50_FRAMES));  /* 83D161 dec */
    if (!IF_Z) goto L_83D15C;  /* 83D163 bne */
    abi_Kind50Still();  /* 83D165 jsr */
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
    abi_AnimStart();  /* 83D17A jsl */
    abi_Kind50Down8();  /* 83D17E jsr */
    goto L_83D168;  /* 83D181 bra */
L_83D183:
    MEM8(dec, ea_dp(K50_POSE));  /* 83D183 dec */
    lda8(ea_dp(K50_TOP));  /* 83D185 lda */
    sta8(ea_dp(K50_FRAMES));  /* 83D187 sta */
    abi_Kind50Still();  /* 83D189 jsr */
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
    abi_ActorSideOfPlayer();  /* 83D1A7 jsl */
    sta8(ea_dp(ACTOR_FACING));  /* 83D1AB sta */
    sta8(ea_dp(ACTOR_FLIP));  /* 83D1AD sta */
}
