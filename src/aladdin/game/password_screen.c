/*
 * password_screen.c: the password screen's four slots, each showing a
 * picture: its tiles drawn in the slot's place, its palette in the
 * slot's.
 */
#include "machine.h"
#include "ops65.h"
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
