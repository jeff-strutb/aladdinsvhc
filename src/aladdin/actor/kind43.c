/*
 * kind43.c: actor kind $43's start.
 */
#include "machine.h"
#include "native.h"
#include "ops65.h"

void abi_ActorEndPlaced(void);
void abi_ActorRiseWhenInReach(void);
void abi_ActorSpawnLookWait(void);
void abi_AnimStart(void);
void abi_SoundQueueAdd(void);

#define K43_WAYS 0xB3B5u        /* its ways by $07 (words, bank $83) */
#define K43_BOXES 0xD2BEu       /* the carry test's box by $07 (bytes, DB) */
#define K43_PIECES 0xD2C1u      /* by $07: pieces, then the first look (byte pairs, DB) */
#define K43_ANIMS 0xD2C7u       /* its animation by $07 (bytes, DB) */
#define PLACEMENT_FLAGS 0x7EC600u /* by placement: $80 once it is gone */

/*
 * @routine 83:B393 Kind43Start
 * @actor
 * WHAT IT DOES: Actor kind $43: a platform the player can ride. Ways 0-2
 * ($07): it carries him for $18 frames once he is on it, then with sound
 * $1A shows its next animation, waits $15 frames, spawns 3 (way 0) or 2
 * actors of its own kind as way 3, each of a look 2 past the one before,
 * marks its placement gone and ends. Way 3: no carrying; it rises when
 * Aladdin is in reach, then ends.
 * HOW: $08 bit 1 and $09 bits $90 set; $14, $15, CARRIES ($43) 0; its
 * look table ($16) $D2BA; JMP ($B3B5,X) by $07. Ways 0-2 ($83:B3BD): $42
 * (the carry count, which Kind43Wait's carry test runs on) $18, its box
 * ($4C) and animation from K43_BOXES and K43_ANIMS by $07; each frame
 * (BRK) until CARRIES is set; then $42 frames counted down; SoundQueueAdd
 * $1A; AnimStart its animation + 1; $40 = $15 frames, CARRIES 0; $40 and
 * $21 from K43_PIECES by $07 * 2; $07 = 3, $4D = its kind ($06); $40 times
 * ActorSpawnLookWait and $21 up by 2; its placement's flag ($7E:C600 by
 * $34) $80; JML ActorEndPlaced. Way 3 ($83:B426): $42 0; AnimStart 6;
 * each frame (BRK) X = 0 and ActorRiseWhenInReach until C is clear; JML
 * ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by the
 * actor loop's jump for kind $43 in state $0C. Exit: never returns.
 */
void abi_Kind43Start(void)
{
    lda8(ea_dp(0x08));
    ora8_v(0x02);
    sta8(ea_dp(0x08));
    lda8(ea_dp(0x09));
    ora8_v(0x90);
    sta8(ea_dp(0x09));
    stz8(ea_dp(0x14));
    stz8(ea_dp(0x15));
    stz8(ea_dp(0x43));
    op_rep(0x20);
    lda16_v(0xD2BA);
    sta16(ea_dp(0x16));
    op_sep(0x20);
    lda8(ea_dp(0x07));
    ACC8(asl);
    tax8();
    switch (cpu.x) {            /* JMP (K43_WAYS,X) */
    case 0: case 2: case 4: goto L_B3BD;
    case 6: goto L_B426;
    default:                    /* past the table's 4 words: no $07 names one */
        jump_routine(0x830000u | rd16(0x830000u | (uint16_t)(K43_WAYS + cpu.x)));
        return;
    }

L_B3BD:     /* ways 0-2: carries, then breaks */
    lda8_v(0x18);
    sta8(ea_dp(0x42));
    ldx8(ea_dp(0x07));
    lda8(ea_absx(K43_BOXES));
    sta8(ea_dp(0x4C));
    ldx8(ea_dp(0x07));
    lda8(ea_absx(K43_ANIMS));
    jsl_frame(0x83B3CDu); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
    do {
        actor_yield(0x83B3D1u);
        lda8(ea_dp(0x43));
    } while (IF_Z);
    do {
        actor_yield(0x83B3D7u);
        MEM8(dec, ea_dp(0x42));
    } while (!IF_Z);
    lda8_v(0x1A);
    jsl_frame(0x83B3DFu);
    abi_SoundQueueAdd();
    cpu.s = (uint16_t)(cpu.s + 3);
    ldx8(ea_dp(0x07));
    lda8(ea_absx(K43_ANIMS));
    ACC8(inc);
    jsl_frame(0x83B3E9u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
    lda8_v(0x15);
    sta8(ea_dp(0x40));
    stz8(ea_dp(0x43));
    do {
        actor_yield(0x83B3F3u);
        MEM8(dec, ea_dp(0x40));
    } while (!IF_Z);
    lda8(ea_dp(0x07));
    ACC8(asl);
    tax8();
    lda8(ea_absx(K43_PIECES));
    sta8(ea_dp(0x40));
    lda8(ea_absx(K43_PIECES + 1));
    sta8(ea_dp(0x21));
    lda8_v(0x03);
    sta8(ea_dp(0x07));
    lda8(ea_dp(0x06));
    sta8(ea_dp(0x4D));
    do {
        call_frame(0xB411, abi_ActorSpawnLookWait);   /* JSR at 83:B40F */
        MEM8(inc, ea_dp(0x21));
        MEM8(inc, ea_dp(0x21));
        MEM8(dec, ea_dp(0x40));
    } while (!IF_Z);
    ldx8(ea_dp(0x34));
    lda8_v(0x80);
    sta8(ea_longx(PLACEMENT_FLAGS));
    abi_ActorEndPlaced();
    return;

L_B426:     /* way 3: rises when Aladdin is in reach */
    stz8(ea_dp(0x42));
    lda8_v(0x06);
    jsl_frame(0x83B42Au); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
    do {
        actor_yield(0x83B42Eu);
        ldx8_v(0x00);
        call_frame(0xB434, abi_ActorRiseWhenInReach);  /* JSR at 83:B432 */
    } while (IF_C);
    abi_ActorEndPlaced();
}
