/*
 * nmi_uploads.c: uploads the NMI makes on request flags the retail game
 * never sets: character sets from $7F:D000 or $7F:D400, a stepped row
 * upload from $7F:B000, two fixed blocks, and a palette upload from a
 * table.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "hw.h"
#include "wram.h"

#define ROWS_ASK 0x082Au        /* nonzero: the stepped upload runs */
#define CHARSET_ASK 0x082Bu     /* 3 or 6: the character set to copy (an offset into CHARSET_SOURCES) */
#define BLOCK_A_ASK 0x082Cu     /* nonzero: the offset of the BLOCKS_A entry to upload */
#define BLOCK_B_ASK 0x082Du     /* nonzero: the offset of the BLOCKS_B entry to read back */
#define ROWS_STEP 0x082Eu       /* the stepped upload's step: 0 start, 1 a row */
#define ROWS_ROW 0x082Fu        /* the next row, two a step; at 0 again the upload is done */
#define ROWS_TILE_HIGH 0x0830u  /* ORed into each nonzero nibble */
#define PALETTE_SOURCE 0x0845u  /* the palette upload's source: an offset into PALETTE_SOURCES */
#define CHARSET_SOURCES 0x8558u /* (this bank) per set: source address (word), bank */
#define ROWS_STEPS 0x8584u      /* the stepped upload's steps (words) */
#define PALETTE_SOURCES 0xB608u /* per source: address (word), bank */
#define BLOCKS_A 0xB6A2u        /* 7 bytes each: source address, bank, size, VRAM address */
#define BLOCKS_B 0xB6EFu        /* 7 bytes each: destination address, bank, size, VRAM address */
#define ROWS_BUFFER 0x7FB000u   /* the stepped upload's nibbles, a byte per two tiles */

/*
 * @routine 80:851B UploadCharsets
 * WHAT IT DOES: When CHARSET_ASK asks, copies one of two character sets
 * from $7F:D000 or $7F:D400 to VRAM from 0, 32 rows of $20 bytes.
 * HOW: CHARSET_ASK (3 or 6) indexes CHARSET_SOURCES (word, bank); it is
 * cleared; DMA channel 0, mode 0 to VMDATAL, 32 transfers of $20 bytes,
 * the VRAM address stepping $80.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR from the NMI. Exit: M
 * 8-bit; Y = 0 after a copy; X = 1; A (16 bits) = $1000.
 */
void abi_UploadCharsets(void)
{
    uint8_t which = rd8(dba(CHARSET_ASK));
    uint16_t at;

    y8_load(which);
    if (!which)
        return;
    wr8(dba(CHARSET_ASK), 0);
    wr8(dba(VMAIN), 0);
    wr8(dba(DMAP0), 0);
    a8_load(0x18);
    wr8(dba(BBAD0), 0x18);
    op_rep(0x20);
    a16_load(rd16(dba((uint16_t)(CHARSET_SOURCES + which))));
    wr16(dba(A1T0L), cpu.a);
    x8_load(rd8(dba((uint16_t)(CHARSET_SOURCES + 2 + which))));
    wr8(dba(A1B0), (uint8_t)cpu.x);
    y8_load(0x20);
    x8_load(0x01);
    a16_load(0x0000);
    at = 0x0000;
    do {
        wr16(dba(VMADDL), at);
        push16(at);
        a16_load(0x0020);
        wr16(dba(DAS0L), 0x0020);
        wr8(dba(MDMAEN), (uint8_t)cpu.x);
        at = pull16();
        flag_set(P_C, 0);
        at = op_adc16(at, 0x0080, &cpu.p);
        cpu.a = at;
        cpu.y = (uint8_t)(cpu.y - 1);
        nz8_of((uint8_t)cpu.y);
    } while (cpu.y);
    op_sep(0x20);
}

/* Step 0: the row back to 0, the step on. */
static void rows_start(void)
{
    wr8(dba(ROWS_ROW), 0);
    (void)inc8_at(dba(ROWS_STEP));
}

/* One tile's word to VMDATA: low byte 0, high byte the nibble, ORed with
   ROWS_TILE_HIGH when nonzero. */
static void rows_tile(uint8_t nibble)
{
    if (nibble)
        a8_load((uint8_t)(nibble | rd8(dba(ROWS_TILE_HIGH))));
    wr8(dba(VMDATAL), 0);
    wr8(dba(VMDATAH), (uint8_t)cpu.a);
}

/* Step 1: a row of $40 bytes of ROWS_BUFFER, each two tiles, to VRAM at
   the row times $40 words; the row on by two, and at 0 the upload done. */
static void rows_row(void)
{
    uint16_t row;
    uint8_t v;

    a8_load(0x80);
    wr8(dba(VMAIN), 0x80);
    op_rep(0x30);
    row = (uint16_t)(rd16(dba(ROWS_ROW)) & 0x00FF);
    a16_load((uint16_t)(row << 5));
    flag_set(P_C, 0);                   /* the ASLs': no bit reaches 15 */
    cpu.x = (uint16_t)(row << 5);
    a16_load((uint16_t)(row << 6));
    wr16(dba(VMADDL), cpu.a);
    op_sep(0x20);
    cpu.y = 0x0040;
    do {
        v = rd8(ROWS_BUFFER + cpu.x);
        op_push8(v);
        a8_load((uint8_t)(v & 0x0F));
        rows_tile((uint8_t)(v & 0x0F));
        a8_load(op_pull8());
        flag_set(P_C, v & 0x08);        /* the fourth LSR's */
        a8_load((uint8_t)(v >> 4));
        rows_tile((uint8_t)(v >> 4));
        cpu.x = (uint16_t)(cpu.x + 1);
        cpu.y = (uint16_t)(cpu.y - 1);
        nz16_of(cpu.y);
    } while (cpu.y);
    flag_set(P_C, 0);
    a8_load((uint8_t)op_adc8(rd8(dba(ROWS_ROW)), 0x02, &cpu.p));
    wr8(dba(ROWS_ROW), (uint8_t)cpu.a);
    if (!(uint8_t)cpu.a)
        wr8(dba(ROWS_ASK), 0);
    op_sep(0x10);
}

/*
 * @routine 80:8577 UploadStepped
 * WHAT IT DOES: When ROWS_ASK asks, runs step ROWS_STEP of a stepped
 * upload: 0 starts it at row 0; 1 writes a row of ROWS_BUFFER's nibbles
 * to VRAM as tiles' high bytes (ROWS_TILE_HIGH ORed into each nonzero
 * one), two rows a call, and ends the upload once the row wraps to 0.
 * HOW: ROWS_STEP * 2 indexes ROWS_STEPS (JMP). Step 0: ROWS_ROW 0,
 * ROWS_STEP up. Step 1: VMAIN $80; VRAM address ROWS_ROW * $40, X =
 * ROWS_ROW * $20 (16-bit); $40 bytes: the low nibble then the high one,
 * each a word (VMDATAL 0, VMDATAH the nibble, ORed with ROWS_TILE_HIGH
 * when nonzero); ROWS_ROW + 2, at 0 ROWS_ASK cleared; X 8-bit.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR from the NMI. Exit: as
 * the step: step 0 N and Z from ROWS_STEP; step 1 N, Z, C and V from the
 * row's sum, A its low byte, Y 0.
 */
void abi_UploadStepped(void)
{
    uint8_t ask = rd8(dba(ROWS_ASK)), step;

    y8_load(ask);
    if (!ask)
        return;
    step = rd8(dba(ROWS_STEP));
    a8_load((uint8_t)(step << 1));
    flag_set(P_C, step & 0x80);
    x8_load((uint8_t)(step << 1));
    if (step == 0)
        rows_start();
    else if (step == 1)
        rows_row();
    else                                /* no other step is ever set: the table's next words are not code */
        jump_routine(0x800000u | rd16(0x800000u | (uint16_t)(ROWS_STEPS + cpu.x)));
}

/* A 7-byte entry's address, bank and size to DMA channel 0. */
static void dma_entry(uint16_t table, uint8_t at)
{
    a8_load(rd8(dba((uint16_t)(table + at))));
    wr8(dba(A1T0L), (uint8_t)cpu.a);
    a8_load(rd8(dba((uint16_t)(table + 1 + at))));
    wr8(dba(A1T0H), (uint8_t)cpu.a);
    a8_load(rd8(dba((uint16_t)(table + 2 + at))));
    wr8(dba(A1B0), (uint8_t)cpu.a);
}

/*
 * @routine 80:869A UploadBlockA
 * WHAT IT DOES: When BLOCK_A_ASK asks, DMAs one of the blocks listed at
 * BLOCKS_A to VRAM.
 * HOW: BLOCK_A_ASK is the entry's offset (7 bytes: source address and
 * bank, size, VRAM address); cleared; VMAIN $80, DMA channel 0, mode 1 to
 * VMDATA.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR from the NMI. Exit: M
 * 8-bit; X = the entry; A = 1.
 */
void abi_UploadBlockA(void)
{
    uint8_t at = rd8(dba(BLOCK_A_ASK));

    x8_load(at);
    if (!at)
        return;
    wr8(dba(BLOCK_A_ASK), 0);
    a8_load(0x80);
    wr8(dba(VMAIN), 0x80);
    a8_load(0x01);
    wr8(dba(DMAP0), 0x01);
    a8_load(0x18);
    wr8(dba(BBAD0), 0x18);
    dma_entry(BLOCKS_A, at);
    op_rep(0x20);
    a16_load(rd16(dba((uint16_t)(BLOCKS_A + 3 + at))));
    wr16(dba(DAS0L), cpu.a);
    a16_load(rd16(dba((uint16_t)(BLOCKS_A + 5 + at))));
    wr16(dba(VMADDL), cpu.a);
    op_sep(0x20);
    a8_load(0x01);
    wr8(dba(MDMAEN), 0x01);
}

/*
 * @routine 80:86D9 UploadBlockB
 * WHAT IT DOES: When BLOCK_B_ASK asks, reads back one of the VRAM blocks
 * listed at BLOCKS_B into work RAM.
 * HOW: BLOCK_B_ASK is the entry's offset (7 bytes: destination address
 * and bank, size, VRAM address); cleared; VMAIN $80, DMA channel 0, mode
 * $81 from RDVRAML, after the dummy read of RDVRAMH.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR from the NMI. Exit: M
 * 8-bit; X = the entry; A = 1.
 */
void abi_UploadBlockB(void)
{
    uint8_t at = rd8(dba(BLOCK_B_ASK));

    x8_load(at);
    if (!at)
        return;
    wr8(dba(BLOCK_B_ASK), 0);
    a8_load(0x80);
    wr8(dba(VMAIN), 0x80);
    a8_load(0x81);
    wr8(dba(DMAP0), 0x81);
    a8_load(0x39);
    wr8(dba(BBAD0), 0x39);
    dma_entry(BLOCKS_B, at);
    op_rep(0x20);
    a16_load(rd16(dba((uint16_t)(BLOCKS_B + 3 + at))));
    wr16(dba(DAS0L), cpu.a);
    op_sep(0x20);
    a8_load(rd8(dba((uint16_t)(BLOCKS_B + 5 + at))));
    wr8(dba(VMADDL), (uint8_t)cpu.a);
    a8_load(rd8(dba((uint16_t)(BLOCKS_B + 6 + at))));
    wr8(dba(VMADDH), (uint8_t)cpu.a);
    a8_load(rd8(dba(RDVRAMH)));         /* the dummy read */
    a8_load(0x01);
    wr8(dba(MDMAEN), 0x01);
}

/*
 * @routine 80:874F PaletteUploadFrom
 * WHAT IT DOES: When PALETTE_UPLOAD asks, DMAs the palettes to CGRAM from
 * the source PALETTE_SOURCE picks in PALETTE_SOURCES: 512 bytes, or 256
 * for source 6.
 * HOW: CGADD 0, PALETTE_UPLOAD cleared; DMA channel 0, mode 0 to CGDATA,
 * the source from the table (3 bytes an entry), the size by the entry;
 * MDMAEN.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR from the NMI. Exit: M
 * 8-bit; X = PALETTE_SOURCE; Y = the size's high byte; A = 1.
 */
void abi_PaletteUploadFrom(void)
{
    uint8_t ask = rd8(dba(PALETTE_UPLOAD)), src;

    a8_load(ask);
    if (!ask)
        return;
    wr8(dba(CGADD), 0);
    wr8(dba(PALETTE_UPLOAD), 0);
    a8_load(0x00);
    wr8(dba(DMAP0), 0x00);
    a8_load(0x22);
    wr8(dba(BBAD0), 0x22);
    src = rd8(dba(PALETTE_SOURCE));
    x8_load(src);
    a8_load(rd8(dba((uint16_t)(PALETTE_SOURCES + src))));
    wr8(dba(A1T0L), (uint8_t)cpu.a);
    a8_load(rd8(dba((uint16_t)(PALETTE_SOURCES + 1 + src))));
    wr8(dba(A1T0H), (uint8_t)cpu.a);
    a8_load(rd8(dba((uint16_t)(PALETTE_SOURCES + 2 + src))));
    wr8(dba(A1B0), (uint8_t)cpu.a);
    cmp8_of(src, 0x06);
    a8_load(0x00);
    y8_load(src == 0x06 ? 0x01 : 0x02);
    wr8(dba(DAS0L), 0x00);
    wr8(dba(DAS0H), (uint8_t)cpu.y);
    a8_load(0x01);
    wr8(dba(MDMAEN), 0x01);
}
