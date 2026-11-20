/*
 * video_tables.c: tables the picture code reads.
 */
#include "data/data.h"
#include "data/video_tables.h"
#include "data/data_internal.h"

/* HalfSineBuild reads every other byte from $82:87D4 */
#define QUARTER_SINE_BANK 0x82
#define QUARTER_SINE_ADDR 0x87D4

uint8_t quarter_sine_step(unsigned step)
{
    return cart8(cart_offset(QUARTER_SINE_BANK, QUARTER_SINE_ADDR) + 2u * (step & 0x3F));
}

/* ---- native-video's tables ---- */

static UploadSource source_at(uint8_t bank, uint16_t addr)
{
    UploadSource s;

    s.cart = 0;
    if (bank == 0x7F) {
        s.from = FROM_UNPACKED;
        s.at = addr;
    } else if (bank == 0x7E && addr >= 0xD800 && addr < 0xE000) {
        s.from = FROM_TEXT_MAP;
        s.at = (uint16_t)(addr - 0xD800);
    } else {
        s.from = FROM_CART;
        s.at = 0;
        s.cart = cart_at(cart_offset(bank, addr));
    }
    return s;
}

/* VramCopy's table at $80:F197; VramCopy2's starts $103 bytes on */
#define VRAM_COPY_BANK 0x80
#define VRAM_COPY_ADDR 0xF197

VramCopyEntry vram_copy_entry(unsigned table, unsigned entry)
{
    uint32_t e = cart_offset(VRAM_COPY_BANK, (uint16_t)(VRAM_COPY_ADDR + 7u * (entry + 37u * table)));
    VramCopyEntry c;

    c.from = cart16(e);
    c.to = cart16(e + 3);
    c.words = cart16(e + 5);
    return c;
}

#define VIDEO_BANK_81 0x81
#define PALETTE_SEQUENCE_SETS 0xBA6A
#define PALETTE_SEQUENCE_FRAMES 0xBA70
#define MODE7_ROWS 0xB407
#define MODE7_SINE 0xB3E5

uint8_t palette_sequence_set(unsigned step) { return cart8(cart_offset(VIDEO_BANK_81, (uint16_t)(PALETTE_SEQUENCE_SETS + step))); }
uint8_t palette_sequence_frames(unsigned step) { return cart8(cart_offset(VIDEO_BANK_81, (uint16_t)(PALETTE_SEQUENCE_FRAMES + step))); }
uint8_t mode7_palette_row(uint8_t at) { return cart8(cart_offset(VIDEO_BANK_81, (uint16_t)(MODE7_ROWS + at))); }
int16_t mode7_quarter_sine(unsigned step) { return (int16_t)cart16(cart_offset(VIDEO_BANK_81, (uint16_t)(MODE7_SINE + 2u * step))); }

#define LEVEL_PALETTES_BANK 0x80
#define LEVEL_PALETTES_ADDR 0xB812

static uint8_t lp8(uint16_t at) { return cart8(cart_offset(LEVEL_PALETTES_BANK, (uint16_t)(LEVEL_PALETTES_ADDR + at))); }
unsigned level_palette_count(uint16_t level) { return lp8(lp8((uint16_t)(level & 0xFF))); }
uint8_t level_palette_set(uint16_t level, unsigned i) { return lp8((uint16_t)(lp8((uint16_t)(level & 0xFF)) + 1u + i)); }

#define NMI_TABLES_BANK 0x80
#define TEXT_PARTS_ADDR 0xB61B
#define NUMBERED_INDEX_ADDR 0xB62F
#define NUMBERED_ADDR 0xB630

TextPart text_part(unsigned part)
{
    uint8_t y = cart8(cart_offset(NMI_TABLES_BANK, (uint16_t)(TEXT_PARTS_ADDR + part)));
    uint32_t e = cart_offset(NMI_TABLES_BANK, (uint16_t)(TEXT_PARTS_ADDR + y));
    TextPart t;

    t.to = cart16(e);
    t.from = (uint16_t)(cart16(e + 2) - 0xD800);   /* in bank $7E, from the text map's start */
    t.bytes = cart16(e + 4);
    return t;
}

NumberedUpload numbered_upload(unsigned number)
{
    uint8_t x = cart8(cart_offset(NMI_TABLES_BANK, (uint16_t)(NUMBERED_INDEX_ADDR + number)));
    uint32_t e = cart_offset(NMI_TABLES_BANK, (uint16_t)(NUMBERED_ADDR + x));
    uint8_t vmain = cart8(e), mode = cart8(e + 3), reg = cart8(e + 4);
    NumberedUpload u;

    /* whole words (the transfer's mode 1), or bytes to the low half alone
       or the high half alone (its target $18 or $19) */
    u.how = mode == 1 ? UPLOAD_WORDS : reg == 0x19 ? UPLOAD_HIGH_BYTES : UPLOAD_LOW_BYTES;
    (void)vmain;
    u.to = cart16(e + 1);
    u.source = source_at(cart8(e + 7), cart16(e + 5));
    u.bytes = (uint16_t)(cart8(e + 8) | cart8(e + 9) << 8);
    return u;
}

DataRef vram_list_at(uint16_t word)
{
    DataRef r;

    r.at = cart_offset(NMI_TABLES_BANK, word);
    return r;
}

VramListRecord vram_list_record(DataRef list, unsigned record)
{
    uint32_t e = list.at + 8u * record;
    VramListRecord r;

    r.to = cart16(e);
    r.source = source_at(cart8(e + 4), cart16(e + 2));
    r.bytes = cart16(e + 5);
    r.more = cart8(e + 7);
    return r;
}
