/*
 * level_end.c: pieces of the screen at a level's end (81:F7B1): 2x2-tile
 * blocks queued for VRAM, the four columns its picture grows in step by
 * step. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram_video.h"
#include "wram.h"
#include "hw.h"

#define BLANK_TILE 0x080Eu          /* the tilemap word a blanked block gets */
#define END_STEPS_DONE 0xADu        /* in the direct page: counts the pieces placed */
/* In the data bank: per column the tile's high byte; per step the VRAM
   word address of column 0 (words); per level (LEVEL_035C less 1) four
   picture indices, a byte per column; per index the picture's first tile */
#define END_COLUMN_ATTR 0xFDF7u
#define END_STEP_VRAM 0xFDC5u
#define END_LEVEL_PICTURES 0xFDD5u
#define END_PICTURE_TILE 0xFDF1u
/* In the data bank: the VRAM word addresses of the 16 blocks blanked (words) */
#define END_BLANK_BLOCKS 0xFE02u

/*
 * @routine 81:F973 TileBlockBlank
 * WHAT IT DOES: Writes a blank 2x2-tile block record at X in the tile-
 * block queue: VRAM address A, its four tilemap words $080E.
 * HOW: Five word stores in the 10-byte record; X moves on by 10.
 * Entry: M 16-bit, X 16-bit; A = the VRAM word address; X = the record's
 * offset in TILE_BLOCKS; called by JSR (by 81:F958, for the 16 blocks of a
 * table). Exit: M 16-bit, X 16-bit; X = A = the next record's offset; C
 * from the add.
 */
void abi_TileBlockBlank(void)
{
    uint32_t block = dba(TILE_BLOCKS) + cpu.x;

    wr16(block, cpu.a);                   /* its VRAM address, then four blank tiles */
    for (int i = 0; i < 4; i++)
        wr16(block + 2 + 2 * i, BLANK_TILE);
    flag_set(P_C, 0);
    cpu.a = op_adc16(cpu.x, 0x000A, &cpu.p);
    cpu.x = cpu.a;
}


/*
 * @routine 81:F98C LevelEndColumnTile
 * WHAT IT DOES: Gives the tilemap word of the picture piece column $000C
 * shows for the level, and counts the piece.
 * HOW: END_STEPS_DONE steps up; the column's attribute byte as the high
 * byte; the level's picture index for the column (LEVEL_035C less 1, times
 * 4, plus the column) picks the picture's first tile, doubled, as the low
 * bits.
 * Entry: M 8-bit, X 8-bit; $000C = the column (0-3); D the direct page of
 * END_STEPS_DONE; called by JSR. Exit: M 16-bit, X 16-bit; A = the tilemap
 * word; $0010-$0011 = the attribute word; X = the picture index; Y = its
 * offset in the level table.
 */
void abi_LevelEndColumnTile(void)
{
    uint8_t column, attr, level, at, picture;

    inc8_at(dpa(END_STEPS_DONE));
    column = rd8(dba(SCRATCH));
    x8_load(column);
    attr = rd8(dba(END_COLUMN_ATTR) + column);
    a8_load(attr);
    wr8(dba(SCRATCH + 5), attr);          /* the attribute word, high byte */
    wr8(dba(SCRATCH + 4), 0);
    /* the picture for this level's column: 4 a level, from level 1 */
    level = (uint8_t)(rd8(dba(LEVEL_035C)) - 1);
    flag_set(P_C, (level >> 6) & 1);     /* the second ASL's carry goes into the add */
    at = op_adc8((uint8_t)(level << 2), column, &cpu.p);
    cpu.y = at;
    x8_load(rd8(dba(END_LEVEL_PICTURES) + at));
    picture = rd8(dba(END_PICTURE_TILE) + cpu.x);
    op_rep(0x30);
    flag_set(P_C, 0);
    a16_load((uint16_t)((picture << 1) | rd16(dba(SCRATCH + 4))));
}


/*
 * @routine 81:F9B4 LevelEndColumnPlace
 * WHAT IT DOES: Gives the VRAM address of column X's piece at step Y of
 * the level-end picture: the step's address for column 0, less 6 words a
 * column.
 * HOW: X times 6 (16-bit, by a double and an add) taken from the step's
 * word in END_STEP_VRAM; kept at $000E.
 * Entry: M 8-bit; X = the column; Y = the step (0, 2, 4, 6); called by
 * JSR. Exit: M 8-bit; $000E = A = the VRAM word address; C from the
 * subtraction.
 */
void abi_LevelEndColumnPlace(void)
{
    uint16_t twice = (uint16_t)((uint8_t)cpu.x << 1), six;

    op_rep(P_M);
    wr16(dba(SCRATCH + 2), twice);
    flag_set(P_C, 0);
    six = op_adc16((uint16_t)(twice << 1), twice, &cpu.p);   /* 6 a column */
    wr16(dba(SCRATCH + 2), six);
    flag_set(P_C, 1);
    cpu.a = op_sbc16(rd16(dba(END_STEP_VRAM) + cpu.y), six, &cpu.p);
    wr16(dba(SCRATCH + 2), cpu.a);
    op_sep(P_M);
}

/*
 * @routine 81:F958 LevelEndBlanks
 * WHAT IT DOES: Queues 16 blank 2x2-tile blocks for VRAM, at the addresses
 * of a table, and has the NMI send them.
 * HOW: From the queue's length, TileBlockBlank for each word of the table
 * at $FE02 (the last first); the new length kept, then the queue marked
 * ready (TILE_BLOCKS_PENDING counted up).
 * Entry: DB = $81 (the table's bank); called by JSR from the level-end
 * screen's set-up (81:F6B7). Exit: M 8-bit, X 8-bit; Y = $FE; A low = the
 * last address's low byte.
 */
void abi_LevelEndBlanks(void)
{
    op_rep(0x30);
    cpu.x = rd16(dba(TILE_BLOCKS_LENGTH));
    cpu.y = 0x001E;
    do {                                  /* the 16 blocks of the table */
        a16_load(rd16(dba(END_BLANK_BLOCKS) + cpu.y));
        abi_TileBlockBlank();
        cpu.y = (uint16_t)(cpu.y - 2);
        nz16_of(cpu.y);
    } while (!(cpu.y & 0x8000));
    wr16(dba(TILE_BLOCKS_LENGTH), cpu.x);
    op_sep(0x30);
    inc8_at(dba(TILE_BLOCKS_PENDING));
}

void abi_DisplayDefaults(void);
void abi_FrameStepLong(void);
void abi_KeptOptionsSeal(void);
void abi_LevelEndBlanks(void);
void abi_LevelEndColumnPlace(void);
void abi_LevelEndColumnTile(void);
void abi_LevelRamSetupLong(void);
void abi_NmiDisable(void);
void abi_NmiEnable(void);
void abi_OamBufferClear(void);
void abi_PaletteCycleStart(void);
void abi_PaletteFromRom(void);
void abi_PaletteLoad(void);
void abi_ScreenBlank(void);
void abi_ScrollStateClear(void);
void abi_TaskCreate(void);
void abi_TaskExec(void);
void abi_TaskWait(void);
void abi_TextLayoutShow(void);
void abi_TileBlockQueueOne(void);
void abi_TileBlockQueueRun(void);
void abi_UnpackSet(void);
void abi_VramCopy(void);
void abi_VramCopy2(void);
void abi_VramFillMap(void);

/*
 * @routine 81:F61D LevelEndTask
 * WHAT IT DOES: The screen between a level's acts: on a PAL console a
 * screen of its own; else the level-end picture set up (level $24's RAM,
 * its graphics, palettes and text), faded in, its four columns grown step
 * by step as the counts at $A5-$A6 go, a pause, then the wait for a
 * button; faded out, and on to the story card (or, for part 7, the task at
 * $81:8181).
 * HOW: $005D set: JML $84:AFCC. $0005 = LEVEL_035C; the display shut and
 * cleared (KeptOptionsSeal, NmiDisable, ScreenBlank, ScrollStateClear,
 * OamBufferClear, DisplayDefaults); $AD, $AE 0; $0376 += $0375; LEVEL
 * $24, LevelRamSetupLong; sets $0E and $23 unpacked and nine VRAM copies,
 * VramCopy2 $70, palette $4D, VramFillMap $4000 from 0, LevelEndBlanks;
 * a frame; palette cycles $20 and $21, $0405 1, set $19, VramCopy $23,
 * palette $3D. Part 5: five tile blocks (TileBlockQueueOne) over two
 * frames. The part's block from $FDFA (TileBlockQueueRun), text layout
 * $6A, the part's four palettes ($FDD1); BG3 x $FC, $0399/$039A $17,
 * colour 0 $1806; a frame; TaskCreate (slot 1, parameter 2,
 * BrightnessUpTask); frames of FrameStepLong until $0087 is 0;
 * brightness 15. Then frames until $A6 is 0 and $A5 reaches $E0: each
 * time $A5 passes $A7 ($20, then 16 more), $AD up and the next block of
 * the picture ($A8) queued. Then ($0405 0, $A9-$AC 0) each frame, while
 * $A6 is 0 or 1, for the four columns ($000C) whose step $A9,x is even
 * and under 8 while $A5 is below the column's limit ($FDCD): the step's
 * tile (LevelEndColumnPlace, LevelEndColumnTile) queued, the step up.
 * $A6 past 1: 255 frames, 16 frames, $AE up, 72 frames; frames until A,
 * B, X, Y or START is newly pressed; TaskCreate (slot 1, parameter 1,
 * BrightnessDownTask), frames until $0087 is 0; forced blank. The copier
 * RAM's count at $70:1399 up: when it keeps it, LEVEL_035C becomes 1 at
 * or past $0003, else 6. $0373-$0375 0; part 7: TaskExec $81:8181,
 * else TaskExec StoryCardTask.
 * Entry: M 8-bit, X 8-bit; DB = $81; D = 0; a task's entry (TaskExec).
 * Exit: never returns.
 */
void abi_LevelEndTask(void)
{
    lda8(ea_abs(0x005D));  /* 81F61D lda */
    if (IF_Z) goto L_81F626;  /* 81F620 beq */
    jump_routine(0x84AFCCu);  /* 81F622 jml: the PAL console's screen */
    return;
L_81F626:
    lda8(ea_abs(LEVEL_035C));  /* 81F626 lda */
    sta8(ea_abs(0x0005));  /* 81F629 sta */
    abi_KeptOptionsSeal();  /* 81F62C jsl */
    abi_NmiDisable();  /* 81F630 jsl */
    abi_ScreenBlank();  /* 81F634 jsl */
    abi_ScrollStateClear();  /* 81F638 jsl */
    jsl_frame(0x81F63Cu); abi_OamBufferClear(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F63C jsl */
    abi_DisplayDefaults();  /* 81F640 jsl */
    stz8(ea_dp(0xAD));  /* 81F644 stz */
    stz8(ea_dp(0xAE));  /* 81F646 stz */
    lda8(ea_abs(0x0376));  /* 81F648 lda */
    clc();  /* 81F64B clc */
    adc8_v(rd8(ea_abs(0x0375)));  /* 81F64C adc */
    sta8(ea_abs(0x0376));  /* 81F64F sta */
    lda8_v(0x24);  /* 81F652 lda */
    sta8(ea_abs(LEVEL));  /* 81F654 sta */
    abi_LevelRamSetupLong();  /* 81F657 jsl */
    lda8_v(0x0E);  /* 81F65B lda */
    jsl_frame(0x81F65Du); abi_UnpackSet(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F65D jsl */
    ldx8_v(0x7E);  /* 81F661 ldx */
    jsl_frame(0x81F663u); abi_VramCopy(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F663 jsl */
    lda8_v(0x23);  /* 81F667 lda */
    jsl_frame(0x81F669u); abi_UnpackSet(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F669 jsl */
    ldx8_v(0x69);  /* 81F66D ldx */
    jsl_frame(0x81F66Fu); abi_VramCopy(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F66F jsl */
    ldx8_v(0x93);  /* 81F673 ldx */
    jsl_frame(0x81F675u); abi_VramCopy(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F675 jsl */
    ldx8_v(0xA1);  /* 81F679 ldx */
    jsl_frame(0x81F67Bu); abi_VramCopy(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F67B jsl */
    ldx8_v(0xAF);  /* 81F67F ldx */
    jsl_frame(0x81F681u); abi_VramCopy(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F681 jsl */
    ldx8_v(0xBD);  /* 81F685 ldx */
    jsl_frame(0x81F687u); abi_VramCopy(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F687 jsl */
    ldx8_v(0xCB);  /* 81F68B ldx */
    jsl_frame(0x81F68Du); abi_VramCopy(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F68D jsl */
    ldx8_v(0xD9);  /* 81F691 ldx */
    jsl_frame(0x81F693u); abi_VramCopy(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F693 jsl */
    ldx8_v(0xE7);  /* 81F697 ldx */
    jsl_frame(0x81F699u); abi_VramCopy(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F699 jsl */
    ldx8_v(0x70);  /* 81F69D ldx */
    jsl_frame(0x81F69Fu); abi_VramCopy2(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F69F jsl */
    lda8_v(0x4D);  /* 81F6A3 lda */
    jsl_frame(0x81F6A5u); abi_PaletteLoad(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F6A5 jsl */
    op_rep(0x10);  /* 81F6A9 rep */
    ldx16_v(0x4000);  /* 81F6AB ldx */
    ldy16_v(0x0000);  /* 81F6AE ldy */
    abi_VramFillMap();  /* 81F6B1 jsl */
    op_sep(0x10);  /* 81F6B5 sep */
    abi_LevelEndBlanks();  /* 81F6B7 jsr */
    abi_NmiEnable();  /* 81F6BA jsl */
    lda8_v(0x01);  /* 81F6BE lda */
    jsl_frame(0x81F6C0u); abi_TaskWait();  /* 81F6C0 jsl */
    abi_NmiDisable();  /* 81F6C4 jsl */
    ldx8_v(0x00);  /* 81F6C8 ldx */
    lda8_v(0x20);  /* 81F6CA lda */
    abi_PaletteCycleStart();  /* 81F6CC jsr */
    ldx8_v(0x0E);  /* 81F6CF ldx */
    lda8_v(0x21);  /* 81F6D1 lda */
    abi_PaletteCycleStart();  /* 81F6D3 jsr */
    lda8_v(0x01);  /* 81F6D6 lda */
    sta8(ea_abs(0x0405));  /* 81F6D8 sta */
    lda8_v(0x19);  /* 81F6DB lda */
    jsl_frame(0x81F6DDu); abi_UnpackSet(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F6DD jsl */
    ldx8_v(0x23);  /* 81F6E1 ldx */
    jsl_frame(0x81F6E3u); abi_VramCopy(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F6E3 jsl */
    lda8_v(0x3D);  /* 81F6E7 lda */
    jsl_frame(0x81F6E9u); abi_PaletteLoad(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F6E9 jsl */
    abi_NmiEnable();  /* 81F6ED jsl */
    ldx8(ea_abs(LEVEL_035C));  /* 81F6F1 ldx */
    cpx8_v(0x05);  /* 81F6F4 cpx */
    if (!IF_Z) goto L_81F746;  /* 81F6F6 bne */
    op_rep(0x30);  /* 81F6F8 rep: level 5's tiles */
    ldy16_v(0x0435);  /* 81F6FA ldy */
    lda16_v(0x488C);  /* 81F6FD lda */
    abi_TileBlockQueueOne();  /* 81F700 jsl */
    op_rep(0x30);  /* 81F704 rep */
    ldy16_v(0x043B);  /* 81F706 ldy */
    lda16_v(0x4886);  /* 81F709 lda */
    abi_TileBlockQueueOne();  /* 81F70C jsl */
    op_rep(0x30);  /* 81F710 rep */
    ldy16_v(0x0454);  /* 81F712 ldy */
    lda16_v(0x4888);  /* 81F715 lda */
    abi_TileBlockQueueOne();  /* 81F718 jsl */
    lda8_v(0x01);  /* 81F71C lda */
    jsl_frame(0x81F71Eu); abi_TaskWait();  /* 81F71E jsl */
    op_rep(0x30);  /* 81F722 rep */
    ldy16_v(0x0572);  /* 81F724 ldy */
    lda16_v(0x488A);  /* 81F727 lda */
    abi_TileBlockQueueOne();  /* 81F72A jsl */
    op_rep(0x30);  /* 81F72E rep */
    ldy16_v(0x0570);  /* 81F730 ldy */
    lda16_v(0x4884);  /* 81F733 lda */
    abi_TileBlockQueueOne();  /* 81F736 jsl */
    lda8_v(0x01);  /* 81F73A lda */
    jsl_frame(0x81F73Cu); abi_TaskWait();  /* 81F73C jsl */
    jsl_frame(0x81F740u); abi_FrameStepLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F740 jsl: FrameStep waits in the actor loop with this return on the stack */
    goto L_81F759;  /* 81F744 bra */
L_81F746:
    op_rep(0x31);  /* 81F746 rep */
    lda16(ea_absx(0xFDFA));  /* 81F748 lda */
    and16_v(0x00FF);  /* 81F74B and */
    adc16_v(0x04C0);  /* 81F74E adc */
    tay16();  /* 81F751 tay */
    lda16_v(0x488F);  /* 81F752 lda */
    abi_TileBlockQueueRun();  /* 81F755 jsl */
L_81F759:
    abi_NmiDisable();  /* 81F759 jsl */
    lda8_v(0x6A);  /* 81F75D lda */
    abi_TextLayoutShow();  /* 81F75F jsl */
    stz8(ea_abs(0x000E));  /* 81F763 stz */
    ldx8_v(0x00);  /* 81F766 ldx */
L_81F768:
    lda8(ea_abs(LEVEL_035C));  /* 81F768 lda */
    ACC8(asl);  /* 81F76B asl */
    ACC8(asl);  /* 81F76C asl */
    adc8_v(rd8(ea_abs(0x000E)));  /* 81F76D adc */
    tay8();  /* 81F770 tay */
    lda8(ea_absy(0xFDD1));  /* 81F771 lda */
    ldx8(ea_abs(0x000E));  /* 81F774 ldx */
    jsl_frame(0x81F777u); abi_PaletteFromRom(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F777 jsl */
    MEM8(inc, ea_abs(0x000E));  /* 81F77B inc */
    lda8(ea_abs(0x000E));  /* 81F77E lda */
    cmp8_v(0x04);  /* 81F781 cmp */
    if (!IF_Z) goto L_81F768;  /* 81F783 bne */
    lda8_v(0xFC);  /* 81F785 lda */
    sta8(ea_abs(BG3_X));  /* 81F787 sta */
    lda8_v(0x17);  /* 81F78A lda */
    sta8(ea_abs(0x0399));  /* 81F78C sta */
    sta8(ea_abs(0x039A));  /* 81F78F sta */
    lda8_v(0x06);  /* 81F792 lda */
    sta8(PALETTE_BUFFER);  /* 81F794 sta */
    lda8_v(0x18);  /* 81F798 lda */
    sta8(0x7EFD01u);  /* 81F79A sta */
    abi_NmiEnable();  /* 81F79E jsl */
    lda8_v(0x01);  /* 81F7A2 lda */
    jsl_frame(0x81F7A4u); abi_TaskWait();  /* 81F7A4 jsl */
    jsl_frame(0x81F7A8u); abi_TaskCreate(); cpu.unwound = 0;  /* 81F7A8 jsl */
L_81F7B1:
    lda8_v(0x01);  /* 81F7B1 lda */
    jsl_frame(0x81F7B3u); abi_TaskWait();  /* 81F7B3 jsl */
    jsl_frame(0x81F7B7u); abi_FrameStepLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F7B7 jsl: FrameStep waits in the actor loop with this return on the stack */
    lda8(ea_abs(0x0087));  /* 81F7BB lda */
    if (!IF_Z) goto L_81F7B1;  /* 81F7BE bne */
    lda8_v(0x0F);  /* 81F7C0 lda */
    sta8(ea_abs(INIDISP_SHADOW));  /* 81F7C2 sta */
    lda8_v(0x20);  /* 81F7C5 lda */
    sta8(ea_dp(0xA7));  /* 81F7C7 sta */
    stz8(ea_dp(0xA8));  /* 81F7C9 stz */
L_81F7CB:
    lda8_v(0x01);  /* 81F7CB lda */
    jsl_frame(0x81F7CDu); abi_TaskWait();  /* 81F7CD jsl */
    jsl_frame(0x81F7D1u); abi_FrameStepLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F7D1 jsl: FrameStep waits in the actor loop with this return on the stack */
    lda8(ea_dp(0xA6));  /* 81F7D5 lda */
    if (!IF_Z) goto L_81F7CB;  /* 81F7D7 bne */
    lda8(ea_dp(0xA5));  /* 81F7D9 lda */
    cmp8_v(0xE0);  /* 81F7DB cmp */
    if (!IF_C) goto L_81F7E2;  /* 81F7DD bcc */
    goto L_81F808;  /* 81F7DF jmp */
L_81F7E2:
    cmp8_v(rd8(ea_dp(0xA7)));  /* 81F7E2 cmp */
    if (!IF_C) goto L_81F7CB;  /* 81F7E4 bcc */
    MEM8(inc, ea_dp(0xAD));  /* 81F7E6 inc */
    lda8(ea_dp(0xA7));  /* 81F7E8 lda */
    adc8_v(0x0F);  /* 81F7EA adc */
    sta8(ea_dp(0xA7));  /* 81F7EC sta */
    op_rep(0x30);  /* 81F7EE rep */
    lda16(ea_dp(0xA8));  /* 81F7F0 lda */
    and16_v(0x00FF);  /* 81F7F2 and */
    ACC16(asl);  /* 81F7F5 asl */
    pha16();  /* 81F7F6 pha */
    ACC16(asl);  /* 81F7F7 asl */
    adc16_v(0x0890);  /* 81F7F8 adc */
    tay16();  /* 81F7FB tay */
    pla16();  /* 81F7FC pla */
    adc16_v(0x4AE4);  /* 81F7FD adc */
    abi_TileBlockQueueRun();  /* 81F800 jsl */
    MEM8(inc, ea_dp(0xA8));  /* 81F804 inc */
    goto L_81F7CB;  /* 81F806 bra */
L_81F808:
    stz8(ea_abs(0x0405));  /* 81F808 stz */
    stz8(ea_dp(0xA9));  /* 81F80B stz */
    stz8(ea_dp(0xAA));  /* 81F80D stz */
    stz8(ea_dp(0xAB));  /* 81F80F stz */
    stz8(ea_dp(0xAC));  /* 81F811 stz */
L_81F813:
    lda8_v(0x01);  /* 81F813 lda */
    jsl_frame(0x81F815u); abi_TaskWait();  /* 81F815 jsl */
    jsl_frame(0x81F819u); abi_FrameStepLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F819 jsl: FrameStep waits in the actor loop with this return on the stack */
    stz8(ea_abs(SCRATCH));  /* 81F81D stz */
    ldx8_v(0x00);  /* 81F820 ldx */
L_81F822:
    lda8(ea_abs(0x00A6));  /* 81F822 lda */
    if (IF_Z) goto L_81F82D;  /* 81F825 beq */
    ACC8(dec);  /* 81F827 dec */
    if (IF_Z) goto L_81F813;  /* 81F828 beq */
    goto L_81F8B0;  /* 81F82A jmp */
L_81F82D:
    lda8(ea_abs(0x00A5));  /* 81F82D lda */
    cmp8_v(rd8(ea_absx(0xFDCD)));  /* 81F830 cmp */
    if (IF_C) goto L_81F8A0;  /* 81F833 bcs */
    lda8(ea_dpx(0xA9));  /* 81F835 lda */
    cmp8_v(0x00);  /* 81F837 cmp */
    if (!IF_Z) goto L_81F84D;  /* 81F839 bne */
    ldy8_v(0x00);  /* 81F83B ldy */
    abi_LevelEndColumnPlace();  /* 81F83D jsr */
    abi_LevelEndColumnTile();  /* 81F840 jsr */
    tay16();  /* 81F843 tay */
    lda16(ea_abs(0x000E));  /* 81F844 lda */
    abi_TileBlockQueueOne();  /* 81F847 jsl */
    goto L_81F89B;  /* 81F84B bra */
L_81F84D:
    cmp8_v(0x02);  /* 81F84D cmp */
    if (!IF_Z) goto L_81F867;  /* 81F84F bne */
    ldy8_v(0x02);  /* 81F851 ldy */
    abi_LevelEndColumnPlace();  /* 81F853 jsr */
    abi_LevelEndColumnTile();  /* 81F856 jsr */
    clc();  /* 81F859 clc */
    adc16_v(0x0006);  /* 81F85A adc */
    tay16();  /* 81F85D tay */
    lda16(ea_abs(0x000E));  /* 81F85E lda */
    abi_TileBlockQueueOne();  /* 81F861 jsl */
    goto L_81F89B;  /* 81F865 bra */
L_81F867:
    cmp8_v(0x04);  /* 81F867 cmp */
    if (!IF_Z) goto L_81F881;  /* 81F869 bne */
    ldy8_v(0x04);  /* 81F86B ldy */
    abi_LevelEndColumnPlace();  /* 81F86D jsr */
    abi_LevelEndColumnTile();  /* 81F870 jsr */
    clc();  /* 81F873 clc */
    adc16_v(0x0002);  /* 81F874 adc */
    tay16();  /* 81F877 tay */
    lda16(ea_abs(0x000E));  /* 81F878 lda */
    abi_TileBlockQueueOne();  /* 81F87B jsl */
    goto L_81F89B;  /* 81F87F bra */
L_81F881:
    cmp8_v(0x06);  /* 81F881 cmp */
    if (!IF_Z) goto L_81F89B;  /* 81F883 bne */
    ldy8_v(0x06);  /* 81F885 ldy */
    abi_LevelEndColumnPlace();  /* 81F887 jsr */
    abi_LevelEndColumnTile();  /* 81F88A jsr */
    clc();  /* 81F88D clc */
    adc16_v(0x0004);  /* 81F88E adc */
    tay16();  /* 81F891 tay */
    lda16(ea_abs(0x000E));  /* 81F892 lda */
    abi_TileBlockQueueOne();  /* 81F895 jsl */
    goto L_81F89B;  /* 81F899 bra */
L_81F89B:
    ldx8(ea_abs(SCRATCH));  /* 81F89B ldx */
    MEM8(inc, ea_dpx(0xA9));  /* 81F89E inc */
L_81F8A0:
    MEM8(inc, ea_abs(SCRATCH));  /* 81F8A0 inc */
    ldx8(ea_abs(SCRATCH));  /* 81F8A3 ldx */
    cpx8_v(0x04);  /* 81F8A6 cpx */
    if (IF_Z) goto L_81F8AD;  /* 81F8A8 beq */
    goto L_81F822;  /* 81F8AA jmp */
L_81F8AD:
    goto L_81F813;  /* 81F8AD jmp */
L_81F8B0:
    lda8_v(0xFF);  /* 81F8B0 lda */
    sta8(ea_dp(0xAF));  /* 81F8B2 sta */
L_81F8B4:
    lda8_v(0x01);  /* 81F8B4 lda */
    jsl_frame(0x81F8B6u); abi_TaskWait();  /* 81F8B6 jsl */
    jsl_frame(0x81F8BAu); abi_FrameStepLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F8BA jsl: FrameStep waits in the actor loop with this return on the stack */
    MEM8(dec, ea_dp(0xAF));  /* 81F8BE dec */
    if (!IF_Z) goto L_81F8B4;  /* 81F8C0 bne */
    lda8_v(0x10);  /* 81F8C2 lda */
    sta8(ea_dp(0xAF));  /* 81F8C4 sta */
L_81F8C6:
    lda8_v(0x01);  /* 81F8C6 lda */
    jsl_frame(0x81F8C8u); abi_TaskWait();  /* 81F8C8 jsl */
    jsl_frame(0x81F8CCu); abi_FrameStepLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F8CC jsl: FrameStep waits in the actor loop with this return on the stack */
    MEM8(dec, ea_dp(0xAF));  /* 81F8D0 dec */
    if (!IF_Z) goto L_81F8C6;  /* 81F8D2 bne */
    MEM8(inc, ea_dp(0xAE));  /* 81F8D4 inc */
    lda8_v(0x48);  /* 81F8D6 lda */
    sta8(ea_dp(0xAF));  /* 81F8D8 sta */
L_81F8DA:
    lda8_v(0x01);  /* 81F8DA lda */
    jsl_frame(0x81F8DCu); abi_TaskWait();  /* 81F8DC jsl */
    jsl_frame(0x81F8E0u); abi_FrameStepLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F8E0 jsl: FrameStep waits in the actor loop with this return on the stack */
    MEM8(dec, ea_dp(0xAF));  /* 81F8E4 dec */
    if (!IF_Z) goto L_81F8DA;  /* 81F8E6 bne */
L_81F8E8:
    lda8_v(0x01);  /* 81F8E8 lda */
    jsl_frame(0x81F8EAu); abi_TaskWait();  /* 81F8EA jsl */
    jsl_frame(0x81F8EEu); abi_FrameStepLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 81F8EE jsl: FrameStep waits in the actor loop with this return on the stack */
    lda8(ea_abs(JOY_PRESSED));  /* 81F8F2 lda */
    and8_v(0xC0);  /* 81F8F5 and */
    ora8_v(rd8(ea_abs(0x037C)));  /* 81F8F7 ora */
    bit8_i(0xD0);  /* 81F8FA bit */
    if (IF_Z) goto L_81F8E8;  /* 81F8FC beq */
    jsl_frame(0x81F8FEu); abi_TaskCreate(); cpu.unwound = 0;  /* 81F8FE jsl */
L_81F907:
    lda8_v(0x01);  /* 81F907 lda */
    jsl_frame(0x81F909u); abi_TaskWait();  /* 81F909 jsl */
    lda8(ea_abs(0x0087));  /* 81F90D lda */
    if (!IF_Z) goto L_81F907;  /* 81F910 bne */
    lda8_v(0x80);  /* 81F912 lda */
    sta8(ea_abs(INIDISP_SHADOW));  /* 81F914 sta */
    lda8(0x701399u);  /* 81F917 lda */
    ACC8(inc);  /* 81F91B inc */
    sta8(0x701399u);  /* 81F91C sta */
    cmp8_v(rd8(0x701399u));  /* 81F920 cmp */
    if (!IF_Z) goto L_81F93A;  /* 81F924 bne */
    lda8(ea_abs(LEVEL_035C));  /* 81F926 lda */
    cmp8_v(rd8(ea_abs(0x0003)));  /* 81F929 cmp */
    if (!IF_C) goto L_81F935;  /* 81F92C bcc */
    lda8_v(0x01);  /* 81F92E lda */
    sta8(ea_abs(LEVEL_035C));  /* 81F930 sta */
    goto L_81F93A;  /* 81F933 bra */
L_81F935:
    lda8_v(0x06);  /* 81F935 lda */
    sta8(ea_abs(LEVEL_035C));  /* 81F937 sta */
L_81F93A:
    stz8(ea_abs(0x0375));  /* 81F93A stz */
    stz8(ea_abs(0x0373));  /* 81F93D stz */
    stz8(ea_abs(0x0374));  /* 81F940 stz */
    lda8(ea_abs(LEVEL_035C));  /* 81F943 lda */
    cmp8_v(0x07);  /* 81F946 cmp */
    if (IF_Z) goto L_81F951;  /* 81F948 beq */
    jsl_frame(0x81F94Au); abi_TaskExec(); return;  /* 81F94A jsl */
L_81F951:
    jsl_frame(0x81F951u); abi_TaskExec(); return;  /* 81F951 jsl */
}
