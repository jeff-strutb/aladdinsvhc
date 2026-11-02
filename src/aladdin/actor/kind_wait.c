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





