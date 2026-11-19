/*
 * kind2b.c: actor kind $2B's start and its partner test.
 */
#include "machine.h"
#include "native.h"
#include "ops65.h"

void abi_ActorAccelerateY(void);
void abi_ActorEndOutOfRange(void);
void abi_ActorEndPlaced(void);
void abi_ActorFall(void);
void abi_ActorMotionReset(void);
void abi_ActorMoveByVector(void);
void abi_ActorMoveX(void);
void abi_ActorNearPlayer(void);
void abi_ActorNearPlayerX(void);
void abi_ActorNearPlayerY(void);
void abi_ActorOutOfGround(void);
void abi_ActorPlayerBelow(void);
void abi_ActorSideOfPlayer(void);
void abi_ActorSnapToGround(void);
void abi_ActorSpeedXSetAccel(void);
void abi_ActorStepY(void);
void abi_AnimStart(void);
void abi_RandomNext(void);
void abi_TileAtProbeLong(void);
void abi_Kind2BNearPartner(void);

#define PLAYER_X 0x08E7u        /* Aladdin's x pixels (word) */
#define K2B_LOOKS 0xD116u       /* its look table */
#define K2B_LEGS 0xD11Au        /* its leg by way (bytes, DB) */
#define K2B_RANGES 0xD11Eu      /* its range across by way (bytes, DB) */
#define K2B_AHEAD 0xD0D6u       /* the point ahead by surface and direction (x, y words, DB) */
#define K2B_ON 0xD0F6u          /* the point ahead on its surface (x, y words, DB) */
#define K2B_FLIPS 0xD0CEu       /* its flip by surface and direction (bytes, DB) */
#define K2B_TURNS 0xD0C6u       /* its animation as it turns (bytes, DB) */
#define K2B_DIRS 0xD0B6u        /* its direction ($14) (bytes, DB) */
#define K2B_WALKS 0xD0BEu       /* its walking animation (bytes, DB) */
#define K2B_PAST 0xD2E3u        /* Kind2BNearPartner's half-span (word, DB) */
#define K2B_SPAN 0xD2E5u        /* and span (word, DB) */

/* A JSL at `site` to a routine whose C uses the stack: its return address
   on the stack while the callee runs, as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:A467 Kind2BStart
 * @actor
 * WHAT IT DOES: Actor kind $2B: a walker along a floor, a wall or a ceiling
 * (ways 0-7 of $07: its surface and first direction), turning at each end
 * of its leg, at a wall and where its surface ends, while Aladdin is near.
 * With bit 3, when he is within its range across: a ceiling or wall walker
 * drops onto him when he is level with or below it, then walks the floor
 * facing him; a floor walker turns to face him when he is within its
 * height. With bit 4 ($10): it walks to and fro on its own, and when he is
 * below and within its span across it shakes and falls, until its $4B is
 * set; then (never ended out of range) it walks to and fro within $1E of
 * its partner's x ($49, a record its maker sets), jumps while the
 * partner's $13 is 3 and ends when the partner's $16 is set.
 * HOW: $4E 0; $07 bit 4: on at $83:A5E8. Else $09 bits $90 set; AnimStart
 * 2; $40 (its direction) 0, 1 when RandomNext is not negative; its look
 * table ($16) $D116; $48 = its x; $47 = $07 bits 0-2 / 2 (its surface);
 * $41 and $45 = its leg from $D11A, $43 its double; $4C its range from
 * $D11E, $4A its double; $4B, $4D 0; then its turn ($83:A524: $41 = $43 -
 * $41, $15 from $D0CE and AnimStart from $D0C6 by $47 * 2 + $40, $14
 * frames (COP), $40 flipped; then $14, $15, its animation from $D0B6,
 * $D0CE, $D0BE). Each frame (BRK, $83:A4C4) until ActorNearPlayer box 4:
 * then its leg ($41, 16-bit) down, at 0 a turn; the tile at the point
 * ahead ($D0D6 offsets) $70, a turn; the tile at the point ahead on its
 * surface ($D0F6 offsets) none, a turn; else with bit 3 its test of him
 * ($83:A57A). Then ActorMoveByVector X = 2, on the floor ($47 1)
 * ActorSnapToGround, and the next frame. Its test: x ($48) + range less
 * his x not below the double, nothing; a floor walker: ActorNearPlayerY 2
 * C clear and ActorSideOfPlayer other than $40: a turn; else
 * ActorPlayerBelow: its drop: ActorMotionReset $28, AnimStart 3, each
 * frame ActorFall, ActorOutOfGround until on ground; $47 1, $41 = $45,
 * $48 = its x, $40 and $15 ActorSideOfPlayer, AnimStart 0, $20 frames.
 * Bit 4 ($83:A5E8): $4B, $47 0; $09 bit 7; AnimStart 4; $14 0, $15 1;
 * ActorSpeedXSetAccel $42; $40 $18, $41 $30; each frame ActorMoveX, then
 * ActorPlayerBelow and ActorNearPlayerX 4 C clear: its fall; else $40
 * down, at 0 from $41, its direction turned and $10 frames. Its fall
 * ($83:A628): 8 times 6 frames, $15 flipped; AnimStart 3; $20 $20, its
 * vertical speed 0; each frame until $4B: ActorAccelerateY unless $1E is
 * 3, ActorStepY. Then $4E and $47 up; $14 1 when the partner is right of
 * it, $15 the other; $19 $50; AnimStart 1; each frame: the partner's $16
 * set, JMP ActorEndPlaced; its $13 3, a jump (speed $FFFC00, $20 $40;
 * each frame ActorFall, ActorEndOutOfRange X = 8, until $4B, then as
 * from $83:A664); else ActorMoveX and Kind2BNearPartner: past, its x
 * back (+5 with $14 set: ADC #4 with C set) and y back and its direction
 * turned.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by the
 * actor loop's jump for kind $2B in state $0C. Exit: never returns.
 */
void abi_Kind2BStart(void)
{
    stz8(ea_dp(0x4E));
    lda8(ea_dp(0x07));
    and8_v(0x10);
    if (!IF_Z) goto L_A5E8;
    lda8_v(0x90);
    tsb8(ea_dp(0x09));
    lda8_v(0x02);
    abi_AnimStart();
    stz8(ea_dp(0x40));
    jsl_call(0x83A47Eu, abi_RandomNext);
    if (!IF_N)
        MEM8(inc, ea_dp(0x40));
    op_rep(0x20);
    lda16_v(K2B_LOOKS);
    sta16(ea_dp(0x16));
    lda16(ea_dp(0x0C));
    sta16(ea_dp(0x48));
    op_sep(0x20);
    lda8(ea_dp(0x07));
    and8_v(0x07);
    ACC8(lsr);
    sta8(ea_dp(0x47));
    lda8(ea_dp(0x07));
    and8_v(0x02);
    adc8_v(0x00);
    tax8();
    lda8(ea_absx(K2B_LEGS));
    sta8(ea_dp(0x41));
    stz8(ea_dp(0x42));
    sta8(ea_dp(0x45));
    stz8(ea_dp(0x46));
    op_rep(0x20);
    and16_v(0x00FF);
    ACC16(asl);
    sta16(ea_dp(0x43));
    op_sep(0x20);
    lda8(ea_absx(K2B_RANGES));
    sta8(ea_dp(0x4C));
    ACC8(asl);
    sta8(ea_dp(0x4A));
    stz8(ea_dp(0x4D));
    stz8(ea_dp(0x4B));
    goto L_A524;

L_A4C4:     /* each frame */
    actor_yield(0x83A4C4u);
    ldy8_v(0x04);
    jsl_frame(0x83A4C8u);
    abi_ActorNearPlayer();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (!IF_C) goto L_A4C4;
    op_rep(0x20);
    MEM16(dec, ea_dp(0x41));
    op_sep(0x20);
    if (IF_Z) goto L_A524;
    lda8(ea_dp(0x47));
    ACC8(asl);
    adc8_v(rd8(ea_dp(0x40)));
    ACC8(asl);
    ACC8(asl);
    tax8();
    op_rep(0x21);
    lda16(ea_absx(K2B_AHEAD));
    adc16_v(rd16(ea_dp(0x0C)));
    sta16(ea_abs(0x000C));
    lda16(ea_absx(K2B_AHEAD + 2));
    adc16_v(rd16(ea_dp(0x0F)));
    sta16(ea_abs(0x000E));
    jsl_call(0x83A4F0u, abi_TileAtProbeLong);
    cmp8_v(0x70);
    if (IF_Z) goto L_A524;      /* a wall ahead */
    lda8(ea_dp(0x47));
    ACC8(asl);
    adc8_v(rd8(ea_dp(0x40)));
    ACC8(asl);
    ACC8(asl);
    tax8();
    op_rep(0x21);
    lda16(ea_absx(K2B_ON));
    adc16_v(rd16(ea_dp(0x0C)));
    sta16(ea_abs(0x000C));
    lda16(ea_absx(K2B_ON + 2));
    clc();
    adc16_v(rd16(ea_dp(0x0F)));
    sta16(ea_abs(0x000E));
    jsl_call(0x83A515u, abi_TileAtProbeLong);
    if (IF_Z) goto L_A524;      /* its surface ends */
    lda8(ea_dp(0x07));
    and8_v(0x08);
    if (IF_Z) goto L_A568;
    goto L_A57A;

L_A524:     /* its turn */
    op_rep(0x20);
    lda16(ea_dp(0x43));
    sec();
    sbc16_v(rd16(ea_dp(0x41)));
    sta16(ea_dp(0x41));
    op_sep(0x20);
    lda8(ea_dp(0x47));
    ACC8(asl);
    adc8_v(rd8(ea_dp(0x40)));
    tax8();
    lda8(ea_absx(K2B_FLIPS));
    sta8(ea_dp(0x15));
    lda8(ea_absx(K2B_TURNS));
    abi_AnimStart();
    lda8_v(0x14);
    actor_wait(0x83A543u);
    lda8(ea_dp(0x40));
    eor8_v(0x01);
    sta8(ea_dp(0x40));
    goto L_A551;

L_A54D:     /* after its drop */
    lda8_v(0x20);
    actor_wait(0x83A54Fu);
L_A551:
    lda8(ea_dp(0x47));
    ACC8(asl);
    adc8_v(rd8(ea_dp(0x40)));
    tax8();
    lda8(ea_absx(K2B_DIRS));
    sta8(ea_dp(0x14));
    lda8(ea_absx(K2B_FLIPS));
    sta8(ea_dp(0x15));
    lda8(ea_absx(K2B_WALKS));
    abi_AnimStart();
L_A568:     /* its step */
    ldx8_v(0x02);
    jsl_frame(0x83A56Au);
    abi_ActorMoveByVector();
    cpu.s = (uint16_t)(cpu.s + 3);
    lda8(ea_dp(0x47));
    ACC8(dec);
    if (IF_Z)
        jsl_call(0x83A573u, abi_ActorSnapToGround);
    goto L_A4C4;

L_A57A:     /* bit 3: its test of Aladdin */
    op_rep(0x21);
    lda16(ea_dp(0x48));
    adc16_v(rd16(ea_dp(0x4C)));
    sec();
    sbc16_v(rd16(ea_abs(PLAYER_X)));
    cmp16_v(rd16(ea_dp(0x4A)));
    op_sep(0x20);
    if (IF_C) goto L_A568;
    lda8(ea_dp(0x47));
    ACC8(dec);
    if (!IF_Z) goto L_A5A4;
    ldy8_v(0x02);
    jsl_frame(0x83A591u);
    abi_ActorNearPlayerY();
    cpu.s = (uint16_t)(cpu.s + 3);
    op_sep(0x10);
    if (IF_C) goto L_A568;
    abi_ActorSideOfPlayer();
    cmp8_v(rd8(ea_dp(0x40)));
    if (IF_Z) goto L_A568;
    goto L_A524;

L_A5A4:     /* off the floor: its drop when he is below */
    abi_ActorPlayerBelow();
    if (IF_Z) goto L_A568;
    lda8_v(0x28);
    jsl_frame(0x83A5ACu);
    abi_ActorMotionReset();
    cpu.s = (uint16_t)(cpu.s + 3);
    lda8_v(0x03);
    abi_AnimStart();
    do {
        actor_yield(0x83A5B6u);
        abi_ActorFall();
        jsl_call(0x83A5BCu, abi_ActorOutOfGround);
    } while (IF_Z);
    lda8_v(0x01);
    sta8(ea_dp(0x47));
    lda8(ea_dp(0x45));
    sta8(ea_dp(0x41));
    stz8(ea_dp(0x42));
    lda8(ea_dp(0x0C));
    sta8(ea_dp(0x48));
    lda8(ea_dp(0x0D));
    sta8(ea_dp(0x49));
    abi_ActorSideOfPlayer();
    sta8(ea_dp(0x40));
    sta8(ea_dp(0x15));
    lda8_v(0x00);
    abi_AnimStart();
    goto L_A54D;

L_A5E8:     /* bit 4 */
    stz8(ea_dp(0x4B));
    stz8(ea_dp(0x47));
    lda8_v(0x80);
    tsb8(ea_dp(0x09));
    lda8_v(0x04);
    abi_AnimStart();
    stz8(ea_dp(0x14));
    lda8_v(0x01);
    sta8(ea_dp(0x15));
    ldy8_v(0x42);
    jsl_frame(0x83A5FEu);
    abi_ActorSpeedXSetAccel();
    cpu.s = (uint16_t)(cpu.s + 3);
    lda8_v(0x18);
    sta8(ea_dp(0x40));
    ACC8(asl);
    sta8(ea_dp(0x41));
L_A609:
    actor_yield(0x83A609u);
    abi_ActorMoveX();
    abi_ActorPlayerBelow();     /* $83:A6F5 */
    if (IF_Z) goto L_A612;
    ldy8_v(0x04);
    jsl_frame(0x83A6FDu);
    abi_ActorNearPlayerX();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (IF_C) goto L_A612;
    goto L_A628;
L_A612:
    MEM8(dec, ea_dp(0x40));
    if (!IF_Z) goto L_A609;
    lda8(ea_dp(0x41));
    sta8(ea_dp(0x40));
    lda8(ea_dp(0x14));
    sta8(ea_dp(0x15));
    eor8_v(0x01);
    sta8(ea_dp(0x14));
    lda8_v(0x10);
    actor_wait(0x83A624u);
    goto L_A609;

L_A628:     /* its shake and fall */
    lda8_v(0x08);
    sta8(ea_dp(0x46));
    do {
        lda8_v(0x06);
        actor_wait(0x83A62Eu);
        lda8(ea_dp(0x15));
        eor8_v(0x01);
        sta8(ea_dp(0x15));
        MEM8(dec, ea_dp(0x46));
    } while (!IF_Z);
    lda8_v(0x03);
    abi_AnimStart();
    lda8_v(0x20);
    sta8(ea_dp(0x20));
    stz8(ea_dp(0x1D));
    stz8(ea_dp(0x1E));
    stz8(ea_dp(0x1F));
    for (;;) {
        actor_yield(0x83A64Au);
        lda8(ea_dp(0x4B));
        if (!IF_Z)
            break;
        lda8(ea_dp(0x1E));
        cmp8_v(0x03);
        if (!IF_Z)
            { jsl_frame(0x83A656u); abi_ActorAccelerateY(); cpu.s = (uint16_t)(cpu.s + 3); }
        jsl_frame(0x83A65Au);
        abi_ActorStepY();
        cpu.s = (uint16_t)(cpu.s + 3);
    }
    MEM8(inc, ea_dp(0x4E));
    MEM8(inc, ea_dp(0x47));

L_A664:     /* by its partner */
    op_rep(0x30);
    ldx16(ea_dp(0x49));
    ldy16_v(0x0000);
    lda16(ea_dp(0x0C));
    cmp16_v(rd16(ea_absx(0x000C)));
    if (!IF_C)
        ldy16_v(0x0001);
    op_sep(0x30);
    tya8();
    sta8(ea_dp(0x14));
    eor8_v(0x01);
    sta8(ea_dp(0x15));
    lda8_v(0x50);
    sta8(ea_dp(0x19));
    lda8_v(0x01);
    abi_AnimStart();
    for (;;) {
        actor_yield(0x83A688u);
        op_rep(0x10);
        ldx16(ea_dp(0x49));
        lda8(ea_absx(0x0016));
        if (!IF_Z) {
            op_sep(0x10);
            abi_ActorEndPlaced();
            return;
        }
        lda8(ea_absx(0x0013));
        cmp8_v(0x03);
        if (IF_Z)
            break;
        op_sep(0x10);
        op_rep(0x20);
        lda16(ea_dp(0x0C));
        sta16(ea_dp(0x42));
        lda16(ea_dp(0x0F));
        sta16(ea_dp(0x44));
        op_sep(0x20);
        abi_ActorMoveX();
        call_frame(0xA6AE, abi_Kind2BNearPartner);    /* JSR at 83:A6AC */
        if (!IF_C)
            continue;
        op_rep(0x20);
        lda16(ea_dp(0x42));
        ldy8(ea_dp(0x14));
        if (!IF_Z)
            adc16_v(0x0004);
        sta16(ea_dp(0x0C));
        lda16(ea_dp(0x44));
        sta16(ea_dp(0x0F));
        op_sep(0x20);
        lda8(ea_dp(0x14));
        sta8(ea_dp(0x15));
        eor8_v(0x01);
        sta8(ea_dp(0x14));
    }
    op_sep(0x10);               /* its partner's $13 is 3: a jump */
    stz8(ea_dp(0x1D));
    lda8_v(0xFC);
    sta8(ea_dp(0x1E));
    lda8_v(0xFF);
    sta8(ea_dp(0x1F));
    lda8_v(0x40);
    sta8(ea_dp(0x20));
    do {
        actor_yield(0x83A6E3u);
        abi_ActorFall();
        ldx8_v(0x08);
        call_frame(0xA6ED, abi_ActorEndOutOfRange);    /* JSR at 83:A6EB */
        lda8(ea_dp(0x4B));
    } while (IF_Z);
    goto L_A664;
}

/*
 * @routine 83:CD47 Kind2BNearPartner
 * WHAT IT DOES: Says whether kind $2B's actor is past $1E of its
 * partner's x either way.
 * HOW: With 16-bit A and X: its x less the partner's ($49, a record) plus
 * the word at $D2E3 ($1E), against the word at $D2E5 ($3C).
 * Entry: D = the actor; DB = $80; called by JSR (83:A6AC). Exit: C set when
 * past; N, Z, C from the compare; A = the sum; X = the partner; M and X
 * 8-bit.
 */
void abi_Kind2BNearPartner(void)
{
    op_rep(0x30);
    ldx16(ea_dp(0x49));
    sec();
    lda16(ea_dp(0x0C));
    sbc16_v(rd16(ea_absx(0x000C)));
    clc();
    adc16_v(rd16(ea_abs(K2B_PAST)));
    cmp16_v(rd16(ea_abs(K2B_SPAN)));
    op_sep(0x30);
}
