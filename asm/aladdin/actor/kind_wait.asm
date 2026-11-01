; kind_wait.asm: what each kind of actor runs, frame by frame, while it
; waits: the actor loop calls it through a table by kind ($83:8F4F for
; kinds below $80, $83:904F for the others) when bit 7 of the record's
; byte at $09 is set (docs/aladdin/ACTORS.md). Each entry here calls the
; kind's work in its own bank.
; C: src/aladdin/actor/kind_wait.c.

incsrc "../include/macros.inc"

arch 65816

; The kinds' work, named by the first kind that uses it.
!kind_00_wait_work = $81E864
!kind_63_wait_work = $8481FC
!kind_a2_wait_work = $84832B
!kind_73_wait_work = $8486F4
!kind_74_wait_work = $848A78
!kind_88_wait_work = $848EF0
!kind_76_wait_work = $84903B
!kind_7f_wait_work = $849387
!kind_82_wait_work = $849533
!kind_85_wait_work = $849628
!kind_95_wait_work = $849EF0
!kind_96_wait_work = $849F4A
!kind_a1_wait_work = $84A9C7
!kind_08_wait_work = $A5F020
!kind_21_wait_work = $A5F3E3

; @routine 83:91F8 ActorKind00Wait
; WHAT IT DOES: The routine actors of kinds $00, $01, $02, $03 and $07
; run each frame they wait (not the last), when bit 7 of their byte at
; $09 is set: the work at $81:E864.
; HOW: JSL $81:E864, then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as the C states.
%at($8391F8)
ActorKind00Wait:
    jsl.l !kind_00_wait_work
    rts

; @routine 83:9209 ActorKind08Wait
; WHAT IT DOES: The routine actors of kind $08 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: the work at
; $A5:F020.
; HOW: JSL $A5:F020, then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as the C states.
%at($839209)
ActorKind08Wait:
    jsl.l !kind_08_wait_work
    rts

; @routine 83:922A ActorKind21Wait
; WHAT IT DOES: The routine actors of kind $21 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: the work at
; $A5:F3E3.
; HOW: JSL $A5:F3E3, then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as the C states.
%at($83922A)
ActorKind21Wait:
    jsl.l !kind_21_wait_work
    rts

; @routine 83:924D ActorKind63Wait
; WHAT IT DOES: The routine actors of kind $63 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: the work at
; $84:81FC.
; HOW: JSL $84:81FC, then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as the C states.
%at($83924D)
ActorKind63Wait:
    jsl.l !kind_63_wait_work
    rts

; @routine 83:925E ActorKind73Wait
; WHAT IT DOES: The routine actors of kind $73 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: the work at
; $84:86F4.
; HOW: JSL $84:86F4, then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as the C states.
%at($83925E)
ActorKind73Wait:
    jsl.l !kind_73_wait_work
    rts

; @routine 83:926B ActorKind74Wait
; WHAT IT DOES: The routine actors of kind $74 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: the work at
; $84:8A78.
; HOW: JSL $84:8A78, then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as the C states.
%at($83926B)
ActorKind74Wait:
    jsl.l !kind_74_wait_work
    rts

; @routine 83:9278 ActorKind75Wait
; WHAT IT DOES: The routine actors of kind $75 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: the work at
; $84:8A78.
; HOW: JSL $84:8A78, then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as the C states.
%at($839278)
ActorKind75Wait:
    jsl.l !kind_74_wait_work
    rts

; @routine 83:9285 ActorKind76Wait
; WHAT IT DOES: The routine actors of kind $76 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: the work at
; $84:903B.
; HOW: JSL $84:903B, then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as the C states.
%at($839285)
ActorKind76Wait:
    jsl.l !kind_76_wait_work
    rts

; @routine 83:92B9 ActorKind7FWait
; WHAT IT DOES: The routine actors of kind $7F run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: the work at
; $84:9387.
; HOW: JSL $84:9387, then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as the C states.
%at($8392B9)
ActorKind7FWait:
    jsl.l !kind_7f_wait_work
    rts

; @routine 83:92D3 ActorKind82Wait
; WHAT IT DOES: The routine actors of kind $82 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: the work at
; $84:9533.
; HOW: JSL $84:9533, then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as the C states.
%at($8392D3)
ActorKind82Wait:
    jsl.l !kind_82_wait_work
    rts

; @routine 83:92F6 ActorKind85Wait
; WHAT IT DOES: The routine actors of kind $85 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: the work at
; $84:9628.
; HOW: JSL $84:9628, then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as the C states.
%at($8392F6)
ActorKind85Wait:
    jsl.l !kind_85_wait_work
    rts

; @routine 83:931D ActorKind88Wait
; WHAT IT DOES: The routine actors of kind $88 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: the work at
; $84:8EF0.
; HOW: JSL $84:8EF0, then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as the C states.
%at($83931D)
ActorKind88Wait:
    jsl.l !kind_88_wait_work
    rts

; @routine 83:9366 ActorKind90Wait
; WHAT IT DOES: The routine actors of kind $90 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: nothing.
; HOW: JSL LongReturn, then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as the C states.
%at($839366)
ActorKind90Wait:
    jsl.l LongReturn
    rts

; @routine 83:93A7 ActorKind95Wait
; WHAT IT DOES: The routine actors of kind $95 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: the work at
; $84:9EF0.
; HOW: JSL $84:9EF0, then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as the C states.
%at($8393A7)
ActorKind95Wait:
    jsl.l !kind_95_wait_work
    rts

; @routine 83:93B0 ActorKind96Wait
; WHAT IT DOES: The routine actors of kind $96 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: the work at
; $84:9F4A.
; HOW: JSL $84:9F4A, then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as the C states.
%at($8393B0)
ActorKind96Wait:
    jsl.l !kind_96_wait_work
    rts

; @routine 83:940D ActorKindA1Wait
; WHAT IT DOES: The routine actors of kind $A1 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: the work at
; $84:A9C7.
; HOW: JSL $84:A9C7, then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as the C states.
%at($83940D)
ActorKindA1Wait:
    jsl.l !kind_a1_wait_work
    rts

; @routine 83:941A ActorKindA2Wait
; WHAT IT DOES: The routine actors of kind $A2 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: the work at
; $84:832B.
; HOW: JSL $84:832B, then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as the C states.
%at($83941A)
ActorKindA2Wait:
    jsl.l !kind_a2_wait_work
    rts
