/*
 * vram_words.c: single words written to VRAM in the vertical blank: a
 * list of address and value pairs the frame's code builds ($81:9A64 adds
 * to it).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "hw.h"

#define VRAM_WORDS_PENDING 0x0616u /* nonzero: the list waits for the upload */
#define VRAM_WORDS_COUNT 0x0617u   /* how many pairs the list holds */
#define VRAM_WORDS 0x0619u         /* the list: VRAM address, then the word (4 bytes a pair) */
#define DMA_FOUR_REGS 0x04         /* DMAP: four B-bus registers in turn */
#define VMAIN_STEP_HIGH 0x80       /* VMAIN: step after the high byte */

/*
 * @routine 80:884C VramWordsUpload
 * WHAT IT DOES: Writes the list of single VRAM words, when one is
 * pending: each pair's word goes to its own VRAM address.
 * HOW: The pending flag is cleared; DMA channel 0, four registers in turn
 * from VMADDL (address low, high, data low, high), copies count * 4 bytes
 * of the list from bank $00.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM and whose
 * $2100-$21FF and $4300-$430F are the PPU's and the DMA's. Exit: none
 * pending: A low = 0, Z set; the rest unchanged. Uploaded: A = 1 (16
 * bits); N and Z clear; C = bit 6 of the count; M 8-bit; X, Y, V and the
 * other flags unchanged.
 */
void abi_VramWordsUpload(void)
{
    uint8_t pending = rd8(dba(VRAM_WORDS_PENDING)), count;

    a8_load(pending);
    if (!pending)
        return;
    wr8(dba(VRAM_WORDS_PENDING), 0);
    /* DMA channel 0: the list, 4 bytes a pair, to VMADDL and VMDATA */
    wr8(dba((uint16_t)VMAIN), VMAIN_STEP_HIGH);
    wr8(dba((uint16_t)DMAP0), DMA_FOUR_REGS);
    wr8(dba((uint16_t)BBAD0), VMADDL & 0xFF);
    wr16(dba((uint16_t)A1T0L), VRAM_WORDS);
    wr16(dba((uint16_t)A1B0), 0x0000);
    count = rd8(dba(VRAM_WORDS_COUNT));
    flag_set(P_C, (count >> 6) & 1);
    cpu.a = (uint8_t)(count << 2);
    wr8(dba((uint16_t)DAS0L), (uint8_t)cpu.a);
    a8_load(0x01);
    wr8(dba((uint16_t)MDMAEN), 0x01);
}
