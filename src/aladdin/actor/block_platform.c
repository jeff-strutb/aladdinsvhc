/*
 * block_platform.c: the platform of kind $68 (its start, Kind68Start,
 * and its pieces), which goes back and forth along a leg and is drawn as blocks
 * of the level map, written ($81:9978) where it moves to, a block at a
 * time. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

/* The platform's own fields of its record */
#define BLOCK_X 0x42u              /* where its next blocks are written: x, y pixels (words) */
#define BLOCK_Y 0x44u
#define BACK_LEG 0x19u             /* 0 on the way out, 1 on the way back (word) */
#define BLOCK_ALT 0x4Au            /* bit 0 flips at every block step */

/* In the bank DB names: by bit 0 of BLOCK_ALT, the offset from here of a
   list of block steps (words), 4 bytes each: x then y (words), picked by
   ACTOR_47 and the facing. */
#define BLOCK_STEPS 0xD5A3u
/* In the bank DB names: by ACTOR_07, the start of its list in MOVE_PICKS;
   the list's bytes, picked by ACTOR_47, the leg and the facing, are
   offsets from MOVE_PICKS of 6-byte moves: x then y (24-bit, fraction
   first). */
#define MOVE_LISTS 0xD517u
#define MOVE_PICKS 0xD51Fu

void abi_ActorCreateChild8(void);
void abi_ActorEndPlaced(void);
void abi_ActorKeepSpot(void);
void abi_AnimStart(void);
void abi_BlockPlatformMove(void);
void abi_BlockPointInBounds(void);
void abi_BlockPointStep(void);
void abi_TileActorDraw(void);

/*
 * @routine 83:E7DE BlockPointInBounds
 * WHAT IT DOES: Tells whether the platform's block point is in the part
 * of the map its blocks may be written to: C clear when its x pixel is
 * from $20 to $DF.
 * HOW: x less $80, plus $60, compared unsigned with $C0, with 16-bit A.
 * Entry: M 8-bit; D = the platform's record. Exit: A = the x pixel less
 * $20 (16 bits); N, Z and C from the compare; V from the addition. M
 * 8-bit; X and Y unchanged.
 */
void abi_BlockPointInBounds(void)
{
    uint16_t d;

    op_rep(P_M);
    flag_set(P_C, 1);
    d = op_sbc16(dp16(BLOCK_X), 0x0080, &cpu.p);
    flag_set(P_C, 0);
    d = op_adc16(d, 0x0060, &cpu.p);      /* x from $20 to $DF */
    cpu.a = d;
    cmp16_of(d, 0x00C0);
    op_sep(P_M);
}

/*
 * @routine 83:E837 BlockPointStep
 * WHAT IT DOES: Moves the platform's block point on by one block step:
 * the step from the list by bit 0 of BLOCK_ALT, picked by ACTOR_47 and the
 * facing, its y step turned round on the way back.
 * HOW: With 16-bit registers: the list's offset from the table by the
 * bit, plus 4 times (ACTOR_47 twice, or the facing word, low byte); x and
 * y added from the entry, y negated when the back-leg word is nonzero.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB the table's
 * bank. Exit: A = the new block y (16 bits); N, V, Z and C from its
 * addition; X = the step's offset (its low byte); Y = the back-leg word's
 * low byte. M 8-bit, X 8-bit.
 */
void abi_BlockPointStep(void)
{
    uint16_t alt, pick, dy, v;

    op_rep(0x30);
    alt = (uint16_t)((dp16(BLOCK_ALT) & 0x0001) << 1);
    v = dp16(ACTOR_47);
    pick = (uint16_t)((((v << 1) | dp16(ACTOR_FACING)) & 0x00FF) << 2);
    flag_set(P_C, 0);
    cpu.x = op_adc16(pick, rd16(dba(BLOCK_STEPS) + alt), &cpu.p);
    /* x by the step; y by it, turned on the way back */
    flag_set(P_C, 0);
    v = dp16(BLOCK_X);
    cpu.a = op_adc16(v, rd16(dba(BLOCK_STEPS) + cpu.x), &cpu.p);
    dp16_set(BLOCK_X, cpu.a);
    dy = rd16(dba(BLOCK_STEPS + 2) + cpu.x);
    cpu.y = dp16(BACK_LEG);
    if (cpu.y)
        dy = (uint16_t)((dy ^ 0xFFFF) + 1);
    flag_set(P_C, 0);
    cpu.a = op_adc16(dy, dp16(BLOCK_Y), &cpu.p);
    dp16_set(BLOCK_Y, cpu.a);
    op_sep(0x30);
}

/*
 * @routine 83:E86A BlockPlatformMove
 * WHAT IT DOES: Moves the platform a frame along its leg: x and y by the
 * move its kind's list picks by ACTOR_47, the leg and the facing.
 * HOW: The pick ((ACTOR_47 * 2 | leg) * 2 | facing) indexes the list
 * ACTOR_07 starts; its byte is the move's offset; two 24-bit additions,
 * a byte at a time.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB the tables'
 * bank. Exit: A = the y position's top byte; N, V, Z and C from its
 * addition; X = the move's offset; Y = the pick's place in the list. M
 * 8-bit, X 8-bit.
 */
void abi_BlockPlatformMove(void)
{
    uint8_t pick, v = 0;

    /* the move picked by its variant, $47, the leg and its facing */
    x8_load(rd8(dpa(ACTOR_07)));
    pick = rd8(dpa(ACTOR_47));
    pick = (uint8_t)((uint8_t)(pick << 1) | rd8(dpa(BACK_LEG)));
    pick = (uint8_t)((uint8_t)(pick << 1) | rd8(dpa(ACTOR_FACING)));
    flag_set(P_C, 0);
    cpu.y = op_adc8(pick, rd8(dba(MOVE_LISTS) + cpu.x), &cpu.p);
    x8_load(rd8(dba(MOVE_PICKS) + cpu.y));
    /* x and y (24-bit each) moved by it */
    for (int axis = 0; axis < 2; axis++) {
        uint16_t at = axis ? ACTOR_Y : ACTOR_X;

        flag_set(P_C, 0);
        for (int i = 0; i < 3; i++) {
            v = rd8(dpa((uint16_t)(at + i)));
            v = op_adc8(v, rd8(dba(MOVE_PICKS + 3 * axis + i) + cpu.x), &cpu.p);
            wr8(dpa((uint16_t)(at + i)), v);
        }
    }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
}

/*
 * @routine 83:E6BB Kind68Start
 * @actor
 * WHAT IT DOES: Actor kind $68 (also kind $67's second state), the block
 * platform: with $07 $0F, a spawner that makes eight platforms ($07 7 down
 * to 0) and ends; else one platform set up by its $07 that, after its
 * wait, moves along its leg a block at a time (written into the level map
 * while in bounds), pauses, and goes back. On a copier (cartridge RAM
 * found at $70:0040) it never moves.
 * HOW: $07 $0F: x, y 0; $07 7; K68_CHILD_SLOT $6A; ActorCreateChild8 $68,
 * the slot down 2, $07 down, while $07 is not negative; ActorEndPlaced.
 * Else: $AB written to and compared at $70:0040 + 0 and $CD at +$40 (DB
 * $70): both equal, K68_COPIER 1. ACTOR_08 3, ACTOR_09 $90; BLOCK_ALT,
 * BACK_LEG (word), gravity and $16-$17 0; BLOCK_X, BLOCK_Y from
 * K68_BLOCKS_AT by $07; K68_LEG_BLOCKS from K68_LEG; facing (flip the
 * opposite) and actor_47 from K68_WAYS; $48 0; K68_LAST_X its x low;
 * ActorKeepSpot, AnimStart by K68_POSES; K68_WAIT frames (BRK) from
 * K68_WAITS. Each frame (BRK): ActorKeepSpot, BlockPlatformMove; its x low
 * 10 or more from K68_LAST_X: K68_LAST_X 16 on toward it,
 * BlockPointInBounds C clear: TileActorDraw; BlockPointStep; the leg's
 * blocks down: at 0 the next leg (K68_LEG_BLOCKS again, BACK_LEG and the
 * facing flipped, BlockPointStep, BLOCK_ALT flipped, K68_PAUSES frames of
 * ActorKeepSpot, or BRK for good with K68_COPIER), else BLOCK_ALT flipped.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $68 in state $0C (the table at $83:8CAB)
 * and kind $67's second state ($83:8DFD). Exit: never returns.
 */
void abi_Kind68Start(void)
{
    lda8(ea_dp(0x07));  /* 83E6BB lda */
    cmp8_v(0x0F);  /* 83E6BD cmp */
    if (!IF_Z) goto L_83E6E2;  /* 83E6BF bne */
    op_rep(0x20);  /* 83E6C1 rep */
    stz16(ea_dp(0x0C));  /* 83E6C3 stz */
    stz16(ea_dp(0x0F));  /* 83E6C5 stz */
    op_sep(0x20);  /* 83E6C7 sep */
    lda8_v(0x07);  /* 83E6C9 lda */
    sta8(ea_dp(0x07));  /* 83E6CB sta */
    lda8_v(0x6A);  /* 83E6CD lda */
    sta8(ea_dp(0x21));  /* 83E6CF sta */
L_83E6D1:
    lda8_v(0x68);  /* 83E6D1 lda */
    abi_ActorCreateChild8();  /* 83E6D3 jsl */
    MEM8(dec, ea_dp(0x21));  /* 83E6D7 dec */
    MEM8(dec, ea_dp(0x21));  /* 83E6D9 dec */
    MEM8(dec, ea_dp(0x07));  /* 83E6DB dec */
    if (!IF_N) goto L_83E6D1;  /* 83E6DD bpl */
    abi_ActorEndPlaced(); return;  /* 83E6DF jmp */
L_83E6E2:
    stz8(ea_dp(0x18));  /* 83E6E2 stz */
    ldx8_v(0x40);  /* 83E6E4 ldx */
    phb();  /* 83E6E6 phb */
    sec();  /* 83E6E7 sec */
    lda8_v(0x80);  /* 83E6E8 lda */
    sbc8_v(0x10);  /* 83E6EA sbc */
    pha8();  /* 83E6EC pha */
    plb();  /* 83E6ED plb */
    lda8_v(0xAB);  /* 83E6EE lda */
    sta8(ea_absx(0x0000));  /* 83E6F0 sta */
    cmp8_v(rd8(ea_absx(0x0000)));  /* 83E6F3 cmp */
    if (!IF_Z) goto L_83E707;  /* 83E6F6 bne */
    lda8_v(0xCD);  /* 83E6F8 lda */
    sta8(ea_absx(0x0040));  /* 83E6FA sta */
    cmp8_v(rd8(ea_absx(0x0040)));  /* 83E6FD cmp */
    if (!IF_Z) goto L_83E707;  /* 83E700 bne */
    plb();  /* 83E702 plb */
    MEM8(inc, ea_dp(0x18));  /* 83E703 inc */
    goto L_83E708;  /* 83E705 bra */
L_83E707:
    plb();  /* 83E707 plb */
L_83E708:
    lda8_v(0x03);  /* 83E708 lda */
    sta8(ea_dp(0x08));  /* 83E70A sta */
    lda8_v(0x90);  /* 83E70C lda */
    sta8(ea_dp(0x09));  /* 83E70E sta */
    stz8(ea_dp(0x4A));  /* 83E710 stz */
    stz8(ea_dp(0x19));  /* 83E712 stz */
    stz8(ea_dp(0x1A));  /* 83E714 stz */
    stz8(ea_dp(0x20));  /* 83E716 stz */
    stz8(ea_dp(0x16));  /* 83E718 stz */
    stz8(ea_dp(0x17));  /* 83E71A stz */
    op_rep(0x20);  /* 83E71C rep */
    lda16(ea_dp(0x07));  /* 83E71E lda */
    ACC16(asl);  /* 83E720 asl */
    ACC16(asl);  /* 83E721 asl */
    tax8();  /* 83E722 tax */
    lda16(ea_absx(0xD5C7));  /* 83E723 lda */
    sta16(ea_dp(0x42));  /* 83E726 sta */
    lda16(ea_absx(0xD5C9));  /* 83E728 lda */
    sta16(ea_dp(0x44));  /* 83E72B sta */
    op_sep(0x20);  /* 83E72D sep */
    ldx8(ea_dp(0x07));  /* 83E72F ldx */
    lda8(ea_absx(0xD5E7));  /* 83E731 lda */
    sta8(ea_dp(0x1C));  /* 83E734 sta */
    lda8(ea_absx(0xD50D));  /* 83E736 lda */
    pha8();  /* 83E739 pha */
    ACC8(lsr);  /* 83E73A lsr */
    ACC8(lsr);  /* 83E73B lsr */
    sta8(ea_dp(0x14));  /* 83E73C sta */
    eor8_v(0x01);  /* 83E73E eor */
    sta8(ea_dp(0x15));  /* 83E740 sta */
    pla8();  /* 83E742 pla */
    and8_v(0x01);  /* 83E743 and */
    sta8(ea_dp(0x47));  /* 83E745 sta */
    stz8(ea_dp(0x48));  /* 83E747 stz */
    op_sep(0x20);  /* 83E749 sep */
    lda8(ea_dp(0x0C));  /* 83E74B lda */
    sta8(ea_dp(0x49));  /* 83E74D sta */
    abi_ActorKeepSpot();  /* 83E74F jsl */
    ldx8(ea_dp(0x47));  /* 83E753 ldx */
    lda8(ea_absx(0xD515));  /* 83E755 lda */
    jsl_frame(0x83E758u);  /* 83E758 jsl: AnimStart's C pushes */
    abi_AnimStart();
    cpu.s = (uint16_t)(cpu.s + 3);
    lda8(ea_dp(0x07));  /* 83E75C lda */
    ACC8(asl);  /* 83E75E asl */
    tax8();  /* 83E75F tax */
    op_rep(0x20);  /* 83E760 rep */
    lda16(ea_absx(0xD5EF));  /* 83E762 lda */
    sta16(ea_dp(0x40));  /* 83E765 sta */
L_83E767:
    actor_yield(0x83E767u);  /* 83E767 brk */
    MEM16(dec, ea_dp(0x40));  /* 83E769 dec */
    if (!IF_Z) goto L_83E767;  /* 83E76B bne */
    op_sep(0x20);  /* 83E76D sep */
L_83E76F:
    actor_yield(0x83E76Fu);  /* 83E76F brk */
    abi_ActorKeepSpot();  /* 83E771 jsl */
    abi_BlockPlatformMove();  /* 83E775 jsr */
    ldy8_v(0x10);  /* 83E778 ldy */
    sec();  /* 83E77A sec */
    lda8(ea_dp(0x0C));  /* 83E77B lda */
    sbc8_v(rd8(ea_dp(0x49)));  /* 83E77D sbc */
    if (!IF_N) goto L_83E786;  /* 83E77F bpl */
    eor8_v(0xFF);  /* 83E781 eor */
    ACC8(inc);  /* 83E783 inc */
    ldy8_v(0xF0);  /* 83E784 ldy */
L_83E786:
    cmp8_v(0x0A);  /* 83E786 cmp */
    if (!IF_C) goto L_83E7A5;  /* 83E788 bcc */
    clc();  /* 83E78A clc */
    tya8();  /* 83E78B tya */
    adc8_v(rd8(ea_dp(0x49)));  /* 83E78C adc */
    sta8(ea_dp(0x49));  /* 83E78E sta */
    abi_BlockPointInBounds();  /* 83E790 jsr */
    if (IF_C) goto L_83E798;  /* 83E793 bcs */
    /* the JSR's return on the stack while it runs: its tile lookup pushes
       there (TileAtPlace's PHA), as the original's does */
    call_frame(0xE797, abi_TileActorDraw);  /* 83E795 jsr */
L_83E798:
    abi_BlockPointStep();  /* 83E798 jsr */
    MEM8(dec, ea_dp(0x1C));  /* 83E79B dec */
    if (IF_Z) goto L_83E7A7;  /* 83E79D beq */
    lda8(ea_dp(0x4A));  /* 83E79F lda */
    eor8_v(0x01);  /* 83E7A1 eor */
    sta8(ea_dp(0x4A));  /* 83E7A3 sta */
L_83E7A5:
    goto L_83E76F;  /* 83E7A5 bra */
L_83E7A7:
    ldx8(ea_dp(0x07));  /* 83E7A7 ldx */
    lda8(ea_absx(0xD5E7));  /* 83E7A9 lda */
    sta8(ea_dp(0x1C));  /* 83E7AC sta */
    lda8(ea_dp(0x19));  /* 83E7AE lda */
    eor8_v(0x01);  /* 83E7B0 eor */
    sta8(ea_dp(0x19));  /* 83E7B2 sta */
    lda8(ea_dp(0x14));  /* 83E7B4 lda */
    eor8_v(0x01);  /* 83E7B6 eor */
    sta8(ea_dp(0x14));  /* 83E7B8 sta */
    abi_BlockPointStep();  /* 83E7BA jsr */
    lda8(ea_dp(0x4A));  /* 83E7BD lda */
    eor8_v(0x01);  /* 83E7BF eor */
    sta8(ea_dp(0x4A));  /* 83E7C1 sta */
    ldx8(ea_dp(0x19));  /* 83E7C3 ldx */
    lda8(ea_absx(0xD655));  /* 83E7C5 lda */
    sta8(ea_dp(0x40));  /* 83E7C8 sta */
    lda8(ea_dp(0x18));  /* 83E7CA lda */
    if (IF_Z) goto L_83E7D2;  /* 83E7CC beq */
L_83E7CE:
    actor_yield(0x83E7CEu);  /* 83E7CE brk */
    goto L_83E7CE;  /* 83E7D0 bra */
L_83E7D2:
    actor_yield(0x83E7D2u);  /* 83E7D2 brk */
    abi_ActorKeepSpot();  /* 83E7D4 jsl */
    MEM8(dec, ea_dp(0x40));  /* 83E7D8 dec */
    if (!IF_Z) goto L_83E7D2;  /* 83E7DA bne */
    goto L_83E76F;  /* 83E7DC bra */
}
