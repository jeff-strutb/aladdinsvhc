/*
 * kind_ends.c: the second entries of the actor kinds that only end the actor (a JML to ActorEndPlaced), those that are one RTS (dead:
 * the actor loop JMPs to them), and kind $52's. No play reaches them: cov_second_ends, unit_second_rts and
 * cov_kind52_second put a record of each kind in state $8C, or call them at their own entry.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorEndPlaced(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_Kind12End(void);
void abi_Kind21End(void);
void abi_Kind52Start(void);
void abi_Kind7FEnd(void);
void abi_Kind8BSecondReturn(void);
void abi_Kind8CSecondReturn(void);
void abi_Kind8FWalkEnd(void);
void abi_Kind99End(void);
void abi_Kind9AEnd(void);
void abi_SoundQueueAdd(void);



/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:921E Kind12Second
 * @actor
 * WHAT IT DOES: Where actors of kind $12 go on from the actor loop in a
 * state with bit 7 set (the table at $83:8DFD): Kind12End. No script
 * runs it by itself: cov_second_ends puts a record of the kind in state
 * $8C.
 * HOW: JML to Kind12End ($81:EC1F).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as Kind12End.
 */
void abi_Kind12Second(void)
{
    abi_Kind12End();
}

/*
 * @routine 83:922F Kind21Second
 * @actor
 * WHAT IT DOES: Where actors of kind $21 go on from the actor loop in a
 * state with bit 7 set (the table at $83:8DFD): Kind21End. No script
 * runs it by itself: cov_second_ends puts a record of the kind in state
 * $8C.
 * HOW: JML to Kind21End ($A5:F3EC).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as Kind21End.
 */
void abi_Kind21Second(void)
{
    abi_Kind21End();
}

/*
 * @routine 83:92BE Kind7FSecond
 * @actor
 * WHAT IT DOES: Where actors of kind $7F go on from the actor loop in a
 * state with bit 7 set (the table at $83:8DFD): Kind7FEnd. No script
 * runs it by itself: cov_second_ends puts a record of the kind in state
 * $8C.
 * HOW: JML to Kind7FEnd ($84:9383).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as Kind7FEnd.
 */
void abi_Kind7FSecond(void)
{
    abi_Kind7FEnd();
}

/*
 * @routine 83:935E Kind8FSecond
 * @actor
 * WHAT IT DOES: Where actors of kind $8F go on from the actor loop in a
 * state with bit 7 set (the table at $83:8EFD): Kind8FWalkEnd. No
 * script runs it by itself: cov_second_ends puts a record of the kind
 * in state $8C.
 * HOW: JML to Kind8FWalkEnd ($83:FAB7).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as Kind8FWalkEnd.
 */
void abi_Kind8FSecond(void)
{
    abi_Kind8FWalkEnd();
}

/*
 * @routine 83:93D4 Kind99Second
 * @actor
 * WHAT IT DOES: Where actors of kind $99 go on from the actor loop in a
 * state with bit 7 set (the table at $83:8EFD): Kind99End. No script
 * runs it by itself: cov_second_ends puts a record of the kind in state
 * $8C.
 * HOW: JML to Kind99End ($84:A434).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as Kind99End.
 */
void abi_Kind99Second(void)
{
    abi_Kind99End();
}

/*
 * @routine 83:93E1 Kind9ASecond
 * @actor
 * WHAT IT DOES: Where actors of kind $9A go on from the actor loop in a
 * state with bit 7 set (the table at $83:8EFD): Kind9AEnd. No script
 * runs it by itself: cov_second_ends puts a record of the kind in state
 * $8C.
 * HOW: JML to Kind9AEnd ($84:A4DF).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as Kind9AEnd.
 */
void abi_Kind9ASecond(void)
{
    abi_Kind9AEnd();
}

/*
 * @routine 83:A03A Kind27Second
 * @actor
 * WHAT IT DOES: Where actors of kind $27 go on from the actor loop in a
 * state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
 * script runs it by itself: cov_second_ends puts a record of the kind
 * in state $8C.
 * HOW: JML to ActorEndPlaced ($83:96CA).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind27Second(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 83:B0C7 Kind36Second
 * @actor
 * WHAT IT DOES: Where actors of kinds $36 and $9D go on from the actor
 * loop in a state with bit 7 set (the tables at $83:8DFD and $83:8EFD):
 * ActorEndPlaced. No script runs it by itself: cov_second_ends puts a
 * record of the kind in state $8C.
 * HOW: JML to ActorEndPlaced ($83:96CA).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind36Second(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 83:B150 Kind3CSecond
 * @actor
 * WHAT IT DOES: Where actors of kind $3C go on from the actor loop in a
 * state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
 * script runs it by itself: cov_second_ends puts a record of the kind
 * in state $8C.
 * HOW: JML to ActorEndPlaced ($83:96CA).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind3CSecond(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 83:B196 Kind3ESecond
 * @actor
 * WHAT IT DOES: Where actors of kind $3E go on from the actor loop in a
 * state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
 * script runs it by itself: cov_second_ends puts a record of the kind
 * in state $8C.
 * HOW: JML to ActorEndPlaced ($83:96CA).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind3ESecond(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 83:B1FD Kind3FSecond
 * @actor
 * WHAT IT DOES: Where actors of kind $3F go on from the actor loop in a
 * state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
 * script runs it by itself: cov_second_ends puts a record of the kind
 * in state $8C.
 * HOW: JML to ActorEndPlaced ($83:96CA).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind3FSecond(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 83:B38F Kind41Second
 * @actor
 * WHAT IT DOES: Where actors of kind $41 go on from the actor loop in a
 * state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
 * script runs it by itself: cov_second_ends puts a record of the kind
 * in state $8C.
 * HOW: JML to ActorEndPlaced ($83:96CA).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind41Second(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 83:B457 Kind43Second
 * @actor
 * WHAT IT DOES: Where actors of kind $43 go on from the actor loop in a
 * state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
 * script runs it by itself: cov_second_ends puts a record of the kind
 * in state $8C.
 * HOW: JML to ActorEndPlaced ($83:96CA).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind43Second(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 83:B492 Kind44Second
 * @actor
 * WHAT IT DOES: Where actors of kind $44 go on from the actor loop in a
 * state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
 * script runs it by itself: cov_second_ends puts a record of the kind
 * in state $8C.
 * HOW: JML to ActorEndPlaced ($83:96CA).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind44Second(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 83:BB96 Kind33Second
 * WHAT IT DOES: Kind $33's second entry (the table at $83:8DFD): a
 * single RTS. Dead: the actor loop jumps to it, so the RTS returns
 * through the loop's own frame.
 * HOW: RTS.
 * Entry: D = the actor's record. Reached by the table at $83:8DFD, from
 * the actor loop's JMP: its RTS pulls the loop's own saved bytes, and
 * the game runs on from garbage.
 * Exit: as above.
 */
void abi_Kind33Second(void)
{
    return;
}

/*
 * @routine 83:C2A5 Kind3BSecond
 * @actor
 * WHAT IT DOES: Where actors of kind $3B go on from the actor loop in a
 * state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
 * script runs it by itself: cov_second_ends puts a record of the kind
 * in state $8C.
 * HOW: JML to ActorEndPlaced ($83:96CA).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind3BSecond(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 83:C4A1 Kind3DSecond
 * @actor
 * WHAT IT DOES: Where actors of kind $3D go on from the actor loop in a
 * state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
 * script runs it by itself: cov_second_ends puts a record of the kind
 * in state $8C.
 * HOW: JMP to ActorEndPlaced ($83:96CA).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind3DSecond(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 83:C565 Kind40Second
 * @actor
 * WHAT IT DOES: Where actors of kind $40 go on from the actor loop in a
 * state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
 * script runs it by itself: cov_second_ends puts a record of the kind
 * in state $8C.
 * HOW: JML to ActorEndPlaced ($83:96CA).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind40Second(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 83:CACE Kind47Second
 * @actor
 * WHAT IT DOES: Where actors of kind $47 go on from the actor loop in a
 * state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
 * script runs it by itself: cov_second_ends puts a record of the kind
 * in state $8C.
 * HOW: JMP to ActorEndPlaced ($83:96CA).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind47Second(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 83:D108 Kind4FSecond
 * WHAT IT DOES: Kind $4F's second entry (the table at $83:8DFD): a
 * single RTS. Dead: see Kind33Second.
 * HOW: RTS.
 * Entry: D = the actor's record. Reached by the table at $83:8DFD, from
 * the actor loop's JMP: its RTS pulls the loop's own saved bytes, and
 * the game runs on from garbage.
 * Exit: as above.
 */
void abi_Kind4FSecond(void)
{
    return;
}

/*
 * @routine 83:D545 Kind52Second
 * @actor
 * WHAT IT DOES: Kind $52's second state (the table at $83:8DFD): its
 * sound $24 and animation 3 for $A0 frames, then it starts over
 * (Kind52Start). Nothing sets state bit 7 on a kind $52;
 * cov_kind52_second puts a record of it in state $8C.
 * HOW: $48 cleared; SoundQueueAdd $24; AnimStart 3; $07 cleared; $47 =
 * $A0; each frame (BRK) AnimStep and $47 down to 0; JMP Kind52Start.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the actor
 * loop's jump for kind $52 in a state with bit 7 set. Exit: never
 * returns: as Kind52Start.
 */
void abi_Kind52Second(void)
{
    stz8(ea_dp(0x48));
    lda8_v(0x24);
    jsl_call(0x83D549u, abi_SoundQueueAdd);
    lda8_v(0x03);
    jsl_call(0x83D54Fu, abi_AnimStart);
    stz8(ea_dp(ACTOR_07));
    lda8_v(0xA0);
    sta8(ea_dp(0x47));
L_83D559:
    actor_yield(0x83D559u);
    jsl_call(0x83D55Bu, abi_AnimStep);
    MEM8(dec, ea_dp(0x47));
    if (!IF_Z)
        goto L_83D559;
    abi_Kind52Start();
}

/*
 * @routine 83:D8DE Kind55Second
 * @actor
 * WHAT IT DOES: Where actors of kind $55 go on from the actor loop in a
 * state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
 * script runs it by itself: cov_second_ends puts a record of the kind
 * in state $8C.
 * HOW: JML to ActorEndPlaced ($83:96CA).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind55Second(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 83:E27E Kind5CSecond
 * @actor
 * WHAT IT DOES: Where actors of kind $5C go on from the actor loop in a
 * state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
 * script runs it by itself: cov_second_ends puts a record of the kind
 * in state $8C.
 * HOW: JML to ActorEndPlaced ($83:96CA).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind5CSecond(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 83:E4E3 Kind60Second
 * @actor
 * WHAT IT DOES: Where actors of kind $60 go on from the actor loop in a
 * state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
 * script runs it by itself: cov_second_ends puts a record of the kind
 * in state $8C.
 * HOW: JML to ActorEndPlaced ($83:96CA).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind.
 * Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind60Second(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 83:F40C Kind71Second
 * WHAT IT DOES: Kind $71's second entry (the table at $83:8DFD): a
 * single RTS. Dead: see Kind33Second.
 * HOW: RTS.
 * Entry: D = the actor's record. Reached by the table at $83:8DFD, from
 * the actor loop's JMP: its RTS pulls the loop's own saved bytes, and
 * the game runs on from garbage.
 * Exit: as above.
 */
void abi_Kind71Second(void)
{
    return;
}

/*
 * @routine 83:FA4E Kind8ASecond
 * WHAT IT DOES: Kind $8A's second entry (the table at $83:8EFD): a
 * single RTS. Dead: see Kind33Second.
 * HOW: RTS.
 * Entry: D = the actor's record. Reached by the table at $83:8EFD, from
 * the actor loop's JMP: its RTS pulls the loop's own saved bytes, and
 * the game runs on from garbage.
 * Exit: as above.
 */
void abi_Kind8ASecond(void)
{
    return;
}

/*
 * @routine 84:9383 Kind7FEnd
 * @actor
 * WHAT IT DOES: Where the second state of kind $7F ends the actor:
 * ActorEndPlaced.
 * HOW: JML to ActorEndPlaced ($83:96CA).
 * Its callers' C calls it inside C, which A5 does not see as a call: cov_end_entries resumes a
 * waiting record here (the actor loop's RTI at the record's resume address).
 * Entry: D = the actor's record; reached by the JML of Kind7FSecond
 * ($83:92BE).
 * Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind7FEnd(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 84:9879 Kind8BSecondReturn
 * WHAT IT DOES: The RTS that kind $8B's second entry (Kind8BSecond)
 * JMLs to. Dead: see Kind33Second.
 * HOW: RTS.
 * Entry: D = the actor's record. Reached by Kind8BSecond ($83:9333),
 * from the actor loop's JMP: its RTS pulls the loop's own saved bytes,
 * and the game runs on from garbage.
 * Exit: as above.
 */
void abi_Kind8BSecondReturn(void)
{
    return;
}

/*
 * @routine 84:99EC Kind8CSecondReturn
 * WHAT IT DOES: The RTS that kind $8C's second entry (Kind8CSecond)
 * JMLs to. Dead: see Kind33Second.
 * HOW: RTS.
 * Entry: D = the actor's record. Reached by Kind8CSecond ($83:9340),
 * from the actor loop's JMP: its RTS pulls the loop's own saved bytes,
 * and the game runs on from garbage.
 * Exit: as above.
 */
void abi_Kind8CSecondReturn(void)
{
    return;
}

/*
 * @routine 84:A434 Kind99End
 * @actor
 * WHAT IT DOES: Where the second state of kind $99 ends the actor:
 * ActorEndPlaced.
 * HOW: JML to ActorEndPlaced ($83:96CA).
 * Its callers' C calls it inside C, which A5 does not see as a call: cov_end_entries resumes a
 * waiting record here (the actor loop's RTI at the record's resume address).
 * Entry: D = the actor's record; reached by the JML of Kind99Second
 * ($83:93D4).
 * Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind99End(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 84:A4DF Kind9AEnd
 * @actor
 * WHAT IT DOES: Where the second state of kind $9A ends the actor:
 * ActorEndPlaced.
 * HOW: JML to ActorEndPlaced ($83:96CA).
 * Its callers' C calls it inside C, which A5 does not see as a call: cov_end_entries resumes a
 * waiting record here (the actor loop's RTI at the record's resume address).
 * Entry: D = the actor's record; reached by the JML of Kind9ASecond
 * ($83:93E1).
 * Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind9AEnd(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine A5:F3EC Kind21End
 * @actor
 * WHAT IT DOES: Where the second state of kind $21 ends the actor:
 * ActorEndPlaced.
 * HOW: JML to ActorEndPlaced ($83:96CA).
 * Its callers' C calls it inside C, which A5 does not see as a call: cov_end_entries resumes a
 * waiting record here (the actor loop's RTI at the record's resume address).
 * Entry: D = the actor's record; reached by the JML of Kind21Second
 * ($83:922F).
 * Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind21End(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 83:9333 Kind8BSecond
 * @actor
 * WHAT IT DOES: Where actors of kind $8B go on from the actor loop in
 * a state with bit 7 set (the table at $83:8EFD): Kind8BSecondReturn,
 * a bare RTS. Dead: the RTS pulls the loop's own frame (see
 * Kind33Second); no script and no unit call can judge it.
 * HOW: JML to Kind8BSecondReturn ($84:9879).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never as a return to anywhere sane: the RTS
 * pulls the loop's saved D ($0000), and execution goes on at $84:0001,
 * work RAM through the mirror in bank $84 (its variables run as code).
 * @unreachable 83:9333-9336 This JML (the RTS it reaches is filed and covered): a record of this
 * kind in state $8C makes the actor loop JMP here with its own frame on
 * the stack (PHD, then the JSL's return); the RTS pulls the saved D ($0000)
 * and the game runs on at $84:0001, the zero page's variables as code (a
 * kind $8B record forced to state $8C in stage a5 s3, kind $8C the same, executed work RAM $0001
 * and $0003). Nothing can make the saved D another value, so no input or
 * poke makes the path sane. A unit call cannot judge it: the JML leaves
 * bank $83, the RTS pulls 2 of the JSL's 3 pushed bytes and lands at
 * $84:1FE4, the harness's loop in another bank, with S one byte off the C's
 * (the four one-RTS seconds of unit_second_rts stay in bank $83 and are
 * called). Jeff's rule of 2026-11-19 (exempt with proof); master's yes 2026-11-21.
 */
void abi_Kind8BSecond(void)
{
    abi_Kind8BSecondReturn();
}

/*
 * @routine 83:9340 Kind8CSecond
 * @actor
 * WHAT IT DOES: Where actors of kind $8C go on from the actor loop in
 * a state with bit 7 set (the table at $83:8EFD): Kind8CSecondReturn,
 * a bare RTS. Dead: the RTS pulls the loop's own frame (see
 * Kind33Second); no script and no unit call can judge it.
 * HOW: JML to Kind8CSecondReturn ($84:99EC).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never as a return to anywhere sane: the RTS
 * pulls the loop's saved D ($0000), and execution goes on at $84:0001,
 * work RAM through the mirror in bank $84 (its variables run as code).
 * @unreachable 83:9340-9343 This JML (the RTS it reaches is filed and covered): a record of this
 * kind in state $8C makes the actor loop JMP here with its own frame on
 * the stack (PHD, then the JSL's return); the RTS pulls the saved D ($0000)
 * and the game runs on at $84:0001, the zero page's variables as code (a
 * kind $8B record forced to state $8C in stage a5 s3, kind $8C the same, executed work RAM $0001
 * and $0003). Nothing can make the saved D another value, so no input or
 * poke makes the path sane. A unit call cannot judge it: the JML leaves
 * bank $83, the RTS pulls 2 of the JSL's 3 pushed bytes and lands at
 * $84:1FE4, the harness's loop in another bank, with S one byte off the C's
 * (the four one-RTS seconds of unit_second_rts stay in bank $83 and are
 * called). Jeff's rule of 2026-11-19 (exempt with proof); master's yes 2026-11-21.
 */
void abi_Kind8CSecond(void)
{
    abi_Kind8CSecondReturn();
}
