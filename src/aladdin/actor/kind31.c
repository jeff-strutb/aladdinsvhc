/*
 * kind31.c: actor kind $31 (its start, its ways of waiting, its walk, its
 * second state) and the tiles it breaks.
 */
#include "machine.h"
#include "native.h"
#include "ops65.h"
#include "wram.h"
#include "wram_player.h"

void abi_ActorAimAt(void);
void abi_ActorFacePlayer(void);
void abi_ActorFall(void);
void abi_ActorGoToPoint(void);
void abi_ActorHopFromTable(void);
void abi_ActorMoveRight(void);
void abi_ActorMoveX(void);
void abi_ActorMoveY(void);
void abi_ActorOutOfWallProbe4(void);
void abi_ActorPaletteFromRom(void);
void abi_ActorProbeAheadInGround(void);
void abi_ActorRiseWhenInReach(void);
void abi_ActorSnapToGround(void);
void abi_ActorSpawnLookWait(void);
void abi_ActorSpawnSameLook(void);
void abi_ActorSpeedAddVector(void);
void abi_ActorSpeedSetVector(void);
void abi_ActorSpeedXSet(void);
void abi_ActorSpeedYSet(void);
void abi_ActorSpeedYSetGravity(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_RandomFromList(void);
void abi_SoundQueueAdd(void);
void abi_TileChangeAdd(void);
void abi_Kind31Drop(void);
void abi_Kind31Walk(void);
void abi_Kind31BreakTiles(void);

#define PLAYER_X 0x08E7u        /* Aladdin's x (word) */
#define PLAYER_Y 0x08EAu        /* his y (word) */
#define K31_WAYS_LOW 0xAC17u    /* its ways below level 2 (words, bank $83) */
#define K31_WAYS 0xAC36u        /* from level 2 */
#define K31_WALKS 0xADC9u       /* its walks by $07 (words, bank $83) */
#define PLACEMENT_FLAGS 0x7EC600u /* by placement: $80 once its tiles are broken */

/* A JSL at `site` to a routine whose C uses the stack: its return address
   on the stack while the callee runs, as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:ABEA Kind31Start
 * @actor
 * WHAT IT DOES: Actor kind $31: by its $07 (bits 0-2) one of eight ways
 * of waiting for Aladdin, then on into its walk (Kind31Walk); from level 2
 * on its palette from the ROM and another table of ways.
 * HOW: $08 bit 1 set, bit 3 clear; $09 |= $90; $16 = $D18B; $42, $14,
 * $15 0. Level below 2: JMP ($AC17,X) by $07 AND 7, times 2; else
 * ActorPaletteFromRom (Y $3F) and JMP ($AC36,X). The ways: rise when in
 * reach; wait till he is $41-$60 left of it (then 1, $20 or $40 frames),
 * or $21-$40 right, or within $20 of its x plus $45 ($70, $30, 0, $C0),
 * or within its next $20; then a spawn of kind $4D's look
 * (ActorSpawnLookWait) and a hop (ActorHopFromTable), or a random wait,
 * its tiles broken (Kind31BreakTiles) and a hop; way 7: Kind31Drop.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by the
 * actor loop's jump for kind $31 in state $0C. Exit: never returns.
 */
void abi_Kind31Start(void)
{
    lda8_v(0x02);
    tsb8(ea_dp(0x08));
    lda8_v(0x08);
    trb8(ea_dp(0x08));
    lda8(ea_dp(0x09));
    ora8_v(0x90);
    sta8(ea_dp(0x09));
    op_rep(0x20);
    lda16_v(0xD18B);
    sta16(ea_dp(0x16));
    op_sep(0x20);
    stz8(ea_dp(0x42));
    stz8(ea_dp(0x14));
    stz8(ea_dp(0x15));
    lda8(ea_abs(LEVEL_035C));
    cmp8_v(0x02);
    if (IF_C) {
        ldy8_v(0x3F);
        jsl_call(0x83AC29u, abi_ActorPaletteFromRom);
        lda8(ea_dp(0x07));
        and8_v(0x07);
        ACC8(asl);
        tax8();
        switch (cpu.x) {        /* JMP (K31_WAYS,X) */
        case 0: goto L_AC46;
        case 2: goto L_ACEF;
        case 4: goto L_AD46;
        case 6: goto L_AD4E;
        case 8: goto L_AD5E;
        case 10: goto L_AD75;
        case 12: goto L_AD56;
        default: abi_Kind31Drop(); return;
        }
    }
    lda8(ea_dp(0x07));
    and8_v(0x07);
    ACC8(asl);
    tax8();
    switch (cpu.x) {            /* JMP (K31_WAYS_LOW,X) */
    case 0: goto L_AC46;
    case 2: goto L_AC6F;
    case 4: goto L_ACCC;
    case 6: goto L_ACD2;
    case 8: case 10: case 12: goto L_ACD8;
    default: abi_Kind31Drop(); return;
    }

L_AC46:     /* way 0: rise when Aladdin is in reach */
    actor_yield(0x83AC46u);
    lda8_v(0x00);
    abi_AnimStart();
    lda8_v(0x08);
    tsb8(ea_dp(0x08));
    ldx8_v(0x00);
    call_frame(0xAC56, abi_ActorRiseWhenInReach);   /* JSR at 83:AC54 */
    lda8_v(0x02);
    abi_AnimStart();
    lda8_v(0x08);
    actor_wait(0x83AC5Fu);
    lda8_v(0x00);
    abi_AnimStart();
    MEM8(inc, ea_dp(0x42));
    abi_ActorFacePlayer();
    abi_Kind31Walk();
    return;

L_ACCC:     /* way 2 (low): $20 frames once he is left of it */
    lda8_v(0x20);
    sta8(ea_dp(0x40));
    goto L_AC73;
L_ACD2:     /* way 3 (low): $40 frames */
    lda8_v(0x40);
    sta8(ea_dp(0x40));
    goto L_AC73;
L_AC6F:     /* way 1 (low): a frame */
    lda8_v(0x01);
    sta8(ea_dp(0x40));
L_AC73:     /* till Aladdin is $41-$60 left of it */
    actor_yield(0x83AC73u);
    op_rep(0x20);
    clc();
    lda16(ea_abs(PLAYER_X));
    adc16_v(0x0060);
    sec();
    sbc16_v(rd16(ea_dp(0x0C)));
    cmp16_v(0x0020);
    op_sep(0x20);
    if (IF_C) goto L_AC73;
    do {
        actor_yield(0x83AC88u);
        MEM8(dec, ea_dp(0x40));
    } while (!IF_Z);
L_AC8E:
    lda8(ea_dp(0x09));
    bit8_i(0x40);
    if (!IF_Z) {
        lda8_v(0x1D);
        abi_SoundQueueAdd();
    }
    lda8_v(0x06);
    abi_AnimStart();
    lda8_v(0x08);
    tsb8(ea_dp(0x08));
    lda8_v(0x00);
    sta8(ea_dp(0x21));
    lda8_v(0x44);
    sta8(ea_dp(0x4D));
    call_frame(0xACAE, abi_ActorSpawnLookWait);     /* JSR at 83:ACAC */
    ldx8_v(0x14);
    call_frame(0xACB3, abi_ActorHopFromTable);      /* JSR at 83:ACB1 */
    lda8_v(0x02);
    abi_AnimStart();
    lda8_v(0x08);
    actor_wait(0x83ACBCu);
    lda8_v(0x00);
    abi_AnimStart();
    MEM8(inc, ea_dp(0x42));
    abi_ActorFacePlayer();
    abi_Kind31Walk();
    return;

L_ACD8:     /* ways 4-6 (low): till Aladdin is $21-$40 right of it */
    actor_yield(0x83ACD8u);
    op_rep(0x20);
    clc();
    lda16(ea_dp(0x0C));
    adc16_v(0x0040);
    sec();
    sbc16_v(rd16(ea_abs(PLAYER_X)));
    cmp16_v(0x0020);
    op_sep(0x20);
    if (IF_C) goto L_ACD8;
    goto L_AC8E;

L_AD46:     /* way 2: within $20 of its x less $30 */
    lda8_v(0x30);
    sta8(ea_dp(0x45));
    stz8(ea_dp(0x46));
    goto L_ACF5;
L_AD4E:     /* way 3: of its x */
    lda8_v(0x00);
    sta8(ea_dp(0x45));
    stz8(ea_dp(0x46));
    goto L_ACF5;
L_AD56:     /* way 6: of its x less $C0 */
    lda8_v(0xC0);
    sta8(ea_dp(0x45));
    stz8(ea_dp(0x46));
    goto L_ACF5;
L_ACEF:     /* way 1: of its x less $70 */
    lda8_v(0x70);
    sta8(ea_dp(0x45));
    stz8(ea_dp(0x46));
L_ACF5:
    actor_yield(0x83ACF5u);
    op_rep(0x20);
    clc();
    lda16(ea_dp(0x45));
    adc16_v(rd16(ea_abs(PLAYER_X)));
    sec();
    sbc16_v(rd16(ea_dp(0x0C)));
    cmp16_v(0x0020);
    op_sep(0x20);
    if (IF_C) goto L_ACF5;
L_AD09:     /* a random wait */
    ldx8_v(0x00);
    jsl_call(0x83AD0Bu, abi_RandomFromList);
    sta8(ea_dp(0x40));
L_AD11:
    do {
        actor_yield(0x83AD11u);
        MEM8(dec, ea_dp(0x40));
    } while (!IF_Z);
    call_frame(0xAD19, abi_Kind31BreakTiles);
    lda8_v(0x2E);
    abi_SoundQueueAdd();
    lda8_v(0x03);
    abi_AnimStart();
    lda8_v(0x08);
    tsb8(ea_dp(0x08));
    ldx8_v(0x14);
    call_frame(0xAD2E, abi_ActorHopFromTable);      /* JSR at 83:AD2C */
    lda8_v(0x02);
    abi_AnimStart();
    lda8_v(0x08);
    actor_wait(0x83AD37u);
    lda8_v(0x00);
    abi_AnimStart();
    MEM8(inc, ea_dp(0x42));
    abi_ActorFacePlayer();
    abi_Kind31Walk();
    return;

L_AD5E:     /* way 4: till he is within its next $20 */
    actor_yield(0x83AD5Eu);
    op_rep(0x20);
    clc();
    lda16(ea_dp(0x0C));
    adc16_v(0x0020);
    sec();
    sbc16_v(rd16(ea_abs(PLAYER_X)));
    cmp16_v(0x0020);
    op_sep(0x20);
    if (!IF_C) goto L_AD09;
    goto L_AD5E;

L_AD75:     /* way 5: the same, then a frame */
    actor_yield(0x83AD75u);
    op_rep(0x20);
    clc();
    lda16(ea_dp(0x0C));
    adc16_v(0x0020);
    sec();
    sbc16_v(rd16(ea_abs(PLAYER_X)));
    cmp16_v(0x0020);
    op_sep(0x20);
    if (IF_C) goto L_AD75;
    lda8_v(0x01);
    sta8(ea_dp(0x40));
    goto L_AD11;
}

/*
 * @routine 83:AD90 Kind31Drop
 * @actor
 * WHAT IT DOES: Kind $31's way 7 (and its second state's end): falls and
 * drifts for good.
 * HOW: $42 0; AnimStart 3; $08 bit 3 set; ActorSpeedXSet 6, ActorSpeedYSet
 * $0F; $20 = $28; each frame (BRK first) ActorMoveX, ActorFall.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * Kind31Start's table or Kind31Second's jump. Exit: never returns.
 */
void abi_Kind31Drop(void)
{
    stz8(ea_dp(0x42));
    lda8_v(0x03);
    jsl_frame(0x83AD94u);
    abi_AnimStart();
    cpu.s = (uint16_t)(cpu.s + 3);
    lda8_v(0x08);
    tsb8(ea_dp(0x08));
    ldy8_v(0x06);
    jsl_frame(0x83AD9Eu);
    abi_ActorSpeedXSet();
    cpu.s = (uint16_t)(cpu.s + 3);
    ldy8_v(0x0F);
    jsl_frame(0x83ADA4u);
    abi_ActorSpeedYSet();
    cpu.s = (uint16_t)(cpu.s + 3);
    lda8_v(0x28);
    sta8(ea_dp(0x20));
    for (;;) {
        actor_yield(0x83ADACu);
        jsl_frame(0x83ADAEu);
        abi_ActorMoveX();
        cpu.s = (uint16_t)(cpu.s + 3);
        jsl_frame(0x83ADB2u);
        abi_ActorFall();
        cpu.s = (uint16_t)(cpu.s + 3);
    }
}

/*
 * @routine 83:ADB8 Kind31Walk
 * @actor
 * WHAT IT DOES: Kind $31's walk once it has met Aladdin: by $07 (bits 3-4
 * at first) it walks, turning at edges and hopping up walls, or waits
 * near him, leaps, lands, flies to where he was and back, and rises.
 * HOW: $07 = ($07 AND $18) / 4; $40 = 1; JMP ($ADC9,X) by $07. 0: speed
 * $15; each frame (BRK first) ActorMoveX; a wall (ActorOutOfWallProbe4): the
 * wall ahead probed (ActorProbeAheadInGround X $10, Y $38), in ground: turn;
 * else AnimStart 1, COP $10, ActorHopFromTable $0F, AnimStart 2, COP 8,
 * AnimStart 0, again; off the ground (ActorSnapToGround C clear): turn
 * ($14 and $15 flipped). 2, 4: every $20 frames, Aladdin within $60 across
 * and $40 up or down: AnimStart 1, a leap (ActorSpeedYSetGravity $38, 3)
 * until it falls, sound $2F, AnimStart 4, $42 $80, COP 8, to his point
 * (ActorGoToPoint), back along its aim $30 frames, back (ActorGoToPoint),
 * ActorRiseWhenInReach 0, AnimStart 0, $42 1, $07 0, again.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * jumps from Kind31Start. Exit: never returns.
 */
void abi_Kind31Walk(void)
{
    lda8(ea_dp(0x07));
    and8_v(0x18);
    ACC8(lsr);
    ACC8(lsr);
    sta8(ea_dp(0x07));
    lda8_v(0x01);
    sta8(ea_dp(0x40));
L_ADC4:
    ldx8(ea_dp(0x07));
    switch (cpu.x) {            /* JMP (K31_WALKS,X) */
    case 0: goto L_ADCF;
    case 2: case 4: goto L_AE1B;
    default: jump_routine(0x830000u | rd16(0x830000u | (uint16_t)(K31_WALKS + cpu.x))); return;
    }

L_ADCF:     /* walk */
    ldy8_v(0x15);
    jsl_frame(0x83ADD1u);
    abi_ActorSpeedXSet();
    cpu.s = (uint16_t)(cpu.s + 3);
L_ADD5:
    actor_yield(0x83ADD5u);
    jsl_frame(0x83ADD7u);
    abi_ActorMoveX();
    cpu.s = (uint16_t)(cpu.s + 3);
    jsl_call(0x83ADDBu, abi_ActorOutOfWallProbe4);
    if (IF_C) goto L_ADF1;
    jsl_call(0x83ADE1u, abi_ActorSnapToGround);
    if (IF_C) goto L_ADC4;
L_ADE7:     /* turn */
    lda8(ea_dp(0x14));
    eor8_v(0x01);
    sta8(ea_dp(0x14));
    sta8(ea_dp(0x15));
    goto L_ADD5;
L_ADF1:     /* a wall: hop up it unless the wall goes on above */
    ldx8_v(0x10);
    ldy8_v(0x38);
    call_frame(0xADF7, abi_ActorProbeAheadInGround);  /* JSR at 83:ADF5 */
    if (IF_C) goto L_ADE7;
    lda8_v(0x01);
    abi_AnimStart();
    lda8_v(0x10);
    actor_wait(0x83AE02u);
    ldx8_v(0x0F);
    call_frame(0xAE08, abi_ActorHopFromTable);      /* JSR at 83:AE06 */
    lda8_v(0x02);
    abi_AnimStart();
    lda8_v(0x08);
    actor_wait(0x83AE11u);
    lda8_v(0x00);
    abi_AnimStart();
    goto L_ADCF;

L_AE1B:     /* wait near Aladdin, then leap at him */
    MEM8(dec, ea_dp(0x40));
    if (!IF_Z) goto L_ADCF;
    lda8_v(0x20);
    sta8(ea_dp(0x40));
    stz8(ea_abs(SCRATCH));
    op_rep(0x20);
    lda16(ea_dp(0x0C));
    sec();
    sbc16_v(rd16(ea_abs(PLAYER_X)));
    clc();
    adc16_v(0x0060);
    cmp16_v(0x00C0);
    op_sep(0x20);
    if (IF_C) goto L_ADCF;
    op_rep(0x20);
    lda16(ea_dp(0x0F));
    sec();
    sbc16_v(rd16(ea_abs(PLAYER_Y)));
    clc();
    adc16_v(0x0040);
    cmp16_v(0x0080);
    op_sep(0x20);
    if (IF_C) goto L_ADCF;      /* BCS $AE19, BRA $ADCF */
    lda8_v(0x01);
    abi_AnimStart();
    lda8_v(0x38);
    ldy8_v(0x03);
    jsl_frame(0x83AE56u);
    abi_ActorSpeedYSetGravity();
    cpu.s = (uint16_t)(cpu.s + 3);
    do {
        actor_yield(0x83AE5Au);
        jsl_frame(0x83AE5Cu);
        abi_ActorFall();
        cpu.s = (uint16_t)(cpu.s + 3);
        lda8(ea_dp(0x1F));
    } while (IF_N);
    lda8_v(0x2F);
    jsl_frame(0x83AE66u);
    abi_SoundQueueAdd();
    cpu.s = (uint16_t)(cpu.s + 3);
    lda8_v(0x04);
    abi_AnimStart();
    lda8_v(0x80);
    sta8(ea_dp(0x42));
    lda8_v(0x08);
    actor_wait(0x83AE76u);
    op_rep(0x20);
    lda16(ea_dp(0x0C));
    sta16(ea_dp(0x44));
    lda16(ea_dp(0x0F));
    sta16(ea_dp(0x46));
    lda16(ea_abs(PLAYER_X));
    sta16(ea_dp(0x48));
    lda16(ea_abs(PLAYER_Y));
    sta16(ea_dp(0x4A));
    ldx8_v(0x0A);
    stx8(ea_dp(0x4C));
    call_frame(0xAE92, abi_ActorGoToPoint);         /* JSR at 83:AE90 */
    op_rep(0x30);
    lda16(ea_dp(0x44));
    sta16(ea_dp(0x48));
    sta16(ea_abs(0x0010));
    lda16(ea_dp(0x46));
    sta16(ea_dp(0x4A));
    sta16(ea_abs(V_0012));
    jsl_frame(0x83AEA3u);
    abi_ActorAimAt();
    cpu.s = (uint16_t)(cpu.s + 3);
    sta8(ea_dp(0x45));
    eor8_v(0x10);
    sta8(ea_dp(0x14));
    ldx8_v(0x0A);
    jsl_frame(0x83AEAFu);
    abi_ActorSpeedSetVector();
    cpu.s = (uint16_t)(cpu.s + 3);
    lda8(ea_dp(0x45));
    sta8(ea_dp(0x14));
    lda8_v(0x30);
    sta8(ea_dp(0x40));
    do {
        actor_yield(0x83AEBBu);
        ldx8_v(0x0C);
        jsl_frame(0x83AEBFu);
        abi_ActorSpeedAddVector();
        cpu.s = (uint16_t)(cpu.s + 3);
        jsl_frame(0x83AEC3u);
        abi_ActorMoveRight();
        cpu.s = (uint16_t)(cpu.s + 3);
        jsl_frame(0x83AEC7u);
        abi_ActorMoveY();
        cpu.s = (uint16_t)(cpu.s + 3);
        MEM8(dec, ea_dp(0x40));
    } while (!IF_Z);
    call_frame(0xAED1, abi_ActorGoToPoint);         /* JSR at 83:AECF */
    ldx8_v(0x08);
    ldx8_v(0x00);
    call_frame(0xAED8, abi_ActorRiseWhenInReach);   /* JSR at 83:AED6 */
    lda8_v(0x00);
    abi_AnimStart();
    lda8_v(0x01);
    sta8(ea_dp(0x42));
    lda8(ea_dp(0x15));
    sta8(ea_dp(0x14));
    stz8(ea_dp(0x07));
    goto L_ADC4;
}

/*
 * @routine 83:AF1C Kind31Second
 * @actor
 * WHAT IT DOES: Kind $31's state $80: a sound, $0C frames of its
 * animation, a spawn of its own look, then on to Kind31Drop.
 * HOW: SoundQueueAdd $0F; state $0C, $11 0; $40 = $0C; AnimStart 2; $0C
 * frames (BRK first) of AnimStep; $07 = 7, $21 = 0; ActorSpawnSameLook of
 * its kind; $14, $15 = 1; $09 bit 7 set; JMP Kind31Drop.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $31 in state $80. Exit: never returns.
 */
void abi_Kind31Second(void)
{
    lda8_v(0x0F);
    jsl_frame(0x83AF1Eu);
    abi_SoundQueueAdd();
    cpu.s = (uint16_t)(cpu.s + 3);
    lda8_v(0x0C);
    sta8(ea_dp(0x00));
    stz8(ea_dp(0x11));
    lda8_v(0x0C);
    sta8(ea_dp(0x40));
    lda8_v(0x02);
    jsl_frame(0x83AF2Eu);
    abi_AnimStart();
    cpu.s = (uint16_t)(cpu.s + 3);
    do {
        actor_yield(0x83AF32u);
        jsl_frame(0x83AF34u);
        abi_AnimStep();
        cpu.s = (uint16_t)(cpu.s + 3);
        MEM8(dec, ea_dp(0x40));
    } while (!IF_Z);
    lda8_v(0x07);
    sta8(ea_dp(0x07));
    lda8_v(0x00);
    sta8(ea_dp(0x21));
    lda8(ea_dp(0x06));
    jsl_frame(0x83AF46u);
    abi_ActorSpawnSameLook();
    cpu.s = (uint16_t)(cpu.s + 3);
    lda8_v(0x01);
    sta8(ea_dp(0x14));
    sta8(ea_dp(0x15));
    lda8_v(0x80);
    tsb8(ea_dp(0x09));
    abi_Kind31Drop();
}

/*
 * @routine 83:AF57 Kind31BreakTiles
 * WHAT IT DOES: Changes the two map tiles above the actor ($7FB8 left,
 * $7FC0 right) and marks its placement ($7E:C600 by $34) $80.
 * HOW: TileChangeAdd at (x-8, y-8) and (x+8, y-8), 16-bit; then 8-bit.
 * Entry: D = the actor's record; called by JSR ($83:AD17). Exit: M and X
 * 8-bit; A = $80; X = its $34.
 */
void abi_Kind31BreakTiles(void)
{
    op_rep(0x30);
    clc();
    lda16(ea_dp(0x0C));
    adc16_v(0xFFF8);
    tax16();
    clc();
    lda16(ea_dp(0x0F));
    adc16_v(0xFFF8);
    tay16();
    lda16_v(0x7FB8);
    abi_TileChangeAdd();
    clc();
    lda16(ea_dp(0x0C));
    adc16_v(0x0008);
    tax16();
    clc();
    lda16(ea_dp(0x0F));
    adc16_v(0xFFF8);
    tay16();
    lda16_v(0x7FC0);
    abi_TileChangeAdd();
    op_sep(0x30);
    ldx8(ea_dp(0x34));
    lda8_v(0x80);
    sta8(ea_longx(PLACEMENT_FLAGS));
}
