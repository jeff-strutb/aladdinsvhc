/*
 * children_start.c: an actor that starts two children at its spawn points, the free list put
 * in order first. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

/*
 * @routine 83:C33A ActorStartTwoChildren
 * WHAT IT DOES: Starts the actor's two children (at spawn points $1C and
 * $1E, their $07 5 and 4), the free list's top two records first put in
 * address order (the lower on top).
 * HOW: The top entry of the free list compared with the one under it
 * (16-bit); not below, the two swapped. Then the child start at 83:C4EC
 * twice, X = $1C, Y = 5 and X = $1E, Y = 4.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
 * through the actor's table at 83:C324. Exit: M 8-bit, X 8-bit; as the
 * child start left it.
 */
void abi_ActorStartTwoChildren(void)
{
    op_rep(0x20);  /* 83C33A rep */
    ldx8(ea_abs(ACTOR_FREE_TOP));  /* 83C33C ldx */
    lda16(ea_absx(ACTOR_FREE_LIST));  /* 83C33F lda */
    cmp16_v(rd16(ea_absx(0x15A0)));  /* 83C342 cmp */
    if (!IF_C) goto L_83C352;  /* 83C345 bcc */
    pha16();  /* 83C347 pha */
    lda16(ea_absx(0x15A0));  /* 83C348 lda */
    sta16(ea_absx(ACTOR_FREE_LIST));  /* 83C34B sta */
    pla16();  /* 83C34E pla */
    sta16(ea_absx(0x15A0));  /* 83C34F sta */
L_83C352:
    op_sep(0x20);  /* 83C352 sep */
    ldx8_v(0x1C);  /* 83C354 ldx */
    ldy8_v(0x05);  /* 83C356 ldy */
    call_routine(0x83C4ECu, 0);  /* 83C358 jsr */
    ldx8_v(0x1E);  /* 83C35B ldx */
    ldy8_v(0x04);  /* 83C35D ldy */
    call_routine(0x83C4ECu, 0);  /* 83C35F jsr */
    return;  /* 83C362 rts */
}

