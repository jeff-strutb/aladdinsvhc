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

/* ---- kind $2B: by way at $D0B6 its direction, $D0BE its walk, $D0C6 its
   turn, $D0CE its flip; by way * 4 at $D0D6 the point ahead and at $D0F6 the
   point on its surface (x, y words); its probe list at $D116; by n at
   $D11A its leg and $D11E its range; its partner's half-span $D2E3, span
   $D2E5 (words) ---- */

uint8_t k2b_direction(uint8_t way) { return at8((uint16_t)(0xD0B6 + way)); }
uint8_t k2b_walk_anim(uint8_t way) { return at8((uint16_t)(0xD0BE + way)); }
uint8_t k2b_turn_anim(uint8_t way) { return at8((uint16_t)(0xD0C6 + way)); }
uint8_t k2b_flip(uint8_t way) { return at8((uint16_t)(0xD0CE + way)); }
uint16_t k2b_point_x(uint8_t way, int on_surface) { return at16((uint16_t)((on_surface ? 0xD0F6 : 0xD0D6) + 4u * way)); }
uint16_t k2b_point_y(uint8_t way, int on_surface) { return at16((uint16_t)((on_surface ? 0xD0F6 : 0xD0D6) + 4u * way + 2)); }
DataRef k2b_probes(void) { return ref(0xD116); }
uint8_t k2b_leg(uint8_t n) { return at8((uint16_t)(0xD11A + n)); }
uint8_t k2b_range(uint8_t n) { return at8((uint16_t)(0xD11E + n)); }
uint16_t k2b_partner_half(void) { return at16(0xD2E3); }
uint16_t k2b_partner_span(void) { return at16(0xD2E5); }

/* ---- kind $33: the frames at $D199; the uploads at $D1AA, 6 bytes each:
   tile, source, bytes (words) ---- */

uint8_t k33_frame(uint8_t n) { return at8((uint16_t)(0xD199 + n)); }
uint16_t k33_upload_tile(uint8_t at) { return at16((uint16_t)(0xD1AA + at)); }
uint16_t k33_upload_from(uint8_t at) { return at16((uint16_t)(0xD1AA + at + 2)); }
uint16_t k33_upload_bytes(uint8_t at) { return at16((uint16_t)(0xD1AA + at + 4)); }

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

/* ---- kind $59: the row lamps' places at $D3D0, 4 bytes each: x, y (words) ---- */

uint16_t k59_lamp_x(uint8_t at) { return at16((uint16_t)(0xD3D0 + at)); }
uint16_t k59_lamp_y(uint8_t at) { return at16((uint16_t)(0xD3D2 + at)); }

/* ---- kind $63: its probe list at $D4E5, its picture list at $891D; by
   +$07 * 2 its reach (word) at $D4ED and its aims (bytes) at $D4F7, $D4F8 ---- */

DataRef k63_probes(void) { return ref(0xD4E5); }
DataRef k63_pictures(void) { return ref(0x891D); }
uint16_t k63_reach_y(uint8_t at) { return at16((uint16_t)(0xD4ED + at)); }
uint8_t k63_aim_near(uint8_t at) { return at8((uint16_t)(0xD4F7 + at)); }
uint8_t k63_aim_mid(uint8_t at) { return at8((uint16_t)(0xD4F8 + at)); }

/* ---- kind $6B: its probe list at $D663 ---- */

DataRef k6b_probes(void) { return ref(0xD663); }

/* ---- kind $83: its probe list at $D836; by +$07 its delays at $D83A, its
   top speeds at $D83D, its presets at $D840 ---- */

DataRef k83_probes(void) { return ref(0xD836); }
uint8_t k83_delay(uint8_t variant) { return at8((uint16_t)(0xD83A + variant)); }
uint8_t k83_top_speed(uint8_t variant) { return at8((uint16_t)(0xD83D + variant)); }
uint8_t k83_preset(uint8_t variant) { return at8((uint16_t)(0xD840 + variant)); }

/* ---- kind $3A: by its picture * 2 the hold's offsets at $D1FE (x) and $D210
   (y) (words); by its frame his frame at $D1DE, facing left at $D1EE ---- */

int16_t k3a_hold_x(uint8_t picture) { return (int16_t)at16((uint16_t)(0xD1FE + 2u * picture)); }
int16_t k3a_hold_y(uint8_t picture) { return (int16_t)at16((uint16_t)(0xD210 + 2u * picture)); }
uint8_t k3a_frame(uint8_t frame, int facing_left) { return at8((uint16_t)((facing_left ? 0xD1EE : 0xD1DE) + frame)); }

/* ---- kind $23: the children's x at $D06E (words), the +$07 they give their
   kind $6D at $D076 (bytes), by part - 1 ---- */

uint16_t k23_x(uint8_t part) { return at16((uint16_t)(0xD06E + 2u * part)); }
uint8_t k23_next_byte07(uint8_t part) { return at8((uint16_t)(0xD076 + part)); }

/* ---- kind $8D: its picture list at $890B ---- */

DataRef k8d_pictures(void) { return ref(0x890B); }

/* ---- kind $3D: by +$07 at $D296 the records needed; the bob's moves at
   $D286, 4 bytes an entry (a word, then its high byte and a pad) ---- */

uint8_t k3d_records_needed(uint8_t set) { return at8((uint16_t)(0xD296 + set)); }
int16_t k3d_bob_step(uint8_t entry) { return (int16_t)at16((uint16_t)(0xD286 + 4u * entry)); }

/* ---- kinds $7E, $7F: the circlers' first angles at $D828 (words); kind
   $7F's probe list at $D832 ---- */

uint16_t k7e_first_angle(uint8_t n) { return at16((uint16_t)(0xD828 + 2u * n)); }
DataRef k7f_probes(void) { return ref(0xD832); }

/* ---- kind $90: its probe offsets at $CF8F; by +$07 its steps at $D878,
   its preset at $D875, its flights a step at $D87B ---- */

DataRef k90_probes(void) { return ref(0xCF8F); }
uint8_t k90_steps(uint8_t slot) { return at8((uint16_t)(0xD878 + slot)); }
uint8_t k90_preset(uint8_t slot) { return at8((uint16_t)(0xD875 + slot)); }
uint8_t k90_flights(uint8_t slot) { return at8((uint16_t)(0xD87B + slot)); }
