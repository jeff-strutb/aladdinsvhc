/*
 * graphics.c: the shared graphics formats: the palette loads, packed
 * graphics, the level graphics lists, the unpack sets (data.h).
 */
#include <stdio.h>

#include "data/data.h"
#include "data/data_internal.h"

/* PaletteLoad's table at $84:C000: 98 entries of 6 bytes (where the colours
   are, from $84:C000; how many words; the byte offset in the palette), then
   the colours (the first entry's start at $84:C24C ends the table). The
   original's sums are kept to 16 bits in bank $84. */
#define PALETTE_LOADS_BANK 0x84
#define PALETTE_LOADS_ADDR 0xC000

static uint16_t palette_entry(uint8_t set, unsigned field)
{
    return cart16(cart_offset(PALETTE_LOADS_BANK, (uint16_t)(PALETTE_LOADS_ADDR + 6u * set + 2u * field)));
}

uint16_t palette_load_count(uint8_t set) { return palette_entry(set, 1); }
uint8_t palette_load_first(uint8_t set) { return (uint8_t)(palette_entry(set, 2) >> 1); }

uint16_t palette_load_colour(uint8_t set, uint16_t i)
{
    return cart16(cart_offset(PALETTE_LOADS_BANK, (uint16_t)(PALETTE_LOADS_ADDR + palette_entry(set, 0) + 2u * i)));
}

/* ---- packed graphics ---- */

/* the original's group count for a length in bytes: rounded up, in 16 bits */
static uint16_t groups_of(uint16_t length)
{
    return (uint16_t)(((uint16_t)(length + 7u) & 0xFFF8u) >> 3);
}

/* a source the original names by its bank and offset; it reads on past
   $FFFF at $8000 of the next bank, which is the next byte of the image */
static DataRef ref_at(uint8_t bank, uint16_t addr)
{
    DataRef r;

    r.at = cart_offset(bank, addr);
    return r;
}

uint32_t unpack_graphics(PackedGraphics p, uint8_t *out)
{
    uint32_t at = p.src.at, n = 0;
    uint16_t g;
    int i;

    for (g = 0; g < p.groups; g++) {
        uint8_t flags = cart8(at++), fill = cart8(at++);

        for (i = 0; i < 8; i++, flags = (uint8_t)(flags << 1))
            out[n++] = flags & 0x80 ? cart8(at++) : fill;
    }
    return n;
}

/* LevelGraphicsLoad's table at $80:DF98: per level a word, its list's
   offset from $DF98. A list: a count byte, then 7-byte parts (source offset,
   bank, length in bytes, VRAM address) from the byte after it; then a count
   byte, then 5-byte parts (source offset, bank, length) from the byte after
   it. (The record's comment names bank $81; the routine runs with DB = $80
   and its bytes there are the table.) */
#define LEVEL_GFX_BANK 0x80
#define LEVEL_GFX_ADDR 0xDF98

static uint16_t level_list(uint16_t level)
{
    return cart16(cart_offset(LEVEL_GFX_BANK, (uint16_t)(LEVEL_GFX_ADDR + 2u * level)));
}

static uint8_t lg8(uint16_t at) { return cart8(cart_offset(LEVEL_GFX_BANK, (uint16_t)(LEVEL_GFX_ADDR + at))); }
static uint16_t lg16(uint16_t at) { return cart16(cart_offset(LEVEL_GFX_BANK, (uint16_t)(LEVEL_GFX_ADDR + at))); }

unsigned level_vram_parts(uint16_t level) { return lg8(level_list(level)); }

LevelVramPart level_vram_part(uint16_t level, unsigned i)
{
    uint16_t e = (uint16_t)(level_list(level) + 7u * i);
    LevelVramPart p;

    p.bytes = lg16((uint16_t)(e + 4));
    p.packed.src = ref_at(lg8((uint16_t)(e + 3)), lg16((uint16_t)(e + 1)));
    p.packed.groups = groups_of(p.bytes);
    p.to = lg16((uint16_t)(e + 6));
    return p;
}

static uint16_t kept_list(uint16_t level)
{
    return (uint16_t)(level_list(level) + 7u * level_vram_parts(level));
}

/* with no parts to video memory the original ends there and keeps none */
unsigned level_kept_parts(uint16_t level)
{
    return level_vram_parts(level) ? lg8((uint16_t)(kept_list(level) + 1)) : 0;
}

PackedGraphics level_kept_part(uint16_t level, unsigned i)
{
    uint16_t e = (uint16_t)(kept_list(level) + 5u * i);
    PackedGraphics p;

    p.src = ref_at(lg8((uint16_t)(e + 4)), lg16((uint16_t)(e + 2)));
    p.groups = groups_of(lg16((uint16_t)(e + 5)));
    return p;
}

/* UnpackSetBegin's table at $80:F039: 7 bytes a set (where it goes in bank
   $7F, source offset, bank, length in bytes) */
#define UNPACK_SETS_BANK 0x80
#define UNPACK_SETS_ADDR 0xF039

UnpackSetEntry unpack_set(uint8_t set)
{
    uint32_t e = cart_offset(UNPACK_SETS_BANK, (uint16_t)(UNPACK_SETS_ADDR + 7u * set));
    UnpackSetEntry u;

    u.to = cart16(e);
    u.packed.src = ref_at(cart8(e + 4), cart16(e + 2));
    u.packed.groups = groups_of(cart16(e + 5));
    return u;
}

/* AnimSetGraphicsUnpack: a word per set at $82:AC18 + set * 2, its entry's
   offset from $82:AC58; an entry is the length (low 14 bits), the source
   offset and the bank. Long addresses, as the original's. */
#define ANIM_GFX_BANK 0x82
#define ANIM_GFX_ENTRIES 0xAC18
#define ANIM_GFX_PARTS 0xAC58

AnimSetGraphics anim_set_graphics(uint8_t set)
{
    uint16_t x = cart16(cart_offset(ANIM_GFX_BANK, (uint16_t)(ANIM_GFX_ENTRIES + 2u * set)));
    uint32_t e = cart_offset(ANIM_GFX_BANK, (uint16_t)(ANIM_GFX_PARTS + x));
    AnimSetGraphics a;

    a.length = cart16(e);
    a.packed.groups = (uint16_t)((a.length & 0x3FFFu) >> 3);
    a.packed.src = ref_at(cart8(e + 4), cart16(e + 2));
    return a;
}

static uint16_t anim_entry(uint8_t set)
{
    static int said;

    if (set < 32 && !said) {
        said = 1;
        fprintf(stderr, "data: actor graphics asked for kind %u: entries below 32 lie in code no trace reads\n", set);
    }
    return cart16(cart_offset(ANIM_GFX_BANK, (uint16_t)(ANIM_GFX_ENTRIES + 2u * set)));
}

PackedGraphics actor_graphics(uint8_t kind, uint8_t *extra_slots)
{
    AnimSetGraphics a;

    (void)anim_entry(kind);
    a = anim_set_graphics(kind);
    if (extra_slots)
        *extra_slots = (uint8_t)(a.length >> 14);
    return a.packed;
}

/* the original's long sum: $82:AC5D + (the entry's offset + the variant, in 16 bits) */
uint8_t actor_graphics_variant_byte(uint8_t kind, uint8_t variant)
{
    uint32_t at = ((uint32_t)ANIM_GFX_BANK << 16 | (ANIM_GFX_PARTS + 5u)) + (uint16_t)(anim_entry(kind) + variant);

    return cart8(cart_offset((uint8_t)(at >> 16), (uint16_t)at));
}
