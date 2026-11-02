/*
 * falling_actor.c: an actor falling down the level: the map tile under it, and its end when
 * it is above the level's top. Drafted by cdraft.py; named and described
 * by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "wram_player.h"
#include "actor.h"

void abi_ActorFreePlaced(void);
void abi_TileAtProbeLong(void);

/*
 * @routine 83:C903 ActorFallTileTest
 * WHAT IT DOES: For a falling actor: the map tile 10 pixels below its
 * position, Z set when its kind is 2; an actor above the level's top (y
 * negative) is ended instead.
 * HOW: Its x pixels to $000C and its y pixels plus 10 to $000E,
 * TileAtProbeLong, the kind ($0012) compared with 2. Its y negative: its
 * return address dropped (PLA PLA, 8-bit), then into ActorEndOutOfRange
 * after its pulls: ActorFreePlaced and the actor loop.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor's code at its own level (83:C85A). Exit: M 16-bit (as
 * TileAtProbeLong left it); A low = the kind; Z set when it is 2, C from
 * the comparison. Above the top: does not return.
 */
void abi_ActorFallTileTest(void)
{
    goto L_83C903;
L_8396CA:
    abi_ActorFreePlaced();  /* 8396CA jsr */
    actor_leave(0x8394BFu); return;  /* 8396CD jmp */
L_83C903:
    op_rep(0x21);  /* 83C903 rep */
    lda16(ea_dp(0x0C));  /* 83C905 lda */
    sta16(ea_abs(SCRATCH));  /* 83C907 sta */
    lda16(ea_dp(0x0F));  /* 83C90A lda */
    if (IF_N) goto L_83C91E;  /* 83C90C bmi */
    adc16_v(0x000A);  /* 83C90E adc */
    sta16(ea_abs(0x000E));  /* 83C911 sta */
    abi_TileAtProbeLong();  /* 83C914 jsl */
    lda8(ea_abs(V_0012));  /* 83C918 lda */
    cmp8_v(0x02);  /* 83C91B cmp */
    return;  /* 83C91D rts */
L_83C91E:
    op_sep(0x20);  /* 83C91E sep */
    pla8();  /* 83C920 pla */
    pla8();  /* 83C921 pla */
    goto L_8396CA;  /* 83C922 jmp */
}

