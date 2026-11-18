/*
 * kind23_25.c: actor kinds $23 and $25. Kind $23 makes four children of
 * its kind across the view, each of which shows itself, waits and makes a
 * kind $6D, then ends; kind $25 plays sound $3E every 291 frames while
 * $1423 is 0. Their waits are Kind23Wait (kind_waits_rest.c) and the
 * loop's default.
 */
#include "machine.h"
#include "native.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorSpawnFacing(void);
void abi_ActorCreateChild8(void);
void abi_AnimStart(void);
void abi_SoundQueueAdd(void);
void abi_ActorEndPlaced(void);

#define K23_PART 0x07u          /* 0: the maker; 1-4: a child, its place in the tables */
#define K23_XS 0xD06Eu          /* (data bank $80) the children's x (words), by part */
#define K23_NEXT_07 0xD076u     /* (data bank $80) the $07 each child gives its kind $6D (bytes), by part */
#define K23_LOOK 0x2Bu          /* sprite attributes (word) */
#define V_1423 0x1423u          /* nonzero: kind $25 keeps quiet */

/* A JSL at `site`, its return address on the stack while the callee runs. */
#define JSL(site, fn) do { jsl_frame(site); fn(); cpu.s = (uint16_t)(cpu.s + 3); } while (0)

/*
 * @routine 83:9CEB Kind23Start
 * @actor
 * WHAT IT DOES: Actor kind $23's start: with PART 0 it makes four children
 * of its kind (parts 4 down to 1) and ends; a child takes its x from
 * K23_XS, y $40, look $0600, shows animation $19 for $1A frames, makes a
 * kind $6D with the $07 of K23_NEXT_07, and ends.
 * HOW: PART 0: spawn point 0; PART = 4; ActorSpawnFacing kind $23 with Y =
 * PART while PART counts down; Kind23Second (BRA). Else X = (PART - 1) * 2:
 * x pixels = K23_XS word, look $0600, y pixels low $40, ACTOR_08 OR 0,
 * ACTOR_09 bits $90; AnimStart $19; COP $1A; PART = K23_NEXT_07 by PART - 1;
 * spawn point 0; ActorCreateChild8 kind $6D; Kind23Second (BRA).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by the
 * actor loop's jump for kind $23 (the table at $83:8CAB). Exit: never
 * returns.
 */
void abi_Kind23Start(void)
{
    lda8(ea_dp(K23_PART));
    if (IF_Z) {
        stz8(ea_dp(ACTOR_SPAWN_AT));
        lda8_v(0x04);
        sta8(ea_dp(K23_PART));
        do {
            lda8(ea_dp(K23_PART));
            tay8();
            lda8_v(0x23);
            JSL(0x839CFAu, abi_ActorSpawnFacing);
            MEM8(dec, ea_dp(K23_PART));
        } while (!IF_Z);
        jump_routine(0x839D43u);        /* its BRA there */
        return;
    }
    lda8(ea_dp(K23_PART));
    ACC8(dec);
    ACC8(asl);
    tax8();
    op_rep(0x20);
    lda16(ea_absx(K23_XS));
    sta16(ea_dp(ACTOR_X + 1));
    lda16_v(0x0600);
    sta16(ea_dp(K23_LOOK));
    op_sep(0x20);
    lda8_v(0x40);
    sta8(ea_dp(ACTOR_Y + 1));
    lda8(ea_dp(ACTOR_08));
    ora8_v(0x00);
    sta8(ea_dp(ACTOR_08));
    lda8(ea_dp(ACTOR_09));
    ora8_v(0x90);
    sta8(ea_dp(ACTOR_09));
    lda8_v(0x19);
    JSL(0x839D29u, abi_AnimStart);
    lda8_v(0x1A);
    actor_wait(0x839D2Fu);
    ldx8(ea_dp(K23_PART));
    dex8();
    lda8(ea_absx(K23_NEXT_07));
    sta8(ea_dp(K23_PART));
    stz8(ea_dp(ACTOR_SPAWN_AT));
    lda8_v(0x6D);
    JSL(0x839D3Du, abi_ActorCreateChild8);
    jump_routine(0x839D43u);            /* its BRA there */
}

/*
 * @routine 83:9D43 Kind23Second
 * @actor
 * WHAT IT DOES: Kind $23's end, and where its actors go on from the actor
 * loop in a state with bit 7 set (the table at $83:8DFD): ActorEndPlaced.
 * HOW: JMP ActorEndPlaced.
 * Entry: D = the actor's record; reached by Kind23Start's branches and the
 * actor loop's jump. Exit: never returns.
 */
void abi_Kind23Second(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 83:9D4B Kind25Start
 * @actor
 * WHAT IT DOES: Actor kind $25's start: for good, every 291 frames (255,
 * then 36) it plays sound $3E unless $1423 is nonzero.
 * HOW: ACTOR_08 0; COP $FF; COP $24; $1423 nonzero: again; else sound $3E
 * and again.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the actor
 * loop's jump for kind $25 (the table at $83:8CAB). Exit: never returns.
 */
void abi_Kind25Start(void)
{
    stz8(ea_dp(ACTOR_08));
    for (;;) {
        lda8_v(0xFF);
        actor_wait(0x839D4Fu);
        lda8_v(0x24);
        actor_wait(0x839D53u);
        lda8(ea_abs(V_1423));
        if (!IF_Z)
            continue;
        lda8_v(0x3E);
        JSL(0x839D5Cu, abi_SoundQueueAdd);
    }
}
