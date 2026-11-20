/*
 * kind_wait.c: the original's wait stubs. The actor loop called a kind's
 * stub through its tables by kind ($83:8F4F below kind $80, $83:904F the
 * rest) when bit 7 of the record's +$09 was set; each stub only called the
 * kind's work in its own bank (one JSL, then RTS). In the native game the
 * work is the kind's table row's wait routine (actor_own.h's KindCode), so
 * each stub is that field, set by whoever owns the kind's row. Each stub
 * below with its kinds and the routine the row names (its record file):
 *
 *   @routine 83:91F8 ActorKind00Wait: kinds $00, $01, $02, $03, $07: PlayerFrameWork (player_frame.c)
 *   @routine 83:9209 ActorKind08Wait: kind $08: GuardWait (guard_states.c)
 *   @routine 83:922A ActorKind21Wait: kind $21: Kind21AnimateSend (kind_steps.c)
 *   @routine 83:9237 ActorKind24Wait: kind $24: Kind24Animate (kind_steps.c)
 *   @routine 83:9240 ActorKind49Wait: kind $49: Kind49Wait (kind49.c)
 *   @routine 83:924D ActorKind63Wait: kind $63: Kind63Work (kind63.c)
 *   @routine 83:925E ActorKind73Wait: kind $73: Kind73Work (kind73.c)
 *   @routine 83:926B ActorKind74Wait: kind $74: ChainPieceWork (chain_piece.c)
 *   @routine 83:9278 ActorKind75Wait: kind $75: ChainPieceWork (chain_piece.c)
 *   @routine 83:9285 ActorKind76Wait: kind $76: KindAnimateSend (kind_steps.c)
 *   @routine 83:9292 ActorKind7CWait: kind $7C: MoverEndSound (backdrop_mover.c)
 *   @routine 83:92AC ActorKind7EWait: kind $7E: Kind7EWaitWork (kind_waits_84.c)
 *   @routine 83:92B9 ActorKind7FWait: kind $7F: KindAnimateFreeOff (kind_steps.c)
 *   @routine 83:92C6 ActorKind81Wait: kind $81: Kind81WaitWork (kind_waits_84.c)
 *   @routine 83:92D3 ActorKind82Wait: kind $82: KindAnimate9533 (kind_steps.c)
 *   @routine 83:92DC ActorKind83Wait: kind $83: ActorWaitGrabsPlayer (actor_waits_small.c)
 *   @routine 83:92E9 ActorKind84Wait: kind $84: Kind84WaitWork (kind_waits_84.c)
 *   @routine 83:92F6 ActorKind85Wait: kind $85: KindAnimate9628 (kind_steps.c)
 *   @routine 83:9310 ActorKind87Wait: kind $87: KindWaitF991 (kind_waits.c)
 *   @routine 83:931D ActorKind88Wait: kind $88: Kind88Work (actor_84_misc.c)
 *   @routine 83:933B ActorKind8CWait: kind $8C: Kind8CWaitWork (kind_waits_84.c)
 *   @routine 83:9348 ActorKind8DWait: kind $8D: SenderWait8E (small_actor_helpers.c)
 *   @routine 83:9359 ActorKind8FWait: kind $8F: KindAnimateSendFAAE (kind_animate.c)
 *   @routine 83:9366 ActorKind90Wait: kind $90: LongReturn (helpers.c) (LongReturn: nothing)
 *   @routine 83:9373 ActorKind91Wait: kind $91: Kind91FreeOff (kind_steps.c)
 *   @routine 83:9380 ActorKind92Wait: kind $92: Kind91FreeOff (kind_steps.c)
 *   @routine 83:938D ActorKind93Wait: kind $93: Kind93AnimateSend (kind_steps.c)
 *   @routine 83:939A ActorKind94Wait: kind $94: Kind94Wait (point_moves.c)
 *   @routine 83:93A7 ActorKind95Wait: kind $95: KindAnimate9EF0 (kind_steps.c)
 *   @routine 83:93B0 ActorKind96Wait: kind $96: KindAnimate9F4A (kind_steps.c)
 *   @routine 83:93B9 ActorKind97Wait: kind $97: Kind97WaitWork (kind97.c) (@unreachable in the record: nothing sets bit 7 of a kind $97's +$09)
 *   @routine 83:93C2 ActorKind98Wait: kind $98: Kind98WaitWork (kind98.c)
 *   @routine 83:93CF ActorKind99Wait: kind $99: Kind99WaitWork (kind_waits_84.c)
 *   @routine 83:93DC ActorKind9AWait: kind $9A: Kind9AWaitWork (kind_waits_84.c)
 *   @routine 83:93E9 ActorKind9BWait: kind $9B: ActorBobAnimate (actor_moves.c)
 *   @routine 83:93F2 ActorKind9CWait: kind $9C: Kind9CAnimate (kind_steps.c)
 *   @routine 83:93FB ActorKind9EWait: kind $9E: Kind9EWaitWork (kind_waits_84.c)
 *   @routine 83:9404 ActorKindA0Wait: kind $A0: KindA0AnimateFreeOff (kind_steps.c)
 *   @routine 83:940D ActorKindA1Wait: kind $A1: KindA1Touch (kind_a1_a6.c)
 *   @routine 83:941A ActorKindA2Wait: kind $A2: ActorAnimFreeFar (actor_84_misc.c)
 *   @routine 83:9427 ActorKindA3Wait: kind $A3: KindA3Animate (kind_steps.c)
 *   @routine 83:9430 ActorKindA4Wait: kind $A4: KindA4WaitWork (kind_waits_84.c)
 *   @routine 83:9439 ActorKindA5Wait: kind $A5: KindA5WaitWork (kind_waits_84.c)
 *
 * Kind $82, $85, $95 and $96's work is AnimateWhileWaiting (kinds.c);
 * kinds $21, $76 and $93's is KindAnimateSend (kind_steps.c). Each row is
 * its kind's owner's (one kind, one owner): this file holds no code.
 */
