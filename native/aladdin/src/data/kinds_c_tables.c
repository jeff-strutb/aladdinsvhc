/*
 * kinds_c_tables.c: the places of the tables the kinds of actor/kinds_c.h
 * read (kinds_c_tables.h).
 *
 * The kinds' code reads its tables through the data bank the actor loop
 * runs with, $80, whatever the record's comments say: the traces
 * (tools/aladdin/codemap.py over every script's) read $80:D122-$80:D13B
 * (kind $2D), $80:D366-$80:D369 (kind $4F) and $80:D875-$80:D87D (kind
 * $90) as data; $83:D122-$83:D13B and $83:D366 up are code there. So too
 * the words the kinds keep at +$16 and +$29: the picture list at
 * $80:8911 ($81:8911 is code) and the probe lists at $80:D16E ($83:D16E is
 * code) and $80:CF8F.
 */
#include "data/kinds_c_tables.h"
#include "data/data_internal.h"

#define BANK 0x80

static uint8_t at8(uint16_t addr) { return cart8(cart_offset(BANK, addr)); }
static uint16_t at16(uint16_t addr) { return cart16(cart_offset(BANK, addr)); }

static DataRef ref(uint16_t addr)
{
    DataRef r;

    r.at = cart_offset(BANK, addr);
    return r;
}

/* ---- kind $2D: by facing * 4 at $D122 a word (where its reach starts)
   then a word (how far); by +$07 at $D130 the throw's frames and at $D133
   the frame its copy leaves; by arc at $D136 the second arc and at $D12A
   the copy's pose; its picture list at $8911 ---- */

uint16_t k2d_reach_from(uint8_t at) { return at16((uint16_t)(0xD122 + at)); }
uint16_t k2d_reach(uint8_t at) { return at16((uint16_t)(0xD124 + at)); }
uint8_t k2d_throw_frames(uint8_t variant) { return at8((uint16_t)(0xD130 + variant)); }
uint8_t k2d_throw_leaves(uint8_t variant) { return at8((uint16_t)(0xD133 + variant)); }
uint8_t k2d_second_arc(uint8_t arc) { return at8((uint16_t)(0xD136 + arc)); }
uint8_t k2d_copy_pose(uint8_t arc) { return at8((uint16_t)(0xD12A + arc)); }
DataRef k2d_pictures(void) { return ref(0x8911); }

/* ---- kind $2F: its probe offsets at $D16E ---- */

DataRef k2f_probes(void) { return ref(0xD16E); }

/* ---- kind $4F: the four bits at $D366 ---- */

uint8_t k4f_opened_bit(uint8_t n) { return at8((uint16_t)(0xD366 + n)); }

/* ---- kind $90: its probe offsets at $CF8F; by +$07 its steps at $D878,
   its preset at $D875, its flights a step at $D87B ---- */

DataRef k90_probes(void) { return ref(0xCF8F); }
uint8_t k90_steps(uint8_t slot) { return at8((uint16_t)(0xD878 + slot)); }
uint8_t k90_preset(uint8_t slot) { return at8((uint16_t)(0xD875 + slot)); }
uint8_t k90_flights(uint8_t slot) { return at8((uint16_t)(0xD87B + slot)); }
