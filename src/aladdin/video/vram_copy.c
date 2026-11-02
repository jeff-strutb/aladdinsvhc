/*
 * vram_copy.c: copying a block of words to VRAM by VMDATA writes, from an
 * entry of a table of copies.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
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
/* The copy at entry X: with D = 0, its source (long) to scratch, its VRAM
   address to VMADDL, then its count of words through VMDATAL; P pulled. */
static void copy_entry(void)
{
    uint32_t entry = dba(VRAM_COPIES) + cpu.x;

    cpu.a = cpu.d = 0;
    wr16(dba(SCRATCH), rd16(entry));
    wr16(dba(SCRATCH + 2), rd16(entry + 2));
    wr16(dba((uint16_t)VMADDL), rd16(entry + 3));
    cpu.x = rd16(entry + 5);
    cpu.y = 0;
    do {
        cpu.a = rd16((rd24(dpa(SCRATCH)) + cpu.y) & 0xFFFFFF);
        wr16(dba((uint16_t)VMDATAL), cpu.a);
        cpu.y = (uint16_t)(cpu.y + 2);
        cpu.x = (uint16_t)(cpu.x - 1);
    } while (cpu.x);
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
    a8_load(0x80);
    wr8(dba((uint16_t)VMAIN), 0x80);
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
    a8_load(0x80);
    wr8(dba((uint16_t)VMAIN), 0x80);
    op_rep(0x30);
    flag_set(P_C, 0);
    cpu.x = op_adc16(cpu.x, VRAM_COPIES_2, &cpu.p);   /* the second table's entry */
    cpu.a = cpu.x;
    copy_entry();
}
