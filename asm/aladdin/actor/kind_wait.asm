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
; (not the last), when bit 7 of their byte at $09 is set: Kind21AnimateSend.
; HOW: JSL Kind21AnimateSend ($A5:F3E3), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind21AnimateSend leaves them.
%at($83922A)
ActorKind21Wait:
    jsl.l Kind21AnimateSend
    rts

; @routine 83:924D ActorKind63Wait
; WHAT IT DOES: The routine actors of kind $63 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind63Work.
; HOW: JSL Kind63Work ($84:81FC), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind63Work leaves them.
%at($83924D)
ActorKind63Wait:
    jsl.l Kind63Work
    rts

; @routine 83:925E ActorKind73Wait
; WHAT IT DOES: The routine actors of kind $73 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind73Work.
; HOW: JSL Kind73Work ($84:86F4), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind73Work leaves them.
%at($83925E)
ActorKind73Wait:
    jsl.l Kind73Work
    rts

; @routine 83:926B ActorKind74Wait
; WHAT IT DOES: The routine actors of kind $74 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: ChainPieceWork.
; HOW: JSL ChainPieceWork ($84:8A78), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as ChainPieceWork leaves them.
%at($83926B)
ActorKind74Wait:
    jsl.l ChainPieceWork
    rts

; @routine 83:9278 ActorKind75Wait
; WHAT IT DOES: The routine actors of kind $75 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: ChainPieceWork.
; HOW: JSL ChainPieceWork ($84:8A78), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as ChainPieceWork leaves them.
%at($839278)
ActorKind75Wait:
    jsl.l ChainPieceWork
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
; (not the last), when bit 7 of their byte at $09 is set: Kind88Work.
; HOW: JSL Kind88Work ($84:8EF0), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind88Work leaves them.
%at($83931D)
ActorKind88Wait:
    jsl.l Kind88Work
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
; (not the last), when bit 7 of their byte at $09 is set: ActorAnimFreeFar.
; HOW: JSL ActorAnimFreeFar ($84:832B), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as ActorAnimFreeFar leaves them.
%at($83941A)
ActorKindA2Wait:
    jsl.l ActorAnimFreeFar
    rts

; @routine 83:9292 ActorKind7CWait
; WHAT IT DOES: The routine actors of kind $7C run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: MoverEndSound.
; HOW: JSL MoverEndSound ($84:90A2), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as MoverEndSound leaves them.
%at($839292)
ActorKind7CWait:
    jsl.l MoverEndSound
    rts

; @routine 83:92DC ActorKind83Wait
; WHAT IT DOES: The routine actors of kind $83 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: ActorWaitGrabsPlayer.
; HOW: JSL ActorWaitGrabsPlayer ($84:95D4), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as ActorWaitGrabsPlayer leaves them.
%at($8392DC)
ActorKind83Wait:
    jsl.l ActorWaitGrabsPlayer
    rts

; @routine 83:9348 ActorKind8DWait
; WHAT IT DOES: The routine actors of kind $8D run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: SenderWait8E.
; HOW: JSL SenderWait8E ($84:9B4D), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as SenderWait8E leaves them.
%at($839348)
ActorKind8DWait:
    jsl.l SenderWait8E
    rts

; @routine 83:9359 ActorKind8FWait
; WHAT IT DOES: The routine actors of kind $8F run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: KindAnimateSendFAAE.
; HOW: JSL KindAnimateSendFAAE ($83:FAAE), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as KindAnimateSendFAAE leaves them.
%at($839359)
ActorKind8FWait:
    jsl.l KindAnimateSendFAAE
    rts

; @routine 83:93E9 ActorKind9BWait
; WHAT IT DOES: The routine actors of kind $9B run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: ActorBobAnimate.
; HOW: JSL ActorBobAnimate ($84:A7A1), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as ActorBobAnimate leaves them.
%at($8393E9)
ActorKind9BWait:
    jsl.l ActorBobAnimate
    rts

; @routine 83:9237 ActorKind24Wait
; WHAT IT DOES: The routine actors of kind $24 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind24Animate.
; HOW: JSL Kind24Animate ($84:8048), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind24Animate leaves them.
%at($839237)
ActorKind24Wait:
    jsl.l Kind24Animate
    rts

; @routine 83:9373 ActorKind91Wait
; WHAT IT DOES: The routine actors of kind $91 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind91FreeOff.
; HOW: JSL Kind91FreeOff ($A5:F603), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind91FreeOff leaves them.
%at($839373)
ActorKind91Wait:
    jsl.l Kind91FreeOff
    rts

; @routine 83:9380 ActorKind92Wait
; WHAT IT DOES: The routine actors of kind $92 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind91FreeOff.
; HOW: JSL Kind91FreeOff ($A5:F603), then RTS.
; No script runs it (no script has a waiting actor of this kind).
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind91FreeOff leaves them.
%at($839380)
ActorKind92Wait:
    jsl.l Kind91FreeOff
    rts

; @routine 83:938D ActorKind93Wait
; WHAT IT DOES: The routine actors of kind $93 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind93AnimateSend.
; HOW: JSL Kind93AnimateSend ($A5:F434), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind93AnimateSend leaves them.
%at($83938D)
ActorKind93Wait:
    jsl.l Kind93AnimateSend
    rts

; @routine 83:93F2 ActorKind9CWait
; WHAT IT DOES: The routine actors of kind $9C run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind9CAnimate.
; HOW: JSL Kind9CAnimate ($84:A81B), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind9CAnimate leaves them.
%at($8393F2)
ActorKind9CWait:
    jsl.l Kind9CAnimate
    rts

; @routine 83:9404 ActorKindA0Wait
; WHAT IT DOES: The routine actors of kind $A0 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: KindA0AnimateFreeOff.
; HOW: JSL KindA0AnimateFreeOff ($84:A975), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as KindA0AnimateFreeOff leaves them.
%at($839404)
ActorKindA0Wait:
    jsl.l KindA0AnimateFreeOff
    rts

; @routine 83:9427 ActorKindA3Wait
; WHAT IT DOES: The routine actors of kind $A3 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: KindA3Animate.
; HOW: JSL KindA3Animate ($84:8399), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as KindA3Animate leaves them.
%at($839427)
ActorKindA3Wait:
    jsl.l KindA3Animate
    rts

; @routine 83:9240 ActorKind49Wait
; WHAT IT DOES: The routine actors of kind $49 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind49Wait.
; HOW: JSL Kind49Wait ($84:805B), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind49Wait leaves them.
%at($839240)
ActorKind49Wait:
    jsl.l Kind49Wait
    rts

; @routine 83:9310 ActorKind87Wait
; WHAT IT DOES: The routine actors of kind $87 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: KindWaitF991.
; HOW: JSL KindWaitF991 ($83:F991), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as KindWaitF991 leaves them.
%at($839310)
ActorKind87Wait:
    jsl.l KindWaitF991
    rts

; @routine 83:939A ActorKind94Wait
; WHAT IT DOES: The routine actors of kind $94 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind94Wait.
; HOW: JSL Kind94Wait ($83:FB15), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind94Wait leaves them.
%at($83939A)
ActorKind94Wait:
    jsl.l Kind94Wait
    rts

; @routine 83:92AC ActorKind7EWait
; WHAT IT DOES: The routine actors of kind $7E run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind7EWaitWork.
; HOW: JSL Kind7EWaitWork ($84:931C), then RTS; its C makes the JSL's
; return frame (jsl_frame), which the work may drop when it ends the
; actor.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind7EWaitWork leaves them.
%at($8392AC)
ActorKind7EWait:
    jsl.l Kind7EWaitWork
    rts

; @routine 83:92C6 ActorKind81Wait
; WHAT IT DOES: The routine actors of kind $81 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind81WaitWork.
; HOW: JSL Kind81WaitWork ($84:951C), then RTS; its C makes the JSL's
; return frame (jsl_frame), which the work may drop when it ends the
; actor.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind81WaitWork leaves them.
%at($8392C6)
ActorKind81Wait:
    jsl.l Kind81WaitWork
    rts

; @routine 83:92E9 ActorKind84Wait
; WHAT IT DOES: The routine actors of kind $84 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind84WaitWork.
; HOW: JSL Kind84WaitWork ($84:95FD), then RTS; its C makes the JSL's
; return frame (jsl_frame), which the work may drop when it ends the
; actor.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind84WaitWork leaves them.
%at($8392E9)
ActorKind84Wait:
    jsl.l Kind84WaitWork
    rts

; @routine 83:933B ActorKind8CWait
; WHAT IT DOES: The routine actors of kind $8C run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind8CWaitWork.
; HOW: JSL Kind8CWaitWork ($84:99E7), then RTS; its C makes the JSL's
; return frame (jsl_frame), which the work may drop when it ends the
; actor.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind8CWaitWork leaves them.
%at($83933B)
ActorKind8CWait:
    jsl.l Kind8CWaitWork
    rts

; @routine 83:93B9 ActorKind97Wait
; WHAT IT DOES: The routine actors of kind $97 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind97WaitWork.
; HOW: JSL Kind97WaitWork ($84:9FFB), then RTS; its C makes the JSL's
; return frame (jsl_frame), which the work may drop when it ends the
; actor.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind97WaitWork leaves them.
%at($8393B9)
ActorKind97Wait:
    jsl.l Kind97WaitWork
    rts

; @routine 83:93C2 ActorKind98Wait
; WHAT IT DOES: The routine actors of kind $98 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind98WaitWork.
; HOW: JSL Kind98WaitWork ($84:A39C), then RTS; its C makes the JSL's
; return frame (jsl_frame).
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind98WaitWork leaves them.
%at($8393C2)
ActorKind98Wait:
    jsl.l Kind98WaitWork
    rts

; @routine 83:93CF ActorKind99Wait
; WHAT IT DOES: The routine actors of kind $99 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind99WaitWork.
; HOW: JSL Kind99WaitWork ($84:A438), then RTS; its C makes the JSL's
; return frame (jsl_frame), which the work may drop when it ends the
; actor.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind99WaitWork leaves them.
%at($8393CF)
ActorKind99Wait:
    jsl.l Kind99WaitWork
    rts

; @routine 83:93DC ActorKind9AWait
; WHAT IT DOES: The routine actors of kind $9A run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind9AWaitWork.
; HOW: JSL Kind9AWaitWork ($84:A4E3), then RTS; its C makes the JSL's
; return frame (jsl_frame), which the work may drop when it ends the
; actor.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind9AWaitWork leaves them.
%at($8393DC)
ActorKind9AWait:
    jsl.l Kind9AWaitWork
    rts

; @routine 83:93FB ActorKind9EWait
; WHAT IT DOES: The routine actors of kind $9E run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind9EWaitWork.
; HOW: JSL Kind9EWaitWork ($84:A8BD), then RTS; its C makes the JSL's
; return frame (jsl_frame), which the work may drop when it ends the
; actor.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind9EWaitWork leaves them.
%at($8393FB)
ActorKind9EWait:
    jsl.l Kind9EWaitWork
    rts

; @routine 83:9430 ActorKindA4Wait
; WHAT IT DOES: The routine actors of kind $A4 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: KindA4WaitWork.
; HOW: JSL KindA4WaitWork ($84:AB97), then RTS; its C makes the JSL's
; return frame (jsl_frame), which the work may drop when it ends the
; actor.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as KindA4WaitWork leaves them.
%at($839430)
ActorKindA4Wait:
    jsl.l KindA4WaitWork
    rts

; @routine 83:9439 ActorKindA5Wait
; WHAT IT DOES: The routine actors of kind $A5 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: KindA5WaitWork.
; HOW: JSL KindA5WaitWork ($84:AC31), then RTS; its C makes the JSL's
; return frame (jsl_frame), which the work may drop when it ends the
; actor.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as KindA5WaitWork leaves them.
%at($839439)
ActorKindA5Wait:
    jsl.l KindA5WaitWork
    rts
