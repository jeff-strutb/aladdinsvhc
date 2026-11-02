/*
 * placement.c: creating a level's placed actors as the camera comes near
 * them.
 *
 * The level's placements are indexed in 32-pixel bands across and down
 * (actor/placement_index.c). Each time the camera has moved 32 pixels
 * from the last band it placed from, the actors of the band now coming
 * into reach are created, those within 384 pixels of the camera and not
 * already placed or gone. A created actor of a kind gets its record from
 * the free list and its kind's set-up (graphics: a tile slot, or the
 * attributes of a slot already loaded).
 */
#include "machine.h"
#include "ops65.h"
void abi_PlacementsIndex(void);
void abi_PlacementsStart(void);
void abi_ShiftRight5(void);
void abi_PlacementsScroll(void);
void abi_PlacementsAcross(void);
void abi_PlacementsDown(void);
void abi_ActorCreate(void);
void abi_ActorCreateIn(void);
void abi_ActorKindSetup(void);
void abi_ActorKindTilesShared(void);
void abi_ActorKindTilesQueue(void);
void abi_ActorKindTiles(void);
void abi_TileSlotsClaim2(void);
void abi_TileSlotsClaim3(void);
void abi_ActorCreateChild(void);
void abi_ActorCreateChild8(void);
void abi_TileSlotFill(void);
void abi_TileSlotClaim(void);
void abi_ActorSpawnInit(void);
void abi_ActorPlace(void);
#include "actor.h"
#include "actor_anim.h"
#include "wram.h"
#include "wram_video.h"

/*
 * @routine 82:A79C PlacementsScroll
 * WHAT IT DOES: Creates the placed actors the camera's move brings into
 * reach: a band of placements across, or down, each time the camera is 32
 * pixels or more from where the last band was placed.
 * HOW: For x, then y: the camera less the last placing point; 32 or more
 * either way moves the point by 32 and the band numbers by one, and places
 * the band ahead (PlacementsAcross, PlacementsDown), the one on the side it
 * moved to.
 * Entry: D = 0; DB any; called by JSL. Exit: M and X 8-bit; DB as at
 * entry.
 */
void abi_PlacementsScroll(void)
{
    op_sep(0x20);  /* 82A79C sep */
    phb();  /* 82A79E phb */
    lda8_v(0x82);  /* 82A79F lda */
    pha8();  /* 82A7A1 pha */
    plb();  /* 82A7A2 plb */
    op_rep(0x30);  /* 82A7A3 rep */
    ldx16_v(0x0000);  /* 82A7A5 ldx */
    stz16(ea_dp(0x2A));  /* 82A7A8 stz */
    lda16(ea_abs(0x1773));  /* 82A7AA lda */
    sec();  /* 82A7AD sec */
    sbc16_v(rd16(ea_abs(0x1BEF)));  /* 82A7AE sbc */
    if (!IF_N) goto L_82A7BB;  /* 82A7B1 bpl */
    MEM16(inc, ea_dp(0x2A));  /* 82A7B3 inc */
    inx16();  /* 82A7B5 inx */
    inx16();  /* 82A7B6 inx */
    eor16_v(0xFFFF);  /* 82A7B7 eor */
    ACC16(inc);  /* 82A7BA inc */
L_82A7BB:
    cmp16_v(0x0020);  /* 82A7BB cmp */
    if (!IF_C) goto L_82A7EE;  /* 82A7BE bcc */
    lda16(ea_abs(0x1BEF));  /* 82A7C0 lda */
    sbc16_v(rd16(ea_absx(0xA794)));  /* 82A7C3 sbc */
    sta16(ea_abs(0x1BEF));  /* 82A7C6 sta */
    op_sep(0x30);  /* 82A7C9 sep */
    lda8(ea_absx(0xA798));  /* 82A7CB lda */
    tay8();  /* 82A7CE tay */
    clc();  /* 82A7CF clc */
    adc8_v(rd8(ea_abs(0x1BEB)));  /* 82A7D0 adc */
    sta8(ea_abs(0x1BEB));  /* 82A7D3 sta */
    tya8();  /* 82A7D6 tya */
    clc();  /* 82A7D7 clc */
    adc8_v(rd8(ea_abs(0x1BED)));  /* 82A7D8 adc */
    sta8(ea_abs(0x1BED));  /* 82A7DB sta */
    tax8();  /* 82A7DE tax */
    lda8(ea_dp(0x2A));  /* 82A7DF lda */
    if (IF_Z) goto L_82A7E8;  /* 82A7E1 beq */
    abi_PlacementsDown();  /* 82A7E3 jsr */
    goto L_82A7EE;  /* 82A7E6 bra */
L_82A7E8:
    ldx8(ea_abs(0x1BEB));  /* 82A7E8 ldx */
    abi_PlacementsDown();  /* 82A7EB jsr */
L_82A7EE:
    op_rep(0x30);  /* 82A7EE rep */
    ldx16_v(0x0000);  /* 82A7F0 ldx */
    stz16(ea_dp(0x2A));  /* 82A7F3 stz */
    lda16(ea_abs(0x1777));  /* 82A7F5 lda */
    sec();  /* 82A7F8 sec */
    sbc16_v(rd16(ea_abs(0x1BF1)));  /* 82A7F9 sbc */
    if (!IF_N) goto L_82A806;  /* 82A7FC bpl */
    MEM16(inc, ea_dp(0x2A));  /* 82A7FE inc */
    inx16();  /* 82A800 inx */
    inx16();  /* 82A801 inx */
    eor16_v(0xFFFF);  /* 82A802 eor */
    ACC16(inc);  /* 82A805 inc */
L_82A806:
    cmp16_v(0x0020);  /* 82A806 cmp */
    if (!IF_C) goto L_82A839;  /* 82A809 bcc */
    lda16(ea_abs(0x1BF1));  /* 82A80B lda */
    sbc16_v(rd16(ea_absx(0xA794)));  /* 82A80E sbc */
    sta16(ea_abs(0x1BF1));  /* 82A811 sta */
    op_sep(0x30);  /* 82A814 sep */
    lda8(ea_absx(0xA798));  /* 82A816 lda */
    tay8();  /* 82A819 tay */
    clc();  /* 82A81A clc */
    adc8_v(rd8(ea_abs(0x1BE9)));  /* 82A81B adc */
    sta8(ea_abs(0x1BE9));  /* 82A81E sta */
    tya8();  /* 82A821 tya */
    clc();  /* 82A822 clc */
    adc8_v(rd8(ea_abs(0x1BE7)));  /* 82A823 adc */
    sta8(ea_abs(0x1BE7));  /* 82A826 sta */
    tax8();  /* 82A829 tax */
    lda8(ea_dp(0x2A));  /* 82A82A lda */
    if (IF_Z) goto L_82A833;  /* 82A82C beq */
    abi_PlacementsAcross();  /* 82A82E jsr */
    goto L_82A839;  /* 82A831 bra */
L_82A833:
    ldx8(ea_abs(0x1BE9));  /* 82A833 ldx */
    abi_PlacementsAcross();  /* 82A836 jsr */
L_82A839:
    op_sep(0x30);  /* 82A839 sep */
    plb();  /* 82A83B plb */
    return;  /* 82A83C rtl */
}

/*
 * @routine 82:A83E PlacementsAcross
 * WHAT IT DOES: Creates the actors of band X across that are near the
 * camera and not yet placed.
 * HOW: The band's run in the placement order; each placement not placed
 * or gone, whose x is within 384 pixels round the camera's band, is created
 * (ActorCreate with its kind) and placed (ActorPlace).
 * Entry: M 8-bit, X 8-bit; D = 0; DB = $82; X = the band; called by JSR.
 * Exit: M and X 8-bit.
 */
void abi_PlacementsAcross(void)
{
    goto L_82A83E;
L_82A83D:
    return;  /* 82A83D rts */
L_82A83E:
    lda8(ea_longx(0x7EC300u));  /* 82A83E lda */
    if (IF_Z) goto L_82A83D;  /* 82A842 beq */
    sta8(ea_dp(0x0C));  /* 82A844 sta */
    lda8(ea_longx(0x7EC400u));  /* 82A846 lda */
    sta8(ea_dp(0x0D));  /* 82A84A sta */
L_82A84C:
    op_rep(0x20);  /* 82A84C rep */
    ldx8(ea_dp(0x0D));  /* 82A84E ldx */
    lda16(ea_longx(0x7EC500u));  /* 82A850 lda */
    and16_v(0x00FF);  /* 82A854 and */
    sta16(ea_dp(0x12));  /* 82A857 sta */
    ACC16(asl);  /* 82A859 asl */
    adc16_v(rd16(ea_dp(0x12)));  /* 82A85A adc */
    ACC16(asl);  /* 82A85C asl */
    adc16_v(rd16(ea_dp(0x6D)));  /* 82A85D adc */
    sta16(ea_dp(0x0E));  /* 82A85F sta */
    MEM16(inc, ea_dp(0x12));  /* 82A861 inc */
    op_sep(0x20);  /* 82A863 sep */
    ldx8(ea_dp(0x12));  /* 82A865 ldx */
    lda8(ea_longx(0x7EC600u));  /* 82A867 lda */
    and8_v(0x81);  /* 82A86B and */
    if (!IF_Z) goto L_82A892;  /* 82A86D bne */
    op_rep(0x31);  /* 82A86F rep */
    ldy16(ea_dp(0x0E));  /* 82A871 ldy */
    lda16(ea_absy(0x8A7C));  /* 82A873 lda */
    adc16_v(0x0040);  /* 82A876 adc */
    sec();  /* 82A879 sec */
    sbc16_v(rd16(ea_abs(0x1BEF)));  /* 82A87A sbc */
    cmp16_v(0x0180);  /* 82A87D cmp */
    if (IF_C) goto L_82A892;  /* 82A880 bcs */
    lda16(ea_absy(0x8A7A));  /* 82A882 lda */
    op_sep(0x10);  /* 82A885 sep */
    abi_ActorCreate();  /* 82A887 jsl */
    if (IF_C) goto L_82A892;  /* 82A88B bcs */
    ldy16(ea_dp(0x0E));  /* 82A88D ldy */
    abi_ActorPlace();  /* 82A88F jsr */
L_82A892:
    op_sep(0x30);  /* 82A892 sep */
    MEM8(inc, ea_dp(0x0D));  /* 82A894 inc */
    MEM8(dec, ea_dp(0x0C));  /* 82A896 dec */
    if (!IF_Z) goto L_82A84C;  /* 82A898 bne */
    op_sep(0x30);  /* 82A89A sep */
    return;  /* 82A89C rts */
}

/*
 * @routine 82:A89D PlacementsDown
 * WHAT IT DOES: Creates the actors of band X down that are near the camera
 * and not yet placed.
 * HOW: The band's placements, from its first; each not placed or gone,
 * whose y is within 384 pixels round the camera's band, is created
 * (ActorCreate with its kind) and placed (ActorPlace).
 * Entry: M 8-bit, X 8-bit; D = 0; DB = $82; X = the band; called by JSR.
 * Exit: M and X 8-bit.
 */
void abi_PlacementsDown(void)
{
    goto L_82A89D;
L_82A83D:
    return;  /* 82A83D rts */
L_82A89D:
    lda8(ea_longx(0x7EC100u));  /* 82A89D lda */
    if (IF_Z) goto L_82A83D;  /* 82A8A1 beq */
    sta8(ea_dp(0x0C));  /* 82A8A3 sta */
    stz8(ea_dp(0x0D));  /* 82A8A5 stz */
    op_rep(0x20);  /* 82A8A7 rep */
    lda16(ea_longx(0x7EC200u));  /* 82A8A9 lda */
    and16_v(0x00FF);  /* 82A8AD and */
    sta16(ea_dp(0x12));  /* 82A8B0 sta */
    ACC16(asl);  /* 82A8B2 asl */
    adc16_v(rd16(ea_dp(0x12)));  /* 82A8B3 adc */
    ACC16(asl);  /* 82A8B5 asl */
    adc16_v(rd16(ea_dp(0x6D)));  /* 82A8B6 adc */
    sta16(ea_dp(0x10));  /* 82A8B8 sta */
    MEM16(inc, ea_dp(0x12));  /* 82A8BA inc */
L_82A8BC:
    op_sep(0x30);  /* 82A8BC sep */
    ldx8(ea_dp(0x12));  /* 82A8BE ldx */
    lda8(ea_longx(0x7EC600u));  /* 82A8C0 lda */
    and8_v(0x81);  /* 82A8C4 and */
    if (!IF_Z) goto L_82A8EF;  /* 82A8C6 bne */
    op_rep(0x31);  /* 82A8C8 rep */
    ldy16(ea_dp(0x10));  /* 82A8CA ldy */
    lda16(ea_absy(0x8A7E));  /* 82A8CC lda */
    adc16_v(0x0040);  /* 82A8CF adc */
    sec();  /* 82A8D2 sec */
    sbc16_v(rd16(ea_abs(0x1BF1)));  /* 82A8D3 sbc */
    cmp16_v(0x0180);  /* 82A8D6 cmp */
    if (IF_C) goto L_82A8EF;  /* 82A8D9 bcs */
    op_sep(0x20);  /* 82A8DB sep */
    ldy16(ea_dp(0x10));  /* 82A8DD ldy */
    lda8(ea_absy(0x8A7A));  /* 82A8DF lda */
    op_sep(0x10);  /* 82A8E2 sep */
    abi_ActorCreate();  /* 82A8E4 jsl */
    if (IF_C) goto L_82A8EF;  /* 82A8E8 bcs */
    ldy16(ea_dp(0x10));  /* 82A8EA ldy */
    abi_ActorPlace();  /* 82A8EC jsr */
L_82A8EF:
    op_rep(0x21);  /* 82A8EF rep */
    lda16(ea_dp(0x10));  /* 82A8F1 lda */
    adc16_v(0x0006);  /* 82A8F3 adc */
    sta16(ea_dp(0x10));  /* 82A8F6 sta */
    MEM16(inc, ea_dp(0x12));  /* 82A8F8 inc */
    MEM16(dec, ea_dp(0x0C));  /* 82A8FA dec */
    if (IF_Z) goto L_82A903;  /* 82A8FC beq */
    if (IF_N) goto L_82A903;  /* 82A8FE bmi */
    goto L_82A8BC;  /* 82A900 jmp */
L_82A903:
    op_sep(0x30);  /* 82A903 sep */
    return;  /* 82A905 rts */
}

/*
 * @routine 82:AA2B ActorCreate
 * WHAT IT DOES: Creates an actor of kind A, if a record is free and its
 * kind's set-up succeeds: C clear.
 * HOW: The free list's top; none free: C set, A = 1. Else ActorCreateIn.
 * Entry: M 8-bit, X 8-bit; A = the kind; DB any; called by JSL. Exit: as
 * ActorCreateIn; none free: M and X 8-bit, C set, A low byte = 1.
 */
void abi_ActorCreate(void)
{
    ldy8(ea_abs(ACTOR_FREE_TOP));  /* 82AA2B ldy */
    if (IF_N) goto L_82AA5C;  /* 82AA2E bmi */
    abi_ActorCreateIn(); return;
L_82AA5C:
    op_sep(0x31);  /* 82AA5C sep */
    lda8_v(0x01);  /* 82AA5E lda */
    return;  /* 82AA60 rtl */
}

/*
 * @routine 82:AA30 ActorCreateIn
 * WHAT IT DOES: Creates an actor of kind A in the free list's top record.
 * HOW: Keeps the kind; the record's $06 = the kind; the kind's set-up
 * (ActorKindSetup); on success the record leaves the free list and the
 * kind's count of live actors steps up.
 * Entry: A = the kind; Y = the free list's top offset; DB = $82; falls
 * in from ActorCreate. Exit: created: C clear, X 16-bit = the record.
 * Failed: M and X 8-bit, C set, A low byte = 1.
 */
void abi_ActorCreateIn(void)
{
    if (cpu.p & P_M) { sta8(ea_abs(0x001C)); } else { sta16(ea_abs(0x001C)); }  /* 82AA30 sta */
    if (cpu.p & P_M) { stz8(ea_abs(0x001D)); } else { stz16(ea_abs(0x001D)); }  /* 82AA33 stz */
    op_rep(0x10);  /* 82AA36 rep */
    ldx16(ea_absy(ACTOR_FREE_LIST));  /* 82AA38 ldx */
    if (cpu.p & P_M) { sta8(ea_absx(ACTOR_06)); } else { sta16(ea_absx(ACTOR_06)); }  /* 82AA3B sta */
    stx16(ea_abs(0x0020));  /* 82AA3E stx */
    abi_ActorKindSetup();  /* 82AA41 jsr */
    if (IF_C) goto L_82AA5C;  /* 82AA44 bcs */
    MEM8(dec, ea_abs(ACTOR_FREE_TOP));  /* 82AA46 dec */
    MEM8(dec, ea_abs(ACTOR_FREE_TOP));  /* 82AA49 dec */
    ldx16(ea_abs(0x001C));  /* 82AA4C ldx */
    lda8(ea_longx(KIND_COUNT));  /* 82AA4F lda */
    ACC8(inc);  /* 82AA53 inc */
    sta8(ea_longx(KIND_COUNT));  /* 82AA54 sta */
    ldx16(ea_abs(0x0020));  /* 82AA58 ldx */
    return;  /* 82AA5B rtl */
L_82AA5C:
    op_sep(0x31);  /* 82AA5C sep */
    lda8_v(0x01);  /* 82AA5E lda */
    return;  /* 82AA60 rtl */
}

/*
 * @routine 82:AA61 ActorKindSetup
 * WHAT IT DOES: Runs the set-up of actor kind A for its new record.
 * HOW: Keeps the kind * 2; with the data bank at $82, the kind's set-up
 * index picks its routine from the set-up table.
 * Entry: A = the kind; called by JSR. Exit: M 8-bit, X 16-bit; C and the
 * rest as the set-up leaves them (C set: it failed).
 */
void abi_ActorKindSetup(void)
{
    op_sep(0x30);  /* 82AA61 sep */
    tay8();  /* 82AA63 tay */
    sta8(ea_abs(0x0022));  /* 82AA64 sta */
    stz8(ea_abs(0x0023));  /* 82AA67 stz */
    MEM8(asl, ea_abs(0x0022));  /* 82AA6A asl */
    MEM8(rol, ea_abs(0x0023));  /* 82AA6D rol */
    phb();  /* 82AA70 phb */
    lda8_v(0x82);  /* 82AA71 lda */
    pha8();  /* 82AA73 pha */
    plb();  /* 82AA74 plb */
    ldx8(ea_absy(0xB000));  /* 82AA75 ldx */
    call_routine(0x820000u | rd16(0x820000u | (uint16_t)(0xAA81 + cpu.x)), 0);  /* 82AA78 jsr */
    plb();  /* 82AA7B plb */
    op_sep(0x20);  /* 82AA7C sep */
    op_rep(0x10);  /* 82AA7E rep */
    return;  /* 82AA80 rts */
}

/*
 * @routine 82:AA89 ActorKindTilesShared
 * WHAT IT DOES: Gives a new actor its kind's graphics: the slot already
 * loaded for the kind, or a new tile slot (one, two or three slots).
 * HOW: Graphics loaded (the cell's flag): the record takes the cell's
 * sprite attributes and slot, whose holders step up. Else claims one slot
 * (TileSlotClaim), two (TileSlotsClaim2) or three (TileSlotsClaim3) as the
 * cell needs, then queues their graphics (ActorKindTilesQueue).
 * Entry: M 8-bit; scratch $1C = the kind, $20 = the record, $22 = the
 * kind * 2; called by JSR. Exit: C clear when given, C set when no slot was
 * free.
 */
void abi_ActorKindTilesShared(void)
{
    op_rep(0x10);  /* 82AA89 rep */
    ldy16(ea_abs(0x0020));  /* 82AA8B ldy */
    ldx16(ea_abs(0x0022));  /* 82AA8E ldx */
    lda8(ea_longx(0x7FE701u));  /* 82AA91 lda */
    if (IF_N) goto L_82AABC;  /* 82AA95 bmi */
    ldx16(ea_abs(0x001C));  /* 82AA97 ldx */
    lda8(ea_longx(CELL_EXTRA_SLOTS));  /* 82AA9A lda */
    if (IF_Z) goto L_82AAB1;  /* 82AA9E beq */
    ACC8(dec);  /* 82AAA0 dec */
    if (IF_Z) goto L_82AAAA;  /* 82AAA1 beq */
    abi_TileSlotsClaim3();  /* 82AAA3 jsr */
    if (IF_C) goto L_82AABB;  /* 82AAA6 bcs */
    goto L_82AAB6;  /* 82AAA8 bra */
L_82AAAA:
    abi_TileSlotsClaim2();  /* 82AAAA jsr */
    if (IF_C) goto L_82AABB;  /* 82AAAD bcs */
    goto L_82AAB6;  /* 82AAAF bra */
L_82AAB1:
    abi_TileSlotClaim();  /* 82AAB1 jsr */
    if (IF_C) goto L_82AABB;  /* 82AAB4 bcs */
L_82AAB6:
    abi_ActorKindTilesQueue();  /* 82AAB6 jsr */
    clc();  /* 82AAB9 clc */
    return;  /* 82AABA rts */
L_82AABB:
    return;  /* 82AABB rts */
L_82AABC:
    lda8(ea_longx(CELL_ATTRS));  /* 82AABC lda */
    sta8(ea_absy(0x002B));  /* 82AAC0 sta */
    lda8(ea_longx(0x7FE701u));  /* 82AAC3 lda */
    sta8(ea_absy(0x002C));  /* 82AAC7 sta */
    lda8_v(0x00);  /* 82AACA lda */
    xba();  /* 82AACC xba */
    ldx16(ea_abs(0x001C));  /* 82AACD ldx */
    lda8(ea_longx(0x7FEB00u));  /* 82AAD0 lda */
    sta8(ea_absy(0x0039));  /* 82AAD4 sta */
    tax16();  /* 82AAD7 tax */
    MEM8(inc, ea_absx(ACTOR_TILE_SLOT_USERS));  /* 82AAD8 inc */
    clc();  /* 82AADB clc */
    return;  /* 82AADC rts */
}

/*
 * @routine 82:AAE7 ActorKindTilesQueue
 * WHAT IT DOES: Queues a kind's graphics for the NMI to upload to the
 * actor's tile slot, and fills the slot's record (TileSlotFill).
 * HOW: A VRAM queue record: the slot's VRAM address from the cell's
 * attributes, the graphics' source and bank $7F, the slot's size; a zero
 * size is not queued. Kind $33 (X = $66) only fills.
 * Entry: M 8-bit, X 16-bit; X = the kind * 2; called by JSR. Exit: as
 * TileSlotFill; C clear.
 */
void abi_ActorKindTilesQueue(void)
{
    goto L_82AAE7;
L_82AAB9:
    clc();  /* 82AAB9 clc */
    return;  /* 82AABA rts */
L_82AAE7:
    op_rep(0x20);  /* 82AAE7 rep */
    cpx16_v(0x0066);  /* 82AAE9 cpx */
    if (IF_Z) goto L_82AB3B;  /* 82AAEC beq */
    lda16(ea_abs(VRAM_QUEUE_COUNT));  /* 82AAEE lda */
    and16_v(0x00FF);  /* 82AAF1 and */
    ACC16(asl);  /* 82AAF4 asl */
    ACC16(asl);  /* 82AAF5 asl */
    ACC16(asl);  /* 82AAF6 asl */
    tay16();  /* 82AAF7 tay */
    lda16(ea_longx(CELL_ATTRS));  /* 82AAF8 lda */
    and16_v(0x01FF);  /* 82AAFC and */
    ACC16(asl);  /* 82AAFF asl */
    ACC16(asl);  /* 82AB00 asl */
    ACC16(asl);  /* 82AB01 asl */
    ACC16(asl);  /* 82AB02 asl */
    clc();  /* 82AB03 clc */
    adc16_v(0x6000);  /* 82AB04 adc */
    sta16(ea_absy(VRAM_QUEUE));  /* 82AB07 sta */
    lda16(ea_longx(0x7FE500u));  /* 82AB0A lda */
    sta16(ea_absy(0x085C));  /* 82AB0E sta */
    lda16_v(0x007F);  /* 82AB11 lda */
    sta16(ea_absy(0x085E));  /* 82AB14 sta */
    lda16(ea_longx(0x82AC18u));  /* 82AB17 lda */
    tax16();  /* 82AB1B tax */
    lda16(ea_longx(0x82AC58u));  /* 82AB1C lda */
    and16_v(0x3FFF);  /* 82AB20 and */
    sta16(ea_absy(0x085F));  /* 82AB23 sta */
    op_sep(0x20);  /* 82AB26 sep */
    if (IF_Z) goto L_82AAB9;  /* 82AB28 beq */
    lda8_v(0x80);  /* 82AB2A lda */
    sta8(ea_absy(0x0861));  /* 82AB2C sta */
    op_sep(0x10);  /* 82AB2F sep */
    MEM8(inc, ea_abs(VRAM_QUEUE_COUNT));  /* 82AB31 inc */
    MEM8(inc, ea_abs(0x0403));  /* 82AB34 inc */
    abi_TileSlotFill();  /* 82AB37 jsr */
    return;  /* 82AB3A rts */
L_82AB3B:
    op_sep(0x30);  /* 82AB3B sep */
    abi_TileSlotFill();  /* 82AB37 jsr */
    return;  /* 82AB3A rts */
}

/*
 * @routine 82:AB3F ActorKindTiles
 * WHAT IT DOES: Gives a new actor a new tile slot (one, two or three) for
 * its kind's graphics.
 * HOW: As the cell needs: TileSlotClaim, TileSlotsClaim2 or
 * TileSlotsClaim3; then TileSlotFill.
 * Entry: scratch $1C = the kind, $20 = the record; called by JSR. Exit: C
 * clear when given; C set when no slot was free.
 */
void abi_ActorKindTiles(void)
{
    op_rep(0x10);  /* 82AB3F rep */
    ldy16(ea_abs(0x0020));  /* 82AB41 ldy */
    ldx16(ea_abs(0x001C));  /* 82AB44 ldx */
    lda8(ea_longx(CELL_EXTRA_SLOTS));  /* 82AB47 lda */
    if (IF_Z) goto L_82AB64;  /* 82AB4B beq */
    ACC8(dec);  /* 82AB4D dec */
    if (IF_Z) goto L_82AB5A;  /* 82AB4E beq */
    abi_TileSlotsClaim3();  /* 82AB50 jsr */
    if (IF_C) goto L_82AB6E;  /* 82AB53 bcs */
    abi_TileSlotFill();  /* 82AB55 jsr */
    clc();  /* 82AB58 clc */
    return;  /* 82AB59 rts */
L_82AB5A:
    abi_TileSlotsClaim2();  /* 82AB5A jsr */
    if (IF_C) goto L_82AB6E;  /* 82AB5D bcs */
    abi_TileSlotFill();  /* 82AB5F jsr */
    clc();  /* 82AB62 clc */
    return;  /* 82AB63 rts */
L_82AB64:
    abi_TileSlotClaim();  /* 82AB64 jsr */
    if (IF_C) goto L_82AB6E;  /* 82AB67 bcs */
    abi_TileSlotFill();  /* 82AB69 jsr */
    clc();  /* 82AB6C clc */
    return;  /* 82AB6D rts */
L_82AB6E:
    return;  /* 82AB6E rts */
}

/*
 * @routine 82:AC10 TileSlotsClaim2
 * WHAT IT DOES: Claims two neighbouring tile slots for a kind's graphics,
 * if two are free: C clear.
 * HOW: The first pair of free neighbours among slots 1-4; its second
 * slot's holders step up, then TileSlotClaim's steps with the first.
 * Entry: M 8-bit, X 16-bit; scratch $1C = the kind, $20 = the record;
 * called by JSR. Exit: C set when no pair was free; else as TileSlotClaim.
 */
void abi_TileSlotsClaim2(void)
{
    goto L_82AC10;
L_82ABDB:
    lda8_v(0x00);  /* 82ABDB lda */
    xba();  /* 82ABDD xba */
    MEM8(inc, ea_absx(0x15E9));  /* 82ABDE inc */
    lda8(ea_abs(0x001C));  /* 82ABE1 lda */
    sta8(ea_absx(TILE_SLOT_CELL));  /* 82ABE4 sta */
    inx16();  /* 82ABE7 inx */
    txa8();  /* 82ABE8 txa */
    sta8(ea_absy(0x0039));  /* 82ABE9 sta */
    ldx16(ea_abs(0x001C));  /* 82ABEC ldx */
    sta8(ea_longx(0x7FEB00u));  /* 82ABEF sta */
    op_rep(0x20);  /* 82ABF3 rep */
    ACC16(asl);  /* 82ABF5 asl */
    tax16();  /* 82ABF6 tax */
    lda16(ea_absx(0xAC06));  /* 82ABF7 lda */
    sta16(ea_absy(0x002B));  /* 82ABFA sta */
    ldx16(ea_abs(0x0022));  /* 82ABFD ldx */
    sta16(ea_longx(CELL_ATTRS));  /* 82AC00 sta */
    op_sep(0x20);  /* 82AC04 sep */
    clc();  /* 82AC06 clc */
    return;  /* 82AC07 rts */
L_82AC10:
    ldx16_v(0x0000);  /* 82AC10 ldx */
    lda8(ea_absx(0x15E9));  /* 82AC13 lda */
    ora8_v(rd8(ea_absx(0x15EA)));  /* 82AC16 ora */
    if (IF_Z) goto L_82AC2F;  /* 82AC19 beq */
    inx16();  /* 82AC1B inx */
    lda8(ea_absx(0x15E9));  /* 82AC1C lda */
    ora8_v(rd8(ea_absx(0x15EA)));  /* 82AC1F ora */
    if (IF_Z) goto L_82AC2F;  /* 82AC22 beq */
    inx16();  /* 82AC24 inx */
    lda8(ea_absx(0x15E9));  /* 82AC25 lda */
    ora8_v(rd8(ea_absx(0x15EA)));  /* 82AC28 ora */
    if (IF_Z) goto L_82AC2F;  /* 82AC2B beq */
    sec();  /* 82AC2D sec */
    return;  /* 82AC2E rts */
L_82AC2F:
    MEM8(inc, ea_absx(0x15EA));  /* 82AC2F inc */
    goto L_82ABDB;  /* 82AC32 bra */
}

/*
 * @routine 82:AC34 TileSlotsClaim3
 * WHAT IT DOES: Claims three neighbouring tile slots for a kind's
 * graphics, if three are free: C clear.
 * HOW: Slots 1-3 or 2-4, the first free run; its second and third slots'
 * holders step up, then TileSlotClaim's steps with the first.
 * Entry: M 8-bit, X 16-bit; scratch $1C = the kind, $20 = the record;
 * called by JSR. Exit: C set when no run was free; else as TileSlotClaim.
 */
void abi_TileSlotsClaim3(void)
{
    goto L_82AC34;
L_82ABDB:
    lda8_v(0x00);  /* 82ABDB lda */
    xba();  /* 82ABDD xba */
    MEM8(inc, ea_absx(0x15E9));  /* 82ABDE inc */
    lda8(ea_abs(0x001C));  /* 82ABE1 lda */
    sta8(ea_absx(TILE_SLOT_CELL));  /* 82ABE4 sta */
    inx16();  /* 82ABE7 inx */
    txa8();  /* 82ABE8 txa */
    sta8(ea_absy(0x0039));  /* 82ABE9 sta */
    ldx16(ea_abs(0x001C));  /* 82ABEC ldx */
    sta8(ea_longx(0x7FEB00u));  /* 82ABEF sta */
    op_rep(0x20);  /* 82ABF3 rep */
    ACC16(asl);  /* 82ABF5 asl */
    tax16();  /* 82ABF6 tax */
    lda16(ea_absx(0xAC06));  /* 82ABF7 lda */
    sta16(ea_absy(0x002B));  /* 82ABFA sta */
    ldx16(ea_abs(0x0022));  /* 82ABFD ldx */
    sta16(ea_longx(CELL_ATTRS));  /* 82AC00 sta */
    op_sep(0x20);  /* 82AC04 sep */
    clc();  /* 82AC06 clc */
    return;  /* 82AC07 rts */
L_82AC34:
    ldx16_v(0x0000);  /* 82AC34 ldx */
    lda8(ea_abs(0x15E9));  /* 82AC37 lda */
    ora8_v(rd8(ea_abs(0x15EA)));  /* 82AC3A ora */
    ora8_v(rd8(ea_abs(0x15EB)));  /* 82AC3D ora */
    if (IF_Z) goto L_82AC50;  /* 82AC40 beq */
    inx16();  /* 82AC42 inx */
    lda8(ea_abs(0x15EA));  /* 82AC43 lda */
    ora8_v(rd8(ea_abs(0x15EB)));  /* 82AC46 ora */
    ora8_v(rd8(ea_abs(0x15EC)));  /* 82AC49 ora */
    if (IF_Z) goto L_82AC50;  /* 82AC4C beq */
    sec();  /* 82AC4E sec */
    return;  /* 82AC4F rts */
L_82AC50:
    MEM8(inc, ea_absx(0x15EA));  /* 82AC50 inc */
    MEM8(inc, ea_absx(0x15EB));  /* 82AC53 inc */
    goto L_82ABDB;  /* 82AC56 bra */
}

/*
 * @routine 81:A9A6 ActorCreateChild
 * WHAT IT DOES: Creates an actor of kind A as a child of the actor,
 * placed at its spawn point, if the kind can be created.
 * HOW: ActorCreate; on success ActorSpawnInit for the new record.
 * Entry: M 8-bit, X 8-bit; D = the spawner; A = the kind; DB = $81;
 * called by JSL. Exit: C clear when created (X 16-bit = the record); else
 * as ActorCreate.
 */
void abi_ActorCreateChild(void)
{
    abi_ActorCreate();  /* 81A9A6 jsl */
    if (IF_C) goto L_81A9B0;  /* 81A9AA bcs */
    abi_ActorSpawnInit();  /* 81A9AC jsr */
    clc();  /* 81A9AF clc */
L_81A9B0:
    return;  /* 81A9B0 rtl */
}

/*
 * @routine 81:A9B1 ActorCreateChild8
 * WHAT IT DOES: ActorCreateChild, returning with 8-bit registers and A =
 * 0 when created.
 * HOW: ActorCreate; on success ActorSpawnInit, then M and X 8-bit and A
 * = 0.
 * Entry: as ActorCreateChild. Exit: created: M and X 8-bit, A low byte =
 * 0, Z set; else as ActorCreate.
 */
void abi_ActorCreateChild8(void)
{
    abi_ActorCreate();  /* 81A9B1 jsl */
    if (IF_C) goto L_81A9BE;  /* 81A9B5 bcs */
    abi_ActorSpawnInit();  /* 81A9B7 jsr */
    op_sep(0x30);  /* 81A9BA sep */
    lda8_v(0x00);  /* 81A9BC lda */
L_81A9BE:
    return;  /* 81A9BE rtl */
}

/*
 * @routine 82:A72C PlacementsStart
 * WHAT IT DOES: Indexes the level's placements as it starts, sets the
 * placing points from the camera, and creates the actors of the 12 bands
 * down from 64 pixels left of the camera.
 * HOW: With D = 0 and DB = $82: PlacementsIndex; the last placement's
 * flags byte ($7E:C6FF) set to $FF; from the scroll y, placed_y and the
 * bands (y - 64) / 32 and (y + 288) / 32 at placed_band_x; from the scroll
 * x, placed_x and the bands (x + 288) / 32 and (x - 64) / 32 at
 * placed_band_y; PlacementsDown for 12 bands from (x - 64) / 32.
 * Entry: DB, D any; called by JSL. Exit: M 8-bit, X 8-bit; D and DB as at
 * entry; Y = 0.
 */
void abi_PlacementsStart(void)
{
    op_sep(0x30);  /* 82A72C sep */
    phd();  /* 82A72E phd */
    lda8_v(0x00);  /* 82A72F lda */
    pha8();  /* 82A731 pha */
    pha8();  /* 82A732 pha */
    pld();  /* 82A733 pld */
    phb();  /* 82A734 phb */
    lda8_v(0x82);  /* 82A735 lda */
    pha8();  /* 82A737 pha */
    plb();  /* 82A738 plb */
    abi_PlacementsIndex();  /* 82A739 jsr */
    lda8_v(0xFF);  /* 82A73C lda */
    sta8(0x7EC6FFu);  /* 82A73E sta */
    op_rep(0x21);  /* 82A742 rep */
    lda16(ea_abs(0x1777));  /* 82A744 lda */
    pha16();  /* 82A747 pha */
    sta16(ea_abs(0x1BF1));  /* 82A748 sta */
    adc16_v(0x0120);  /* 82A74B adc */
    abi_ShiftRight5();  /* 82A74E jsr */
    sta16(ea_abs(0x1BE9));  /* 82A751 sta */
    pla16();  /* 82A754 pla */
    sec();  /* 82A755 sec */
    sbc16_v(0x0040);  /* 82A756 sbc */
    abi_ShiftRight5();  /* 82A759 jsr */
    sta16(ea_abs(0x1BE7));  /* 82A75C sta */
    lda16(ea_abs(0x1773));  /* 82A75F lda */
    pha16();  /* 82A762 pha */
    sta16(ea_abs(0x1BEF));  /* 82A763 sta */
    clc();  /* 82A766 clc */
    adc16_v(0x0120);  /* 82A767 adc */
    abi_ShiftRight5();  /* 82A76A jsr */
    sta16(ea_abs(0x1BEB));  /* 82A76D sta */
    pla16();  /* 82A770 pla */
    sec();  /* 82A771 sec */
    sbc16_v(0x0040);  /* 82A772 sbc */
    abi_ShiftRight5();  /* 82A775 jsr */
    sta16(ea_abs(0x1BED));  /* 82A778 sta */
    op_sep(0x30);  /* 82A77B sep */
    tax8();  /* 82A77D tax */
    ldy8_v(0x0C);  /* 82A77E ldy */
L_82A780:
    phx8();  /* 82A780 phx */
    phy8();  /* 82A781 phy */
    abi_PlacementsDown();  /* 82A782 jsr */
    ply8();  /* 82A785 ply */
    plx8();  /* 82A786 plx */
    inx8();  /* 82A787 inx */
    dey8();  /* 82A788 dey */
    if (!IF_Z) goto L_82A780;  /* 82A789 bne */
    plb();  /* 82A78B plb */
    pld();  /* 82A78C pld */
    return;  /* 82A78D rtl */
}
