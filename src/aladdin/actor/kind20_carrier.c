/*
 * kind20_carrier.c: actor kind $20's guard with $07 bit 4, which stands
 * with a pot (kind $42, kind42.c) over its head and throws it at Aladdin
 * when he comes near and it sees him; then it walks its beat as another
 * guard, or charges. No level places such a guard (its placements' $07
 * are $21, $22, $29 and $80), so no play runs it.
 */
#include "machine.h"
#include "native.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"

void abi_ActorSideOfPlayer(void);
void abi_Actor44Set40And80(void);
void abi_AnimStart(void);
void abi_ActorCreate(void);
void abi_ActorSpawnPlace(void);
void abi_ActorNearPlayer(void);
void abi_ActorSeesPlayerLine(void);
void abi_ActorPaletteFromRom(void);
void abi_Kind20CarrierWalks(void);
void abi_Kind20CarrierWaits(void);

#define PLACE 0x07u             /* its placement's byte: bit 0 its facing, bit 1 the pot's way (to its $07) */
#define POT 0x42u               /* the pot's record (word) */
#define SIGNAL 0x4Fu            /* set by the pot: 1 it hit Aladdin, 2 it passed him */
#define COPY_LEFT 0x000Eu       /* words of the records left to swap */
#define COPY_TMP 0x0010u        /* the pot's word being swapped */
#define KIND_POT 0x42

/* A JSL at `site`, its return address on the stack while the callee runs. */
#define JSL(site, fn) do { jsl_frame(site); fn(); cpu.s = (uint16_t)(cpu.s + 3); } while (0)

/*
 * @routine 83:9AB9 Kind20CarrierWalks
 * @actor
 * WHAT IT DOES: The guard without its pot (none could be made, or it is
 * gone) becomes a walking guard: facing Aladdin, $07 that facing with
 * bits 1 and 4 ($12), its beat from here; on as Kind20Start's walk.
 * HOW: 8-bit registers; ActorSideOfPlayer OR $12 into PLACE;
 * Actor44Set40And80; JMP to Kind20StartWalk.
 * Entry: D = the guard's record; DB = $80; reached by Kind20Carrier's
 * branch (ActorCreate failed) and its JMP (the pot gone, or its throw
 * done). Exit: never returns.
 */
void abi_Kind20CarrierWalks(void)
{
    op_sep(0x30);
    JSL(0x839ABBu, abi_ActorSideOfPlayer);
    ora8_v(0x12);
    sta8(ea_dp(PLACE));
    call_frame(0x9AC5, abi_Actor44Set40And80);
    jump_routine(0x8397E5u);
}

/* The pot's record comes first: the guard's resume address made
   $83:9B53 (its P with it), the two records swapped word by word, and on
   as the pot (JMP Kind42Start); the guard goes on at Kind20CarrierWaits
   from the pot's old record. M and X 16-bit, X = the pot's record. */
static void swap_with_pot(void)
{
    txa16();
    sta16(ea_absx(ACTOR_40));
    lda16(ea_abs(SCRATCH));
    sta16(ea_dp(POT));
    phk(0x83);
    lda16_v(0x9B53);
    pha16();
    php();
    pla16();
    sta16(ea_dp(ACTOR_STATE + 2));
    pla16();
    sta16(ea_dp(ACTOR_STATE + 4));
    lda16_v(0x0028);
    sta16(ea_abs(COPY_LEFT));
    txy16();
    ldx16_v(0x0000);
    do {
        lda16(ea_absy(ACTOR_STATE));
        sta16(ea_abs(COPY_TMP));
        lda16(ea_dpx(ACTOR_STATE));
        sta16(ea_absy(ACTOR_STATE));
        lda16(ea_abs(COPY_TMP));
        sta16(ea_dpx(ACTOR_STATE));
        inx16();
        inx16();
        iny16();
        iny16();
        MEM16(dec, ea_abs(COPY_LEFT));
    } while (!IF_Z);
    op_sep(0x30);
    jump_routine(0x83C5DEu);
}

/*
 * @routine 83:9AC9 Kind20Carrier
 * @actor
 * WHAT IT DOES: Kind $20's guard with PLACE bit 4: makes its pot (kind
 * $42) over its head and waits for Aladdin; when he is near and it sees
 * him, turns to him and throws (the pot flies when its animation shows
 * frame $15); then, told by the pot that it hit him, laughs and walks
 * (Kind20CarrierWalks); that it passed him, turns red and charges
 * (Kind20Spotted); the pot gone first, walks.
 * HOW: Facing and flip = A AND 1; AnimStart 6; spawn point $20; SIGNAL
 * 0; ActorCreate kind $42 (C set: Kind20CarrierWalks). The pot: state
 * $0C, $34 $FF, $07 = PLACE AND 2, facing and flip from the guard's,
 * ActorSpawnPlace; its $40 = this record, POT = its record. When the pot's
 * record comes first, the guard's own resume address is made
 * $83:9B53 (PHK, PEA-like PHA, PHP: P and the address into $02-$05), the
 * two records swapped word by word ($28 words) and on at Kind42Start as
 * the pot (JMP), the guard to go on at Kind20CarrierWaits in the pot's
 * old record; else each frame (BRK) Kind20CarrierWaits.
 * Entry: M 8-bit, X 8-bit; A = PLACE; D = the guard's record; DB = $80;
 * reached by Kind20Start's JMP. Exit: never returns.
 */
void abi_Kind20Carrier(void)
{
    and8_v(0x01);
    sta8(ea_dp(ACTOR_FACING));
    sta8(ea_dp(ACTOR_FLIP));
    lda8_v(0x06);
    JSL(0x839AD1u, abi_AnimStart);
    lda8_v(0x20);
    sta8(ea_dp(ACTOR_SPAWN_AT));
    stz8(ea_dp(SIGNAL));
    lda8_v(KIND_POT);
    JSL(0x839ADDu, abi_ActorCreate);
    if (IF_C) {
        abi_Kind20CarrierWalks();       /* its branch there */
        return;
    }
    lda8_v(0x0C);
    sta8(ea_absx(ACTOR_STATE));
    lda8_v(0xFF);
    sta8(ea_absx(ACTOR_34));
    lda8(ea_dp(PLACE));
    and8_v(0x02);
    sta8(ea_absx(ACTOR_07));
    lda8(ea_dp(ACTOR_FACING));
    sta8(ea_absx(ACTOR_FACING));
    eor8_v(0x01);
    sta8(ea_absx(ACTOR_FLIP));
    JSL(0x839AFEu, abi_ActorSpawnPlace);
    op_rep(0x20);
    tdc();
    sta16(ea_abs(SCRATCH));
    sta16(ea_absx(ACTOR_40));
    stx16(ea_dp(POT));
    cpx16_v(rd16(ea_abs(SCRATCH)));
    if (!IF_C) {
        swap_with_pot();
        return;
    }
    op_sep(0x30);
    actor_yield(0x839B51u);
    abi_Kind20CarrierWaits();           /* resumed at its BRK's end */
}

/*
 * @routine 83:9B53 Kind20CarrierWaits
 * @actor
 * WHAT IT DOES: The guard with its pot waits for Aladdin: near it (box
 * $1C) and in its sight, it turns to him, its pot turned too, and
 * throws: 16 frames, AnimStart 7, the pot placed at spawn point $3A, 31
 * frames, AnimStart 0; then each frame it waits for the pot's word: none
 * while the pot lives; the pot gone: Kind20CarrierWalks; SIGNAL 1: 32
 * frames, AnimStart 8, 96 frames, Kind20CarrierWalks; SIGNAL 2:
 * palette $20, AnimStart 9, 32 frames, palette 5, PLACE $10,
 * Actor44Set40And80, Kind20Spotted.
 * HOW: ActorNearPlayer (box $1C) and ActorSeesPlayerLine (C clear both),
 * else BRK and again. ActorSideOfPlayer as facing and flip, the pot's
 * the same and its flip the other, ActorSpawnPlace for the pot; COP $10;
 * AnimStart 7; spawn point $3A; ActorSpawnPlace; COP $1F; AnimStart 0.
 * Each frame (BRK first): the pot's state 0: JMP Kind20CarrierWalks;
 * SIGNAL 0: again; 1: COP $20, AnimStart 8, COP $60, JMP
 * Kind20CarrierWalks; else ActorPaletteFromRom Y = $20, AnimStart 9, COP
 * $20, ActorPaletteFromRom Y = 5, PLACE $10, Actor44Set40And80, JMP
 * Kind20Spotted.
 * Entry: D = the guard's record; DB = $80; reached by the actor loop's RTI
 * at the guard's BRK before it, or (the records swapped) at the resume
 * address its code wrote. Exit: never returns.
 */
void abi_Kind20CarrierWaits(void)
{
    for (;;) {
        op_sep(0x30);
        ldy8_v(0x1C);
        JSL(0x839B57u, abi_ActorNearPlayer);
        if (!IF_C) {
            call_frame(0x9B5F, abi_ActorSeesPlayerLine);
            if (!IF_C)
                break;
        }
        actor_yield(0x839B51u);
    }
    JSL(0x839B62u, abi_ActorSideOfPlayer);
    sta8(ea_dp(ACTOR_FACING));
    sta8(ea_dp(ACTOR_FLIP));
    op_rep(0x10);
    ldx16(ea_dp(POT));
    lda8(ea_dp(ACTOR_FACING));
    sta8(ea_absx(ACTOR_FACING));
    eor8_v(0x01);
    sta8(ea_absx(ACTOR_FLIP));
    JSL(0x839B78u, abi_ActorSpawnPlace);
    op_sep(0x10);
    lda8_v(0x10);
    actor_wait(0x839B80u);
    lda8_v(0x07);
    JSL(0x839B84u, abi_AnimStart);
    lda8_v(0x3A);
    sta8(ea_dp(ACTOR_SPAWN_AT));
    op_rep(0x10);
    ldx16(ea_dp(POT));
    JSL(0x839B90u, abi_ActorSpawnPlace);
    op_sep(0x10);
    lda8_v(0x1F);
    actor_wait(0x839B98u);
    lda8_v(0x00);
    JSL(0x839B9Cu, abi_AnimStart);
    for (;;) {
        actor_yield(0x839BA0u);
        op_rep(0x10);
        ldx16(ea_dp(POT));
        lda8(ea_absx(ACTOR_STATE));
        op_sep(0x10);
        if (IF_Z) {                     /* the pot gone */
            jump_routine(0x839AB9u);
            return;
        }
        lda8(ea_dp(SIGNAL));
        if (IF_Z)
            continue;
        ACC8(dec);
        if (!IF_Z)
            break;
        lda8_v(0x20);                   /* it hit Aladdin */
        actor_wait(0x839BB6u);
        lda8_v(0x08);
        JSL(0x839BBAu, abi_AnimStart);
        lda8_v(0x60);
        actor_wait(0x839BC0u);
        jump_routine(0x839AB9u);
        return;
    }
    ldy8_v(0x20);                       /* it passed him */
    JSL(0x839BC7u, abi_ActorPaletteFromRom);
    lda8_v(0x09);
    JSL(0x839BCDu, abi_AnimStart);
    lda8_v(0x20);
    actor_wait(0x839BD3u);
    ldy8_v(0x05);
    JSL(0x839BD7u, abi_ActorPaletteFromRom);
    lda8_v(0x10);
    sta8(ea_dp(PLACE));
    call_frame(0x9BE1, abi_Actor44Set40And80);
    jump_routine(0x839894u);
}
