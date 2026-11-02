/*
 * level_tables.c: values the level takes from per-level tables.
 */
#include "machine.h"
#include "ops65.h"
void abi_LevelExitCheck(void);
void abi_LevelStartPlace(void);
void abi_LevelCompanionStart(void);
void abi_LevelSongPlay(void);
void abi_RestartPointCheck(void);
void abi_PaletteLoad(void);
void abi_SongPlay(void);
#include "wram.h"
#include "wram_video.h"
#include "wram_player.h"

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

/* Per level: two bytes for LEVEL_035C (bank $81, word per level). */
#define LEVEL_035C_TABLE 0xB933u
/* Per level, the offset of its trigger-spot list from LEVEL_SPOT_TABLE;
   the list is a count, then 6-byte spots. */
#define LEVEL_SPOT_TABLE 0xEC1Eu

/*
 * @routine 81:878B Level035CSet
 * WHAT IT DOES: Sets $035C-$035D from the level's entry in the table at
 * $81:B933.
 * HOW: The level * 2 indexes the table; its two bytes are copied.
 * Entry: M 8-bit, X 8-bit; DB = $81. Exit: A low byte = the second byte;
 * N, Z from it; X = the level * 2; C = the level's bit 7; Y, V unchanged.
 */
void abi_Level035CSet(void)
{
    uint8_t lv = rd8(abs_db(LEVEL)), v;

    cpu.p = (uint8_t)((cpu.p & ~P_C) | (lv >> 7));
    cpu.x = (uint8_t)(lv << 1);
    v = rd8(abs_db((uint16_t)(LEVEL_035C_TABLE + cpu.x)));
    wr8(abs_db(LEVEL_035C), v);
    v = rd8(abs_db((uint16_t)(LEVEL_035C_TABLE + 1 + cpu.x)));
    wr8(abs_db(LEVEL_035C + 1), v);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/*
 * @routine 81:8716 LevelSpotsSet
 * WHAT IT DOES: Finds the level's trigger spots: how many, and where their
 * list is.
 * HOW: The level's word in the spot table is the list's offset; its first
 * byte is the count.
 * Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: M and X 8-bit;
 * A low byte = the count, high byte = the level word * 2's; X = the low
 * byte of the list's offset; Y = the level * 2 (low byte); N, Z from the
 * count; C = bit 15 of the level word; V unchanged.
 */
void abi_LevelSpotsSet(void)
{
    uint16_t lv, x;
    uint8_t n;

    op_rep(P_M | P_X);
    lv = rd16(abs_db(LEVEL));
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (lv >> 15));
    cpu.a = (uint16_t)(lv << 1);
    cpu.y = cpu.a;
    x = rd16(abs_db((uint16_t)(LEVEL_SPOT_TABLE + cpu.y)));
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
    op_sep(P_M);
    n = rd8(abs_db((uint16_t)(LEVEL_SPOT_TABLE + x)));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | n);
    cpu.p = nz8(cpu.p, n);
    wr8(abs_db(LEVEL_SPOT_COUNT), n);
    wr16(abs_db(LEVEL_SPOTS), x);
    op_sep(P_X);
}

/*
 * @routine 81:84B2 LevelStartPlace
 * WHAT IT DOES: Puts the player and the camera at the level's start (its
 * restart point $0360): the player's position and facing, both layers'
 * scroll and the point actors are kept in range of.
 * HOW: $1738 cleared; the level's start list (a word at $80:ED9D + level
 * * 2, plus $0360 * 4) gives the player's x (bit 15: facing left, $08EF =
 * $0101) and y; the camera list ($80:EF15) gives layer 0's scroll x and
 * y, also its last and drawn positions; layer 1 takes them halved (x
 * whole on level $10 or with $083F set; y whole on levels 0, 3, $0D,
 * $14, $1A); the range point = the scroll + $80 each way; $1A40 = 0,
 * $1A42 = $7F00.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: M and X 8-bit;
 * A = $7F00 (16 bits); X, Y as the tables left them (low bytes);
 * scratch $0C = $0360 * 4.
 */
void abi_LevelStartPlace(void)
{
    ldx8_v(0x00);  /* 8184B2 ldx */
L_8184B4:
    stz8(ea_absx(LAYER0));  /* 8184B4 stz */
    dex8();  /* 8184B7 dex */
    if (!IF_N) goto L_8184B4;  /* 8184B8 bpl */
    lda8(ea_abs(LEVEL));  /* 8184BA lda */
    ACC8(asl);  /* 8184BD asl */
    tay8();  /* 8184BE tay */
    lda8(ea_abs(0x0360));  /* 8184BF lda */
    ACC8(asl);  /* 8184C2 asl */
    ACC8(asl);  /* 8184C3 asl */
    sta8(ea_abs(SCRATCH));  /* 8184C4 sta */
    stz8(ea_abs(0x000D));  /* 8184C7 stz */
    op_rep(0x30);  /* 8184CA rep */
    clc();  /* 8184CC clc */
    lda16(ea_absy(0xED9D));  /* 8184CD lda */
    adc16_v(rd16(ea_abs(SCRATCH)));  /* 8184D0 adc */
    tax16();  /* 8184D3 tax */
    lda16(ea_absx(0xED9D));  /* 8184D4 lda */
    pha16();  /* 8184D7 pha */
    and16_v(0x7FFF);  /* 8184D8 and */
    sta16(ea_abs(0x08E7));  /* 8184DB sta */
    stz16(ea_abs(0x08EF));  /* 8184DE stz */
    pla16();  /* 8184E1 pla */
    if (!IF_N) goto L_8184EA;  /* 8184E2 bpl */
    lda16_v(0x0101);  /* 8184E4 lda */
    sta16(ea_abs(0x08EF));  /* 8184E7 sta */
L_8184EA:
    lda16(ea_absx(0xED9F));  /* 8184EA lda */
    sta16(ea_abs(0x08EA));  /* 8184ED sta */
    clc();  /* 8184F0 clc */
    lda16(ea_absy(0xEF15));  /* 8184F1 lda */
    adc16_v(rd16(ea_abs(SCRATCH)));  /* 8184F4 adc */
    tax16();  /* 8184F7 tax */
    lda16(ea_absx(0x0000));  /* 8184F8 lda */
    sta16(ea_abs(0x1773));  /* 8184FB sta */
    sta16(ea_abs(SCROLL_X_BEFORE));  /* 8184FE sta */
    sta16(ea_abs(0x176C));  /* 818501 sta */
    ldy16(ea_abs(LEVEL));  /* 818504 ldy */
    cpy16_v(0x0010);  /* 818507 cpy */
    if (IF_Z) goto L_818512;  /* 81850A beq */
    ldy16(ea_abs(0x083F));  /* 81850C ldy */
    if (!IF_Z) goto L_818512;  /* 81850F bne */
    ACC16(lsr);  /* 818511 lsr */
L_818512:
    sta16(ea_abs(0x18C9));  /* 818512 sta */
    sta16(ea_abs(0x18C2));  /* 818515 sta */
    lda16(ea_absx(SOUND_SEQ));  /* 818518 lda */
    sta16(ea_abs(0x1777));  /* 81851B sta */
    sta16(ea_abs(SCROLL_Y_BEFORE));  /* 81851E sta */
    sta16(ea_abs(0x176E));  /* 818521 sta */
    ldy16(ea_abs(LEVEL));  /* 818524 ldy */
    if (IF_Z) goto L_81853E;  /* 818527 beq */
    cpy16_v(0x0003);  /* 818529 cpy */
    if (IF_Z) goto L_81853E;  /* 81852C beq */
    cpy16_v(0x000D);  /* 81852E cpy */
    if (IF_Z) goto L_81853E;  /* 818531 beq */
    cpy16_v(0x0014);  /* 818533 cpy */
    if (IF_Z) goto L_81853E;  /* 818536 beq */
    cpy16_v(0x001A);  /* 818538 cpy */
    if (IF_Z) goto L_81853E;  /* 81853B beq */
    ACC16(lsr);  /* 81853D lsr */
L_81853E:
    sta16(ea_abs(0x18CD));  /* 81853E sta */
    sta16(ea_abs(0x18C4));  /* 818541 sta */
    clc();  /* 818544 clc */
    lda16(ea_abs(0x1773));  /* 818545 lda */
    adc16_v(0x0080);  /* 818548 adc */
    sta16(ea_abs(0x1A44));  /* 81854B sta */
    clc();  /* 81854E clc */
    lda16(ea_abs(0x1777));  /* 81854F lda */
    adc16_v(0x0080);  /* 818552 adc */
    sta16(ea_abs(0x1A46));  /* 818555 sta */
    stz16(ea_abs(0x1A40));  /* 818558 stz */
    lda16_v(0x7F00);  /* 81855B lda */
    sta16(ea_abs(0x1A42));  /* 81855E sta */
    op_sep(0x30);  /* 818561 sep */
    return;  /* 818563 rts */
}

/*
 * @routine 81:8623 LevelCompanionStart
 * WHAT IT DOES: On a level with a companion actor (the table at $80:B7BB),
 * starts it at the player's place, and loads its palettes.
 * HOW: The level's byte, less one, is the companion's $07; the record at
 * $097B is made new ($0C) with hit box $72, look $0240 and the player's
 * position; palette set $3A, and on level $12 set $48 too (PaletteLoad).
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: as PaletteLoad,
 * or with no companion A low byte = 0, Z set.
 */
void abi_LevelCompanionStart(void)
{
    ldx8(ea_abs(LEVEL));  /* 818623 ldx */
    lda8(ea_absx(0xB7BB));  /* 818626 lda */
    if (IF_Z) goto L_818663;  /* 818629 beq */
    ACC8(dec);  /* 81862B dec */
    sta8(ea_abs(0x0982));  /* 81862C sta */
    lda8_v(0x0C);  /* 81862F lda */
    sta8(ea_abs(0x097B));  /* 818631 sta */
    lda8_v(0x72);  /* 818634 lda */
    sta8(ea_abs(0x0981));  /* 818636 sta */
    op_rep(0x20);  /* 818639 rep */
    lda16_v(0x0240);  /* 81863B lda */
    sta16(ea_abs(0x09A6));  /* 81863E sta */
    lda16(ea_abs(0x08E7));  /* 818641 lda */
    sta16(ea_abs(0x0987));  /* 818644 sta */
    lda16(ea_abs(0x08EA));  /* 818647 lda */
    sta16(ea_abs(0x098A));  /* 81864A sta */
    op_sep(0x20);  /* 81864D sep */
    lda8_v(0x3A);  /* 81864F lda */
    abi_PaletteLoad();  /* 818651 jsl */
    lda8(ea_abs(LEVEL));  /* 818655 lda */
    sec();  /* 818658 sec */
    cmp8_v(0x12);  /* 818659 cmp */
    if (!IF_Z) goto L_818663;  /* 81865B bne */
    lda8_v(0x48);  /* 81865D lda */
    abi_PaletteLoad();  /* 81865F jsl */
L_818663:
    return;  /* 818663 rts */
}

/*
 * @routine 81:86DE LevelSongPlay
 * WHAT IT DOES: Plays the level's song, or sends its sound command when
 * the level's entry is one.
 * HOW: The level's byte in the table at $80:B8EF: below $80, SongPlay;
 * else $81:F9F0 (command $F0).
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: as SongPlay
 * or $81:F9F0.
 */
void abi_LevelSongPlay(void)
{
    ldx8(ea_abs(LEVEL));  /* 8186DE ldx */
    lda8(ea_absx(0xB8EF));  /* 8186E1 lda */
    if (IF_N) goto L_8186EB;  /* 8186E4 bmi */
    abi_SongPlay();  /* 8186E6 jsl */
    return;  /* 8186EA rts */
L_8186EB:
    call_routine(0x81F9F0u, 1);  /* 8186EB jsl */
    return;  /* 8186EF rts */
}

/*
 * @routine 81:86F0 RestartPointCheck
 * WHAT IT DOES: Sets the level's restart point once the camera has
 * passed it: from there a lost life starts the level at its second start.
 * HOW: With no restart point yet ($0360 = 0) and $1423 clear, the level's
 * word in the table at $80:F015 (negative: none) below layer 0's scroll x
 * sets $0360 = 1.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: M and X 8-bit;
 * A = the last word read (16 bits); X as the test left it.
 */
void abi_RestartPointCheck(void)
{
    op_rep(0x20);  /* 8186F0 rep */
    ldx8(ea_abs(0x0360));  /* 8186F2 ldx */
    if (!IF_Z) goto L_818713;  /* 8186F5 bne */
    lda16(ea_abs(0x1423));  /* 8186F7 lda */
    if (!IF_Z) goto L_818713;  /* 8186FA bne */
    lda16(ea_abs(LEVEL));  /* 8186FC lda */
    and16_v(0x00FF);  /* 8186FF and */
    ACC16(asl);  /* 818702 asl */
    tax8();  /* 818703 tax */
    lda16(ea_absx(0xF015));  /* 818704 lda */
    if (IF_N) goto L_818713;  /* 818707 bmi */
    cmp16_v(rd16(ea_abs(0x1773)));  /* 818709 cmp */
    if (IF_C) goto L_818713;  /* 81870C bcs */
    ldx8_v(0x01);  /* 81870E ldx */
    stx8(ea_abs(0x0360));  /* 818710 stx */
L_818713:
    op_sep(0x30);  /* 818713 sep */
    return;  /* 818715 rts */
}

/*
 * @routine 81:872E LevelExitCheck
 * WHAT IT DOES: Ends the level when the player reaches its exit spot (on
 * some levels, when the camera passes it): sets the next stage and starts
 * the player's exit walk.
 * HOW: Not while $141C, $1423 or $141B is set, nor with no spots. Levels
 * marked in the table at $80:B96B: the first spot's x below layer 0's
 * scroll x. Others: each spot in turn, the player within 8 pixels of its
 * x, then of the first spot's y. Reached ($81:879D on): the spot's byte
 * at +6 (low 7 bits) is the stage ($035D), 0 moving the sequence on
 * through the table at $80:B988; the player's code = $E730 (marked
 * levels) or $CBF9, its facing from the byte's bit 7; $0347 bit 7 set;
 * $1423 and $1454 up by one; the restart point cleared.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: M and X 8-bit;
 * A, X, Y as the tests left them.
 */
void abi_LevelExitCheck(void)
{
    lda8(ea_abs(V_141C));  /* 81872E lda */
    ora8_v(rd8(ea_abs(0x1423)));  /* 818731 ora */
    ora8_v(rd8(ea_abs(0x141B)));  /* 818734 ora */
    if (!IF_Z) goto L_81877A;  /* 818737 bne */
    lda8(ea_abs(LEVEL_SPOT_COUNT));  /* 818739 lda */
    if (IF_Z) goto L_81877A;  /* 81873C beq */
    tay8();  /* 81873E tay */
    ldx8(ea_abs(LEVEL));  /* 81873F ldx */
    lda8(ea_absx(0xB96B));  /* 818742 lda */
    if (!IF_Z) goto L_81877B;  /* 818745 bne */
    op_rep(0x30);  /* 818747 rep */
    ldx16(ea_abs(LEVEL_SPOTS));  /* 818749 ldx */
L_81874C:
    sec();  /* 81874C sec */
    lda16(ea_absx(0xEC1F));  /* 81874D lda */
    sbc16_v(rd16(ea_abs(0x08E7)));  /* 818750 sbc */
    clc();  /* 818753 clc */
    adc16_v(0x0008);  /* 818754 adc */
    cmp16_v(0x0010);  /* 818757 cmp */
    if (IF_C) goto L_81876F;  /* 81875A bcs */
    ldx16(ea_abs(LEVEL_SPOTS));  /* 81875C ldx */
    sec();  /* 81875F sec */
    lda16(ea_absx(0xEC21));  /* 818760 lda */
    sbc16_v(rd16(ea_abs(0x08EA)));  /* 818763 sbc */
    clc();  /* 818766 clc */
    adc16_v(0x0008);  /* 818767 adc */
    cmp16_v(0x0010);  /* 81876A cmp */
    if (!IF_C) goto L_81879D;  /* 81876D bcc */
L_81876F:
    clc();  /* 81876F clc */
    txa16();  /* 818770 txa */
    adc16_v(0x0006);  /* 818771 adc */
    tax16();  /* 818774 tax */
    dey16();  /* 818775 dey */
    if (!IF_Z) goto L_81874C;  /* 818776 bne */
L_818778:
    op_sep(0x30);  /* 818778 sep */
L_81877A:
    return;  /* 81877A rts */
L_81877B:
    op_rep(0x30);  /* 81877B rep */
    ldx16(ea_abs(LEVEL_SPOTS));  /* 81877D ldx */
    sec();  /* 818780 sec */
    lda16(ea_absx(0xEC1F));  /* 818781 lda */
    sbc16_v(rd16(ea_abs(0x1773)));  /* 818784 sbc */
    if (IF_C) goto L_818778;  /* 818787 bcs */
    goto L_81879D;  /* 818789 bra */
L_81879D:
    op_sep(0x20);  /* 81879D sep */
    ldx16(ea_abs(LEVEL_SPOTS));  /* 81879F ldx */
    lda8(ea_absx(0xEC23));  /* 8187A2 lda */
    lda8(ea_absx(0xEC24));  /* 8187A5 lda */
    pha8();  /* 8187A8 pha */
    and8_v(0x7F);  /* 8187A9 and */
    sta8(ea_abs(0x035D));  /* 8187AB sta */
    if (!IF_Z) goto L_8187B9;  /* 8187AE bne */
    ldx16(ea_abs(LEVEL_035C));  /* 8187B0 ldx */
    lda8(ea_absx(0xB988));  /* 8187B3 lda */
    sta8(ea_abs(LEVEL_035C));  /* 8187B6 sta */
L_8187B9:
    ldx16_v(0x0000);  /* 8187B9 ldx */
    pla8();  /* 8187BC pla */
    ACC8(asl);  /* 8187BD asl */
    if (!IF_C) goto L_8187C1;  /* 8187BE bcc */
    inx16();  /* 8187C0 inx */
L_8187C1:
    ldy16(ea_abs(LEVEL));  /* 8187C1 ldy */
    lda8(ea_absy(0xB96B));  /* 8187C4 lda */
    if (IF_Z) goto L_8187CE;  /* 8187C7 beq */
    ldy16_v(0xE730);  /* 8187C9 ldy */
    goto L_8187D1;  /* 8187CC bra */
L_8187CE:
    ldy16_v(0xCBF9);  /* 8187CE ldy */
L_8187D1:
    sty16(ea_abs(0x08DE));  /* 8187D1 sty */
    lda8_v(0x80);  /* 8187D4 lda */
    tsb8(ea_abs(0x0347));  /* 8187D6 tsb */
    op_sep(0x30);  /* 8187D9 sep */
    stx8(ea_abs(0x08F0));  /* 8187DB stx */
    stx8(ea_abs(0x08EF));  /* 8187DE stx */
    MEM8(inc, ea_abs(0x1423));  /* 8187E1 inc */
    MEM8(inc, ea_abs(0x1454));  /* 8187E4 inc */
    stz8(ea_abs(0x0360));  /* 8187E7 stz */
    return;  /* 8187EA rts */
}
