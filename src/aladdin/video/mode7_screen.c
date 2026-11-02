/*
 * mode7_screen.c: putting up a Mode 7 screen.
 *
 * The screen's graphics are kept in VRAM as 4-bit planar tiles; for Mode 7
 * they are read back into the staging buffer at $7F:5000, converted to
 * Mode 7's one byte a pixel (each tile's colours taking a palette row from
 * a table), and written to VRAM's high bytes; the map is read back,
 * cleared and written again (video/mode7.c).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "cpu_ops.h"
#include "native.h"
void abi_Mode7ScreenSetup(void);
void abi_Mode7GraphicsConvert(void);
void abi_Mode7TilesConvert(void);
void abi_Mode7TileRowsConvert(void);
void abi_TileBlockQueue(void);
void abi_TileBlockQueueRun(void);
void abi_TileBlockQueueOne(void);
void abi_VramWordsToBuffer(void);
void abi_SineTableBuild(void);
void abi_Mode7MapToBuffer(void);
void abi_Mode7MapFromBuffer(void);
void abi_Mode7MapClear(void);
#include "hw.h"
#include "wram.h"
#include "wram_video.h"

/*
 * @routine 81:AF3B Mode7ScreenSetup
 * WHAT IT DOES: Puts up a Mode 7 screen from the graphics and map in VRAM,
 * with the interrupts off while VRAM is rewritten.
 * HOW: Builds the sine tables; NMI off and forced blank; four 2 KB parts of
 * the graphics read back and converted (Mode7GraphicsConvert); the map read
 * back, cleared and written again; then Mode 7 (BGMODE 7), its centre and
 * parameters, the screen as its shadow says, M7SEL $80, the NMI on.
 * Entry: M 8-bit, X 8-bit; D = 0; A low byte = the row flag for the
 * palette lookup; DB a bank whose $2100-$4200 are the PPU's and CPU's;
 * called by JSL. Exit: M and X 8-bit; interrupts on; A low byte = NMITIMEN
 * written; N set, Z clear.
 */
void abi_Mode7ScreenSetup(void)
{
    uint8_t v;

    wr8(dpa(0x30), (uint8_t)cpu.a);
    abi_SineTableBuild();
    /* the screen off and no NMI while VRAM is written */
    sei();
    wr8(dba((uint16_t)NMITIMEN), rd8(dba(NMITIMEN_SHADOW)) & 0x7F);
    wr8(dba((uint16_t)INIDISP), rd8(dba(INIDISP_SHADOW)) | 0x80);
    /* the graphics: 4 blocks, VRAM $3000 down by $1000, source $0C00
       down by $400 */
    wr8(dpa(0x0F), 0x0C);
    wr8(dpa(0x0E), 0x00);
    a8_load(0x30);
    wr8(dpa(0x0D), 0x30);
    wr8(dpa(0x0C), 0x00);
    do {
        abi_VramWordsToBuffer();
        abi_Mode7GraphicsConvert();
        flag_set(P_C, 1);
        wr8(dpa(0x0D), op_sbc8(rd8(dpa(0x0D)), 0x10, &cpu.p));
        flag_set(P_C, 1);
        v = op_sbc8(rd8(dpa(0x0F)), 0x04, &cpu.p);
        a8_load(v);
        wr8(dpa(0x0F), v);
    } while (cpu.p & P_C);
    /* the map */
    a8_load(0x40);
    wr8(dpa(0x0F), 0x40);
    wr8(dpa(0x0E), 0x00);
    abi_Mode7MapToBuffer();
    abi_Mode7MapClear();
    wr8(dpa(0x0D), 0x00);
    wr8(dpa(0x0C), 0x00);
    abi_Mode7MapFromBuffer();
    /* mode 7, its centre (128, 64) and an empty matrix */
    wr8(dba(0x0829), 0x02);
    wr8(dba(0x039D), 0x07);
    op_rep(P_M);
    wr16(dba(0x0395), 0x0080);
    cpu.a = 0x0040;
    wr16(dba(0x0397), 0x0040);
    wr16(dba(0x038D), 0);
    wr16(dba(0x038F), 0);
    wr16(dba(0x0391), 0);
    wr16(dba(0x0393), 0);
    op_sep(0x30);
    wr8(dba((uint16_t)INIDISP), rd8(dba(INIDISP_SHADOW)));
    wr8(dba((uint16_t)M7SEL), 0x80);
    a8_load((uint8_t)(0x80 | rd8(dba(NMITIMEN_SHADOW))));
    wr8(dba((uint16_t)NMITIMEN), (uint8_t)cpu.a);
    cli();
}

/*
 * @routine 81:B059 Mode7GraphicsConvert
 * WHAT IT DOES: Converts the staged graphics to Mode 7's pixels and writes
 * them to VRAM's high bytes.
 * HOW: Mode7TilesConvert, then 4,096 bytes to VMDATAH from the VRAM word
 * address at $0C, stepping after the high byte.
 * Entry: D = 0; DB a bank whose $2115-$2119 are the PPU's; called by JSR.
 * Exit: M 8-bit, X 8-bit; X = Y = 0; Z set; A low byte = the last pixel.
 */
void abi_Mode7GraphicsConvert(void)
{
    uint16_t x;

    op_rep(0x30);
    abi_Mode7TilesConvert();
    op_sep(P_M);
    /* the $1000 pixels to VRAM's high bytes from the address at $0C */
    wr8(dba((uint16_t)VMAIN), 0x80);
    wr16(dba((uint16_t)VMADDL), dp16(0x0C));
    for (x = 0; x < 0x1000; x++) {
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(0x7F5800u + x));
        wr8(dba((uint16_t)VMDATAH), (uint8_t)cpu.a);
    }
    cpu.x = 0;
    cpu.y = 0;
    nz16_of(0);
    op_sep(P_X);
}

/*
 * @routine 81:B07E Mode7TilesConvert
 * WHAT IT DOES: Converts the 64 staged planar tiles to a byte a pixel.
 * HOW: With the data bank at $7F, Mode7TileRowsConvert over 64 tiles from
 * the staging buffer's start.
 * Entry: D = 0; called by JSR. Exit: M 8-bit, X and Y 16-bit; DB as at
 * entry, N and Z from it (PLB).
 */
void abi_Mode7TilesConvert(void)
{
    uint8_t db = cpu.db;

    op_rep(0x30);
    dp16_set(0x2C, 0x0040);
    op_sep(P_M);
    cpu.db = 0x7F;
    a8_load(0x7F);
    cpu.x = 0;
    cpu.y = 0;
    abi_Mode7TileRowsConvert();
    cpu.db = db;
    nz8_of(db);
}

/*
 * @routine 81:B097 Mode7TileRowsConvert
 * WHAT IT DOES: Converts staged 4-bit planar tiles into a byte a pixel,
 * each tile's colours in its own palette row.
 * HOW: Per tile: its palette row from the table at $B407 (indexed by its
 * place and the row flag); per row of 8 pixels, the four bit-planes
 * shifted out a bit at a time into a 4-bit colour, nonzero colours ORed
 * with the palette row, each pixel to $5800 + X.
 * Entry: M 16-bit, X 16-bit; D = 0; DB = $7F; X = the first pixel's
 * offset; Y = the first tile's offset in the staging buffer; $2C = the
 * tiles; called by JSR. Exit: M 16-bit; X, Y past the last tile; $2C =
 * 0; Z set.
 */
void abi_Mode7TileRowsConvert(void)
{
    uint16_t x = cpu.x, y = cpu.y;
    uint8_t palette, row, col, p0, p1, p2, p3, px;

    do {
        /* the tile's palette, by its row in the source and the flag */
        op_sep(P_M);
        dp16_set(0x32, x);
        palette = rd8(0x81B407u + (uint8_t)((((x >> 8) >> 2) + rd8(dpa(0x0F))) | rd8(dpa(0x30))));
        wr8(dpa(0x32), palette);
        /* each row's four bitplanes to 8 pixels, a byte each */
        for (row = 0; row < 8; row++) {
            p0 = rd8(dba(0x5000) + y);
            p1 = rd8(dba(0x5001) + y);
            p2 = rd8(dba(0x5010) + y);
            p3 = rd8(dba(0x5011) + y);
            for (col = 0; col < 8; col++) {
                px = (uint8_t)((p3 >> 7) << 3 | (p2 >> 7) << 2 | (p1 >> 7) << 1 | p0 >> 7);
                p0 <<= 1;
                p1 <<= 1;
                p2 <<= 1;
                p3 <<= 1;
                if (px)
                    px |= palette;
                wr8(dba(0x5800) + x, px);
                x++;
            }
            y += 2;
        }
        wr8(dpa(0x10), 0);
        wr8(dpa(0x11), 0);
        wr8(dpa(0x12), 0);
        wr8(dpa(0x13), 0);
        wr8(dpa(0x2E), 0);
        wr8(dpa(0x2F), 0);
        op_rep(P_M);
        flag_set(P_C, 0);
        y = op_adc16(y, 0x0010, &cpu.p);
        cpu.a = y;
    } while (dec16_at(dpa(0x2C)));
    cpu.x = x;
    cpu.y = y;
}

/* The queue's next record (X) and its first word, the VRAM address. */
static uint16_t block_start(void)
{
    uint16_t x = rd16(dba((uint16_t)TILE_BLOCKS_LENGTH));

    cpu.x = x;
    wr16(dba((uint16_t)(TILE_BLOCKS + x)), cpu.a);
    return x;
}

/* The record's length added, and the update asked for. */
static void block_end(uint16_t x)
{
    flag_set(P_C, 0);
    cpu.a = op_adc16(x, 0x000A, &cpu.p);
    wr16(dba((uint16_t)TILE_BLOCKS_LENGTH), cpu.a);
    op_sep(0x30);
    inc8_at(dba((uint16_t)TILE_BLOCKS_PENDING));
}

/*
 * @routine 81:A23C TileBlockQueueRun
 * WHAT IT DOES: Queues one 16x16 block for the NMI to write to VRAM at
 * address A: four tiles numbered on from Y, two on top, two under.
 * HOW: A queue record of 10 bytes at the queue's end: the address, then
 * the tilemap words Y, Y + 1, Y + 2, Y + 3; the length steps by 10 and the
 * queue is marked ready.
 * Entry: M 16-bit, X 16-bit; A = the VRAM address; Y = the first tile's
 * tilemap word; DB a bank whose $0000-$1FFF is work RAM; called by JSL.
 * Exit: M and X 8-bit; A low byte = the new length's; X = the record's
 * offset (low byte); Y = its low byte at entry; N, Z from the ready flag's
 * increment.
 */
void abi_TileBlockQueueRun(void)
{
    uint16_t x = block_start(), t = cpu.y;

    /* four tiles in a row */
    wr16(dba((uint16_t)(TILE_BLOCKS + 2 + x)), t);
    wr16(dba((uint16_t)(TILE_BLOCKS + 4 + x)), (uint16_t)(t + 1));
    wr16(dba((uint16_t)(TILE_BLOCKS + 6 + x)), (uint16_t)(t + 2));
    wr16(dba((uint16_t)(TILE_BLOCKS + 8 + x)), (uint16_t)(t + 3));
    block_end(x);
}

/*
 * @routine 81:A260 TileBlockQueueOne
 * WHAT IT DOES: Queues one 16x16 block for the NMI to write to VRAM at
 * address A: a square of tiles from Y in a sheet 16 tiles wide.
 * HOW: A queue record of 10 bytes at the queue's end: the address, then
 * the tilemap words Y, Y + 1, Y + 16, Y + 17; the length steps by 10 and
 * the queue is marked ready.
 * Entry: M 16-bit, X 16-bit; A = the VRAM address; Y = the first tile's
 * tilemap word; DB a bank whose $0000-$1FFF is work RAM; called by JSL.
 * Exit: M and X 8-bit; A low byte = the new length's; X = the record's
 * offset (low byte); Y = its low byte at entry; N, Z from the ready flag's
 * increment.
 */
void abi_TileBlockQueueOne(void)
{
    uint16_t x = block_start(), t = cpu.y;

    /* a 16x16 tile: two tiles, and the two $10 on */
    wr16(dba((uint16_t)(TILE_BLOCKS + 2 + x)), t);
    wr16(dba((uint16_t)(TILE_BLOCKS + 4 + x)), (uint16_t)(t + 1));
    flag_set(P_C, 0);
    t = op_adc16((uint16_t)(t + 1), 0x000F, &cpu.p);
    wr16(dba((uint16_t)(TILE_BLOCKS + 6 + x)), t);
    wr16(dba((uint16_t)(TILE_BLOCKS + 8 + x)), (uint16_t)(t + 1));
    block_end(x);
}

/*
 * @routine 81:A287 TileBlockQueue
 * WHAT IT DOES: Queues a block of tiles for the NMI to write to VRAM: four
 * tilemap rows of four tiles at VRAM address A, the tiles numbered from Y.
 * HOW: Four queue records of 10 bytes (a VRAM address, then four tilemap
 * words: tile, tile + 1, tile + 16, tile + 17), the address stepping a
 * row ($20 words) at a time; the queue's length steps by 40 and the queue
 * is marked ready.
 * Entry: M 16-bit, X 16-bit; A = the VRAM address; Y = the first tile's
 * tilemap word; DB a bank whose $0000-$1FFF is work RAM; called by JSL.
 * Exit: M and X 8-bit; X = the new length's low byte; scratch $0C = 0; N,
 * Z from the ready flag's increment.
 */
void abi_TileBlockQueue(void)
{
    uint16_t x = block_start(), a = cpu.a, t;

    /* a 4x4 block of 16x16 tiles: the four rows' VRAM addresses $20
       apart, each row's four tiles */
    a = (uint16_t)(a + 2);
    wr16(dba((uint16_t)(0x1B4E + x)), a);
    flag_set(P_C, 0);
    a = op_adc16(a, 0x003E, &cpu.p);
    wr16(dba((uint16_t)(0x1B58 + x)), a);
    wr16(dba((uint16_t)(0x1B62 + x)), (uint16_t)(a + 2));
    t = cpu.y;
    cpu.y = 0x0004;
    wr16(dba(SCRATCH), 0x0004);
    do {
        wr16(dba((uint16_t)(0x1B46 + x)), t);
        wr16(dba((uint16_t)(0x1B48 + x)), (uint16_t)(t + 1));
        flag_set(P_C, 0);
        a = op_adc16((uint16_t)(t + 1), 0x000F, &cpu.p);
        wr16(dba((uint16_t)(0x1B4A + x)), a);
        wr16(dba((uint16_t)(0x1B4C + x)), (uint16_t)(a + 1));
        flag_set(P_C, 1);
        t = op_sbc16((uint16_t)(a + 1), 0x000F, &cpu.p);
        flag_set(P_C, 0);
        x = op_adc16(x, 0x000A, &cpu.p);
        cpu.a = t;
        cpu.x = x;
        cpu.y = t;
    } while (dec16_at(dba(SCRATCH)));
    wr16(dba((uint16_t)TILE_BLOCKS_LENGTH), x);
    op_sep(0x30);
    inc8_at(dba((uint16_t)TILE_BLOCKS_PENDING));
}
