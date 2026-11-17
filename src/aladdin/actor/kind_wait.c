/*
 * kind_wait.c: what each kind of actor runs, frame by frame, while it
 * waits: the actor loop calls it through a table by kind ($83:8F4F for
 * kinds below $80, $83:904F for the others) when bit 7 of the record's
 * byte at $09 is set (docs/aladdin/ACTORS.md). Each entry here calls the
 * kind's work in its own bank.
 */
#include "machine.h"
#include "ops65.h"

void abi_LongReturn(void);
void abi_Kind49Wait(void);
void abi_MoverEndSound(void);
void abi_ActorWaitGrabsPlayer(void);
void abi_SenderWait8E(void);
void abi_KindAnimateSendFAAE(void);
void abi_ActorBobAnimate(void);
void abi_ActorAnimFreeFar(void);
void abi_Kind21AnimateSend(void);
void abi_Kind24Animate(void);
void abi_Kind63Work(void);
void abi_Kind73Work(void);
void abi_Kind88Work(void);
void abi_Kind91FreeOff(void);
void abi_Kind93AnimateSend(void);
void abi_Kind9CAnimate(void);
void abi_KindA0AnimateFreeOff(void);
void abi_KindA3Animate(void);
void abi_ChainPieceWork(void);

/* The kinds' work, named by the first kind that uses it */
#define KIND_00_WAIT_WORK 0x81E864u
#define KIND_63_WAIT_WORK 0x8481FCu
#define KIND_A2_WAIT_WORK 0x84832Bu
#define KIND_73_WAIT_WORK 0x8486F4u
#define KIND_74_WAIT_WORK 0x848A78u
#define KIND_88_WAIT_WORK 0x848EF0u
#define KIND_76_WAIT_WORK 0x84903Bu
#define KIND_7F_WAIT_WORK 0x849387u
#define KIND_82_WAIT_WORK 0x849533u
#define KIND_85_WAIT_WORK 0x849628u
#define KIND_95_WAIT_WORK 0x849EF0u
#define KIND_96_WAIT_WORK 0x849F4Au
#define KIND_A1_WAIT_WORK 0x84A9C7u
#define KIND_08_WAIT_WORK 0xA5F020u
#define KIND_21_WAIT_WORK 0xA5F3E3u













/*
 * @routine 83:9366 ActorKind90Wait
 * WHAT IT DOES: The routine actors of kind $90 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: nothing.
 * HOW: JSL LongReturn, then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: everything unchanged.
 */
void abi_ActorKind90Wait(void)
{
    abi_LongReturn();
}






/*
 * @routine 83:9285 ActorKind76Wait
 * WHAT IT DOES: The routine actors of kind $76 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: the work at
 * $84:903B.
 * HOW: JSL $84:903B, then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as $84:903B leaves them.
 */
void abi_ActorKind76Wait(void)
{
    call_routine(KIND_76_WAIT_WORK, 1);
}
/*
 * @routine 83:92B9 ActorKind7FWait
 * WHAT IT DOES: The routine actors of kind $7F run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: the work at
 * $84:9387.
 * HOW: JSL $84:9387, then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as $84:9387 leaves them.
 */
void abi_ActorKind7FWait(void)
{
    call_routine(KIND_7F_WAIT_WORK, 1);
}
/*
 * @routine 83:93B0 ActorKind96Wait
 * WHAT IT DOES: The routine actors of kind $96 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: the work at
 * $84:9F4A.
 * HOW: JSL $84:9F4A, then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as $84:9F4A leaves them.
 */
void abi_ActorKind96Wait(void)
{
    call_routine(KIND_96_WAIT_WORK, 1);
}
/*
 * @routine 83:93A7 ActorKind95Wait
 * WHAT IT DOES: The routine actors of kind $95 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: the work at
 * $84:9EF0.
 * HOW: JSL $84:9EF0, then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as $84:9EF0 leaves them.
 */
void abi_ActorKind95Wait(void)
{
    call_routine(KIND_95_WAIT_WORK, 1);
}
/*
 * @routine 83:92F6 ActorKind85Wait
 * WHAT IT DOES: The routine actors of kind $85 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: the work at
 * $84:9628.
 * HOW: JSL $84:9628, then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as $84:9628 leaves them.
 */
void abi_ActorKind85Wait(void)
{
    call_routine(KIND_85_WAIT_WORK, 1);
}
/*
 * @routine 83:92D3 ActorKind82Wait
 * WHAT IT DOES: The routine actors of kind $82 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: the work at
 * $84:9533.
 * HOW: JSL $84:9533, then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as $84:9533 leaves them.
 */
void abi_ActorKind82Wait(void)
{
    call_routine(KIND_82_WAIT_WORK, 1);
}






/*
 * @routine 83:9292 ActorKind7CWait
 * WHAT IT DOES: The routine actors of kind $7C run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: MoverEndSound.
 * HOW: JSL MoverEndSound ($84:90A2), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as MoverEndSound leaves them.
 */
void abi_ActorKind7CWait(void)
{
    abi_MoverEndSound();
}

/*
 * @routine 83:92DC ActorKind83Wait
 * WHAT IT DOES: The routine actors of kind $83 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: ActorWaitGrabsPlayer.
 * HOW: JSL ActorWaitGrabsPlayer ($84:95D4), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as ActorWaitGrabsPlayer leaves them.
 */
void abi_ActorKind83Wait(void)
{
    abi_ActorWaitGrabsPlayer();
}

/*
 * @routine 83:9348 ActorKind8DWait
 * WHAT IT DOES: The routine actors of kind $8D run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: SenderWait8E.
 * HOW: JSL SenderWait8E ($84:9B4D), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as SenderWait8E leaves them.
 */
void abi_ActorKind8DWait(void)
{
    abi_SenderWait8E();
}

/*
 * @routine 83:9359 ActorKind8FWait
 * WHAT IT DOES: The routine actors of kind $8F run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: KindAnimateSendFAAE.
 * HOW: JSL KindAnimateSendFAAE ($83:FAAE), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as KindAnimateSendFAAE leaves them.
 */
void abi_ActorKind8FWait(void)
{
    abi_KindAnimateSendFAAE();
}

/*
 * @routine 83:93E9 ActorKind9BWait
 * WHAT IT DOES: The routine actors of kind $9B run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: ActorBobAnimate.
 * HOW: JSL ActorBobAnimate ($84:A7A1), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as ActorBobAnimate leaves them.
 */
void abi_ActorKind9BWait(void)
{
    abi_ActorBobAnimate();
}

/*
 * @routine 83:9237 ActorKind24Wait
 * WHAT IT DOES: The routine actors of kind $24 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind24Animate.
 * HOW: JSL Kind24Animate ($84:8048), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind24Animate leaves them.
 */
void abi_ActorKind24Wait(void)
{
    abi_Kind24Animate();
}

/*
 * @routine 83:9373 ActorKind91Wait
 * WHAT IT DOES: The routine actors of kind $91 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind91FreeOff.
 * HOW: JSL Kind91FreeOff ($A5:F603), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind91FreeOff leaves them.
 */
void abi_ActorKind91Wait(void)
{
    abi_Kind91FreeOff();
}

/*
 * @routine 83:9380 ActorKind92Wait
 * WHAT IT DOES: The routine actors of kind $92 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind91FreeOff.
 * HOW: JSL Kind91FreeOff ($A5:F603), then RTS.
 * No script runs it (no script has a waiting actor of this kind).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind91FreeOff leaves them.
 */
void abi_ActorKind92Wait(void)
{
    abi_Kind91FreeOff();
}

/*
 * @routine 83:938D ActorKind93Wait
 * WHAT IT DOES: The routine actors of kind $93 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind93AnimateSend.
 * HOW: JSL Kind93AnimateSend ($A5:F434), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind93AnimateSend leaves them.
 */
void abi_ActorKind93Wait(void)
{
    abi_Kind93AnimateSend();
}

/*
 * @routine 83:93F2 ActorKind9CWait
 * WHAT IT DOES: The routine actors of kind $9C run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind9CAnimate.
 * HOW: JSL Kind9CAnimate ($84:A81B), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind9CAnimate leaves them.
 */
void abi_ActorKind9CWait(void)
{
    abi_Kind9CAnimate();
}

/*
 * @routine 83:9404 ActorKindA0Wait
 * WHAT IT DOES: The routine actors of kind $A0 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: KindA0AnimateFreeOff.
 * HOW: JSL KindA0AnimateFreeOff ($84:A975), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as KindA0AnimateFreeOff leaves them.
 */
void abi_ActorKindA0Wait(void)
{
    abi_KindA0AnimateFreeOff();
}

/*
 * @routine 83:9427 ActorKindA3Wait
 * WHAT IT DOES: The routine actors of kind $A3 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: KindA3Animate.
 * HOW: JSL KindA3Animate ($84:8399), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as KindA3Animate leaves them.
 */
void abi_ActorKindA3Wait(void)
{
    abi_KindA3Animate();
}

/*
 * @routine 83:922A ActorKind21Wait
 * WHAT IT DOES: The routine actors of kind $21 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind21AnimateSend.
 * HOW: JSL Kind21AnimateSend ($A5:F3E3), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind21AnimateSend leaves them.
 */
void abi_ActorKind21Wait(void)
{
    abi_Kind21AnimateSend();
}

/*
 * @routine 83:924D ActorKind63Wait
 * WHAT IT DOES: The routine actors of kind $63 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind63Work.
 * HOW: JSL Kind63Work ($84:81FC), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind63Work leaves them.
 */
void abi_ActorKind63Wait(void)
{
    abi_Kind63Work();
}

/*
 * @routine 83:925E ActorKind73Wait
 * WHAT IT DOES: The routine actors of kind $73 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind73Work.
 * HOW: JSL Kind73Work ($84:86F4), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind73Work leaves them.
 */
void abi_ActorKind73Wait(void)
{
    abi_Kind73Work();
}

/*
 * @routine 83:931D ActorKind88Wait
 * WHAT IT DOES: The routine actors of kind $88 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind88Work.
 * HOW: JSL Kind88Work ($84:8EF0), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind88Work leaves them.
 */
void abi_ActorKind88Wait(void)
{
    abi_Kind88Work();
}

/*
 * @routine 83:941A ActorKindA2Wait
 * WHAT IT DOES: The routine actors of kind $A2 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: ActorAnimFreeFar.
 * HOW: JSL ActorAnimFreeFar ($84:832B), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as ActorAnimFreeFar leaves them.
 */
void abi_ActorKindA2Wait(void)
{
    abi_ActorAnimFreeFar();
}

/*
 * @routine 83:926B ActorKind74Wait
 * WHAT IT DOES: The routine actors of kind $74 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: ChainPieceWork.
 * HOW: JSL ChainPieceWork ($84:8A78), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as ChainPieceWork leaves them.
 */
void abi_ActorKind74Wait(void)
{
    abi_ChainPieceWork();
}

/*
 * @routine 83:9278 ActorKind75Wait
 * WHAT IT DOES: The routine actors of kind $75 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: ChainPieceWork.
 * HOW: JSL ChainPieceWork ($84:8A78), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as ChainPieceWork leaves them.
 */
void abi_ActorKind75Wait(void)
{
    abi_ChainPieceWork();
}

/*
 * @routine 83:9240 ActorKind49Wait
 * WHAT IT DOES: The routine actors of kind $49 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind49Wait.
 * HOW: JSL Kind49Wait ($84:805B), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind49Wait leaves them.
 */
void abi_ActorKind49Wait(void)
{
    abi_Kind49Wait();
}
