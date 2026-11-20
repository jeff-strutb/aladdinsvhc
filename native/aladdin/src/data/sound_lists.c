/*
 * sound_lists.c: the sound upload lists (a shared format, data.h). The table at $85:8000 has 3 bytes
 * a list: the list's address (bit 15 dropped) and its bank (bit 7 set). A
 * list is blocks of a length word, a sound-memory address word and the
 * bytes; a length of 0 ends it, its address where the driver goes on. The
 * original reads a list straight on across a bank's end (its Y steps the
 * bank), so a list is contiguous in the image.
 */
#include "data/data.h"
#include "data/data_internal.h"

#define LISTS_BANK 0x85
#define LISTS_ADDR 0x8000
#define LIST_ENTRIES 55         /* $85:8000 to $85:80A4: the driver's list starts at $85:80A5 */

static int list_at(unsigned list, uint32_t *at)
{
    uint32_t e = cart_offset(LISTS_BANK, LISTS_ADDR) + 3 * list;
    uint16_t addr = cart16(e);
    uint8_t bank = cart8(e + 2);

    if (list >= LIST_ENTRIES || (addr == 0 && bank == 0))
        return -1;
    *at = cart_offset((uint8_t)(bank | 0x80), (uint16_t)(addr | 0x8000));
    return 0;
}

int sound_list_blocks(unsigned list)
{
    uint32_t at;
    int n = 0;
    uint16_t len;

    if (list_at(list, &at))
        return -1;
    while ((len = cart16(at)) != 0) {
        at += 4u + len;
        n++;
    }
    return n;
}

SoundBlock sound_list_block(unsigned list, unsigned block)
{
    SoundBlock b = {0, 0, 0};
    uint32_t at;
    unsigned i;

    if (list_at(list, &at))
        return b;
    for (i = 0; i <= block; i++) {
        b.length = cart16(at);
        b.to = cart16(at + 2);
        b.bytes = cart_at(at + 4);
        if (b.length == 0)
            break;
        at += 4u + b.length;
    }
    return b;
}

uint16_t sound_list_start(unsigned list)
{
    uint32_t at;
    uint16_t len;

    if (list_at(list, &at))
        return 0;
    while ((len = cart16(at)) != 0)
        at += 4u + len;
    return cart16(at + 2);
}
