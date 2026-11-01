/*
 * nmi_upload.c: what the NMI sends to the PPU each frame: the text buffer
 * when asked for, and the scroll positions.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"
#include "hw.h"
#include "wram_video.h"

/*
 * @routine 80:8469 TextBufferUpload
 * WHAT IT DOES: Sends the text buffer to VRAM when one was asked for since
 * the last NMI.
 * HOW: When the pending flag is set it is cleared, and DMA channel 0
 * copies the requested number of bytes from $7E:D800 to VMDATA at the
 * requested VRAM address, the VRAM address stepping after each high byte.
 * The bank store writes the low byte of the count too ($00), before the
 * count is stored.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
 * whose $2100-$43FF are the registers. Exit: nothing pending: A low = 0,
 * N clear, Z set. Sent: A low = 1 and A high = the count's high byte; M
 * 8-bit; N, Z clear. X, Y and the other flags unchanged.
 */
void abi_TextBufferUpload(void)
{
    uint8_t a = rd8(TEXT_TILEMAP_UPLOAD);

    cpu.p = nz8(cpu.p, a);
    if (a == 0) {
        cpu.a &= 0xFF00;
        return;
    }
    wr8(VMAIN, 0x80);
    wr8(TEXT_TILEMAP_UPLOAD, 0);
    op_rep(P_M);
    wr16(VMADDL, rd16(TEXT_UPLOAD_VRAM));
    wr16(DMAP0, 0x1801);
    wr16(A1T0L, (uint16_t)TEXT_TILEMAP);
    wr16(A1B0, TEXT_TILEMAP >> 16);
    op_lda16(TEXT_UPLOAD_SIZE);
    wr16(DAS0L, cpu.a);
    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 1);
    cpu.p = nz8(cpu.p, 1);
    wr8(MDMAEN, 1);
}

/* CLC, then the two bytes of a word plus its shift, low then high (the
   carry from the low byte into the high), each to a write-twice scroll
   register. `hi_shift` is the byte added to the high byte. */
static void scroll_out(uint32_t reg, uint32_t pos, uint32_t lo_shift, uint32_t hi_shift)
{
    uint8_t a;

    cpu.p &= (uint8_t)~P_C;
    a = rd8(pos);
    a = op_adc8(a, rd8(lo_shift), &cpu.p);
    wr8(reg, a);
    a = rd8(pos + 1);
    a = op_adc8(a, rd8(hi_shift), &cpu.p);
    wr8(reg, a);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
}

/*
 * @routine 80:8793 ScrollRegistersWrite
 * WHAT IT DOES: Sets the PPU's scroll registers for BG1, BG2 and BG3 from
 * the game's scroll positions.
 * HOW: BG1's and BG2's positions go out with their shifts added (each a
 * 16-bit sum, low byte then high, to the write-twice register); BG2's
 * horizontal register is skipped while it is held. For BG2's horizontal
 * high byte the original adds the shift's low byte again, not its high
 * byte. BG3's positions go out as they are.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM and whose
 * $2100-$21FF are the PPU registers. Exit: A low = BG3's vertical
 * position's high byte, N and Z from it; C clear; V from the last
 * addition. X, Y and the other flags unchanged.
 */
void abi_ScrollRegistersWrite(void)
{
    uint8_t a;

    scroll_out(BG1HOFS, BG1_X, BG1_X_SHIFT, BG1_X_SHIFT + 1);
    scroll_out(BG1VOFS, BG1_Y, LAYER_OFFSET_Y, LAYER_OFFSET_Y + 1);
    a = rd8(BG2_X_HELD);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = nz8(cpu.p, a);
    if (a == 0)
        scroll_out(BG2HOFS, BG2_X, BG2_X_SHIFT, BG2_X_SHIFT);
    scroll_out(BG2VOFS, BG2_Y, BG2_Y_SHIFT, BG2_Y_SHIFT + 1);
    cpu.p &= (uint8_t)~P_C;
    wr8(BG3HOFS, rd8(BG3_X));
    wr8(BG3HOFS, rd8(BG3_X + 1));
    wr8(BG3VOFS, rd8(BG3_Y));
    a = rd8(BG3_Y + 1);
    wr8(BG3VOFS, a);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = nz8(cpu.p, a);
}
