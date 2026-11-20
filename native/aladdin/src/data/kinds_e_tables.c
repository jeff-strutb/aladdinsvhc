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
DataRef k98_pictures(void) { return ref(0x80, 0x891B); }   /* the traces read $80:891B as data, $81:891B never */

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
DataRef k98_probes(void) { return ref(0x80, 0xD90A); }
DataRef k54_probes(void) { return ref(0x80, 0xD3A4); }
DataRef k55_probes(void) { return ref(0x80, 0xD3AC); }

/* ---- kind $3B's track: $80:D222 (words, x then y, 4 bytes a direction),
   $80:D232, $80:D23A, $80:D242 (bytes); the traces read them in bank $80 ---- */

static uint8_t at8(uint8_t bank, uint16_t addr) { return cart8(cart_offset(bank, addr)); }

uint16_t k3b_ahead_x(uint8_t at) { return at16(0x80, (uint16_t)(0xD222 + at)); }
uint16_t k3b_ahead_y(uint8_t at) { return at16(0x80, (uint16_t)(0xD224 + at)); }
uint8_t k3b_turn(uint8_t at) { return at8(0x80, (uint16_t)(0xD232 + at)); }
uint8_t k3b_spot_dir(uint8_t at) { return at8(0x80, (uint16_t)(0xD23A + at)); }
uint8_t k3b_spot_turn(uint8_t at) { return at8(0x80, (uint16_t)(0xD242 + at)); }

/* ---- kind $2A: $80:D0A2 (bytes by +$07), $80:D0A4 (words), $80:D0B0
   (bytes); the traces read them in bank $80 ---- */

uint8_t k2a_reach(uint8_t at) { return at8(0x80, (uint16_t)(0xD0A2 + at)); }
uint16_t k2a_speed(uint8_t at) { return at16(0x80, (uint16_t)(0xD0A4 + at)); }
uint8_t k2a_leg(uint8_t variant) { return at8(0x80, (uint16_t)(0xD0B0 + variant)); }

/* ---- kinds $54 and $55: $80:D3A8 and $80:D3B0, bytes by +$07 bits 0-1
   (the traces read $80:D3A4-$80:D3B3 as data) ---- */

uint8_t k54_rise(uint8_t way) { return at8(0x80, (uint16_t)(0xD3A8 + way)); }
uint8_t k55_across(uint8_t way) { return at8(0x80, (uint16_t)(0xD3B0 + way)); }

/* ---- kind $97: words at $80:D8FE-$80:D909. No trace reads them (no
   script meets kind $97); bank $80 is the data bank the actor loop runs
   every kind with, as the traces show for the other kinds' tables ---- */

uint16_t k97_window(uint8_t at) { return at16(0x80, (uint16_t)(0xD8FE + at)); }

/* ---- kind $28: its wave, $24 signed bytes at $80:D07A (the traces read
   them in bank $80) ---- */

uint8_t k28_wave(uint8_t at) { return at8(0x80, (uint16_t)(0xD07A + at)); }
