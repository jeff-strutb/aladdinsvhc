/*
 * kind41.c: actor kind $41's start. With $07 bits 1-2 clear it is the
 * walker: it rises out of the ground when Aladdin is in reach, starts the
 * layer 1 wobble, hops twice and walks, hopping again where its ground
 * ends. With them set it is a maker of walkers: one each time Aladdin is
 * in reach (bits 3-4 clear), or one every so often at its own place.
 * Its wait is Kind41Wait (kind_waits_rest.c).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_PlayerInReach(void);
void abi_ActorSpawnLookWait(void);
void abi_AnimStart(void);
void abi_ActorRiseWhenInReach(void);
void abi_TaskCreate(void);
void abi_SoundQueueAdd(void);
void abi_ActorHopFromTable(void);
void abi_ActorSpeedYSet(void);
void abi_ActorMoveX(void);
void abi_ActorOutOfWallProbe4(void);
void abi_ActorSnapToGround(void);

#define K41_LOOKS 0xD29Fu       /* its look table */
#define K41_LOOK_AT 0x16u       /* where its look table is kept (word) */
#define K41_ACTIVE 0x42u        /* nonzero: the walker (Kind41Wait: it turns shots back, Aladdin can stand on it) */
#define K41_FRAMES 0x43u        /* Kind41Wait: frames before he can stand on it again */
#define K41_PAUSE 0x41u         /* the maker: frames between its walkers */
#define K41_PAUSES 0xD2A7u      /* bank $80: the maker's pause by $07 bits 1-2 */
#define K41_SPAWN_ATS 0xD2AEu   /* bank $80: the timed maker's spawn point by $07 bits 3-4 */
#define K41_SPAWN_LOOK 0x4Du    /* ActorSpawnLookWait's look: its own kind */
#define K41_X_LIMIT 0x48u       /* a walker the timed maker makes: the x (word) its side is judged by */
#define K41_Y_LIMIT 0x4Au       /* and the y (word) below which it may end */
#define K41_REACH_SPAWN 0x2Eu   /* the maker in reach: its spawn point */
#define K41_SRAM 0x701200u      /* cartridge RAM (none on a retail cartridge): the animation it picks */
#define K41_HOP_SOUND 0x1Bu     /* the sound of each hop */

/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/* TaskCreate at `site`, its five argument bytes after the JSL: its frame
   as the JSL pushes it; TaskCreate returns past them by its own RTL and
   the C goes on. */
static void task_create(uint32_t site)
{
    jsl_frame(site);
    abi_TaskCreate();
    cpu.unwound = 0;
}

/* The maker in reach ($07 bits 3-4 clear): each frame, when Aladdin is
   in reach, one more of its kind, then its pause. */
static void k41_reach_maker(void)
{
    lda8_v(K41_REACH_SPAWN);
    sta8(ea_dp(ACTOR_SPAWN_AT));
    for (;;) {
        actor_yield(0x83B253u);
        ldx8_v(0x05);
        call_frame(0xB259, abi_PlayerInReach);
        op_sep(0x20);
        if (IF_C)
            continue;
        call_frame(0xB260, abi_ActorSpawnLookWait);
        lda8(ea_dp(K41_PAUSE));
        actor_wait(0x83B263u);
    }
}

/* The timed maker ($07 bits 3-4 set, in A): one more of its kind each
   pause, given its x and y as limits. */
static void k41_timed_maker(void)
{
    ACC8(lsr);
    ACC8(lsr);
    ACC8(lsr);
    tax8();
    lda8(ea_absx(K41_SPAWN_ATS));
    sta8(ea_dp(ACTOR_SPAWN_AT));
    for (;;) {
        actor_yield(0x83B270u);
        call_frame(0xB274, abi_ActorSpawnLookWait);
        ldy8(ea_abs(ACTOR_FREE_TOP));
        iny8();
        iny8();
        op_rep(0x30);
        ldx16(ea_absy(ACTOR_FREE_LIST));
        lda16(ea_dp(ACTOR_X + 1));
        sta16(ea_absx(K41_X_LIMIT));
        lda16(ea_dp(ACTOR_Y + 1));
        sta16(ea_absx(K41_Y_LIMIT));
        op_sep(0x30);
        lda8(ea_dp(K41_PAUSE));
        actor_wait(0x83B28Du);
    }
}

/*
 * @routine 83:B201 Kind41Start
 * @actor
 * WHAT IT DOES: Actor kind $41. The walker ($07 bits 1-2 clear): faces by
 * $07 bit 0, rises out of the ground when Aladdin is in reach, starts the
 * layer 1 wobble (Bg1WobbleTask), hops twice with sound $1B, then walks,
 * and where it finds no ground under it hops again. The maker (bits 1-2
 * set): with bits 3-4 clear, each time Aladdin is in reach (entry 5) it
 * makes one more of its kind ($07 AND $19, so a walker), then pauses; with
 * them set it makes one every pause at its spawn point and gives it its
 * own x and y as that walker's limits.
 * HOW: $08 bit 1, $09 bits 7 and 4 set; $16 K41_LOOKS; K41_ACTIVE and
 * K41_FRAMES 0. The walker: facing and flip from $07 bit 0, K41_ACTIVE 1;
 * AnimStart with (K41_SRAM less itself after two DECs: 0 with no cartridge
 * RAM); ActorRiseWhenInReach entry 0; TaskCreate (slot 4, parameter 4,
 * Bg1WobbleTask); SoundQueueAdd $1B, ActorHopFromTable 0, the sound, hop
 * 5, the sound; AnimStart 1; ActorSpeedYSet $1E. Each frame (BRK first):
 * ActorMoveX, ActorOutOfWallProbe4 (C set: LDA $14, EOR 1, LDA $14, LDA
 * $15, which changes nothing but the flags), ActorSnapToGround; C clear:
 * ActorHopFromTable $1E and the sound. The maker: facing and flip 0;
 * K41_PAUSE from K41_PAUSES by $07 bits 1-2; $07 AND $19; $08 bit 3
 * cleared; K41_SPAWN_LOOK its kind. Bits 3-4 clear: $21 $2E; each frame
 * (BRK first) PlayerInReach entry 5; in reach: ActorSpawnLookWait, COP
 * K41_PAUSE. Set: $21 from K41_SPAWN_ATS; each turn (BRK first)
 * ActorSpawnLookWait, the record it took (the free list's entry above its
 * top) given K41_X_LIMIT and K41_Y_LIMIT its x and y, COP K41_PAUSE.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $41 in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind41Start(void)
{
    uint8_t sram;

    lda8(ea_dp(ACTOR_08));
    ora8_v(0x02);
    sta8(ea_dp(ACTOR_08));
    lda8(ea_dp(ACTOR_09));
    ora8_v(0x90);
    sta8(ea_dp(ACTOR_09));
    op_rep(0x20);
    lda16_v(K41_LOOKS);
    sta16(ea_dp(K41_LOOK_AT));
    op_sep(0x20);
    stz8(ea_dp(K41_ACTIVE));
    stz8(ea_dp(K41_FRAMES));
    lda8(ea_dp(ACTOR_07));
    bit8_i(0x06);
    if (!IF_Z) {
        stz8(ea_dp(ACTOR_FACING));
        stz8(ea_dp(ACTOR_FLIP));
        tay8();
        ACC8(lsr);
        and8_v(0x03);
        tax8();
        lda8(ea_absx(K41_PAUSES));
        sta8(ea_dp(K41_PAUSE));
        tya8();
        and8_v(0x19);
        sta8(ea_dp(ACTOR_07));
        lda8(ea_dp(ACTOR_08));
        and8_v(0xF7);
        sta8(ea_dp(ACTOR_08));
        lda8(ea_dp(ACTOR_06));
        sta8(ea_dp(K41_SPAWN_LOOK));
        tya8();
        and8_v(0x18);
        if (IF_Z)
            k41_reach_maker();
        else
            k41_timed_maker();
        return;
    }
    lda8(ea_dp(ACTOR_07));
    and8_v(0x01);
    sta8(ea_dp(ACTOR_FACING));
    sta8(ea_dp(ACTOR_FLIP));
    MEM8(inc, ea_dp(K41_ACTIVE));
    /* the walker */
    lda8(ea_long(K41_SRAM));
    pha8();
    ACC8(dec);
    ACC8(dec);
    sta8(ea_long(K41_SRAM));
    pla8();
    sec();
    sram = rd8(ea_long(K41_SRAM));
    sbc8_v(sram);
    jsl_call(0x83B2A2u, abi_AnimStart);
    ldx8_v(0x00);
    call_frame(0xB2AA, abi_ActorRiseWhenInReach);
    task_create(0x83B2ABu);
    lda8_v(K41_HOP_SOUND);
    jsl_call(0x83B2B6u, abi_SoundQueueAdd);
    ldx8_v(0x00);
    call_frame(0xB2BE, abi_ActorHopFromTable);
    lda8_v(K41_HOP_SOUND);
    jsl_call(0x83B2C1u, abi_SoundQueueAdd);
    ldx8_v(0x05);
    call_frame(0xB2C9, abi_ActorHopFromTable);
    lda8_v(K41_HOP_SOUND);
    jsl_call(0x83B2CCu, abi_SoundQueueAdd);
    lda8_v(0x01);
    jsl_call(0x83B2D2u, abi_AnimStart);
    ldy8_v(0x1E);
    jsl_call(0x83B2D8u, abi_ActorSpeedYSet);
    for (;;) {
        actor_yield(0x83B2DCu);
        jsl_call(0x83B2DEu, abi_ActorMoveX);
        jsl_call(0x83B2E2u, abi_ActorOutOfWallProbe4);
        if (IF_C) {
            lda8(ea_dp(ACTOR_FACING));
            eor8_v(0x01);
            lda8(ea_dp(ACTOR_FACING));
            lda8(ea_dp(ACTOR_FLIP));
        }
        jsl_call(0x83B2F0u, abi_ActorSnapToGround);
        if (IF_C)
            continue;
        ldx8_v(0x1E);
        call_frame(0xB2FA, abi_ActorHopFromTable);
        lda8_v(K41_HOP_SOUND);
        jsl_call(0x83B2FDu, abi_SoundQueueAdd);
    }
}
