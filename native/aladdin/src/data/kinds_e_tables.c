/*
 * kinds_e_tables.c: the places of the tables kinds E's actors read
 * (kinds_e_tables.h).
 *
 * Banks are the ones the traces read (build/aladdin/box/<script>/rommap.bin,
 * through tools/aladdin/codemap.py), not the record's comments where they
 * differ: the kinds run with the data bank the actor loop leaves, $80.
 */
#include "data/kinds_e_tables.h"
#include "data/data_internal.h"

static uint16_t at16(uint8_t bank, uint16_t addr) { return cart16(cart_offset(bank, addr)); }
static DataRef ref(uint8_t bank, uint16_t addr) { DataRef r = { cart_offset(bank, addr) }; return r; }

/* ---- picture lists: ActorPictureQueue reads (word at +$29) and the list
   that word names with the data bank the actor loop leaves. The record says
   DB = $81 there; the traces read $80:8909-$80:890A as data (194 scripts)
   and $81:8909 never (code only), so the bank is $80. Those bytes are code
   the game also reads as data, which cart.dat leaves out until that is
   settled (data/NOTES): until then the word reads 0. ---- */

DataRef k21_pictures(void) { return ref(0x80, 0x8909); }

/* ---- kind $2E: the offsets at $80:D152 (x) and $80:D160 (y), words by
   Aladdin's frame times 2 (the record's comment says bank $83: the traces
   read $80:D152-$80:D16F as data, and $83:D152 on is code) ---- */

uint16_t k2e_beside_x(uint8_t at) { return at16(0x80, (uint16_t)(0xD152 + at)); }
uint16_t k2e_beside_y(uint8_t at) { return at16(0x80, (uint16_t)(0xD160 + at)); }

/* ---- kind $35: the set gems' bits, words at $80:D1B6 read at the digit's
   byte offset (the record's comment says bank $80; the traces read
   $80:D1B6 on as data, $83:D1B6 is code) ---- */

uint16_t k35_set_bit(uint8_t at) { return at16(0x80, (uint16_t)(0xD1B6 + at)); }

/* ---- probe lists: the map's probes read them with the data bank the
   kinds run with, $80 (the traces read $80:D18B on as data) ---- */

DataRef k31_probes(void) { return ref(0x80, 0xD18B); }
