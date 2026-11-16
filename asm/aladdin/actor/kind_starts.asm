; kind_starts.asm: where the actor loop starts each kind of actor. In a
; record's start state the loop jumps through a table by kind ($83:8CAB
; for kinds below $80, $83:8DAB for the others); with bit 7 of the state
; set, through a second pair ($83:8DFD, $83:8EFD). Each entry is a JML to
; the kind's code in its own bank (docs/aladdin/ACTORS.md).
; C: src/aladdin/actor/kind_starts.c.

incsrc "../include/macros.inc"

arch 65816

; @routine 83:920E Kind10Start
; @actor
; WHAT IT DOES: Where actors of kinds $10 and $14-$17 go on from the actor loop in
; its start state (the table at $83:8CAB): ShotFly.
; HOW: JML to ShotFly ($81:EABA).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as ShotFly.
%at($83920E)
Kind10Start:
    jml.l ShotFly

; @routine 83:9212 Kind10Second
; @actor
; WHAT IT DOES: Where actors of kind $10 go on from the actor loop in
; a state with bit 7 set (the table at $83:8DFD): ShotHit.
; HOW: JML to ShotHit ($81:EB44).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as ShotHit.
%at($839212)
Kind10Second:
    jml.l ShotHit

; @routine 83:9216 Kind11Start
; @actor
; WHAT IT DOES: Where actors of kind $11 go on from the actor loop in
; its start state (the table at $83:8CAB): ActorBriefAnim8.
; HOW: JML to ActorBriefAnim8 ($81:EB90).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as ActorBriefAnim8.
%at($839216)
Kind11Start:
    jml.l ActorBriefAnim8

; @routine 83:921A Kind12Start
; @actor
; WHAT IT DOES: Where actors of kind $12 go on from the actor loop in
; its start state (the table at $83:8CAB): Kind12AtPlayer.
; HOW: JML to Kind12AtPlayer ($81:EBDE).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind12AtPlayer.
%at($83921A)
Kind12Start:
    jml.l Kind12AtPlayer

; @routine 83:9222 Kind13Start
; @actor
; WHAT IT DOES: Where actors of kind $13 go on from the actor loop in
; its start state (the table at $83:8CAB): ActorBriefAnimB.
; HOW: JML to ActorBriefAnimB ($81:EBB2).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as ActorBriefAnimB.
%at($839222)
Kind13Start:
    jml.l ActorBriefAnimB

; @routine 83:928E Kind7CStart
; @actor
; WHAT IT DOES: Where actors of kind $7C go on from the actor loop in
; its start state (the table at $83:8CAB): BackdropMover.
; HOW: JML to BackdropMover ($84:9044).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as BackdropMover.
%at($83928E)
Kind7CStart:
    jml.l BackdropMover

; @routine 83:929B Kind7DStart
; @actor
; WHAT IT DOES: Where actors of kind $7D go on from the actor loop in
; its start state (the table at $83:8CAB): Kind7DKeepSpot.
; HOW: JML to Kind7DKeepSpot ($84:90CE).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind7DKeepSpot.
%at($83929B)
Kind7DStart:
    jml.l Kind7DKeepSpot

; @routine 83:92B5 Kind7FStart
; @actor
; WHAT IT DOES: Where actors of kind $7F go on from the actor loop in
; its start state (the table at $83:8CAB): Kind7FDrift.
; HOW: JML to Kind7FDrift ($84:934B).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind7FDrift.
%at($8392B5)
Kind7FStart:
    jml.l Kind7FDrift

; @routine 83:92CF Kind82Start
; @actor
; WHAT IT DOES: Where actors of kind $82 go on from the actor loop in
; its start state (the table at $83:8DAB): Kind82Show.
; HOW: JML to Kind82Show ($84:951D).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind82Show.
%at($8392CF)
Kind82Start:
    jml.l Kind82Show

; @routine 83:9319 Kind88Start
; @actor
; WHAT IT DOES: Where actors of kind $88 go on from the actor loop in
; its start state (the table at $83:8DAB): RiserActor.
; HOW: JML to RiserActor ($84:8E6A).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as RiserActor.
%at($839319)
Kind88Start:
    jml.l RiserActor

; @routine 83:9326 Kind89Start
; @actor
; WHAT IT DOES: Where actors of kind $89 go on from the actor loop in
; its start state (the table at $83:8DAB): Kind89PlayerPriority.
; HOW: JML to Kind89PlayerPriority ($84:8F9C).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind89PlayerPriority.
%at($839326)
Kind89Start:
    jml.l Kind89PlayerPriority

; @routine 83:9355 Kind8FStart
; @actor
; WHAT IT DOES: Where actors of kind $8F go on from the actor loop in
; its start state (the table at $83:8DAB): Kind8FWalk.
; HOW: JML to Kind8FWalk ($83:FA4F).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind8FWalk.
%at($839355)
Kind8FStart:
    jml.l Kind8FWalk

; @routine 83:9389 Kind93Start
; @actor
; WHAT IT DOES: Where actors of kind $93 go on from the actor loop in
; its start state (the table at $83:8DAB): Kind93WalkIn.
; HOW: JML to Kind93WalkIn ($A5:F3F0).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind93WalkIn.
%at($839389)
Kind93Start:
    jml.l Kind93WalkIn

; @routine 83:9396 Kind94Start
; @actor
; WHAT IT DOES: Where actors of kind $94 go on from the actor loop in
; its start state (the table at $83:8DAB): Kind94Spark.
; HOW: JML to Kind94Spark ($83:FABB).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind94Spark.
%at($839396)
Kind94Start:
    jml.l Kind94Spark

; @routine 83:93AC Kind96Start
; @actor
; WHAT IT DOES: Where actors of kind $96 go on from the actor loop in
; its start state (the table at $83:8DAB): SteppedShowActor.
; HOW: JML to SteppedShowActor ($84:9EF5).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as SteppedShowActor.
%at($8393AC)
Kind96Start:
    jml.l SteppedShowActor

; @routine 83:9412 KindA1Second
; @actor
; WHAT IT DOES: Where actors of kind $A1 go on from the actor loop in
; a state with bit 7 set (the table at $83:8EFD): KindA1Hit.
; HOW: JML to KindA1Hit ($84:A9FC).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as KindA1Hit.
%at($839412)
KindA1Second:
    jml.l KindA1Hit

; @routine 83:9416 KindA2Start
; @actor
; WHAT IT DOES: Where actors of kind $A2 go on from the actor loop in
; its start state (the table at $83:8DAB): KindA2Sent.
; HOW: JML to KindA2Sent ($84:8249).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as KindA2Sent.
%at($839416)
KindA2Start:
    jml.l KindA2Sent

; @routine 83:941F KindA2Second
; @actor
; WHAT IT DOES: Where actors of kind $A2 go on from the actor loop in
; a state with bit 7 set (the table at $83:8EFD): KindA2Hit.
; HOW: JML to KindA2Hit ($84:82F5).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as KindA2Hit.
%at($83941F)
KindA2Second:
    jml.l KindA2Hit

; @routine 83:9423 KindA3Start
; @actor
; WHAT IT DOES: Where actors of kind $A3 go on from the actor loop in
; its start state (the table at $83:8DAB): KindA3Shot.
; HOW: JML to KindA3Shot ($84:8349).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as KindA3Shot.
%at($839423)
KindA3Start:
    jml.l KindA3Shot

; @routine 83:943E KindA6Start
; @actor
; WHAT IT DOES: Where actors of kind $A6 go on from the actor loop in
; its start state (the table at $83:8DAB): KindA6Stay.
; HOW: JML to KindA6Stay ($84:AC3F).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as KindA6Stay.
%at($83943E)
KindA6Start:
    jml.l KindA6Stay
