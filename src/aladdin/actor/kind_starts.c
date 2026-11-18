/*
 * kind_starts.c: where the actor loop starts each kind of actor. In a
 * record's start state the loop jumps through a table by kind ($83:8CAB
 * for kinds below $80, $83:8DAB for the others); with bit 7 of the state
 * set, through a second pair ($83:8DFD, $83:8EFD). Each entry is a JML to
 * the kind's code in its own bank (docs/aladdin/ACTORS.md).
 */
#include "machine.h"
#include "ops65.h"

void abi_ShotFly(void);
void abi_KindA0Start(void);
void abi_Kind75Start(void);
void abi_Kind74Start(void);
void abi_KindA7Start(void);
void abi_Kind63Knocked(void);
void abi_Kind85Start(void);
void abi_Kind8EStart(void);
void abi_Kind90Start(void);
void abi_KindA1Start(void);
void abi_KindA8Start(void);
void abi_ShotHit(void);
void abi_ActorBriefAnim8(void);
void abi_Kind12AtPlayer(void);
void abi_ActorBriefAnimB(void);
void abi_BackdropMover(void);
void abi_Kind7DKeepSpot(void);
void abi_Kind7FDrift(void);
void abi_Kind82Show(void);
void abi_RiserActor(void);
void abi_Kind89PlayerPriority(void);
void abi_Kind8FWalk(void);
void abi_Kind93WalkIn(void);
void abi_Kind94Spark(void);
void abi_SteppedShowActor(void);
void abi_KindA1Hit(void);
void abi_KindA2Sent(void);
void abi_KindA2Hit(void);
void abi_KindA3Shot(void);
void abi_KindA6Stay(void);
void abi_ActorWaitGrabsPlayer(void);
void abi_Kind21Start(void);
void abi_Kind24Start(void);
void abi_Kind49Carry(void);
void abi_Kind63ShotHit(void);
void abi_Kind63Start(void);
void abi_Kind81Scene(void);
void abi_Kind8DCircle(void);
void abi_Kind91FreeOff(void);
void abi_Kind93AnimateSend(void);
void abi_Kind9BStory(void);
void abi_Kind9CStart(void);
void abi_KindAnimate9628(void);
void abi_LandingPad(void);
void abi_LongReturn(void);
void abi_Kind76Start(void);
void abi_PlayerStart(void);
void abi_PlayerKnockback(void);
void abi_PlayerCarpetCrash(void);
void abi_Kind83Start(void);
void abi_Kind95Start(void);
void abi_Kind87Start(void);
void abi_Kind91Start(void);

/*
 * @routine 83:920E Kind10Start
 * @actor
 * WHAT IT DOES: Where actors of kinds $10 and $14-$17 go on from the actor loop in
 * its start state (the table at $83:8CAB): ShotFly.
 * HOW: JML to ShotFly ($81:EABA).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as ShotFly.
 */
void abi_Kind10Start(void)
{
    abi_ShotFly();
}

/*
 * @routine 83:9212 Kind10Second
 * @actor
 * WHAT IT DOES: Where actors of kind $10 go on from the actor loop in
 * a state with bit 7 set (the table at $83:8DFD): ShotHit.
 * HOW: JML to ShotHit ($81:EB44).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as ShotHit.
 */
void abi_Kind10Second(void)
{
    abi_ShotHit();
}

/*
 * @routine 83:9216 Kind11Start
 * @actor
 * WHAT IT DOES: Where actors of kind $11 go on from the actor loop in
 * its start state (the table at $83:8CAB): ActorBriefAnim8.
 * HOW: JML to ActorBriefAnim8 ($81:EB90).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as ActorBriefAnim8.
 */
void abi_Kind11Start(void)
{
    abi_ActorBriefAnim8();
}

/*
 * @routine 83:921A Kind12Start
 * @actor
 * WHAT IT DOES: Where actors of kind $12 go on from the actor loop in
 * its start state (the table at $83:8CAB): Kind12AtPlayer.
 * HOW: JML to Kind12AtPlayer ($81:EBDE).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind12AtPlayer.
 */
void abi_Kind12Start(void)
{
    abi_Kind12AtPlayer();
}

/*
 * @routine 83:9222 Kind13Start
 * @actor
 * WHAT IT DOES: Where actors of kind $13 go on from the actor loop in
 * its start state (the table at $83:8CAB): ActorBriefAnimB.
 * HOW: JML to ActorBriefAnimB ($81:EBB2).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as ActorBriefAnimB.
 */
void abi_Kind13Start(void)
{
    abi_ActorBriefAnimB();
}

/*
 * @routine 83:928E Kind7CStart
 * @actor
 * WHAT IT DOES: Where actors of kind $7C go on from the actor loop in
 * its start state (the table at $83:8CAB): BackdropMover.
 * HOW: JML to BackdropMover ($84:9044).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as BackdropMover.
 */
void abi_Kind7CStart(void)
{
    abi_BackdropMover();
}

/*
 * @routine 83:929B Kind7DStart
 * @actor
 * WHAT IT DOES: Where actors of kind $7D go on from the actor loop in
 * its start state (the table at $83:8CAB): Kind7DKeepSpot.
 * HOW: JML to Kind7DKeepSpot ($84:90CE).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind7DKeepSpot.
 */
void abi_Kind7DStart(void)
{
    abi_Kind7DKeepSpot();
}

/*
 * @routine 83:92B5 Kind7FStart
 * @actor
 * WHAT IT DOES: Where actors of kind $7F go on from the actor loop in
 * its start state (the table at $83:8CAB): Kind7FDrift.
 * HOW: JML to Kind7FDrift ($84:934B).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind7FDrift.
 */
void abi_Kind7FStart(void)
{
    abi_Kind7FDrift();
}

/*
 * @routine 83:92CF Kind82Start
 * @actor
 * WHAT IT DOES: Where actors of kind $82 go on from the actor loop in
 * its start state (the table at $83:8DAB): Kind82Show.
 * HOW: JML to Kind82Show ($84:951D).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind82Show.
 */
void abi_Kind82Start(void)
{
    abi_Kind82Show();
}

/*
 * @routine 83:9319 Kind88Start
 * @actor
 * WHAT IT DOES: Where actors of kind $88 go on from the actor loop in
 * its start state (the table at $83:8DAB): RiserActor.
 * HOW: JML to RiserActor ($84:8E6A).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as RiserActor.
 */
void abi_Kind88Start(void)
{
    abi_RiserActor();
}

/*
 * @routine 83:9326 Kind89Start
 * @actor
 * WHAT IT DOES: Where actors of kind $89 go on from the actor loop in
 * its start state (the table at $83:8DAB): Kind89PlayerPriority.
 * HOW: JML to Kind89PlayerPriority ($84:8F9C).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind89PlayerPriority.
 */
void abi_Kind89Start(void)
{
    abi_Kind89PlayerPriority();
}

/*
 * @routine 83:9355 Kind8FStart
 * @actor
 * WHAT IT DOES: Where actors of kind $8F go on from the actor loop in
 * its start state (the table at $83:8DAB): Kind8FWalk.
 * HOW: JML to Kind8FWalk ($83:FA4F).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind8FWalk.
 */
void abi_Kind8FStart(void)
{
    abi_Kind8FWalk();
}

/*
 * @routine 83:9389 Kind93Start
 * @actor
 * WHAT IT DOES: Where actors of kind $93 go on from the actor loop in
 * its start state (the table at $83:8DAB): Kind93WalkIn.
 * HOW: JML to Kind93WalkIn ($A5:F3F0).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind93WalkIn.
 */
void abi_Kind93Start(void)
{
    abi_Kind93WalkIn();
}

/*
 * @routine 83:9396 Kind94Start
 * @actor
 * WHAT IT DOES: Where actors of kind $94 go on from the actor loop in
 * its start state (the table at $83:8DAB): Kind94Spark.
 * HOW: JML to Kind94Spark ($83:FABB).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind94Spark.
 */
void abi_Kind94Start(void)
{
    abi_Kind94Spark();
}

/*
 * @routine 83:93AC Kind96Start
 * @actor
 * WHAT IT DOES: Where actors of kind $96 go on from the actor loop in
 * its start state (the table at $83:8DAB): SteppedShowActor.
 * HOW: JML to SteppedShowActor ($84:9EF5).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as SteppedShowActor.
 */
void abi_Kind96Start(void)
{
    abi_SteppedShowActor();
}

/*
 * @routine 83:9412 KindA1Second
 * @actor
 * WHAT IT DOES: Where actors of kind $A1 go on from the actor loop in
 * a state with bit 7 set (the table at $83:8EFD): KindA1Hit.
 * HOW: JML to KindA1Hit ($84:A9FC).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as KindA1Hit.
 */
void abi_KindA1Second(void)
{
    abi_KindA1Hit();
}

/*
 * @routine 83:9416 KindA2Start
 * @actor
 * WHAT IT DOES: Where actors of kind $A2 go on from the actor loop in
 * its start state (the table at $83:8DAB): KindA2Sent.
 * HOW: JML to KindA2Sent ($84:8249).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as KindA2Sent.
 */
void abi_KindA2Start(void)
{
    abi_KindA2Sent();
}

/*
 * @routine 83:941F KindA2Second
 * @actor
 * WHAT IT DOES: Where actors of kind $A2 go on from the actor loop in
 * a state with bit 7 set (the table at $83:8EFD): KindA2Hit.
 * HOW: JML to KindA2Hit ($84:82F5).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as KindA2Hit.
 */
void abi_KindA2Second(void)
{
    abi_KindA2Hit();
}

/*
 * @routine 83:9423 KindA3Start
 * @actor
 * WHAT IT DOES: Where actors of kind $A3 go on from the actor loop in
 * its start state (the table at $83:8DAB): KindA3Shot.
 * HOW: JML to KindA3Shot ($84:8349).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as KindA3Shot.
 */
void abi_KindA3Start(void)
{
    abi_KindA3Shot();
}

/*
 * @routine 83:943E KindA6Start
 * @actor
 * WHAT IT DOES: Where actors of kind $A6 go on from the actor loop in
 * its start state (the table at $83:8DAB): KindA6Stay.
 * HOW: JML to KindA6Stay ($84:AC3F).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as KindA6Stay.
 */
void abi_KindA6Start(void)
{
    abi_KindA6Stay();
}

/*
 * @routine 83:9226 Kind21Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $21 go on from the actor loop in
 * its start state (the table at $83:8CAB): Kind21Start.
 * HOW: JML to Kind21Start ($A5:F309).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind21Start.
 */
void abi_Kind21Enter(void)
{
    abi_Kind21Start();
}

/*
 * @routine 83:9233 Kind24Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $24 go on from the actor loop in
 * its start state (the table at $83:8CAB): Kind24Start.
 * HOW: JML to Kind24Start ($84:8000).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind24Start.
 */
void abi_Kind24Enter(void)
{
    abi_Kind24Start();
}

/*
 * @routine 83:923C Kind49Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $49 go on from the actor loop in
 * its start state (the table at $83:8CAB): Kind49Carry.
 * HOW: JML to Kind49Carry ($84:804D).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind49Carry.
 */
void abi_Kind49Enter(void)
{
    abi_Kind49Carry();
}

/*
 * @routine 83:9249 Kind63Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $63 go on from the actor loop in
 * its start state (the table at $83:8CAB): Kind63Start.
 * HOW: JML to Kind63Start ($84:8080).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind63Start.
 */
void abi_Kind63Enter(void)
{
    abi_Kind63Start();
}

/*
 * @routine 83:9256 Kind63Third
 * @actor
 * WHAT IT DOES: Where actors of kind $63 go on from the actor loop in
 * a state with bits 7 and 6 set (the table at $83:90A1): Kind63ShotHit.
 * HOW: JML to Kind63ShotHit ($84:81CD).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind63ShotHit.
 */
void abi_Kind63Third(void)
{
    abi_Kind63ShotHit();
}

/*
 * @routine 83:928A Kind76Second
 * @actor
 * WHAT IT DOES: Where actors of kind $76 go on from the actor loop in
 * a state with bit 7 set (the table at $83:8DFD): BackdropMover.
 * HOW: JML to BackdropMover ($84:9044).
 * No script runs it (no script reaches this entry of the table).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as BackdropMover.
 */
void abi_Kind76Second(void)
{
    abi_BackdropMover();
}

/*
 * @routine 83:9297 Kind7CSecond
 * @actor
 * WHAT IT DOES: Where actors of kind $7C go on from the actor loop in
 * a state with bit 7 set (the table at $83:8DFD): Kind7DKeepSpot.
 * HOW: JML to Kind7DKeepSpot ($84:90CE).
 * No script runs it (no script reaches this entry of the table).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind7DKeepSpot.
 */
void abi_Kind7CSecond(void)
{
    abi_Kind7DKeepSpot();
}

/*
 * @routine 83:92B1 Kind7ESecond
 * @actor
 * WHAT IT DOES: Where actors of kind $7E go on from the actor loop in
 * a state with bit 7 set (the table at $83:8DFD): Kind7FDrift.
 * HOW: JML to Kind7FDrift ($84:934B).
 * No script runs it (no script reaches this entry of the table).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind7FDrift.
 */
void abi_Kind7ESecond(void)
{
    abi_Kind7FDrift();
}

/*
 * @routine 83:92C2 Kind81Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $81 go on from the actor loop in
 * its start state (the table at $83:8DAB): Kind81Scene.
 * HOW: JML to Kind81Scene ($84:9391).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind81Scene.
 */
void abi_Kind81Enter(void)
{
    abi_Kind81Scene();
}

/*
 * @routine 83:92CB Kind81Second
 * @actor
 * WHAT IT DOES: Where actors of kind $81 go on from the actor loop in
 * a state with bit 7 set (the table at $83:8EFD): Kind82Show.
 * HOW: JML to Kind82Show ($84:951D).
 * No script runs it (no script reaches this entry of the table).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind82Show.
 */
void abi_Kind81Second(void)
{
    abi_Kind82Show();
}

/*
 * @routine 83:92E1 Kind83Second
 * @actor
 * WHAT IT DOES: Where actors of kind $83 go on from the actor loop in
 * a state with bit 7 set (the table at $83:8EFD): ActorWaitGrabsPlayer.
 * HOW: JML to ActorWaitGrabsPlayer ($84:95D4).
 * No script runs it (no script reaches this entry of the table).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: as ActorWaitGrabsPlayer leaves them: its RTL here pulls
 * three bytes of the actor loop's own frame (its saved D, then its return).
 */
void abi_Kind83Second(void)
{
    abi_ActorWaitGrabsPlayer();
}

/*
 * @routine 83:92FB Kind85Second
 * @actor
 * WHAT IT DOES: Where actors of kind $85 go on from the actor loop in
 * a state with bit 7 set (the table at $83:8EFD): KindAnimate9628.
 * HOW: JML to KindAnimate9628 ($84:9628).
 * No script runs it (no script reaches this entry of the table).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: as KindAnimate9628 leaves them: its RTL here pulls
 * three bytes of the actor loop's own frame (its saved D, then its return).
 */
void abi_Kind85Second(void)
{
    abi_KindAnimate9628();
}

/*
 * @routine 83:9315 Kind87Second
 * @actor
 * WHAT IT DOES: Where actors of kind $87 go on from the actor loop in
 * a state with bit 7 set (the table at $83:8EFD): LandingPad.
 * HOW: JML to LandingPad ($83:F9BA).
 * No script runs it (no script reaches this entry of the table).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as LandingPad.
 */
void abi_Kind87Second(void)
{
    abi_LandingPad();
}

/*
 * @routine 83:9322 Kind88Second
 * @actor
 * WHAT IT DOES: Where actors of kind $88 go on from the actor loop in
 * a state with bit 7 set (the table at $83:8EFD): Kind89PlayerPriority.
 * HOW: JML to Kind89PlayerPriority ($84:8F9C).
 * No script runs it (no script reaches this entry of the table).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind89PlayerPriority.
 */
void abi_Kind88Second(void)
{
    abi_Kind89PlayerPriority();
}

/*
 * @routine 83:9344 Kind8DEnter
 * @actor
 * WHAT IT DOES: Where actors of kind $8D go on from the actor loop in
 * its start state (the table at $83:8DAB): Kind8DCircle.
 * HOW: JML to Kind8DCircle ($84:99ED).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind8DCircle.
 */
void abi_Kind8DEnter(void)
{
    abi_Kind8DCircle();
}

/*
 * @routine 83:936B Kind90Second
 * @actor
 * WHAT IT DOES: Where actors of kind $90 go on from the actor loop in
 * a state with bit 7 set (the table at $83:8EFD): LongReturn.
 * HOW: JML to LongReturn ($A5:F533).
 * No script runs it (no script reaches this entry of the table).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: as LongReturn leaves them: its RTL here pulls
 * three bytes of the actor loop's own frame (its saved D, then its return).
 */
void abi_Kind90Second(void)
{
    abi_LongReturn();
}

/*
 * @routine 83:9378 Kind91Second
 * @actor
 * WHAT IT DOES: Where actors of kind $91 go on from the actor loop in
 * a state with bit 7 set (the table at $83:8EFD): Kind91FreeOff.
 * HOW: JML to Kind91FreeOff ($A5:F603).
 * No script runs it (no script reaches this entry of the table).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: as Kind91FreeOff leaves them: its RTL here pulls
 * three bytes of the actor loop's own frame (its saved D, then its return).
 */
void abi_Kind91Second(void)
{
    abi_Kind91FreeOff();
}

/*
 * @routine 83:9385 Kind92Second
 * @actor
 * WHAT IT DOES: Where actors of kind $92 go on from the actor loop in
 * a state with bit 7 set (the table at $83:8EFD): Kind91FreeOff.
 * HOW: JML to Kind91FreeOff ($A5:F603).
 * No script runs it (no script reaches this entry of the table).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: as Kind91FreeOff leaves them: its RTL here pulls
 * three bytes of the actor loop's own frame (its saved D, then its return).
 */
void abi_Kind92Second(void)
{
    abi_Kind91FreeOff();
}

/*
 * @routine 83:9392 Kind93Second
 * @actor
 * WHAT IT DOES: Where actors of kind $93 go on from the actor loop in
 * a state with bit 7 set (the table at $83:8EFD): Kind93AnimateSend.
 * HOW: JML to Kind93AnimateSend ($A5:F434).
 * No script runs it (no script reaches this entry of the table).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: as Kind93AnimateSend leaves them: its RTL here pulls
 * three bytes of the actor loop's own frame (its saved D, then its return).
 */
void abi_Kind93Second(void)
{
    abi_Kind93AnimateSend();
}

/*
 * @routine 83:93E5 Kind9BEnter
 * @actor
 * WHAT IT DOES: Where actors of kind $9B go on from the actor loop in
 * its start state (the table at $83:8DAB): Kind9BStory.
 * HOW: JML to Kind9BStory ($84:A509).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind9BStory.
 */
void abi_Kind9BEnter(void)
{
    abi_Kind9BStory();
}

/*
 * @routine 83:93EE Kind9CEnter
 * @actor
 * WHAT IT DOES: Where actors of kind $9C go on from the actor loop in
 * its start state (the table at $83:8DAB): Kind9CStart.
 * HOW: JML to Kind9CStart ($84:A7E5).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind9CStart.
 */
void abi_Kind9CEnter(void)
{
    abi_Kind9CStart();
}

/*
 * @routine 83:9281 Kind76Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $76 go on from the actor loop in
 * its start state (the table at $83:8CAB): Kind76Start.
 * HOW: JML to Kind76Start ($84:8FBA).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind76Start.
 */
void abi_Kind76Enter(void)
{
    abi_Kind76Start();
}

/*
 * @routine 83:91F4 Kind00Start
 * @actor
 * WHAT IT DOES: Where actors of kinds $00-$03 and $07 (the player) go on from the actor loop
 * in its start state (the table at $83:8CAB): PlayerStart.
 * HOW: JML to PlayerStart ($81:CB3B).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as PlayerStart.
 */
void abi_Kind00Start(void)
{
    abi_PlayerStart();
}

/*
 * @routine 83:91FD Kind00Second
 * @actor
 * WHAT IT DOES: Where actors of kinds $00, $02, $03 and $07 (the player) go on from the actor loop
 * in a state with bit 7 set (the table at $83:8DFD): PlayerKnockback.
 * HOW: JML to PlayerKnockback ($81:DADD).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as PlayerKnockback.
 */
void abi_Kind00Second(void)
{
    abi_PlayerKnockback();
}

/*
 * @routine 83:9201 Kind01Second
 * @actor
 * WHAT IT DOES: Where actors of kind $01 (the player) go on from the actor loop
 * in a state with bit 7 set (the table at $83:8DFD): PlayerCarpetCrash.
 * HOW: JML to PlayerCarpetCrash ($81:E7E9).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as PlayerCarpetCrash.
 */
void abi_Kind01Second(void)
{
    abi_PlayerCarpetCrash();
}

/*
 * @routine 83:92D8 Kind83Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $83 go on from the actor loop
 * in its start state (the table at $83:8DAB): Kind83Start.
 * HOW: JML to Kind83Start ($84:9538).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind83Start.
 */
void abi_Kind83Enter(void)
{
    abi_Kind83Start();
}

/*
 * @routine 83:93A3 Kind95Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $95 go on from the actor loop
 * in its start state (the table at $83:8DAB): Kind95Start.
 * HOW: JML to Kind95Start ($84:9BE3).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind95Start.
 */
void abi_Kind95Enter(void)
{
    abi_Kind95Start();
}

/*
 * @routine 83:9362 Kind90Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $90 go on from the actor loop in
 * its start state (the table at $83:8DAB): Kind90Start.
 * HOW: JML to Kind90Start ($A5:F4D2).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind90Start.
 */
void abi_Kind90Enter(void)
{
    abi_Kind90Start();
}

/*
 * @routine 83:9409 KindA1Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $A1 go on from the actor loop in
 * its start state (the table at $83:8DAB): KindA1Start.
 * HOW: JML to KindA1Start ($84:A97F).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as KindA1Start.
 */
void abi_KindA1Enter(void)
{
    abi_KindA1Start();
}

/*
 * @routine 83:9446 KindA8Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $A8 go on from the actor loop in
 * its start state (the table at $83:8DAB): KindA8Start.
 * HOW: JML to KindA8Start ($83:FB1A).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as KindA8Start.
 */
void abi_KindA8Enter(void)
{
    abi_KindA8Start();
}

/*
 * @routine 83:939F Kind94Second
 * @actor
 * WHAT IT DOES: Where actors of kind $94 go on from the actor loop in
 * a state with bit 7 set (the table at $83:8EFD): KindA8Start.
 * HOW: JML to KindA8Start ($83:FB1A).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as KindA8Start.
 */
void abi_Kind94Second(void)
{
    abi_KindA8Start();
}

/*
 * @routine 83:9252 Kind63Second
 * @actor
 * WHAT IT DOES: Where actors of kind $63 go on from the actor loop in
 * a state with bit 7 set (the table at $83:8DFD): Kind63Knocked.
 * HOW: JML to Kind63Knocked ($84:81BE).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind63Knocked.
 */
void abi_Kind63Second(void)
{
    abi_Kind63Knocked();
}

/*
 * @routine 83:92F2 Kind85Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $85 go on from the actor loop in
 * its start state (the table at $83:8DAB): Kind85Start.
 * HOW: JML to Kind85Start ($84:9602).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind85Start.
 */
void abi_Kind85Enter(void)
{
    abi_Kind85Start();
}

/*
 * @routine 83:9351 Kind8EEnter
 * @actor
 * WHAT IT DOES: Where actors of kind $8E go on from the actor loop in
 * its start state (the table at $83:8DAB): Kind8EStart.
 * HOW: JML to Kind8EStart ($84:9B6B).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind8EStart.
 */
void abi_Kind8EEnter(void)
{
    abi_Kind8EStart();
}

/*
 * @routine 83:930C Kind87Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $87 go on from the actor loop in
 * its start state (the table at $83:8DAB): Kind87Start.
 * HOW: JML to Kind87Start ($83:F77F).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind87Start.
 */
void abi_Kind87Enter(void)
{
    abi_Kind87Start();
}

/*
 * @routine 83:936F Kind91Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $91 go on from the actor loop in
 * its start state (the table at $83:8DAB): Kind91Start.
 * HOW: JML to Kind91Start ($A5:F534).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind91Start.
 */
void abi_Kind91Enter(void)
{
    abi_Kind91Start();
}

/*
 * @routine 83:9442 KindA7Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $A7 go on from the actor loop in
 * its start state (the table at $83:8DAB): KindA7Start.
 * HOW: JML to KindA7Start ($84:AC66).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as KindA7Start.
 */
void abi_KindA7Enter(void)
{
    abi_KindA7Start();
}

/*
 * @routine 83:9267 Kind74Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $74 go on from the actor loop in
 * its start state (the table at $83:8CAB): Kind74Start.
 * HOW: JML to Kind74Start ($84:87C2).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind74Start.
 */
void abi_Kind74Enter(void)
{
    abi_Kind74Start();
}

/*
 * @routine 83:9274 Kind75Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $75 go on from the actor loop in
 * its start state (the table at $83:8CAB): Kind75Start.
 * HOW: JML to Kind75Start ($84:87D1).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind75Start.
 */
void abi_Kind75Enter(void)
{
    abi_Kind75Start();
}

/*
 * @routine 83:9400 KindA0Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $A0 go on from the actor loop in
 * its start state (the table at $83:8DAB): KindA0Start.
 * HOW: JML to KindA0Start ($84:A8E0).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as KindA0Start.
 */
void abi_KindA0Enter(void)
{
    abi_KindA0Start();
}
