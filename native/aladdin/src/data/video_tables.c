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

/* PaletteSequenceTask reads them through the data bank, which a task runs
   with as $80 (the record's comment says $81, where these bytes are code) */
#define PALETTE_SEQUENCE_BANK 0x80
uint8_t palette_sequence_set(unsigned step) { return cart8(cart_offset(PALETTE_SEQUENCE_BANK, (uint16_t)(PALETTE_SEQUENCE_SETS + step))); }
uint8_t palette_sequence_frames(unsigned step) { return cart8(cart_offset(PALETTE_SEQUENCE_BANK, (uint16_t)(PALETTE_SEQUENCE_FRAMES + step))); }
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

#define TEXT_LAYOUTS_BANK 0x82
#define TEXT_LAYOUTS_ADDR 0xB575

DataRef text_layout(uint8_t layout)
{
    DataRef r;

    r.at = cart_offset(TEXT_LAYOUTS_BANK, cart16(cart_offset(TEXT_LAYOUTS_BANK, (uint16_t)(TEXT_LAYOUTS_ADDR + (layout & 0x7F)))));
    return r;
}

/* within the layout's bank, as the original's 16-bit index keeps it */
static uint32_t layout_at(DataRef at, unsigned i)
{
    uint32_t bank_start = at.at & ~0x7FFFu;

    return bank_start | ((at.at + i) & 0x7FFFu);
}

uint8_t text_layout_byte(DataRef at, unsigned i) { return cart8(layout_at(at, i)); }
uint16_t text_layout_word(DataRef at, unsigned i) { return (uint16_t)(cart8(layout_at(at, i)) | cart8(layout_at(at, i + 1)) << 8); }

#define PALETTE_CYCLES_BANK 0x84
#define PALETTE_CYCLES_ADDR 0xE000
#define PALETTE_FRAMES_ADDR 0xE004

uint16_t palette_cycle_script(uint8_t cycle) { return palette_cycle_table16((uint16_t)(cycle * 2u)); }
uint16_t palette_cycle_table16(uint16_t at) { return cart16(cart_offset(PALETTE_CYCLES_BANK, (uint16_t)(PALETTE_CYCLES_ADDR + at))); }
uint8_t palette_cycle_frame8(uint16_t at) { return cart8(cart_offset(PALETTE_CYCLES_BANK, (uint16_t)(PALETTE_FRAMES_ADDR + at))); }
uint16_t palette_cycle_frame16(uint16_t at) { return cart16(cart_offset(PALETTE_CYCLES_BANK, (uint16_t)(PALETTE_FRAMES_ADDR + at))); }

#define ROTATE_SINES_BANK 0x82
#define ROTATE_SINES_ADDR 0x87D3

uint16_t rotate_sine_word(unsigned at) { return cart16(cart_offset(ROTATE_SINES_BANK, (uint16_t)(ROTATE_SINES_ADDR + at))); }

#define ACTOR_PALETTES_BANK 0x84
#define ACTOR_PALETTES_ADDR 0xB000
#define ROM_PALETTES_BANK 0x87
#define ROM_PALETTES_ADDR 0xFF40

uint16_t actor_palette_colour(uint8_t n, unsigned i) { return cart16(cart_offset(ACTOR_PALETTES_BANK, (uint16_t)(ACTOR_PALETTES_ADDR + 32u * n + 2u * i))); }
uint16_t rom_palette_colour(uint8_t n, unsigned i) { return cart16(cart_offset(ROM_PALETTES_BANK, (uint16_t)(ROM_PALETTES_ADDR + 32u * n + 2u * i))); }

#define SHAKES_BANK 0x80
#define SHAKES_ADDR 0xBAA3
#define WOBBLES_ADDR 0xBA76

ShakeEntry shake_entry(uint8_t at)
{
    uint32_t e = cart_offset(SHAKES_BANK, (uint16_t)(SHAKES_ADDR + at));
    ShakeEntry s = { cart8(e), cart8(e + 1), cart8(e + 2), cart8(e + 3) };

    return s;
}

WobbleEntry wobble_entry(uint8_t at)
{
    uint32_t e = cart_offset(SHAKES_BANK, (uint16_t)(WOBBLES_ADDR + at));
    WobbleEntry w = { cart8(e), cart8(e + 1), cart8(e + 2), cart8(e + 3) };

    return w;
}

#define HEALTH_MARKS_BANK 0x81
#define HEALTH_MARKS_ADDR 0xF58D

uint8_t health_mark_place(uint8_t n) { return cart8(cart_offset(HEALTH_MARKS_BANK, (uint16_t)(HEALTH_MARKS_ADDR + n))); }

#define WINDOW_TABLES_BANK 0x80
#define WINDOW_REGISTER_SETS 0xBCA9
#define WINDOW_TABLE_SETS 0xBCBB
#define WINDOW_SHAPE_BANK 0x82
#define WINDOW_SHAPE_ADDR 0xBBAE

WindowTableEntry window_table_entry(uint8_t set, unsigned i)
{
    uint16_t at = (uint16_t)(cart16(cart_offset(WINDOW_TABLES_BANK, (uint16_t)(WINDOW_TABLE_SETS + set))) + 3u * i);
    uint32_t e = cart_offset(WINDOW_TABLES_BANK, (uint16_t)(WINDOW_TABLE_SETS + at));
    WindowTableEntry w = { cart8(e), cart16(e + 1) };

    return w;
}

uint8_t window_register_mode(uint8_t regs) { return cart8(cart_offset(WINDOW_TABLES_BANK, (uint16_t)(WINDOW_REGISTER_SETS + regs))); }
uint8_t window_register_target(uint8_t regs) { return cart8(cart_offset(WINDOW_TABLES_BANK, (uint16_t)(WINDOW_REGISTER_SETS + 1 + regs))); }
uint16_t window_shape_last(void) { return cart16(cart_offset(WINDOW_SHAPE_BANK, WINDOW_SHAPE_ADDR)); }
uint8_t window_shape_byte(uint16_t at) { return cart8(cart_offset(WINDOW_SHAPE_BANK, (uint16_t)(WINDOW_SHAPE_ADDR + at))); }

#define SCROLL_TABLES_BANK 0x80
#define CAMERA_WINDOWS_ADDR 0xC672
#define SCROLL_SPEEDS_ADDR 0xC6EA

uint16_t camera_window_word(uint8_t w, unsigned field) { return cart16(cart_offset(SCROLL_TABLES_BANK, (uint16_t)(CAMERA_WINDOWS_ADDR + w + field))); }

uint32_t scroll_speed(uint8_t s)
{
    uint32_t e = cart_offset(SCROLL_TABLES_BANK, (uint16_t)(SCROLL_SPEEDS_ADDR + s));

    return (uint32_t)cart16(e) | (uint32_t)cart16(e + 2) << 16;
}

#define RANDOM_LISTS_BANK 0x80
#define RANDOM_LISTS_ADDR 0xDCE7

uint8_t random_list_byte(uint8_t list, unsigned i)
{
    uint16_t at = cart16(cart_offset(RANDOM_LISTS_BANK, (uint16_t)(RANDOM_LISTS_ADDR + 2u * list)));

    return cart8(cart_offset(RANDOM_LISTS_BANK, (uint16_t)(RANDOM_LISTS_ADDR + at + i)));
}

#define COLOUR_BANDS_BANK 0x80
#define COLOUR_BAND_COUNT_ADDR 0xBA7E
#define COLOUR_BAND_PAIRS_ADDR 0xBA7F
#define COLOUR_BAND_BUFFERS_ADDR 0xBAA1

uint8_t colour_band_count(void) { return cart8(cart_offset(COLOUR_BANDS_BANK, COLOUR_BAND_COUNT_ADDR)); }
uint8_t colour_band_byte(unsigned i) { return cart8(cart_offset(COLOUR_BANDS_BANK, (uint16_t)(COLOUR_BAND_PAIRS_ADDR + i))); }
uint8_t colour_band_buffer(uint8_t buffer) { return cart8(cart_offset(COLOUR_BANDS_BANK, (uint16_t)(COLOUR_BAND_BUFFERS_ADDR + buffer))); }

/* the picture lists: the list, its words and the parts are all read with the
   data bank $80 (the traces read $80:8909, $80:890F, $80:8911, $80:891B) */
#define PICTURE_LIST_BANK 0x80

unsigned picture_parts(DataRef pictures, uint8_t picture, PicturePart *parts, unsigned max)
{
    uint16_t list = cart16(pictures.at);
    uint16_t y = cart16(cart_offset(PICTURE_LIST_BANK, (uint16_t)(list + 2u * picture)));
    unsigned n = 0;
    uint16_t step = 0;

    while (n < max) {
        uint32_t at = cart_offset(PICTURE_LIST_BANK, y);
        PicturePart *p = &parts[n++];

        p->source = source_at(cart8(cart_offset(PICTURE_LIST_BANK, (uint16_t)(y + 2))), cart16(at));
        p->bytes = cart16(cart_offset(PICTURE_LIST_BANK, (uint16_t)(y + 3)));
        p->step = step;
        y = (uint16_t)(y + 5);
        if (cart8(cart_offset(PICTURE_LIST_BANK, y)) == 0xFF)
            return n;
        step = cart16(cart_offset(PICTURE_LIST_BANK, y));
        y = (uint16_t)(y + 2);
    }
    return n;
}

/* IrisCloseTask (80:B263), through the data bank $80: the radius a closing
   iris starts at, by iris, at $80:DB87; its centre (x then y, words), by
   iris * 4, at $80:DB89 */
#define IRIS_BANK 0x80
#define IRIS_RADII_ADDR 0xDB87
#define IRIS_CENTRES_ADDR 0xDB89

uint8_t iris_radius(uint8_t iris)
{
    return cart8(cart_offset(IRIS_BANK, (uint16_t)(IRIS_RADII_ADDR + iris)));
}

void iris_centre(uint8_t iris, uint16_t *x, uint16_t *y)
{
    uint32_t at = cart_offset(IRIS_BANK, (uint16_t)(IRIS_CENTRES_ADDR + (uint8_t)(iris << 2)));

    *x = cart16(at);
    *y = cart16(at + 2);
}

DataRef line_table_at(uint8_t bank, uint16_t address)
{
    DataRef r;

    r.at = cart_offset(bank, address);
    return r;
}

uint8_t line_table_byte(DataRef table, uint32_t at) { return cart8(table.at + at); }

/* the text bands: channel 5's tables and the band pairs' bytes */
#define TEXT_BANDS_BANK 0x80

DataRef text_band_table(unsigned buffer)
{
    return line_table_at(TEXT_BANDS_BANK, buffer & 1 ? 0xCF3B : 0xCEEF);
}

uint8_t text_band_row_a(uint8_t step) { return cart8(cart_offset(TEXT_BANDS_BANK, (uint16_t)(0xCF87 + step))); }
uint8_t text_band_row_b(uint8_t step) { return cart8(cart_offset(TEXT_BANDS_BANK, (uint16_t)(0xCF8B + step))); }

/* a page of text: its first byte the lines, then per line the attribute, the
   length and the characters; each line's place the next word of the list */
unsigned text_page_begin(DataRef *page)
{
    unsigned lines = cart8(page->at);

    page->at++;
    return lines;
}

TextPageLine text_page_line(DataRef *page, DataRef *places)
{
    TextPageLine l;
    uint32_t at = page->at;

    l.at = cart16(places->at);
    places->at += 2;
    l.attribute = cart8(at);
    l.length = cart8(at + 1);
    l.chars = cart_at(at + 2);
    page->at = at + 2u + l.length;
    return l;
}
