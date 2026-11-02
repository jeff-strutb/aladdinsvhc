/*
 * pool.c: actor records taken from the free list.
 *
 * The records an actor can be created in are kept on a stack of their
 * addresses (ACTOR_FREE_LIST, ACTOR_FREE_TOP the offset of its top word).
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorFreeListSortHigh(void);
void abi_ActorFreeListTake(void);
#include "actor.h"
#include "wram.h"

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

/*
 * @routine 81:AB3F ActorAlloc
 * WHAT IT DOES: Takes a free actor record, if there is one.
 * HOW: The free list's top offset, negative when none is free; else the
 * offset steps down a word and the address at the old top is the record.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: none free: Y = the offset; N set; X 8-bit and unchanged.
 * Taken: X 16-bit; X = the record's address; Y = the old top offset; N, Z
 * from X (N clear). A, C, V unchanged.
 */
void abi_ActorAlloc(void)
{
    uint8_t top = rd8(abs_db(ACTOR_FREE_TOP));

    cpu.y = top;
    cpu.p = nz8(cpu.p, top);
    if (top & 0x80)
        return;
    op_rep(P_X);
    wr8(abs_db(ACTOR_FREE_TOP), (uint8_t)(rd8_rmw(abs_db(ACTOR_FREE_TOP)) - 1));
    wr8(abs_db(ACTOR_FREE_TOP), (uint8_t)(rd8_rmw(abs_db(ACTOR_FREE_TOP)) - 1));
    cpu.x = rd16(abs_db((uint16_t)(ACTOR_FREE_LIST + cpu.y)));
    cpu.p = nz16(cpu.p, cpu.x);
}

/*
 * @routine 81:EA41 ActorAllocB
 * WHAT IT DOES: Takes a free record from the second pool (the five
 * records from $09CB), if there is one.
 * HOW: As ActorAlloc, on the second free list: its top offset, negative
 * when none is free; else the offset steps down a word and the address at
 * the old top is the record.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSR. Exit: none free: Y = the offset; N set; X 8-bit and
 * unchanged. Taken: X 16-bit; X = the record's address; Y = the old top
 * offset; N, Z from X (N clear). A, C, V unchanged.
 */
void abi_ActorAllocB(void)
{
    uint8_t top = rd8(abs_db(ACTOR_FREE_TOP_B));

    cpu.y = top;
    cpu.p = nz8(cpu.p, top);
    if (top & 0x80)
        return;
    op_rep(P_X);
    wr8(abs_db(ACTOR_FREE_TOP_B), (uint8_t)(rd8_rmw(abs_db(ACTOR_FREE_TOP_B)) - 1));
    wr8(abs_db(ACTOR_FREE_TOP_B), (uint8_t)(rd8_rmw(abs_db(ACTOR_FREE_TOP_B)) - 1));
    cpu.x = rd16(abs_db((uint16_t)(ACTOR_FREE_LIST_B + cpu.y)));
    cpu.p = nz16(cpu.p, cpu.x);
}

/*
 * @routine 81:EA3D ActorAllocBLong
 * WHAT IT DOES: ActorAllocB for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorAllocB; called by JSL. Exit: as ActorAllocB.
 */
void abi_ActorAllocBLong(void)
{
    abi_ActorAllocB();
}

/*
 * @routine 81:ADC0 ActorFreeListSort
 * WHAT IT DOES: Orders the top of the free list so the lowest record
 * addresses are taken first, when at least A records are free.
 * HOW: With fewer than A free, nothing (a path the scripts never take,
 * the code after this routine). Else A passes of a bubble sort from the
 * top down: each pass compares A pairs of neighbouring entries, and swaps
 * a pair whose lower entry is the smaller; the passes are counted in
 * scratch $0C.
 * Entry: M 8-bit, X 8-bit; A low byte = the passes; DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none free: Y = the top offset; N set.
 * Sorted: M 8-bit; scratch $0C = 0; Y = the top offset; X = 0; A = the
 * last entry compared or swapped (16 bits); Z set; C from the last
 * compare; V unchanged.
 */
void abi_ActorFreeListSort(void)
{
    uint8_t top = rd8(abs_db(ACTOR_FREE_TOP)), x;
    uint16_t passes;

    cpu.y = top;
    cpu.p = nz8(cpu.p, top);
    if (top & 0x80)
        return;
    wr8(abs_db(SCRATCH), (uint8_t)cpu.a);
    wr8(abs_db(SCRATCH + 1), 0);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | (uint8_t)(top >> 1));
    cpu.p = cmp8_flags(cpu.p, (uint8_t)(top >> 1), rd8(abs_db(SCRATCH)));
    if (!(cpu.p & P_C)) {
        jump_routine(0x81ADF9);
        return;
    }
    op_rep(P_M);
    do {
        uint8_t y = top;

        op_push8(y);                           /* PHY */
        x = rd8(abs_db(SCRATCH));
        do {
            uint16_t lower = rd16(abs_db((uint16_t)(ACTOR_FREE_LIST - 2 + y)));

            cpu.a = lower;
            op_cmp16(rd16(abs_db((uint16_t)(ACTOR_FREE_LIST + y))));
            if (!(cpu.p & P_C)) {
                op_push8((uint8_t)(lower >> 8));   /* PHA */
                op_push8((uint8_t)lower);
                cpu.a = rd16(abs_db((uint16_t)(ACTOR_FREE_LIST + y)));
                wr16(abs_db((uint16_t)(ACTOR_FREE_LIST - 2 + y)), cpu.a);
                cpu.a = op_pull8();                /* PLA */
                cpu.a |= (uint16_t)(op_pull8() << 8);
                wr16(abs_db((uint16_t)(ACTOR_FREE_LIST + y)), cpu.a);
            }
            y = (uint8_t)(y - 2);
            x--;
        } while (x != 0);
        top = op_pull8();                      /* PLY */
        passes = (uint16_t)(rd16_rmw(abs_db(SCRATCH)) - 1);
        wr16_rmw(abs_db(SCRATCH), passes);
    } while (passes != 0);
    cpu.y = top;
    cpu.x = 0;
    cpu.p = nz16(cpu.p, 0);
    op_sep(P_M);
}

/*
 * @routine 81:AADD ActorRecordsInit
 * WHAT IT DOES: Empties every actor record and fills the free lists for a
 * new level.
 * HOW: Clears all the records; the second free list holds the 5 records
 * from $09CB and the first the 28 from $0B5B, the highest address at the
 * bottom; each of the 28 gets its number (28 down to 1) at $33; the four
 * animated-tile slots get no holders.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: M and X 8-bit; X = $FF; A = the pool walk's last index less $50
 * (16 bits; $FFB0); N set, Z clear (DEX); C clear (the last SBC); Y, V
 * unchanged; scratch $0C = 0.
 */
void abi_ActorRecordsInit(void)
{
    int x;
    uint16_t a;
    uint8_t n;

    op_rep(P_X);
    for (x = 0x0AF0; x >= 0; x--)
        wr8(abs_db((uint16_t)(ACTOR_RECORDS + x)), 0);
    op_rep(P_M);
    op_sep(P_X);
    wr8(abs_db(ACTOR_FREE_TOP_B), 0x08);
    for (x = 8, a = 0x09CB; x >= 0; x -= 2, a += 0x50)
        wr16(abs_db((uint16_t)(ACTOR_FREE_LIST_B + x)), a);
    wr8(abs_db(ACTOR_FREE_TOP), 0x36);
    for (x = 0x36, a = ACTOR_POOL; x >= 0; x -= 2, a += 0x50)
        wr16(abs_db((uint16_t)(ACTOR_FREE_LIST + x)), a);
    op_sep(P_M);
    op_rep(P_X);
    wr8(abs_db(SCRATCH), 0x1C);
    x = 0x0870;
    do {
        n = rd8(abs_db(SCRATCH));
        wr8(abs_db((uint16_t)(ACTOR_POOL + ACTOR_33 + x)), n);
        wr8(abs_db(SCRATCH), (uint8_t)(rd8_rmw(abs_db(SCRATCH)) - 1));
        cpu.p |= P_C;
        a = op_sbc16((uint16_t)x, 0x0050, &cpu.p);
        x = a;
    } while (!(a & 0x8000));
    cpu.a = a;
    for (x = 3; x >= 0; x--)
        wr8(abs_db((uint16_t)(ACTOR_TILE_SLOT_USERS + 1 + x)), 0);
    cpu.x = 0xFFFF;
    cpu.p = nz16(cpu.p, 0xFFFF);
    op_sep(P_M | P_X);
}

/*
 * @routine 81:ADFC ActorFreeListSortHigh
 * WHAT IT DOES: Orders the top of the free list so the highest record
 * addresses are taken first, when at least A records are free; with
 * fewer, says so.
 * HOW: With fewer than A free (the top offset / 2 below A), A = $FF. Else
 * A passes of a bubble sort from the top down: each pass compares A pairs
 * of neighbouring entries, and swaps a pair whose lower entry is the
 * larger or the same; the passes are counted in scratch $0C.
 * Entry: M 8-bit, X 8-bit; A low byte = the passes; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSL. Exit: none free: Y = the top
 * offset; N set. Too few: A low byte = $FF; N set. Sorted: M 8-bit; scratch $0C = 0; Y = the top
 * offset; X = 0; A = the last entry compared or swapped (16 bits); Z set;
 * C from the last compare; V unchanged.
 */
void abi_ActorFreeListSortHigh(void)
{
    ldy8(ea_abs(ACTOR_FREE_TOP));  /* 81ADFC ldy */
    if (IF_N) goto L_81AE34;  /* 81ADFF bmi */
    sta8(ea_abs(SCRATCH));  /* 81AE01 sta */
    stz8(ea_abs(0x000D));  /* 81AE04 stz */
    tya8();  /* 81AE07 tya */
    ACC8(lsr);  /* 81AE08 lsr */
    cmp8_v(rd8(ea_abs(SCRATCH)));  /* 81AE09 cmp */
    if (!IF_C) goto L_81AE35;  /* 81AE0C bcc */
    op_rep(0x20);  /* 81AE0E rep */
L_81AE10:
    phy8();  /* 81AE10 phy */
    ldx8(ea_abs(SCRATCH));  /* 81AE11 ldx */
L_81AE14:
    lda16(ea_absy(0x15A0));  /* 81AE14 lda */
    cmp16_v(rd16(ea_absy(ACTOR_FREE_LIST)));  /* 81AE17 cmp */
    if (!IF_C) goto L_81AE27;  /* 81AE1A bcc */
    pha16();  /* 81AE1C pha */
    lda16(ea_absy(ACTOR_FREE_LIST));  /* 81AE1D lda */
    sta16(ea_absy(0x15A0));  /* 81AE20 sta */
    pla16();  /* 81AE23 pla */
    sta16(ea_absy(ACTOR_FREE_LIST));  /* 81AE24 sta */
L_81AE27:
    dey8();  /* 81AE27 dey */
    dey8();  /* 81AE28 dey */
    dex8();  /* 81AE29 dex */
    if (!IF_Z) goto L_81AE14;  /* 81AE2A bne */
    ply8();  /* 81AE2C ply */
    MEM16(dec, ea_abs(SCRATCH));  /* 81AE2D dec */
    if (!IF_Z) goto L_81AE10;  /* 81AE30 bne */
    op_sep(0x20);  /* 81AE32 sep */
L_81AE34:
    return;  /* 81AE34 rtl */
L_81AE35:
    lda8_v(0xFF);  /* 81AE35 lda */
    return;  /* 81AE37 rtl */
}

/*
 * @routine 81:AE38 ActorFreeListTake
 * WHAT IT DOES: Takes A / 2 + 1 records off the top of the free list at
 * once, ordered so the lowest address comes first; with too few free, says
 * so.
 * HOW: With the top offset below A (or none free), C clear. Else the top
 * steps down A + 2 bytes, and A / 2 passes of a bubble sort (counted in
 * scratch $0E) order the entries taken: each pass compares neighbouring
 * pairs from the old top down and swaps a pair whose upper entry is the
 * smaller.
 * Entry: M 8-bit, X 8-bit; A low byte = the bytes (twice the records less
 * one); DB a bank whose $0000-$1FFF is work RAM; called by JSL. Exit: too
 * few: C clear; Y = the top offset. Taken: C set; Y = the offset of the
 * first record taken (the new top + 2); M 8-bit; scratch $0C = A + 2, $0E
 * = 0; X = 0.
 */
void abi_ActorFreeListTake(void)
{
    ldy8(ea_abs(ACTOR_FREE_TOP));  /* 81AE38 ldy */
    if (IF_N) goto L_81AE88;  /* 81AE3B bmi */
    sta8(ea_abs(SCRATCH));  /* 81AE3D sta */
    cpy8_v(rd8(ea_abs(SCRATCH)));  /* 81AE40 cpy */
    if (!IF_C) goto L_81AE88;  /* 81AE43 bcc */
    pha8();  /* 81AE45 pha */
    ACC8(lsr);  /* 81AE46 lsr */
    sta8(ea_abs(0x000E));  /* 81AE47 sta */
    stz8(ea_abs(0x000F));  /* 81AE4A stz */
    pla8();  /* 81AE4D pla */
    ACC8(inc);  /* 81AE4E inc */
    ACC8(inc);  /* 81AE4F inc */
    sta8(ea_abs(SCRATCH));  /* 81AE50 sta */
    sec();  /* 81AE53 sec */
    tya8();  /* 81AE54 tya */
    sbc8_v(rd8(ea_abs(SCRATCH)));  /* 81AE55 sbc */
    sta8(ea_abs(ACTOR_FREE_TOP));  /* 81AE58 sta */
    op_rep(0x20);  /* 81AE5B rep */
L_81AE5D:
    phy8();  /* 81AE5D phy */
    ldx8(ea_abs(0x000E));  /* 81AE5E ldx */
L_81AE61:
    lda16(ea_absy(ACTOR_FREE_LIST));  /* 81AE61 lda */
    cmp16_v(rd16(ea_absy(0x15A0)));  /* 81AE64 cmp */
    if (IF_C) goto L_81AE74;  /* 81AE67 bcs */
    pha16();  /* 81AE69 pha */
    lda16(ea_absy(0x15A0));  /* 81AE6A lda */
    sta16(ea_absy(ACTOR_FREE_LIST));  /* 81AE6D sta */
    pla16();  /* 81AE70 pla */
    sta16(ea_absy(0x15A0));  /* 81AE71 sta */
L_81AE74:
    dey8();  /* 81AE74 dey */
    dey8();  /* 81AE75 dey */
    dex8();  /* 81AE76 dex */
    if (!IF_Z) goto L_81AE61;  /* 81AE77 bne */
    ply8();  /* 81AE79 ply */
    MEM16(dec, ea_abs(0x000E));  /* 81AE7A dec */
    if (!IF_Z) goto L_81AE5D;  /* 81AE7D bne */
    op_sep(0x20);  /* 81AE7F sep */
    ldy8(ea_abs(ACTOR_FREE_TOP));  /* 81AE81 ldy */
    iny8();  /* 81AE84 iny */
    iny8();  /* 81AE85 iny */
    sec();  /* 81AE86 sec */
    return;  /* 81AE87 rtl */
L_81AE88:
    clc();  /* 81AE88 clc */
    return;  /* 81AE89 rtl */
}
