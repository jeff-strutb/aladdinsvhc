/*
 * kind_animate.c: per-frame routines of actor kinds that animate and send
 * their picture each frame they wait. Drafted by cdraft.py; named and
 * described by hand.
 */
#include "machine.h"
#include "ops65.h"

void abi_ActorPictureSend(void);
void abi_AnimStep(void);

/*
 * @routine 83:FAAE KindAnimateSendFAAE
 * WHAT IT DOES: One frame of an actor kind (the one the kind table's stub
 * at 83:9355 starts at 83:FA4F): animates and sends its picture.
 * HOW: AnimStep, then ActorPictureSend.
 * Entry: D = the actor; called by JSL from the kind's stub (83:9359) while
 * the actor waits. Exit: as ActorPictureSend.
 */
void abi_KindAnimateSendFAAE(void)
{
    jsl_frame(0x83FAAEu);  /* 83FAAE jsl */
    abi_AnimStep();
    cpu.s = (uint16_t)(cpu.s + 3);
    jsl_frame(0x83FAB2u);  /* 83FAB2 jsl */
    abi_ActorPictureSend();
    cpu.s = (uint16_t)(cpu.s + 3);
    return;  /* 83FAB6 rtl */
}

