/*
 * vram_queue.c: the VRAM uploads the game asks for between frames, which
 * the NMI makes by DMA (channel 0) or by VMDATA writes while the screen is
 * in V-blank.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"
#include "hw.h"
#include "wram_video.h"

/* In the bank DB names (the NMI's, $80): text buffer parts, a byte offset
   for each of 3 parts and then their 6-byte records (VRAM address, source
   in bank $7E, byte count); uploads by number, a byte offset for each and
   then their 10-byte records (VMAIN, VRAM address, DMAP0 and BBAD0,
   source address, then its bank and the byte count's low byte as one
   word, then the count's high byte). */
#define TEXT_PART_TABLE 0xB61Bu
#define UPLOAD_TABLE_INDEX 0xB62Fu
#define UPLOAD_TABLE 0xB630u

/* Either nonzero: no slot's list is uploaded this frame. */
#define VRAM_LISTS_HELD 0x001737u
#define VRAM_LISTS_HELD_TOO 0x000403u

static uint32_t in_db(uint32_t addr)
{
    return (uint32_t)cpu.db << 16 | addr;
}

/* The last steps of every DMA upload: SEP #$20, LDA #$01, STA MDMAEN. */
static void dma_start(void)
{
    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 1);
    cpu.p = nz8(cpu.p, 1);
    wr8(MDMAEN, 1);
}

/*
 * @routine 80:84AA TextBufferPartUpload
 * WHAT IT DOES: Sends one part of the text buffer to VRAM when it was
 * asked for since the last NMI.
 * HOW: X picks the part and its pending flag. When the flag is set it is
 * cleared, and the part's record gives the VRAM address, the source in
 * bank $7E and the byte count for DMA channel 0 to VMDATA, the address
 * stepping after each high byte. The bank store writes the count's low
 * byte too ($00), before the count is stored.
 * Entry: M 8-bit, X 8-bit; X = the part (0-2); DB $80 (the table's bank,
 * with work RAM at $0000-$1FFF and the registers). Exit: nothing pending:
 * A low = 0, N clear, Z set, Y unchanged. Sent: A low = 1, A high = the
 * count's high byte, N and Z clear, Y = the record's offset; M 8-bit. X
 * and the other flags unchanged.
 */
void abi_TextBufferPartUpload(void)
{
    uint8_t x = (uint8_t)cpu.x, a = rd8(TEXT_PARTS_PENDING + x), y;

    cpu.p = nz8(cpu.p, a);
    if (a == 0) {
        cpu.a &= 0xFF00;
        return;
    }
    wr8(VMAIN, 0x80);
    wr8(TEXT_PARTS_PENDING + x, 0);
    y = rd8(in_db(TEXT_PART_TABLE + x));
    cpu.y = y;
    op_rep(P_M);
    wr16(VMADDL, rd16(in_db(TEXT_PART_TABLE + y)));
    wr16(DMAP0, 0x1801);
    wr16(A1T0L, rd16(in_db(TEXT_PART_TABLE + 2 + y)));
    wr16(A1B0, TEXT_TILEMAP >> 16);
    op_lda16(in_db(TEXT_PART_TABLE + 4 + y));
    wr16(DAS0L, cpu.a);
    dma_start();
}

/*
 * @routine 80:849E TextBufferPartsUpload
 * WHAT IT DOES: Sends whichever of the text buffer's three parts were
 * asked for since the last NMI.
 * HOW: TextBufferPartUpload for parts 0 and 1, then on into it for part 2.
 * Entry: M 8-bit, X 8-bit; DB $80. Exit: as TextBufferPartUpload for part
 * 2 (X = 2).
 */
void abi_TextBufferPartsUpload(void)
{
    cpu.x = 0;
    cpu.p = nz8(cpu.p, 0);
    abi_TextBufferPartUpload();
    cpu.x = 1;
    cpu.p = nz8(cpu.p, 1);
    abi_TextBufferPartUpload();
    cpu.x = 2;
    cpu.p = nz8(cpu.p, 2);
    abi_TextBufferPartUpload();
}

/*
 * @routine 80:84E2 VramUploadByNumber
 * WHAT IT DOES: Makes the VRAM upload the game asked for by number since
 * the last NMI, if any.
 * HOW: The number (1 up) picks a record whose bytes set VMAIN, the VRAM
 * address, DMA channel 0's mode and B-bus register, source address, bank
 * and byte count; the request is cleared and the DMA started. The source
 * bank and the count's low byte go out as one word, the count's high byte
 * on its own.
 * Entry: M 8-bit, X 8-bit; DB $80 (the table's bank, with work RAM at
 * $0000-$1FFF and the registers). Exit: none asked for: Y = 0, N clear,
 * Z set; A and X unchanged. Made: A low = 1, A high = the count's low
 * byte; X = the record's offset; Y = the count's high byte; N and Z clear;
 * M 8-bit. The other flags unchanged.
 */
void abi_VramUploadByNumber(void)
{
    uint8_t y = rd8(VRAM_UPLOAD_NUMBER), x;

    cpu.y = y;
    cpu.p = nz8(cpu.p, y);
    if (y == 0)
        return;
    wr8(VRAM_UPLOAD_NUMBER, 0);
    x = rd8(in_db(UPLOAD_TABLE_INDEX + y));
    cpu.x = x;
    op_rep(P_M);
    wr8(VMAIN, rd8(in_db(UPLOAD_TABLE + x)));
    wr16(VMADDL, rd16(in_db(UPLOAD_TABLE + 1 + x)));
    wr16(DMAP0, rd16(in_db(UPLOAD_TABLE + 3 + x)));
    wr16(A1T0L, rd16(in_db(UPLOAD_TABLE + 5 + x)));
    op_lda16(in_db(UPLOAD_TABLE + 7 + x));
    wr16(A1B0, cpu.a);
    y = rd8(in_db(UPLOAD_TABLE + 9 + x));
    cpu.y = y;
    wr8(DAS0H, y);
    dma_start();
}

/*
 * @routine 80:841E VramQueueFlush
 * WHAT IT DOES: Makes every DMA upload queued since the last NMI.
 * HOW: When the queue is marked pending, each of its count records (8
 * bytes: VRAM address, source address, bank, byte count, VMAIN) sets DMA
 * channel 0 to VMDATA and starts it, VMAIN set last; then the count and
 * the mark are cleared. The bank store writes the count's low byte too,
 * before the count is stored.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
 * whose $2100-$43FF are the registers. Exit: not pending: A low = 0, N
 * clear, Z set. Pending with none queued: A low = the mark, Y = 0, N clear,
 * Z set. Flushed: A = 8 times the count (16 bits), X its low byte, Y = 0;
 * N clear, Z set; V and C from the last step of X; M 8-bit. The other
 * registers and flags unchanged.
 */
void abi_VramQueueFlush(void)
{
    uint8_t a = rd8(VRAM_QUEUE_PENDING), x, y;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = nz8(cpu.p, a);
    if (a == 0)
        return;
    y = rd8(VRAM_QUEUE_COUNT);
    cpu.y = y;
    cpu.p = nz8(cpu.p, y);
    if (y != 0) {
        x = 0;
        op_rep(P_M);
        do {
            wr16(VMADDL, rd16(VRAM_QUEUE + x));
            wr16(DMAP0, 0x1801);
            wr16(A1T0L, rd16(VRAM_QUEUE + 2 + x));
            wr16(A1B0, rd16(VRAM_QUEUE + 4 + x));
            op_lda16(VRAM_QUEUE + 5 + x);
            wr16(DAS0L, cpu.a);
            wr8(VMAIN, rd8(VRAM_QUEUE + 7 + x));
            wr8(MDMAEN, 1);
            cpu.a = x;
            op_add16(8);
            x = (uint8_t)cpu.a;
            y--;
            cpu.p = nz8(cpu.p, y);
        } while (y != 0);
        cpu.x = x;
        cpu.y = y;
        op_sep(P_M);
        wr8(VRAM_QUEUE_COUNT, 0);
    }
    wr8(VRAM_QUEUE_PENDING, 0);
}

/*
 * @routine 80:87FE TileBlockQueueFlush
 * WHAT IT DOES: Writes every 16x16 tilemap block queued since the last
 * NMI into VRAM.
 * HOW: When blocks are pending, each 10-byte record gives a VRAM address
 * and four tilemap words: two at the address, two one row (32 words) on.
 * The writes go through VMDATA, the address stepping after each high
 * byte. Then the queue's length and the pending flag are cleared.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
 * whose $2100-$21FF are the PPU registers. Exit: not pending: A low = 0,
 * N clear, Z set. Pending with none queued: A low = the flag, X = 0, N
 * clear, Z set. Written: A = the queue's length (16 bits), X its low byte;
 * N clear, Z and C set; V from the last step of X; M 8-bit. Y and the
 * other flags unchanged.
 */
void abi_TileBlockQueueFlush(void)
{
    uint8_t a = rd8(TILE_BLOCKS_PENDING), x, end;
    uint16_t vram;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = nz8(cpu.p, a);
    if (a == 0)
        return;
    end = rd8(TILE_BLOCKS_LENGTH);
    cpu.x = end;
    cpu.p = nz8(cpu.p, end);
    if (end != 0) {
        wr8(VMAIN, 0x80);
        x = 0;
        op_rep(P_M);
        do {
            vram = rd16(TILE_BLOCKS + x);
            wr16(VMADDL, vram);
            wr16(VMDATAL, rd16(TILE_BLOCKS + 2 + x));
            wr16(VMDATAL, rd16(TILE_BLOCKS + 4 + x));
            cpu.a = vram;
            op_add16(32);
            wr16(VMADDL, cpu.a);
            wr16(VMDATAL, rd16(TILE_BLOCKS + 6 + x));
            wr16(VMDATAL, rd16(TILE_BLOCKS + 8 + x));
            cpu.a = x;
            op_add16(10);
            x = (uint8_t)cpu.a;
            end = rd8(TILE_BLOCKS_LENGTH);
            cpu.p = cmp8_flags(cpu.p, x, end);
        } while (x != end);
        cpu.x = x;
        op_sep(P_M);
        wr8(TILE_BLOCKS_LENGTH, 0);
    }
    wr8(TILE_BLOCKS_PENDING, 0);
}

/*
 * @routine 80:8899 VramListUpload
 * WHAT IT DOES: Makes a list of DMA uploads to VRAM that a sprite or
 * effect slot has asked for.
 * HOW: Y picks the slot. When its pending byte is set it is cleared, and
 * the slot's pointer (in the bank DB names) leads to 8-byte records: VRAM
 * address, source address, bank, byte count, and a byte that is nonzero
 * when another record follows. Each sets DMA channel 0 to VMDATA, VMAIN
 * $80, and starts it. X and Y are 16-bit while it works.
 * Entry: M 8-bit, X 8-bit; Y = the slot's offset; DB a bank whose
 * $0000-$1FFF is work RAM and whose $2100-$43FF are the registers, the
 * bank of the records. Exit: not pending: A low = 0, N clear, Z set, X
 * unchanged. Made: A low = 0 and N clear, Z set; X = the last record's
 * address, low byte; Y = the last count's low byte; A high, V and C from
 * the last step of X when there were two records or more, else as at
 * entry. The other flags unchanged.
 */
void abi_VramListUpload(void)
{
    uint8_t y = (uint8_t)cpu.y, a = rd8(VRAM_LIST_PENDING + y);
    uint16_t x;

    cpu.p = nz8(cpu.p, a);
    if (a == 0) {
        cpu.a &= 0xFF00;
        return;
    }
    wr8(VRAM_LIST_PENDING + y, 0);
    op_rep(P_X);
    x = rd16(VRAM_LIST_POINTER + y);
    for (;;) {
        wr16(VMADDL, rd16(in_db(x)));
        wr16(DMAP0, 0x1801);
        wr16(A1T0L, rd16(in_db((uint32_t)x + 2)));
        wr8(A1B0, rd8(in_db((uint32_t)x + 4)));
        cpu.y = rd16(in_db((uint32_t)x + 5));
        wr16(DAS0L, cpu.y);
        wr8(VMAIN, 0x80);
        wr8(MDMAEN, 1);
        a = rd8(in_db((uint32_t)x + 7));
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
        cpu.p = nz8(cpu.p, a);
        if (a == 0)
            break;
        cpu.p &= (uint8_t)~P_C;
        op_rep(P_M);
        cpu.a = x;
        cpu.a = op_adc16(cpu.a, 8, &cpu.p);
        x = cpu.a;
        op_sep(P_M);
    }
    cpu.x = x;
    op_sep(P_X);
}

/*
 * @routine 80:8883 VramListsUpload
 * WHAT IT DOES: Makes the VRAM lists of all ten slots that asked for one,
 * unless uploads are held this frame.
 * HOW: Held while either of $1737 and $0403 is nonzero. VramListUpload
 * runs for slots $90, $80 ... $00, the slot offset kept on the stack
 * across each.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
 * whose $2100-$43FF are the registers. Exit: held: A low = the two ORed,
 * N and Z from it. Run: A low = Y = $F0; N set, Z and C clear; V from the
 * last subtraction; X as the last VramListUpload leaves it. A high and the
 * other flags as the calls leave them.
 */
void abi_VramListsUpload(void)
{
    uint8_t a = rd8(VRAM_LISTS_HELD);

    a |= rd8(VRAM_LISTS_HELD_TOO);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = nz8(cpu.p, a);
    if (a != 0)
        return;
    cpu.y = 0x90;
    cpu.p = nz8(cpu.p, 0x90);
    do {
        a = (uint8_t)cpu.y;
        abi_VramListUpload();
        cpu.p |= P_C;
        a = op_sbc8(a, 0x10, &cpu.p);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
        cpu.y = a;
        cpu.p = nz8(cpu.p, a);
    } while (cpu.p & P_C);
}
