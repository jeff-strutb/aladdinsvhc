/*
 * data.c: cart.dat, loaded once, and the cartridge's places, each written
 * once here. A place is the original's bank and address; cart.dat is the
 * cartridge image (LoROM: bank $80 + n's $8000-$FFFF at n * $8000).
 */
#include <stdio.h>
#include <stdlib.h>

#include "nshvc/nshvc.h"
#include "data/data.h"
#include "data/data_internal.h"

#define CART_SIZE 0x140000u

static uint8_t cart[CART_SIZE];

void data_load(void)
{
    nshvc_asset a = nshvc_asset_open("cart.dat");
    uint32_t got = 0, n;

    if (!a.id) {
        fprintf(stderr, "cart.dat is not in the game's package: run tools/extract_assets.py on your cartridge\n");
        exit(1);
    }
    while (got < CART_SIZE && (n = nshvc_asset_read(a, cart + got, CART_SIZE - got)) != 0)
        got += n;
    nshvc_asset_close(a);
    if (got != CART_SIZE) {
        fprintf(stderr, "cart.dat is %u bytes, not %u: run tools/extract_assets.py again\n", got, CART_SIZE);
        exit(1);
    }
}

uint32_t cart_offset(uint8_t bank, uint16_t addr)
{
    return ((uint32_t)(bank & 0x7F) << 15 | (addr & 0x7FFFu)) % CART_SIZE;
}

const uint8_t *cart_at(uint32_t offset)
{
    return &cart[offset % CART_SIZE];
}

/* whether `p` points into the cartridge image, and where (the snapshot writes such a pointer as its offset) */
int data_cart_offset(const void *p, uint32_t *offset)
{
    const uint8_t *b = (const uint8_t *)p;

    if (b < cart || b >= cart + CART_SIZE)
        return 0;
    *offset = (uint32_t)(b - cart);
    return 1;
}

const uint8_t *data_cart_ptr(uint32_t offset)
{
    return offset < CART_SIZE ? &cart[offset] : NULL;
}

uint8_t cart8(uint32_t offset)
{
    return cart[offset % CART_SIZE];
}

uint16_t cart16(uint32_t offset)
{
    return (uint16_t)(cart8(offset) | cart8(offset + 1) << 8);
}
