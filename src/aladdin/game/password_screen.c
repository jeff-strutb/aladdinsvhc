/*
 * password_screen.c: the password screen's four slots, each showing a
 * picture: its tiles drawn in the slot's place, its palette in the
 * slot's.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
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

#define PICTURE_ROW_BITS 0xFE22u  /* by row, the tilemap bits (words, in the bank DB names) */
#define PICTURE_TILES 0xFE2Eu     /* by column, the picture's first tile (words) */

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
    uint8_t row = rd8(dba(SCRATCH + 3)), col = rd8(dba(SCRATCH + 2));
    uint16_t bits;

    cpu.y = (uint8_t)(row << 1);
    cpu.x = (uint8_t)(col << 1);
    op_rep(0x30);
    bits = (uint16_t)(rd16(dba(SCRATCH + 1)) & 0xFF00);   /* the column, as a high byte */
    flag_set(P_C, (bits >> 14) & 1);
    bits = (uint16_t)((bits << 2) | 0x1000 | rd16(dba(PICTURE_ROW_BITS) + cpu.y));
    cpu.y = bits;
    a16_load(rd16(dba(PICTURE_TILES) + cpu.x));
    abi_TileBlockQueue();
    a8_load(row);
    x8_load(col);
    abi_PaletteFromRom();
}



