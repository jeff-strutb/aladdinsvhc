/*
 * tilemap.c: the text layer's tilemap buffer and plain VRAM fills.
 *
 * The text layer's 32x32 tilemap is kept in work RAM at $7E:D800 (2 KB);
 * setting the upload request ($0835) has the NMI copy it to VRAM.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "hw.h"
#include "wram.h"

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

/*
 * @routine 81:95B8 TextLayerFill
 * WHAT IT DOES: Fills the whole text layer's tilemap buffer with one
 * tilemap entry.
 * HOW: Stores A as a word in each of the buffer's 1,024 entries, last to
 * first.
 * Entry: A = the entry (16 bits; M either width). Exit: X = $FFFE, or $FE
 * with X 8-bit; A, Y and the flags as at entry (P pulled back); the buffer
 * filled.
 */
void abi_TextLayerFill(void)
{
    uint8_t p = cpu.p;
    int x;

    op_rep(P_M | P_X);
    for (x = 0x07FE; x >= 0; x -= 2)
        wr16(TEXT_TILEMAP + x, cpu.a);
    cpu.x = 0xFFFE;
    op_plp(p);
}

/*
 * @routine 81:95C8 TextLayerClear
 * WHAT IT DOES: Blanks the text layer and has it uploaded to VRAM.
 * HOW: TextLayerFill with entry $0020 (tile $20, the blank), then sets the
 * upload request.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: A = $0020; X = $FE (X 8-bit) or $FFFE; N, Z from the request's
 * increment; Y, C, V unchanged.
 */
void abi_TextLayerClear(void)
{
    uint8_t v;

    cpu.a &= 0xFF00;
    op_xba();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x20);
    cpu.p = nz8(cpu.p, 0x20);
    abi_TextLayerFill();
    v = (uint8_t)(rd8_rmw(abs_db(TEXT_TILEMAP_UPLOAD)) + 1);
    wr8(abs_db(TEXT_TILEMAP_UPLOAD), v);
    cpu.p = nz8(cpu.p, v);
}

/*
 * @routine 81:9607 TextLayerClearFront
 * WHAT IT DOES: Blanks the text layer with blank tiles in front of the
 * other layers, and has it uploaded to VRAM.
 * HOW: Stores $2020 (tile $20, priority set) in each of the buffer's 1,024
 * entries, first to last, counting in $000C; then sets the upload request.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: M and X 8-bit; A = $2020; X = $0800; Y unchanged; $000C = 0; N, Z
 * from the request's increment; C, V unchanged.
 */
void abi_TextLayerClearFront(void)
{
    uint16_t x = 0, n;
    uint8_t v;

    op_rep(P_M | P_X);
    cpu.x = 0x0000;
    cpu.a = 0x0400;
    wr16(abs_db(SCRATCH), 0x0400);
    cpu.a = 0x2020;
    do {
        wr16(TEXT_TILEMAP + x, 0x2020);
        x += 2;
        n = (uint16_t)(rd16_rmw(abs_db(SCRATCH)) - 1);
        wr16_rmw(abs_db(SCRATCH), n);
    } while (n != 0);
    cpu.x = x;
    op_sep(P_M | P_X);
    v = (uint8_t)(rd8_rmw(abs_db(TEXT_TILEMAP_UPLOAD)) + 1);
    wr8(abs_db(TEXT_TILEMAP_UPLOAD), v);
    cpu.p = nz8(cpu.p, v);
}

/*
 * @routine 81:95F5 VramFillMap
 * WHAT IT DOES: Fills a 32x32 tilemap in VRAM with one entry.
 * HOW: Word access stepping after the high byte, address X, then Y written
 * 1,024 times.
 * Entry: M 8-bit, X 16-bit; X = the VRAM word address; Y = the entry; DB a
 * bank whose $2115-$2119 are the PPU's. Exit: A low byte = $80; X = 0;
 * N clear, Z set; Y, C, V unchanged.
 */
void abi_VramFillMap(void)
{
    uint16_t x;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x80);
    wr8(abs_db(VMAIN & 0xFFFF), 0x80);
    wr16(abs_db(VMADDL & 0xFFFF), cpu.x);
    for (x = 0x0400; x != 0; x--)
        wr16(abs_db(VMDATAL & 0xFFFF), cpu.y);
    cpu.x = 0;
    cpu.p = nz16(cpu.p, 0);
}
