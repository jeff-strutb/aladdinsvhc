/*
 * kind_wait.c: what each kind of actor runs, frame by frame, while it
 * waits: the actor loop calls it through a table by kind ($83:8F4F for
 * kinds below $80, $83:904F for the others) when bit 7 of the record's
 * byte at $09 is set (docs/aladdin/ACTORS.md). Each entry here calls the
 * kind's work in its own bank.
 */
#include "machine.h"
#include "ops65.h"

void abi_PlayerFrameWork(void);
void abi_KindAnimateSend(void);
void abi_KindAnimateFreeOff(void);
void abi_KindAnimate9533(void);
void abi_KindAnimate9628(void);
void abi_KindAnimate9EF0(void);
void abi_KindAnimate9F4A(void);
void abi_LongReturn(void);
void abi_Kind94Wait(void);
void abi_KindWaitF991(void);
void abi_KindA1Touch(void);
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
void abi_Kind7EWaitWork(void);
void abi_Kind81WaitWork(void);
void abi_Kind84WaitWork(void);
void abi_Kind8CWaitWork(void);
void abi_Kind97WaitWork(void);
void abi_Kind98WaitWork(void);
void abi_Kind99WaitWork(void);
void abi_Kind9AWaitWork(void);
void abi_Kind9EWaitWork(void);
void abi_KindA4WaitWork(void);
void abi_KindA5WaitWork(void);
void abi_GuardWait(void);

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
    jsl_frame(0x839366u);
    abi_LongReturn();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x839285u);
    abi_KindAnimateSend();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x8392B9u);
    abi_KindAnimateFreeOff();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x8393B0u);
    abi_KindAnimate9F4A();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x8393A7u);
    abi_KindAnimate9EF0();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x8392F6u);
    abi_KindAnimate9628();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x8392D3u);
    abi_KindAnimate9533();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x839292u);
    abi_MoverEndSound();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x8392DCu);
    abi_ActorWaitGrabsPlayer();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x839348u);
    abi_SenderWait8E();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x839359u);
    abi_KindAnimateSendFAAE();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x8393E9u);
    abi_ActorBobAnimate();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x839237u);
    abi_Kind24Animate();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x839373u);
    abi_Kind91FreeOff();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x839380u);
    abi_Kind91FreeOff();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x83938Du);
    abi_Kind93AnimateSend();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x8393F2u);
    abi_Kind9CAnimate();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x839404u);
    abi_KindA0AnimateFreeOff();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x839427u);
    abi_KindA3Animate();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x83922Au);
    abi_Kind21AnimateSend();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x83924Du);
    abi_Kind63Work();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x83925Eu);
    abi_Kind73Work();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x83931Du);
    abi_Kind88Work();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x83941Au);
    abi_ActorAnimFreeFar();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x83926Bu);
    abi_ChainPieceWork();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x839278u);
    abi_ChainPieceWork();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x839240u);
    abi_Kind49Wait();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:91F8 ActorKind00Wait
 * WHAT IT DOES: The routine actors of kinds $00, $01, $02, $03 and $07
 * run each frame they wait (not the last), when bit 7 of their byte at
 * $09 is set: the work at $81:E864.
 * HOW: JSL $81:E864, then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as $81:E864 leaves them.
 */
void abi_ActorKind00Wait(void)
{
    jsl_frame(0x8391F8u);
    abi_PlayerFrameWork();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:9310 ActorKind87Wait
 * WHAT IT DOES: The routine actors of kind $87 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: KindWaitF991.
 * HOW: JSL KindWaitF991 ($83:F991), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as KindWaitF991 leaves them.
 */
void abi_ActorKind87Wait(void)
{
    jsl_frame(0x839310u);
    abi_KindWaitF991();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:939A ActorKind94Wait
 * WHAT IT DOES: The routine actors of kind $94 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind94Wait.
 * HOW: JSL Kind94Wait ($83:FB15), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind94Wait leaves them.
 */
void abi_ActorKind94Wait(void)
{
    jsl_frame(0x83939Au);
    abi_Kind94Wait();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:940D ActorKindA1Wait
 * WHAT IT DOES: The routine actors of kind $A1 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: the work at
 * $84:A9C7.
 * HOW: JSL KindA1Touch, then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as KindA1Touch leaves them (it may end the actor: never returns).
 */
void abi_ActorKindA1Wait(void)
{
    /* the JSL's return address, as KindA1Touch finds it: it drops it (and
       the loop's) when it ends the actor */
    jsl_frame(0x83940Du);
    abi_KindA1Touch();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:92AC ActorKind7EWait
 * WHAT IT DOES: The routine actors of kind $7E run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind7EWaitWork.
 * HOW: JSL Kind7EWaitWork ($84:931C), then RTS; its C makes the JSL's
 * return frame (jsl_frame), which the work may drop when it ends the
 * actor.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind7EWaitWork leaves them.
 */
void abi_ActorKind7EWait(void)
{
    jsl_frame(0x8392ACu);
    abi_Kind7EWaitWork();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:92C6 ActorKind81Wait
 * WHAT IT DOES: The routine actors of kind $81 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind81WaitWork.
 * HOW: JSL Kind81WaitWork ($84:951C), then RTS; its C makes the JSL's
 * return frame (jsl_frame), which the work may drop when it ends the
 * actor.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind81WaitWork leaves them.
 */
void abi_ActorKind81Wait(void)
{
    jsl_frame(0x8392C6u);
    abi_Kind81WaitWork();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:92E9 ActorKind84Wait
 * WHAT IT DOES: The routine actors of kind $84 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind84WaitWork.
 * HOW: JSL Kind84WaitWork ($84:95FD), then RTS; its C makes the JSL's
 * return frame (jsl_frame), which the work may drop when it ends the
 * actor.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind84WaitWork leaves them.
 */
void abi_ActorKind84Wait(void)
{
    jsl_frame(0x8392E9u);
    abi_Kind84WaitWork();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:933B ActorKind8CWait
 * WHAT IT DOES: The routine actors of kind $8C run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind8CWaitWork.
 * HOW: JSL Kind8CWaitWork ($84:99E7), then RTS; its C makes the JSL's
 * return frame (jsl_frame), which the work may drop when it ends the
 * actor.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind8CWaitWork leaves them.
 */
void abi_ActorKind8CWait(void)
{
    jsl_frame(0x83933Bu);
    abi_Kind8CWaitWork();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:93B9 ActorKind97Wait
 * WHAT IT DOES: The routine actors of kind $97 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind97WaitWork.
 * HOW: JSL Kind97WaitWork ($84:9FFB), then RTS; the work's RTS pulls two
 * of the JSL's three bytes, so the game goes on at $84:93BD, not at this
 * RTS: nothing sets bit 7 of a kind $97's $09, so the game never runs it.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind97WaitWork leaves them.
 */
void abi_ActorKind97Wait(void)
{
    uint16_t ret;

    jsl_frame(0x8393B9u);
    abi_Kind97WaitWork();
    /* The work ends in RTS: it pulls two of the JSL's three bytes, and
       the game goes on at the word + 1 in bank $84 ($84:93BD), the JSL's
       bank byte left on the stack. */
    ret = pull16();
    jump_routine(0x840000u | (uint16_t)(ret + 1));
}

/*
 * @routine 83:93C2 ActorKind98Wait
 * WHAT IT DOES: The routine actors of kind $98 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind98WaitWork.
 * HOW: JSL Kind98WaitWork ($84:A39C), then RTS; its C makes the JSL's
 * return frame (jsl_frame).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind98WaitWork leaves them.
 */
void abi_ActorKind98Wait(void)
{
    jsl_frame(0x8393C2u);
    abi_Kind98WaitWork();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:93CF ActorKind99Wait
 * WHAT IT DOES: The routine actors of kind $99 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind99WaitWork.
 * HOW: JSL Kind99WaitWork ($84:A438), then RTS; its C makes the JSL's
 * return frame (jsl_frame), which the work may drop when it ends the
 * actor.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind99WaitWork leaves them.
 */
void abi_ActorKind99Wait(void)
{
    jsl_frame(0x8393CFu);
    abi_Kind99WaitWork();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:93DC ActorKind9AWait
 * WHAT IT DOES: The routine actors of kind $9A run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind9AWaitWork.
 * HOW: JSL Kind9AWaitWork ($84:A4E3), then RTS; its C makes the JSL's
 * return frame (jsl_frame), which the work may drop when it ends the
 * actor.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind9AWaitWork leaves them.
 */
void abi_ActorKind9AWait(void)
{
    jsl_frame(0x8393DCu);
    abi_Kind9AWaitWork();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:93FB ActorKind9EWait
 * WHAT IT DOES: The routine actors of kind $9E run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind9EWaitWork.
 * HOW: JSL Kind9EWaitWork ($84:A8BD), then RTS; its C makes the JSL's
 * return frame (jsl_frame), which the work may drop when it ends the
 * actor.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind9EWaitWork leaves them.
 */
void abi_ActorKind9EWait(void)
{
    jsl_frame(0x8393FBu);
    abi_Kind9EWaitWork();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:9430 ActorKindA4Wait
 * WHAT IT DOES: The routine actors of kind $A4 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: KindA4WaitWork.
 * HOW: JSL KindA4WaitWork ($84:AB97), then RTS; its C makes the JSL's
 * return frame (jsl_frame), which the work may drop when it ends the
 * actor.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as KindA4WaitWork leaves them.
 */
void abi_ActorKindA4Wait(void)
{
    jsl_frame(0x839430u);
    abi_KindA4WaitWork();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:9439 ActorKindA5Wait
 * WHAT IT DOES: The routine actors of kind $A5 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: KindA5WaitWork.
 * HOW: JSL KindA5WaitWork ($84:AC31), then RTS; its C makes the JSL's
 * return frame (jsl_frame), which the work may drop when it ends the
 * actor.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as KindA5WaitWork leaves them.
 */
void abi_ActorKindA5Wait(void)
{
    jsl_frame(0x839439u);
    abi_KindA5WaitWork();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:9209 ActorKind08Wait
 * WHAT IT DOES: The routine actors of kind $08 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: the work at
 * $A5:F020.
 * HOW: JSL GuardWait ($A5:F020), then RTS; its C makes the JSL's return
 * frame (jsl_frame).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as $A5:F020 leaves them.
 */
void abi_ActorKind08Wait(void)
{
    jsl_frame(0x839209u);
    abi_GuardWait();
    cpu.s = (uint16_t)(cpu.s + 3);
}
