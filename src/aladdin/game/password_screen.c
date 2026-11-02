/*
 * password_screen.c: the password screen's four slots, each showing a
 * picture: its tiles drawn in the slot's place, its palette in the
 * slot's.
 */
#include "machine.h"
#include "ops65.h"
void abi_CopierCheck(void);
void abi_DisplayDefaults(void);
void abi_KeptOptionsSeal(void);
void abi_NmiDisable(void);
void abi_NmiEnable(void);
void abi_OamBufferClear(void);
void abi_PaletteLoad(void);
void abi_PasswordScreen(void);
void abi_RegionCheck(void);
void abi_ScrollStateClear(void);
void abi_SongPlay(void);
void abi_SoundQueueAdd(void);
void abi_SoundQueueF6With1(void);
void abi_TaskCreate(void);
void abi_TaskExec(void);
void abi_TaskWait(void);
void abi_VramCopy(void);
void abi_VramCopy2(void);
void abi_PaletteFromRom(void);
void abi_PasswordPictureDraw(void);
void abi_TileBlockQueue(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/*
 * @routine 84:AF52 PasswordPictureDraw
 * WHAT IT DOES: Shows picture $000F in password slot $000E: its tiles in
 * the slot's place and its palette in the slot's.
 * HOW: TileBlockQueue at the slot's VRAM address ($80:FE2E) with the
 * picture's first tile ($80:FE22) in palette 4 + the slot; then
 * PaletteFromRom, ROM palette = the picture, palette = the slot.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: as
 * PaletteFromRom.
 */
void abi_PasswordPictureDraw(void)
{
    lda8(ea_abs(0x000F));  /* 84AF52 lda */
    ACC8(asl);  /* 84AF55 asl */
    tay8();  /* 84AF56 tay */
    lda8(ea_abs(0x000E));  /* 84AF57 lda */
    ACC8(asl);  /* 84AF5A asl */
    tax8();  /* 84AF5B tax */
    op_rep(0x30);  /* 84AF5C rep */
    lda16(ea_abs(0x000D));  /* 84AF5E lda */
    and16_v(0xFF00);  /* 84AF61 and */
    ACC16(asl);  /* 84AF64 asl */
    ACC16(asl);  /* 84AF65 asl */
    ora16_v(0x1000);  /* 84AF66 ora */
    ora16_v(rd16(ea_absy(0xFE22)));  /* 84AF69 ora */
    tay16();  /* 84AF6C tay */
    lda16(ea_absx(0xFE2E));  /* 84AF6D lda */
    abi_TileBlockQueue();  /* 84AF70 jsl */
    lda8(ea_abs(0x000F));  /* 84AF74 lda */
    ldx8(ea_abs(0x000E));  /* 84AF77 ldx */
    abi_PaletteFromRom();  /* 84AF7A jsl */
    return;  /* 84AF7E rts */
}

/*
 * @routine 84:AF7F CopierCheck
 * WHAT IT DOES: A check for cartridge RAM at $70:1522 (a copier's): when
 * $38 and $82 both read back there, lives, health and $0365 are set to 1.
 * This board has no such RAM (each read is the bus's last byte), so the
 * punishment never runs and no script covers it.
 * HOW: DB = $70 ($28 + $48); $38 written and compared, then $82; both
 * equal: $0364, $0367 and $0365 = 1.
 * Entry: M 8-bit, X 8-bit; called by JSR. Exit: DB as at entry; RAM
 * found: Y = 0, A = 1.
 */
void abi_CopierCheck(void)
{
    phb();  /* 84AF7F phb */
    lda8_v(0x28);  /* 84AF80 lda */
    clc();  /* 84AF82 clc */
    adc8_v(0x48);  /* 84AF83 adc */
    pha8();  /* 84AF85 pha */
    plb();  /* 84AF86 plb */
    ldy8_v(0x00);  /* 84AF87 ldy */
    lda8_v(0x38);  /* 84AF89 lda */
    sta8(ea_absy(0x1522));  /* 84AF8B sta */
    cmp8_v(rd8(ea_absy(0x1522)));  /* 84AF8E cmp */
    if (!IF_Z) goto L_84AFAC;  /* 84AF91 bne */
    lda8_v(0x82);  /* 84AF93 lda (no script reaches it) */
    sta8(ea_absy(0x1522));  /* 84AF95 sta */
    cmp8_v(rd8(ea_absy(0x1522)));  /* 84AF98 cmp */
    if (!IF_Z) goto L_84AFAC;  /* 84AF9B bne */
    plb();  /* 84AF9D plb */
    lda8_v(0x01);  /* 84AF9E lda */
    sta8(ea_abs(0x0364));  /* 84AFA0 sta */
    lda8_v(0x01);  /* 84AFA3 lda */
    sta8(ea_abs(0x0367));  /* 84AFA5 sta */
    sta8(ea_abs(0x0365));  /* 84AFA8 sta */
    return;  /* 84AFAB rts */
L_84AFAC:
    plb();  /* 84AFAC plb */
    return;  /* 84AFAD rts */
}

/*
 * @routine 84:AFAE RegionCheck
 * WHAT IT DOES: Clears $005D, and on a PAL console (STAT78 bit 4) sets it
 * when the cartridge's region byte ($80:FFD9) is one of those listed at
 * $FE54. This board is NTSC: the PAL branch never runs and no script
 * covers it.
 * HOW: STAT78 and $10; set: the byte compared with the list from its last
 * entry (its count at $FE54) down; found: $005D up.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSL. Exit: M 8-bit; A =
 * the masked STAT78 or the region byte.
 */
void abi_RegionCheck(void)
{
    stz8(ea_abs(0x005D));  /* 84AFAE stz */
    lda8(ea_abs(STAT78));  /* 84AFB1 lda */
    and8_v(0x10);  /* 84AFB4 and */
    if (IF_Z) goto L_84AFC7;  /* 84AFB6 beq */
    lda8(0x80FFD9u);  /* 84AFB8 lda (no script reaches it) */
    ldx8(ea_abs(0xFE54));  /* 84AFBC ldx */
L_84AFBF:
    cmp8_v(rd8(ea_absx(0xFE54)));  /* 84AFBF cmp */
    if (IF_Z) goto L_84AFC8;  /* 84AFC2 beq */
    dex8();  /* 84AFC4 dex */
    if (!IF_Z) goto L_84AFBF;  /* 84AFC5 bne */
L_84AFC7:
    return;  /* 84AFC7 rtl */
L_84AFC8:
    MEM8(inc, ea_abs(0x005D));  /* 84AFC8 inc (no script reaches it) */
    return;  /* 84AFCB rtl */
}

