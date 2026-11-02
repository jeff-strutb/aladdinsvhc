/*
 * vram_words.c: single words written to VRAM in the vertical blank: a
 * list of address and value pairs the frame's code builds ($81:9A64 adds
 * to it).
 */
#include "machine.h"
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
    lda8(ea_abs(VRAM_WORDS_PENDING));
    if (IF_Z) return;
    stz8(ea_abs(VRAM_WORDS_PENDING));
    lda8_v(VMAIN_STEP_HIGH);
    sta8(ea_abs(VMAIN));
    lda8_v(DMA_FOUR_REGS);
    sta8(ea_abs(DMAP0));
    lda8_v(VMADDL & 0xFF);
    sta8(ea_abs(BBAD0));
    op_rep(0x20);
    lda16_v(VRAM_WORDS);
    sta16(ea_abs(A1T0L));
    lda16_v(0x0000);
    sta16(ea_abs(A1B0));
    op_sep(0x20);
    lda8(ea_abs(VRAM_WORDS_COUNT));
    ACC8(asl);
    ACC8(asl);
    sta8(ea_abs(DAS0L));
    op_sep(0x20);
    lda8_v(0x01);
    sta8(ea_abs(MDMAEN));
}
