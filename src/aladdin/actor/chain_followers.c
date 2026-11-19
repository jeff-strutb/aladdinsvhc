/*
 * chain_followers.c: the codes the actors of a chain (kinds $73 and
 * $75's lists) are started on by ActorListStart: each waits the frames its
 * start gave it in $44 (the leader's step apart), then starts its byte
 * stream (ActorStreamStart) or slides its $42 to a target, and yields for
 * good (ActorYieldForever). Written instruction for instruction from the
 * assembly (chain_followers.asm), every JSR with its return frame.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorStreamStart(void);
void abi_ActorYieldForever(void);
void abi_ChainFollowStream2(void);

#define FOLLOW_WAIT ACTOR_44        /* frames left before it goes on */
#define FOLLOW_INDEX ACTOR_40       /* its place in the chain */
#define FOLLOW_AT ACTOR_42          /* the value it slides (word) */
#define FOLLOW_ROW 0x16u            /* the slide's row: 0, 2, 4 or 6 (word) */
#define FOLLOW_DONE ACTOR_18        /* counted up once the slide has arrived */
#define FOLLOW_SIDE 0x000Eu         /* 0: the target is above FOLLOW_AT, 2: below */
/* Tables in the data bank ($80) */
#define FOLLOW_TARGETS 0xD6D3u      /* by row (words): the offset of the row's targets; then by place (words) */
#define FOLLOW_STEP_ROWS 0xD6BAu    /* by row * 2, less one: the row's steps (words): toward a target above, below */
#define FOLLOW_WAITS 0xD6CBu        /* by place: the frames ChainFollowLate waits */

/* ChainSlideRow0's body from its STA (84:89D7), A = the row. */
static void slide(void)
{
    sta8(ea_dp(FOLLOW_ROW));
    stz8(ea_dp(FOLLOW_ROW + 1));
    lda8(ea_dp(FOLLOW_WAIT));
    if (!IF_Z) {
        do {
            actor_yield(0x8489DFu);
            MEM8(dec, ea_dp(FOLLOW_WAIT));
        } while (!IF_Z);
    }
    for (;;) {
        actor_yield(0x8489E5u);
        op_rep(0x30);
        lda16(ea_dp(FOLLOW_ROW));
        tax16();
        ACC16(asl);
        tay16();
        lda16(ea_dp(FOLLOW_INDEX));
        and16_v(0x00FF);
        ACC16(asl);
        clc();
        adc16_v(rd16(ea_absx(FOLLOW_TARGETS)));
        tax16();
        stz16(ea_abs(FOLLOW_SIDE));
        sec();
        lda16(ea_absx(FOLLOW_TARGETS));
        sbc16_v(rd16(ea_dp(FOLLOW_AT)));
        pha16();
        if (IF_N) {
            MEM16(inc, ea_abs(FOLLOW_SIDE));
            MEM16(inc, ea_abs(FOLLOW_SIDE));
            eor16_v(0xFFFF);
            ACC16(inc);
        }
        cmp16_v(rd16(ea_absy(FOLLOW_STEP_ROWS + 1)));
        pla16();
        if (IF_C) {
            tya16();
            adc16_v(rd16(ea_abs(FOLLOW_SIDE)));
            tay16();
            lda16(ea_absy(FOLLOW_STEP_ROWS));
        }
        clc();
        adc16_v(rd16(ea_dp(FOLLOW_AT)));
        sta16(ea_dp(FOLLOW_AT));
        cmp16_v(rd16(ea_absx(FOLLOW_TARGETS)));
        op_sep(0x30);
        if (IF_Z)
            break;
    }
    MEM8(inc, ea_dp(FOLLOW_DONE));
    abi_ActorYieldForever();
}

/*
 * @routine 84:89A0 ChainFollowStream0
 * @actor
 * WHAT IT DOES: A chain's actor that waits its turn, then starts byte
 * stream 0 and yields for good.
 * HOW: Each frame (BRK first) FOLLOW_WAIT down, until 0; ActorStreamStart
 * with X = 0; on at ActorYieldForever.
 * Entry: D = the actor's record; DB = $80; reached by the actor loop's
 * RTI where ActorListStart (84:8802) set its code. Exit: never returns.
 */
void abi_ChainFollowStream0(void)
{
    do {
        actor_yield(0x8489A0u);
        MEM8(dec, ea_dp(FOLLOW_WAIT));
    } while (!IF_Z);
    ldx8_v(0x00);
    call_frame(0x89AA, abi_ActorStreamStart);
    abi_ActorYieldForever();
}

/*
 * @routine 84:89AD ChainFollowStream2
 * @actor
 * WHAT IT DOES: A chain's actor that waits its turn, then starts byte
 * stream 2 and yields for good.
 * HOW: Each frame (BRK first) FOLLOW_WAIT down, until 0; ActorStreamStart
 * with X = 2; on at ActorYieldForever.
 * Entry: D = the actor's record; DB = $80; reached by the actor loop's
 * RTI where ActorListStartAD set its code, and from ChainFollowStream14.
 * Exit: never returns.
 */
void abi_ChainFollowStream2(void)
{
    do {
        actor_yield(0x8489ADu);
        MEM8(dec, ea_dp(FOLLOW_WAIT));
    } while (!IF_Z);
    ldx8_v(0x02);
    call_frame(0x89B7, abi_ActorStreamStart);
    abi_ActorYieldForever();
}

/*
 * @routine 84:89BB ChainFollowStream14
 * @actor
 * WHAT IT DOES: A chain's actor that waits its turn and then starts byte
 * stream 2 as ChainFollowStream2, unless its wait was one frame: then
 * byte stream $14.
 * HOW: BRK; FOLLOW_WAIT down: not 0, on in ChainFollowStream2's wait.
 * Else (its start gives $10 steps: no script, nor any start, reaches it)
 * ActorStreamStart with X = $14; on at ActorYieldForever.
 * Entry: D = the actor's record; DB = $80; reached by the actor loop's
 * RTI where its leader (84:8B18) set its code. Exit: never returns.
 */
void abi_ChainFollowStream14(void)
{
    actor_yield(0x8489BBu);
    MEM8(dec, ea_dp(FOLLOW_WAIT));
    if (!IF_Z) {
        abi_ChainFollowStream2();
        return;
    }
    ldx8_v(0x14);
    call_frame(0x89C5, abi_ActorStreamStart);
    abi_ActorYieldForever();
}

/*
 * @routine 84:89C9 ChainSlideRow6
 * @actor
 * WHAT IT DOES: A chain's actor that waits its turn, then slides its value
 * to its place's target in row 6, marks it done and yields for good.
 * HOW: A = 6, on in ChainSlideRow0's body.
 * Entry: D = the actor's record; DB = $80; reached by the actor loop's
 * RTI where its leader (84:8C41) set its code. Exit: never returns.
 */
void abi_ChainSlideRow6(void)
{
    lda8_v(0x06);
    slide();
}

/*
 * @routine 84:89CD ChainSlideRow4
 * @actor
 * WHAT IT DOES: As ChainSlideRow6, in row 4.
 * HOW: A = 4, on in ChainSlideRow0's body.
 * Entry: D = the actor's record; DB = $80; reached by the actor loop's
 * RTI where its leader (84:8B08) set its code. Exit: never returns.
 */
void abi_ChainSlideRow4(void)
{
    lda8_v(0x04);
    slide();
}

/*
 * @routine 84:89D1 ChainSlideRow2
 * @actor
 * WHAT IT DOES: As ChainSlideRow6, in row 2.
 * HOW: A = 2, on in ChainSlideRow0's body.
 * Entry: D = the actor's record; DB = $80; reached by the actor loop's
 * RTI where its leader (84:8942, 84:8C2C) set its code. Exit: never
 * returns.
 */
void abi_ChainSlideRow2(void)
{
    lda8_v(0x02);
    slide();
}

/*
 * @routine 84:89D5 ChainSlideRow0
 * @actor
 * WHAT IT DOES: A chain's actor that waits its turn, then each frame moves
 * its value FOLLOW_AT toward its place's target in its row (all at once
 * when nearer than the row's step, else by the step), and once there marks
 * itself done and yields for good.
 * HOW: A = 0 (the others' A): FOLLOW_ROW = A, its high byte 0. FOLLOW_WAIT
 * nonzero: each frame (BRK first) down, until 0. Each frame (BRK first),
 * 16-bit: X = FOLLOW_ROW, Y = it * 2; X = FOLLOW_TARGETS,X + FOLLOW_INDEX's
 * low byte * 2; FOLLOW_SIDE 0; the target FOLLOW_TARGETS,X less FOLLOW_AT
 * (pushed); negative: FOLLOW_SIDE 2 and the difference negated. Less than
 * the row's step (FOLLOW_STEP_ROWS+1,Y): the difference (pulled); else the
 * step at FOLLOW_STEP_ROWS + Y + FOLLOW_SIDE + 1 (the carry). Added to
 * FOLLOW_AT; until it is the target. FOLLOW_DONE up; on at
 * ActorYieldForever.
 * Entry: D = the actor's record; DB = $80; M 8-bit, X 8-bit; reached by
 * the actor loop's RTI where its leader (84:88DF) set its code. Exit:
 * never returns.
 */
void abi_ChainSlideRow0(void)
{
    lda8_v(0x00);
    slide();
}

/*
 * @routine 84:8A2E ChainFollowLate
 * @actor
 * WHAT IT DOES: A chain's actor that waits the frames its place gives,
 * then starts the byte stream its place picks and yields for good.
 * HOW: FOLLOW_WAIT = FOLLOW_WAITS,X (X = FOLLOW_INDEX); each frame (BRK
 * first) down, until 0; ActorStreamStart with X = (FOLLOW_INDEX + 2) * 2;
 * on at ActorYieldForever.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's RTI where its leader (84:8909, 84:8C67) set its code.
 * Exit: never returns.
 */
void abi_ChainFollowLate(void)
{
    ldx8(ea_dp(FOLLOW_INDEX));
    lda8(ea_absx(FOLLOW_WAITS));
    sta8(ea_dp(FOLLOW_WAIT));
    do {
        actor_yield(0x848A35u);
        MEM8(dec, ea_dp(FOLLOW_WAIT));
    } while (!IF_Z);
    clc();
    lda8(ea_dp(FOLLOW_INDEX));
    adc8_v(0x02);
    ACC8(asl);
    tax8();
    call_frame(0x8A44, abi_ActorStreamStart);
    abi_ActorYieldForever();
}
