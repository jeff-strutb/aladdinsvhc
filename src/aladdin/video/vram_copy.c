/*
 * vram_copy.c: copying a block of words to VRAM by VMDATA writes, from an
 * entry of a table of copies.
 */
#include "machine.h"
#include "ops65.h"
#include "hw.h"
#include "wram.h"

/* In the bank DB names, 7 bytes an entry: the source's long address, the
   VRAM word address, the count of words. A second table follows $103
   bytes on. */
#define VRAM_COPIES 0xF197u
#define VRAM_COPIES_2 0x0103

/* From LDA #0; TCD: D 0, the entry at X to the direct page's long pointer
   at $0C, VMADD, then the words through VMDATA; PLP; RTL. */
static void copy_entry(void)
{
    lda16_v(0x0000);
    tcd();
    lda16(ea_absx(VRAM_COPIES));
    sta16(ea_abs(SCRATCH));
    lda16(ea_absx(VRAM_COPIES + 2));
    sta16(ea_abs(SCRATCH + 2));
    lda16(ea_absx(VRAM_COPIES + 3));
    sta16(ea_abs(VMADDL));
    lda16(ea_absx(VRAM_COPIES + 5));
    tax16();
    ldy16_v(0x0000);
    do {
        lda16(ea_dpily(0x0C));
        sta16(ea_abs(VMDATAL));
        iny16();
        iny16();
        dex16();
    } while (!IF_Z);
    plp();
}

/*
 * @routine 81:F5DC VramCopy
 * WHAT IT DOES: Copies the block of words entry X of the copy table names
 * to VRAM.
 * HOW: VMAIN steps the address after each word; with 16-bit registers
 * and D set to 0, the source's long pointer goes in direct-page $0C-$0E
 * and the words through VMDATA.
 * Entry: X 16-bit or 8-bit; X = the entry's offset; DB the table's bank,
 * whose $0000-$1FFF is work RAM and whose $2100-$21FF are the PPU
 * registers; called by JSL. Exit: D = 0; A = the last word; X = 0; Y =
 * twice the count; P as at entry.
 */
void abi_VramCopy(void)
{
    php();
    lda8_v(0x80);
    sta8(ea_abs(VMAIN));
    op_rep(0x30);
    copy_entry();
}

/*
 * @routine 81:F60D VramCopy2
 * WHAT IT DOES: VramCopy for entry X of the second copy table.
 * HOW: X plus $103, then as VramCopy.
 * Entry: as VramCopy. Exit: as VramCopy.
 */
void abi_VramCopy2(void)
{
    php();
    lda8_v(0x80);
    sta8(ea_abs(VMAIN));
    op_rep(0x30);
    txa16();
    clc();
    adc16_v(VRAM_COPIES_2);
    tax16();
    copy_entry();
}
