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

; @routine 83:9226 Kind21Enter
; @actor
; WHAT IT DOES: Where actors of kind $21 go on from the actor loop in
; its start state (the table at $83:8CAB): Kind21Start.
; HOW: JML to Kind21Start ($A5:F309).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind21Start.
%at($839226)
Kind21Enter:
    jml.l Kind21Start

; @routine 83:9233 Kind24Enter
; @actor
; WHAT IT DOES: Where actors of kind $24 go on from the actor loop in
; its start state (the table at $83:8CAB): Kind24Start.
; HOW: JML to Kind24Start ($84:8000).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind24Start.
%at($839233)
Kind24Enter:
    jml.l Kind24Start

; @routine 83:923C Kind49Enter
; @actor
; WHAT IT DOES: Where actors of kind $49 go on from the actor loop in
; its start state (the table at $83:8CAB): Kind49Carry.
; HOW: JML to Kind49Carry ($84:804D).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind49Carry.
%at($83923C)
Kind49Enter:
    jml.l Kind49Carry

; @routine 83:9249 Kind63Enter
; @actor
; WHAT IT DOES: Where actors of kind $63 go on from the actor loop in
; its start state (the table at $83:8CAB): Kind63Start.
; HOW: JML to Kind63Start ($84:8080).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind63Start.
%at($839249)
Kind63Enter:
    jml.l Kind63Start

; @routine 83:9256 Kind63Third
; @actor
; WHAT IT DOES: Where actors of kind $63 go on from the actor loop in
; a state with bits 7 and 6 set (the table at $83:90A1): Kind63ShotHit.
; HOW: JML to Kind63ShotHit ($84:81CD).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind63ShotHit.
%at($839256)
Kind63Third:
    jml.l Kind63ShotHit

; @routine 83:928A Kind76Second
; @actor
; WHAT IT DOES: Where actors of kind $76 go on from the actor loop in
; a state with bit 7 set (the table at $83:8DFD): BackdropMover.
; HOW: JML to BackdropMover ($84:9044).
; No script runs it (no script reaches this entry of the table).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as BackdropMover.
%at($83928A)
Kind76Second:
    jml.l BackdropMover

; @routine 83:9297 Kind7CSecond
; @actor
; WHAT IT DOES: Where actors of kind $7C go on from the actor loop in
; a state with bit 7 set (the table at $83:8DFD): Kind7DKeepSpot.
; HOW: JML to Kind7DKeepSpot ($84:90CE).
; No script runs it (no script reaches this entry of the table).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind7DKeepSpot.
%at($839297)
Kind7CSecond:
    jml.l Kind7DKeepSpot

; @routine 83:92B1 Kind7ESecond
; @actor
; WHAT IT DOES: Where actors of kind $7E go on from the actor loop in
; a state with bit 7 set (the table at $83:8DFD): Kind7FDrift.
; HOW: JML to Kind7FDrift ($84:934B).
; No script runs it (no script reaches this entry of the table).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind7FDrift.
%at($8392B1)
Kind7ESecond:
    jml.l Kind7FDrift

; @routine 83:92C2 Kind81Enter
; @actor
; WHAT IT DOES: Where actors of kind $81 go on from the actor loop in
; its start state (the table at $83:8DAB): Kind81Scene.
; HOW: JML to Kind81Scene ($84:9391).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind81Scene.
%at($8392C2)
Kind81Enter:
    jml.l Kind81Scene

; @routine 83:92CB Kind81Second
; @actor
; WHAT IT DOES: Where actors of kind $81 go on from the actor loop in
; a state with bit 7 set (the table at $83:8EFD): Kind82Show.
; HOW: JML to Kind82Show ($84:951D).
; No script runs it (no script reaches this entry of the table).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind82Show.
%at($8392CB)
Kind81Second:
    jml.l Kind82Show

; @routine 83:92E1 Kind83Second
; @actor
; WHAT IT DOES: Where actors of kind $83 go on from the actor loop in
; a state with bit 7 set (the table at $83:8EFD): ActorWaitGrabsPlayer.
; HOW: JML to ActorWaitGrabsPlayer ($84:95D4).
; No script runs it (no script reaches this entry of the table).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: as ActorWaitGrabsPlayer leaves them: its RTL here pulls
; three bytes of the actor loop's own frame (its saved D, then its return).
%at($8392E1)
Kind83Second:
    jml.l ActorWaitGrabsPlayer

; @routine 83:92FB Kind85Second
; @actor
; WHAT IT DOES: Where actors of kind $85 go on from the actor loop in
; a state with bit 7 set (the table at $83:8EFD): KindAnimate9628.
; HOW: JML to KindAnimate9628 ($84:9628).
; No script runs it (no script reaches this entry of the table).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: as KindAnimate9628 leaves them: its RTL here pulls
; three bytes of the actor loop's own frame (its saved D, then its return).
%at($8392FB)
Kind85Second:
    jml.l KindAnimate9628

; @routine 83:9315 Kind87Second
; @actor
; WHAT IT DOES: Where actors of kind $87 go on from the actor loop in
; a state with bit 7 set (the table at $83:8EFD): LandingPad.
; HOW: JML to LandingPad ($83:F9BA).
; No script runs it (no script reaches this entry of the table).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as LandingPad.
%at($839315)
Kind87Second:
    jml.l LandingPad

; @routine 83:9322 Kind88Second
; @actor
; WHAT IT DOES: Where actors of kind $88 go on from the actor loop in
; a state with bit 7 set (the table at $83:8EFD): Kind89PlayerPriority.
; HOW: JML to Kind89PlayerPriority ($84:8F9C).
; No script runs it (no script reaches this entry of the table).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind89PlayerPriority.
%at($839322)
Kind88Second:
    jml.l Kind89PlayerPriority

; @routine 83:9344 Kind8DEnter
; @actor
; WHAT IT DOES: Where actors of kind $8D go on from the actor loop in
; its start state (the table at $83:8DAB): Kind8DCircle.
; HOW: JML to Kind8DCircle ($84:99ED).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind8DCircle.
%at($839344)
Kind8DEnter:
    jml.l Kind8DCircle

; @routine 83:936B Kind90Second
; @actor
; WHAT IT DOES: Where actors of kind $90 go on from the actor loop in
; a state with bit 7 set (the table at $83:8EFD): LongReturn.
; HOW: JML to LongReturn ($A5:F533).
; No script runs it (no script reaches this entry of the table).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: as LongReturn leaves them: its RTL here pulls
; three bytes of the actor loop's own frame (its saved D, then its return).
%at($83936B)
Kind90Second:
    jml.l LongReturn

; @routine 83:9378 Kind91Second
; @actor
; WHAT IT DOES: Where actors of kind $91 go on from the actor loop in
; a state with bit 7 set (the table at $83:8EFD): Kind91FreeOff.
; HOW: JML to Kind91FreeOff ($A5:F603).
; No script runs it (no script reaches this entry of the table).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: as Kind91FreeOff leaves them: its RTL here pulls
; three bytes of the actor loop's own frame (its saved D, then its return).
%at($839378)
Kind91Second:
    jml.l Kind91FreeOff

; @routine 83:9385 Kind92Second
; @actor
; WHAT IT DOES: Where actors of kind $92 go on from the actor loop in
; a state with bit 7 set (the table at $83:8EFD): Kind91FreeOff.
; HOW: JML to Kind91FreeOff ($A5:F603).
; No script runs it (no script reaches this entry of the table).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: as Kind91FreeOff leaves them: its RTL here pulls
; three bytes of the actor loop's own frame (its saved D, then its return).
%at($839385)
Kind92Second:
    jml.l Kind91FreeOff

; @routine 83:9392 Kind93Second
; @actor
; WHAT IT DOES: Where actors of kind $93 go on from the actor loop in
; a state with bit 7 set (the table at $83:8EFD): Kind93AnimateSend.
; HOW: JML to Kind93AnimateSend ($A5:F434).
; No script runs it (no script reaches this entry of the table).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: as Kind93AnimateSend leaves them: its RTL here pulls
; three bytes of the actor loop's own frame (its saved D, then its return).
%at($839392)
Kind93Second:
    jml.l Kind93AnimateSend

; @routine 83:93E5 Kind9BEnter
; @actor
; WHAT IT DOES: Where actors of kind $9B go on from the actor loop in
; its start state (the table at $83:8DAB): Kind9BStory.
; HOW: JML to Kind9BStory ($84:A509).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind9BStory.
%at($8393E5)
Kind9BEnter:
    jml.l Kind9BStory

; @routine 83:93EE Kind9CEnter
; @actor
; WHAT IT DOES: Where actors of kind $9C go on from the actor loop in
; its start state (the table at $83:8DAB): Kind9CStart.
; HOW: JML to Kind9CStart ($84:A7E5).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind9CStart.
%at($8393EE)
Kind9CEnter:
    jml.l Kind9CStart

; @routine 83:93F7 Kind9EEnter
; @actor
; WHAT IT DOES: Where actors of kind $9E go on from the actor loop in
; its start state (the table at $83:8DAB): Kind9EStart.
; HOW: JML to Kind9EStart ($84:A820).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind9EStart.
%at($8393F7)
Kind9EEnter:
    jml.l Kind9EStart

; @routine 83:9281 Kind76Enter
; @actor
; WHAT IT DOES: Where actors of kind $76 go on from the actor loop in
; its start state (the table at $83:8CAB): Kind76Start.
; HOW: JML to Kind76Start ($84:8FBA).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind76Start.
%at($839281)
Kind76Enter:
    jml.l Kind76Start

; @routine 83:91F4 Kind00Start
; @actor
; WHAT IT DOES: Where actors of kinds $00-$03 and $07 (the player) go on from the actor loop
; in its start state (the table at $83:8CAB): PlayerStart.
; HOW: JML to PlayerStart ($81:CB3B).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as PlayerStart.
%at($8391F4)
Kind00Start:
    jml.l PlayerStart

; @routine 83:91FD Kind00Second
; @actor
; WHAT IT DOES: Where actors of kinds $00, $02, $03 and $07 (the player) go on from the actor loop
; in a state with bit 7 set (the table at $83:8DFD): PlayerKnockback.
; HOW: JML to PlayerKnockback ($81:DADD).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as PlayerKnockback.
%at($8391FD)
Kind00Second:
    jml.l PlayerKnockback

; @routine 83:9201 Kind01Second
; @actor
; WHAT IT DOES: Where actors of kind $01 (the player) go on from the actor loop
; in a state with bit 7 set (the table at $83:8DFD): PlayerCarpetCrash.
; HOW: JML to PlayerCarpetCrash ($81:E7E9).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as PlayerCarpetCrash.
%at($839201)
Kind01Second:
    jml.l PlayerCarpetCrash

; @routine 83:92D8 Kind83Enter
; @actor
; WHAT IT DOES: Where actors of kind $83 go on from the actor loop
; in its start state (the table at $83:8DAB): Kind83Start.
; HOW: JML to Kind83Start ($84:9538).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind83Start.
%at($8392D8)
Kind83Enter:
    jml.l Kind83Start

; @routine 83:93A3 Kind95Enter
; @actor
; WHAT IT DOES: Where actors of kind $95 go on from the actor loop
; in its start state (the table at $83:8DAB): Kind95Start.
; HOW: JML to Kind95Start ($84:9BE3).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind95Start.
%at($8393A3)
Kind95Enter:
    jml.l Kind95Start

; @routine 83:9362 Kind90Enter
; @actor
; WHAT IT DOES: Where actors of kind $90 go on from the actor loop in
; its start state (the table at $83:8DAB): Kind90Start.
; HOW: JML to Kind90Start ($A5:F4D2).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind90Start.
%at($839362)
Kind90Enter:
    jml.l Kind90Start

; @routine 83:9409 KindA1Enter
; @actor
; WHAT IT DOES: Where actors of kind $A1 go on from the actor loop in
; its start state (the table at $83:8DAB): KindA1Start.
; HOW: JML to KindA1Start ($84:A97F).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as KindA1Start.
%at($839409)
KindA1Enter:
    jml.l KindA1Start

; @routine 83:9446 KindA8Enter
; @actor
; WHAT IT DOES: Where actors of kind $A8 go on from the actor loop in
; its start state (the table at $83:8DAB): KindA8Start.
; HOW: JML to KindA8Start ($83:FB1A).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as KindA8Start.
%at($839446)
KindA8Enter:
    jml.l KindA8Start

; @routine 83:939F Kind94Second
; @actor
; WHAT IT DOES: Where actors of kind $94 go on from the actor loop in
; a state with bit 7 set (the table at $83:8EFD): KindA8Start.
; HOW: JML to KindA8Start ($83:FB1A).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as KindA8Start.
%at($83939F)
Kind94Second:
    jml.l KindA8Start

; @routine 83:9252 Kind63Second
; @actor
; WHAT IT DOES: Where actors of kind $63 go on from the actor loop in
; a state with bit 7 set (the table at $83:8DFD): Kind63Knocked.
; HOW: JML to Kind63Knocked ($84:81BE).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind63Knocked.
%at($839252)
Kind63Second:
    jml.l Kind63Knocked

; @routine 83:92F2 Kind85Enter
; @actor
; WHAT IT DOES: Where actors of kind $85 go on from the actor loop in
; its start state (the table at $83:8DAB): Kind85Start.
; HOW: JML to Kind85Start ($84:9602).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind85Start.
%at($8392F2)
Kind85Enter:
    jml.l Kind85Start

; @routine 83:9351 Kind8EEnter
; @actor
; WHAT IT DOES: Where actors of kind $8E go on from the actor loop in
; its start state (the table at $83:8DAB): Kind8EStart.
; HOW: JML to Kind8EStart ($84:9B6B).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind8EStart.
%at($839351)
Kind8EEnter:
    jml.l Kind8EStart

; @routine 83:925A Kind73Enter
; @actor
; WHAT IT DOES: Where actors of kind $73 go on from the actor loop in
; its start state (the table at $83:8CAB): Kind73Start.
; HOW: JML to Kind73Start ($84:8629).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind73Start.
%at($83925A)
Kind73Enter:
    jml.l Kind73Start

; @routine 83:9263 Kind73Second
; @actor
; WHAT IT DOES: Where actors of kind $73 go on from the actor loop in
; a state with bit 7 set (the table at $83:8DFD): Kind73Hit.
; HOW: JML to Kind73Hit ($84:8766).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind73Hit.
%at($839263)
Kind73Second:
    jml.l Kind73Hit

; @routine 83:9270 Kind74Second
; @actor
; WHAT IT DOES: Where actors of kind $74 go on from the actor loop in
; a state with bit 7 set (the table at $83:8DFD): ChainPieceGone.
; HOW: JML to ChainPieceGone ($84:8AC2).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as ChainPieceGone.
%at($839270)
Kind74Second:
    jml.l ChainPieceGone

; @routine 83:927D Kind75Second
; @actor
; WHAT IT DOES: Where actors of kind $75 go on from the actor loop in
; a state with bit 7 set (the table at $83:8DFD): ChainPieceGone.
; HOW: JML to ChainPieceGone ($84:8AC2).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as ChainPieceGone.
%at($83927D)
Kind75Second:
    jml.l ChainPieceGone

; @routine 83:92A4 Kind7DSecond
; @actor
; WHAT IT DOES: Where actors of kind $7D go on from the actor loop in
; a state with bit 7 set (the table at $83:8DFD): Kind7EStart.
; HOW: JML to Kind7EStart ($84:91A5).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: as Kind7EStart.
%at($8392A4)
Kind7DSecond:
    jml.l Kind7EStart

; @routine 83:92EE Kind84Second
; @actor
; WHAT IT DOES: Where actors of kind $84 go on from the actor loop in
; a state with bit 7 set (the table at $83:8EFD): Kind84WaitWork.
; HOW: JML to Kind84WaitWork ($84:95FD).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: as Kind84WaitWork.
%at($8392EE)
Kind84Second:
    jml.l Kind84WaitWork

; @routine 83:944A Kind66Enter
; @actor
; WHAT IT DOES: Where actors of kind $66 go on from the actor loop in
; state $0C (the table at $83:8CAB): Kind66Start.
; HOW: JML to Kind66Start ($84:839E).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind66Start.
%at($83944A)
Kind66Enter:
    jml.l Kind66Start

; @routine 83:944E ActorKind66Wait
; WHAT IT DOES: What actors of kind $66 do each frame they wait, when bit
; 7 of their byte at $09 is set: Kind66WaitWork.
; HOW: JSL Kind66WaitWork ($84:85DC), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor loop's wait table. Exit: as Kind66WaitWork.
%at($83944E)
ActorKind66Wait:
    jsl.l Kind66WaitWork
    rts

; @routine 83:9453 Kind66Second
; @actor
; WHAT IT DOES: Where actors of kind $66 go on from the actor loop in
; a state with bit 7 set (the table at $83:8DFD): Kind73Start.
; HOW: JML to Kind73Start ($84:8629).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: as Kind73Start.
%at($839453)
Kind66Second:
    jml.l Kind73Start

; @routine 83:942C KindA4Enter
; @actor
; WHAT IT DOES: Where actors of kind $A4 go on from the actor loop in
; its start state (the table at $83:8DAB): KindA4Start.
; HOW: JML to KindA4Start ($84:AA24).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as KindA4Start.
%at($83942C)
KindA4Enter:
    jml.l KindA4Start

; @routine 83:92FF Kind86Enter
; @actor
; WHAT IT DOES: Where actors of kind $86 go on from the actor loop in
; its start state (the table at $83:8DAB): Kind86Start.
; HOW: JML to Kind86Start ($84:962D).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind86Start.
%at($8392FF)
Kind86Enter:
    jml.l Kind86Start

; @routine 83:9308 Kind86Second
; @actor
; WHAT IT DOES: Where actors of kind $86 go on from the actor loop in
; a state with bit 7 set (the table at $83:8EFD): Kind86End.
; HOW: JML to Kind86End ($84:980F).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: as Kind86End.
%at($839308)
Kind86Second:
    jml.l Kind86End

; @routine 83:930C Kind87Enter
; @actor
; WHAT IT DOES: Where actors of kind $87 go on from the actor loop in
; its start state (the table at $83:8DAB): Kind87Start.
; HOW: JML to Kind87Start ($83:F77F).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind87Start.
%at($83930C)
Kind87Enter:
    jml.l Kind87Start

; @routine 83:936F Kind91Enter
; @actor
; WHAT IT DOES: Where actors of kind $91 go on from the actor loop in
; its start state (the table at $83:8DAB): Kind91Start.
; HOW: JML to Kind91Start ($A5:F534).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind91Start.
%at($83936F)
Kind91Enter:
    jml.l Kind91Start

; @routine 83:9442 KindA7Enter
; @actor
; WHAT IT DOES: Where actors of kind $A7 go on from the actor loop in
; its start state (the table at $83:8DAB): KindA7Start.
; HOW: JML to KindA7Start ($84:AC66).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as KindA7Start.
%at($839442)
KindA7Enter:
    jml.l KindA7Start

; @routine 83:9267 Kind74Enter
; @actor
; WHAT IT DOES: Where actors of kind $74 go on from the actor loop in
; its start state (the table at $83:8CAB): Kind74Start.
; HOW: JML to Kind74Start ($84:87C2).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind74Start.
%at($839267)
Kind74Enter:
    jml.l Kind74Start

; @routine 83:9274 Kind75Enter
; @actor
; WHAT IT DOES: Where actors of kind $75 go on from the actor loop in
; its start state (the table at $83:8CAB): Kind75Start.
; HOW: JML to Kind75Start ($84:87D1).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind75Start.
%at($839274)
Kind75Enter:
    jml.l Kind75Start

; @routine 83:9400 KindA0Enter
; @actor
; WHAT IT DOES: Where actors of kind $A0 go on from the actor loop in
; its start state (the table at $83:8DAB): KindA0Start.
; HOW: JML to KindA0Start ($84:A8E0).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as KindA0Start.
%at($839400)
KindA0Enter:
    jml.l KindA0Start

; @routine 83:92E5 Kind84Enter
; @actor
; WHAT IT DOES: Where actors of kind $84 go on from the actor loop in
; its start state (the table at $83:8DAB): Kind84Start.
; HOW: JML to Kind84Start ($84:95E3).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind84Start.
%at($8392E5)
Kind84Enter:
    jml.l Kind84Start

; @routine 83:93BE Kind98Enter
; @actor
; WHAT IT DOES: Where actors of kind $98 go on from the actor loop in
; its start state (the table at $83:8DAB): Kind98Start.
; HOW: JML to Kind98Start ($84:A112).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind98Start.
%at($8393BE)
Kind98Enter:
    jml.l Kind98Start

; @routine 83:93C7 Kind98Second
; @actor
; WHAT IT DOES: Where actors of kind $98 go on from the actor loop in
; a state with bit 7 set (the table at $83:8EFD): Kind98Hit.
; HOW: JML to Kind98Hit ($84:A2A6).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind98Hit.
%at($8393C7)
Kind98Second:
    jml.l Kind98Hit

; @routine 83:93CB Kind99Enter
; @actor
; WHAT IT DOES: Where actors of kind $99 go on from the actor loop in
; its start state (the table at $83:8DAB): Kind99Start.
; HOW: JML to Kind99Start ($84:A3B7).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind99Start.
%at($8393CB)
Kind99Enter:
    jml.l Kind99Start

; @routine 83:93D8 Kind9AEnter
; @actor
; WHAT IT DOES: Where actors of kind $9A go on from the actor loop in
; its start state (the table at $83:8DAB): Kind9AStart.
; HOW: JML to Kind9AStart ($84:A446).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind9AStart.
%at($8393D8)
Kind9AEnter:
    jml.l Kind9AStart

; @routine 83:9435 KindA5Enter
; @actor
; WHAT IT DOES: Where actors of kind $A5 go on from the actor loop in
; its start state (the table at $83:8DAB): KindA5Start.
; HOW: JML to KindA5Start ($84:AB9C).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as KindA5Start.
%at($839435)
KindA5Enter:
    jml.l KindA5Start

; @routine 83:9245 Kind4CEnter
; @actor
; WHAT IT DOES: Where actors of kind $4C go on from the actor loop in
; its start state (the table at $83:8CAB): Kind4CStart.
; HOW: JML to Kind4CStart ($A5:F43D).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as Kind4CStart.
%at($839245)
Kind4CEnter:
    jml.l Kind4CStart

; @routine 83:932A Kind8BEnter
; @actor
; WHAT IT DOES: Where actors of kind $8B go on from the actor loop in
; its start state (the table at $83:8DAB): Kind8BStart.
; HOW: JML to Kind8BStart ($84:9813).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as Kind8BStart.
%at($83932A)
Kind8BEnter:
    jml.l Kind8BStart

; @routine 83:9337 Kind8CEnter
; @actor
; WHAT IT DOES: Where actors of kind $8C go on from the actor loop in
; its start state (the table at $83:8DAB): Kind8CStart.
; HOW: JML to Kind8CStart ($84:9979).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as Kind8CStart.
%at($839337)
Kind8CEnter:
    jml.l Kind8CStart

; @routine 83:937C Kind92Enter
; @actor
; WHAT IT DOES: Where actors of kind $92 go on from the actor loop in
; its start state (the table at $83:8DAB): Kind92Start.
; HOW: JML to Kind92Start ($A5:F5BA).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind.
; Exit: never returns: as Kind92Start.
%at($83937C)
Kind92Enter:
    jml.l Kind92Start
