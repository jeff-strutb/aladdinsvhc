/*
 * actor_loop.c: the actor loop (docs/aladdin/ACTORS.md). Each frame the
 * player's record and then every other record ($50 bytes apart from
 * $092B) is looked at: a free one is skipped; a waiting one counts down
 * and runs its kind's wait routine (bit 7 of $09); one whose wait ran out
 * goes back into its actor by RTI at the resume address in its $02-$05;
 * one to start goes to its kind's code by JMP through the kind tables.
 * The actor comes back by BRK or COP (the handler at $80:88E5 JMLs to
 * PlayerLoopBack or ActorLoopBack) or by ending (a jump to ActorLoopNext's
 * re-entry ActorLoopBack). The C ends where the loop hands control to an
 * actor (actor_hand_off), as the original's record does.
 */
#include "machine.h"
#include "ops65.h"
#include "cpu_ops.h"
#include "actor.h"

#define PLAYER_LOOP_ON 0x08DAu      /* 1 while the player's record is run */
#define RECORDS_LEFT 0x032Fu        /* records still to look at this frame */
#define RECORD_NUMBER 0x0330u       /* the record being looked at, from 0 */
#define RECORDS_END 0x0B5Bu         /* past the last record */
#define PAUSE_BLOCK 0x0404u         /* nonzero: only the first records run */
#define ACTOR_RESUME_P 0x02u        /* where it resumes: P, then PC and bank */
#define WAITS 0x8F4Fu               /* wait routines by kind, kinds below $80 */
#define WAITS_80 0x904Fu            /* and from $80 */
#define STARTS 0x8CABu              /* start entries by kind: state $0C-$7F */
#define STARTS_80 0x8DABu
#define SECONDS 0x8DFDu             /* state bit 7 ($80-$BF) */
#define SECONDS_80 0x8EFDu
#define THIRDS 0x90A1u              /* state bits 7 and 6 ($C0 up) */
#define THIRDS_80 0x91A1u

/* The record's wait routine, by kind: JSR through its table, its return
   address pushed as the JSR pushes it (`at`, the JSR's address: the
   player's at 83:9495 and 83:949C, the others' at 83:94C9 and 83:94D0), so
   a wait routine that drops it to end its actor finds it there. */
static void wait_work(uint16_t at_80, uint16_t at)
{
    uint32_t to;

    lda8(ea_dp(ACTOR_06));
    if (IF_N) {
        ACC8(asl);
        tax8();
        to = 0x830000u | rd16(0x830000u | (uint16_t)(WAITS_80 + cpu.x));
        push16((uint16_t)(at_80 + 2));
    } else {
        ACC8(asl);
        tax8();
        to = 0x830000u | rd16(0x830000u | (uint16_t)(WAITS + cpu.x));
        push16((uint16_t)(at + 2));
    }
    call_routine(to, 0);
    cpu.s = (uint16_t)(cpu.s + 2);
}

/* From 83:94F1: the record runs: state 8; below $0C it goes back into its
   actor by RTI; else to the start entry its state and kind pick. Never
   returns: the C ends at the hand-off. */
static void run_record(void)
{
    uint32_t to;

    ldx8_v(0x08);
    stx8(ea_dp(ACTOR_STATE));
    cmp8_v(0x0C);
    if (!IF_C) {
        op_rep(0x20);
        lda16(ea_dp(ACTOR_RESUME_P + 2));
        pha16();
        lda16(ea_dp(ACTOR_RESUME_P));
        pha16();
        op_plp(op_pull8());             /* RTI: P, then PC and bank */
        to = op_pull8();
        to |= (uint32_t)op_pull8() << 8;
        to |= (uint32_t)op_pull8() << 16;
        actor_hand_off(to, 0x40);
        return;
    }
    ACC8(asl);
    if (!IF_C) {
        lda8(ea_dp(ACTOR_06));
        ACC8(asl);
        tax8();
        to = 0x830000u | rd16(0x830000u | (uint16_t)((IF_C ? STARTS_80 : STARTS) + cpu.x));
    } else {
        ACC8(asl);
        if (!IF_C) {
            lda8(ea_dp(ACTOR_06));
            ACC8(asl);
            tax8();
            to = 0x830000u | rd16(0x830000u | (uint16_t)((IF_C ? SECONDS_80 : SECONDS) + cpu.x));
        } else {
            lda8(ea_dp(ACTOR_06));
            ACC8(asl);
            tax8();
            to = 0x830000u | rd16(0x830000u | (uint16_t)((IF_C ? THIRDS_80 : THIRDS) + cpu.x));
        }
    }
    actor_hand_off(to, 0x7C);
}

/* From 83:949F: the player's turn done. */
static void player_done(void)
{
    stz8(ea_abs(PLAYER_LOOP_ON));
    pld();
}

/* From 83:948B: a waiting player's wait routine (bit 7 of $09), then done. */
static void player_back(void)
{
    lda8(ea_dp(ACTOR_09));
    if (IF_N)
        wait_work(0x9495, 0x949C);
    player_done();
}

/*
 * @routine 83:9475 PlayerLoop
 * @actorloop
 * WHAT IT DOES: The player's turn of the actor loop: its record run (to
 * its actor's code) when its wait runs out or it starts, else its wait
 * routine.
 * HOW: D kept; PLAYER_LOOP_ON counted up; D = the player's record. State
 * 0: done. State 1: frames left ($01) counted down; at 0, run_record;
 * else on at PlayerLoopBack. Any other state: run_record. Done:
 * PLAYER_LOOP_ON 0, D back, RTL.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; called
 * by JSL. Exit: by RTL, or none: handed to the player's code.
 */
void abi_PlayerLoop(void)
{
    phd();
    MEM8(inc, ea_abs(PLAYER_LOOP_ON));
    lda8_v((uint8_t)(PLAYER >> 8));
    xba();
    lda8_v((uint8_t)PLAYER);
    tcd();
    lda8(ea_dp(ACTOR_STATE));
    if (IF_Z) { player_done(); return; }
    cmp8_v(0x01);
    if (IF_Z) {
        MEM8(dec, ea_dp(ACTOR_01));
        if (!IF_Z) { player_back(); return; }
    }
    run_record();
}

/*
 * @routine 83:948B PlayerLoopBack
 * @actorloop
 * @pushed 2
 * WHAT IT DOES: The player's turn of the actor loop after its actor came
 * back (the BRK handler's JML) or while it waits: its wait routine, then
 * the turn ends.
 * HOW: ACTOR_09 bit 7: its kind's wait routine (JSR through WAITS or
 * WAITS_80, the latter run by no script); PLAYER_LOOP_ON 0; D back; RTL.
 * Entry: M 8-bit, X 8-bit; D = the player's record; the loop's D pushed
 * above PlayerLoop's return. Exit: by RTL to PlayerLoop's caller.
 */
void abi_PlayerLoopBack(void)
{
    player_back();
}

/* From 83:94D3: on to the next record; past the last (or past the first
   with PAUSE_BLOCK clear and the end reached), done. */
static void others_from(int at);
enum { AT_RECORD, AT_BACK, AT_NEXT };

static void others_from(int at)
{
    for (;;) {
        if (at == AT_RECORD) {
            lda8(ea_dp(ACTOR_STATE));
            if (!IF_Z) {
                cmp8_v(0x01);
                if (!IF_Z) { run_record(); return; }
                MEM8(dec, ea_dp(ACTOR_01));
                if (IF_Z) { run_record(); return; }
                at = AT_BACK;
                continue;
            }
            at = AT_NEXT;
        }
        if (at == AT_BACK) {
            lda8(ea_dp(ACTOR_09));
            if (IF_N)
                wait_work(0x94C9, 0x94D0);
            at = AT_NEXT;
        }
        op_rep(0x20);
        clc();
        tdc();
        adc16_v(0x0050);
        tcd();
        cmp16_v(RECORDS_END);
        op_sep(0x20);
        if (IF_C) {
            lda8(ea_abs(PAUSE_BLOCK));
            if (!IF_Z) { pld(); return; }
        }
        MEM8(inc, ea_abs(RECORD_NUMBER));
        MEM8(dec, ea_abs(RECORDS_LEFT));
        if (IF_Z) { pld(); return; }
        at = AT_RECORD;
    }
}

/*
 * @routine 83:94A4 ActorLoop
 * @actorloop
 * WHAT IT DOES: The other actors' turns: each of the $23 records from
 * $092B looked at in turn and run (handed to its actor's code) when due.
 * HOW: D kept; RECORDS_LEFT $23, RECORD_NUMBER 0; D = $092B; then as
 * ActorLoopNext from its record test: state 0 skipped; state 1 counted
 * down, at 0 run_record, else the wait routine; other states run_record.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; called
 * by JSL. Exit: by RTL, or none: handed to an actor's code.
 */
void abi_ActorLoop(void)
{
    phd();
    lda8_v(0x23);
    sta8(ea_abs(RECORDS_LEFT));
    stz8(ea_abs(RECORD_NUMBER));
    lda8_v(0x09);
    xba();
    lda8_v(0x2B);
    tcd();
    others_from(AT_RECORD);
}

/*
 * @routine 83:94BF ActorLoopBack
 * @actorloop
 * @pushed 2
 * WHAT IT DOES: The actor loop after an actor came back (the BRK handler's
 * JML, or the actor ending) or while it waits: the record's wait routine,
 * then on to the next record.
 * HOW: ACTOR_09 bit 7: its kind's wait routine; then as ActorLoopNext.
 * Entry: M 8-bit, X 8-bit; D = the record; the loop's D pushed above
 * ActorLoop's return. Exit: by RTL to ActorLoop's caller, or none: handed
 * to an actor's code.
 */
void abi_ActorLoopBack(void)
{
    others_from(AT_BACK);
}

/*
 * @routine 83:94D3 ActorLoopNext
 * @actorloop
 * @pushed 2
 * WHAT IT DOES: The actor loop's step to the next record, and the record
 * run (run_record, from 83:94F1) its tests share.
 * HOW: D + $50; at RECORDS_END with PAUSE_BLOCK set, done; RECORD_NUMBER
 * up, RECORDS_LEFT down, at 0 done; else the next record's test. Done: D
 * back, RTL. Run: state 8; below $0C, $04-$05 and $02-$03 pushed and RTI
 * into the actor; else JMP through STARTS, SECONDS or THIRDS (or their
 * _80 halves) by the state's bits 7-6 and the kind (THIRDS_80 run by no
 * script).
 * @unreachable 83:952D-9531 JMP (THIRDS_80,X): THIRDS_80 ($83:91A1) holds
 * $FFFF for every kind $80-$A8, so the jump always lands on $FFFF (Jeff,
 * 2026-11-19: exempt).
 * Entry: M 8-bit, X 8-bit; D = the record; the loop's D pushed above
 * ActorLoop's return. Exit: as ActorLoopBack.
 */
void abi_ActorLoopNext(void)
{
    others_from(AT_NEXT);
}
