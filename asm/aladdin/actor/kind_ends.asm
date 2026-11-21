; kind_ends.asm: the second entries of the actor kinds that only end the actor (a JML to ActorEndPlaced), those that are one RTS (dead:
; the actor loop JMPs to them), and kind $52's. No play reaches them: cov_second_ends, unit_second_rts and
; cov_kind52_second put a record of each kind in state $8C, or call them at their own entry.
; C: src/aladdin/actor/kind_ends.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816



; @routine 83:921E Kind12Second
; @actor
; WHAT IT DOES: Where actors of kind $12 go on from the actor loop in a
; state with bit 7 set (the table at $83:8DFD): Kind12End. No script
; runs it by itself: cov_second_ends puts a record of the kind in state
; $8C.
; HOW: JML to Kind12End ($81:EC1F).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as Kind12End.
%at($83921E)
Kind12Second:
    jml.l Kind12End

; @routine 83:922F Kind21Second
; @actor
; WHAT IT DOES: Where actors of kind $21 go on from the actor loop in a
; state with bit 7 set (the table at $83:8DFD): Kind21End. No script
; runs it by itself: cov_second_ends puts a record of the kind in state
; $8C.
; HOW: JML to Kind21End ($A5:F3EC).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as Kind21End.
%at($83922F)
Kind21Second:
    jml.l Kind21End

; @routine 83:92BE Kind7FSecond
; @actor
; WHAT IT DOES: Where actors of kind $7F go on from the actor loop in a
; state with bit 7 set (the table at $83:8DFD): Kind7FEnd. No script
; runs it by itself: cov_second_ends puts a record of the kind in state
; $8C.
; HOW: JML to Kind7FEnd ($84:9383).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as Kind7FEnd.
%at($8392BE)
Kind7FSecond:
    jml.l Kind7FEnd

; @routine 83:935E Kind8FSecond
; @actor
; WHAT IT DOES: Where actors of kind $8F go on from the actor loop in a
; state with bit 7 set (the table at $83:8EFD): Kind8FWalkEnd. No script
; runs it by itself: cov_second_ends puts a record of the kind in state
; $8C.
; HOW: JML to Kind8FWalkEnd ($83:FAB7).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as Kind8FWalkEnd.
%at($83935E)
Kind8FSecond:
    jml.l Kind8FWalkEnd

; @routine 83:93D4 Kind99Second
; @actor
; WHAT IT DOES: Where actors of kind $99 go on from the actor loop in a
; state with bit 7 set (the table at $83:8EFD): Kind99End. No script
; runs it by itself: cov_second_ends puts a record of the kind in state
; $8C.
; HOW: JML to Kind99End ($84:A434).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as Kind99End.
%at($8393D4)
Kind99Second:
    jml.l Kind99End

; @routine 83:93E1 Kind9ASecond
; @actor
; WHAT IT DOES: Where actors of kind $9A go on from the actor loop in a
; state with bit 7 set (the table at $83:8EFD): Kind9AEnd. No script
; runs it by itself: cov_second_ends puts a record of the kind in state
; $8C.
; HOW: JML to Kind9AEnd ($84:A4DF).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as Kind9AEnd.
%at($8393E1)
Kind9ASecond:
    jml.l Kind9AEnd

; @routine 83:A03A Kind27Second
; @actor
; WHAT IT DOES: Where actors of kind $27 go on from the actor loop in a
; state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
; script runs it by itself: cov_second_ends puts a record of the kind in
; state $8C.
; HOW: JML to ActorEndPlaced ($83:96CA).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as ActorEndPlaced.
%at($83A03A)
Kind27Second:
    jml.l ActorEndPlaced

; @routine 83:B0C7 Kind36Second
; @actor
; WHAT IT DOES: Where actors of kinds $36 and $9D go on from the actor
; loop in a state with bit 7 set (the tables at $83:8DFD and $83:8EFD):
; ActorEndPlaced. No script runs it by itself: cov_second_ends puts a
; record of the kind in state $8C.
; HOW: JML to ActorEndPlaced ($83:96CA).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as ActorEndPlaced.
%at($83B0C7)
Kind36Second:
    jml.l ActorEndPlaced

; @routine 83:B150 Kind3CSecond
; @actor
; WHAT IT DOES: Where actors of kind $3C go on from the actor loop in a
; state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
; script runs it by itself: cov_second_ends puts a record of the kind in
; state $8C.
; HOW: JML to ActorEndPlaced ($83:96CA).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as ActorEndPlaced.
%at($83B150)
Kind3CSecond:
    jml.l ActorEndPlaced

; @routine 83:B196 Kind3ESecond
; @actor
; WHAT IT DOES: Where actors of kind $3E go on from the actor loop in a
; state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
; script runs it by itself: cov_second_ends puts a record of the kind in
; state $8C.
; HOW: JML to ActorEndPlaced ($83:96CA).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as ActorEndPlaced.
%at($83B196)
Kind3ESecond:
    jml.l ActorEndPlaced

; @routine 83:B1FD Kind3FSecond
; @actor
; WHAT IT DOES: Where actors of kind $3F go on from the actor loop in a
; state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
; script runs it by itself: cov_second_ends puts a record of the kind in
; state $8C.
; HOW: JML to ActorEndPlaced ($83:96CA).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as ActorEndPlaced.
%at($83B1FD)
Kind3FSecond:
    jml.l ActorEndPlaced

; @routine 83:B38F Kind41Second
; @actor
; WHAT IT DOES: Where actors of kind $41 go on from the actor loop in a
; state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
; script runs it by itself: cov_second_ends puts a record of the kind in
; state $8C.
; HOW: JML to ActorEndPlaced ($83:96CA).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as ActorEndPlaced.
%at($83B38F)
Kind41Second:
    jml.l ActorEndPlaced

; @routine 83:B457 Kind43Second
; @actor
; WHAT IT DOES: Where actors of kind $43 go on from the actor loop in a
; state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
; script runs it by itself: cov_second_ends puts a record of the kind in
; state $8C.
; HOW: JML to ActorEndPlaced ($83:96CA).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as ActorEndPlaced.
%at($83B457)
Kind43Second:
    jml.l ActorEndPlaced

; @routine 83:B492 Kind44Second
; @actor
; WHAT IT DOES: Where actors of kind $44 go on from the actor loop in a
; state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
; script runs it by itself: cov_second_ends puts a record of the kind in
; state $8C.
; HOW: JML to ActorEndPlaced ($83:96CA).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as ActorEndPlaced.
%at($83B492)
Kind44Second:
    jml.l ActorEndPlaced

; @routine 83:BB96 Kind33Second
; WHAT IT DOES: Kind $33's second entry (the table at $83:8DFD): a
; single RTS. Dead: the actor loop jumps to it, so the RTS returns
; through the loop's own frame.
; HOW: RTS.
; Entry: D = the actor's record. Reached by the table at $83:8DFD, from
; the actor loop's JMP: its RTS pulls the loop's own saved bytes, and
; the game runs on from garbage.
; Exit: as above.
%at($83BB96)
Kind33Second:
    rts

; @routine 83:C2A5 Kind3BSecond
; @actor
; WHAT IT DOES: Where actors of kind $3B go on from the actor loop in a
; state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
; script runs it by itself: cov_second_ends puts a record of the kind in
; state $8C.
; HOW: JML to ActorEndPlaced ($83:96CA).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as ActorEndPlaced.
%at($83C2A5)
Kind3BSecond:
    jml.l ActorEndPlaced

; @routine 83:C4A1 Kind3DSecond
; @actor
; WHAT IT DOES: Where actors of kind $3D go on from the actor loop in a
; state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
; script runs it by itself: cov_second_ends puts a record of the kind in
; state $8C.
; HOW: JMP to ActorEndPlaced ($83:96CA).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as ActorEndPlaced.
%at($83C4A1)
Kind3DSecond:
    jmp.w ActorEndPlaced

; @routine 83:C565 Kind40Second
; @actor
; WHAT IT DOES: Where actors of kind $40 go on from the actor loop in a
; state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
; script runs it by itself: cov_second_ends puts a record of the kind in
; state $8C.
; HOW: JML to ActorEndPlaced ($83:96CA).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as ActorEndPlaced.
%at($83C565)
Kind40Second:
    jml.l ActorEndPlaced

; @routine 83:CACE Kind47Second
; @actor
; WHAT IT DOES: Where actors of kind $47 go on from the actor loop in a
; state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
; script runs it by itself: cov_second_ends puts a record of the kind in
; state $8C.
; HOW: JMP to ActorEndPlaced ($83:96CA).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as ActorEndPlaced.
%at($83CACE)
Kind47Second:
    jmp.w ActorEndPlaced

; @routine 83:D108 Kind4FSecond
; WHAT IT DOES: Kind $4F's second entry (the table at $83:8DFD): a
; single RTS. Dead: see Kind33Second.
; HOW: RTS.
; Entry: D = the actor's record. Reached by the table at $83:8DFD, from
; the actor loop's JMP: its RTS pulls the loop's own saved bytes, and
; the game runs on from garbage.
; Exit: as above.
%at($83D108)
Kind4FSecond:
    rts

; @routine 83:D545 Kind52Second
; @actor
; WHAT IT DOES: Kind $52's second state (the table at $83:8DFD): its
; sound $24 and animation 3 for $A0 frames, then it starts over
; (Kind52Start). Nothing sets state bit 7 on a kind $52;
; cov_kind52_second puts a record of it in state $8C.
; HOW: $48 cleared; SoundQueueAdd $24; AnimStart 3; $07 cleared; $47 =
; $A0; each frame (BRK) AnimStep and $47 down to 0; JMP Kind52Start.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the actor
; loop's jump for kind $52 in a state with bit 7 set. Exit: never
; returns: as Kind52Start.
%at($83D545)
Kind52Second:
    stz.b !actor_48
    lda.b #$24
    jsl.l SoundQueueAdd
    lda.b #$03
    jsl.l AnimStart
    stz.b !actor_07
    lda.b #$A0
    sta.b !actor_47
.frame:
    brk.b #$00
    jsl.l AnimStep
    dec.b !actor_47
    bne .frame
    jmp.w Kind52Start

; @routine 83:D8DE Kind55Second
; @actor
; WHAT IT DOES: Where actors of kind $55 go on from the actor loop in a
; state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
; script runs it by itself: cov_second_ends puts a record of the kind in
; state $8C.
; HOW: JML to ActorEndPlaced ($83:96CA).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as ActorEndPlaced.
%at($83D8DE)
Kind55Second:
    jml.l ActorEndPlaced

; @routine 83:E27E Kind5CSecond
; @actor
; WHAT IT DOES: Where actors of kind $5C go on from the actor loop in a
; state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
; script runs it by itself: cov_second_ends puts a record of the kind in
; state $8C.
; HOW: JML to ActorEndPlaced ($83:96CA).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as ActorEndPlaced.
%at($83E27E)
Kind5CSecond:
    jml.l ActorEndPlaced

; @routine 83:E4E3 Kind60Second
; @actor
; WHAT IT DOES: Where actors of kind $60 go on from the actor loop in a
; state with bit 7 set (the table at $83:8DFD): ActorEndPlaced. No
; script runs it by itself: cov_second_ends puts a record of the kind in
; state $8C.
; HOW: JML to ActorEndPlaced ($83:96CA).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as ActorEndPlaced.
%at($83E4E3)
Kind60Second:
    jml.l ActorEndPlaced

; @routine 83:F40C Kind71Second
; WHAT IT DOES: Kind $71's second entry (the table at $83:8DFD): a
; single RTS. Dead: see Kind33Second.
; HOW: RTS.
; Entry: D = the actor's record. Reached by the table at $83:8DFD, from
; the actor loop's JMP: its RTS pulls the loop's own saved bytes, and
; the game runs on from garbage.
; Exit: as above.
%at($83F40C)
Kind71Second:
    rts

; @routine 83:FA4E Kind8ASecond
; WHAT IT DOES: Kind $8A's second entry (the table at $83:8EFD): a
; single RTS. Dead: see Kind33Second.
; HOW: RTS.
; Entry: D = the actor's record. Reached by the table at $83:8EFD, from
; the actor loop's JMP: its RTS pulls the loop's own saved bytes, and
; the game runs on from garbage.
; Exit: as above.
%at($83FA4E)
Kind8ASecond:
    rts

; @routine 84:9383 Kind7FEnd
; @actor
; WHAT IT DOES: Where the second state of kind $7F ends the actor:
; ActorEndPlaced.
; HOW: JML to ActorEndPlaced ($83:96CA).
; Its callers' C calls it inside C, which A5 does not see as a call: cov_end_entries resumes a
; waiting record here (the actor loop's RTI at the record's resume address).
; Entry: D = the actor's record; reached by the JML of Kind7FSecond
; ($83:92BE).
; Exit: never returns: as ActorEndPlaced.
%at($849383)
Kind7FEnd:
    jml.l ActorEndPlaced

; @routine 84:9879 Kind8BSecondReturn
; WHAT IT DOES: The RTS that kind $8B's second entry (Kind8BSecond) JMLs
; to. Dead: see Kind33Second.
; HOW: RTS.
; Entry: D = the actor's record. Reached by Kind8BSecond ($83:9333),
; from the actor loop's JMP: its RTS pulls the loop's own saved bytes,
; and the game runs on from garbage.
; Exit: as above.
%at($849879)
Kind8BSecondReturn:
    rts

; @routine 84:99EC Kind8CSecondReturn
; WHAT IT DOES: The RTS that kind $8C's second entry (Kind8CSecond) JMLs
; to. Dead: see Kind33Second.
; HOW: RTS.
; Entry: D = the actor's record. Reached by Kind8CSecond ($83:9340),
; from the actor loop's JMP: its RTS pulls the loop's own saved bytes,
; and the game runs on from garbage.
; Exit: as above.
%at($8499EC)
Kind8CSecondReturn:
    rts

; @routine 84:A434 Kind99End
; @actor
; WHAT IT DOES: Where the second state of kind $99 ends the actor:
; ActorEndPlaced.
; HOW: JML to ActorEndPlaced ($83:96CA).
; Its callers' C calls it inside C, which A5 does not see as a call: cov_end_entries resumes a
; waiting record here (the actor loop's RTI at the record's resume address).
; Entry: D = the actor's record; reached by the JML of Kind99Second
; ($83:93D4).
; Exit: never returns: as ActorEndPlaced.
%at($84A434)
Kind99End:
    jml.l ActorEndPlaced

; @routine 84:A4DF Kind9AEnd
; @actor
; WHAT IT DOES: Where the second state of kind $9A ends the actor:
; ActorEndPlaced.
; HOW: JML to ActorEndPlaced ($83:96CA).
; Its callers' C calls it inside C, which A5 does not see as a call: cov_end_entries resumes a
; waiting record here (the actor loop's RTI at the record's resume address).
; Entry: D = the actor's record; reached by the JML of Kind9ASecond
; ($83:93E1).
; Exit: never returns: as ActorEndPlaced.
%at($84A4DF)
Kind9AEnd:
    jml.l ActorEndPlaced

; @routine A5:F3EC Kind21End
; @actor
; WHAT IT DOES: Where the second state of kind $21 ends the actor:
; ActorEndPlaced.
; HOW: JML to ActorEndPlaced ($83:96CA).
; Its callers' C calls it inside C, which A5 does not see as a call: cov_end_entries resumes a
; waiting record here (the actor loop's RTI at the record's resume address).
; Entry: D = the actor's record; reached by the JML of Kind21Second
; ($83:922F).
; Exit: never returns: as ActorEndPlaced.
%at($A5F3EC)
Kind21End:
    jml.l ActorEndPlaced
