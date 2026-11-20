/*
 * kinds_b_tables.c: the places of the tables kinds B's actor kinds read
 * (kinds_b_tables.h). Every kind runs with the data bank $80, the actor
 * loop's; where a record comment names another bank, the traces
 * (tools/aladdin/codemap.py over build/aladdin/box) settle it, as noted.
 */
#include "data/kinds_b_tables.h"
#include "data/data_internal.h"

static uint8_t at8(uint8_t bank, uint16_t addr) { return cart8(cart_offset(bank, addr)); }
static uint16_t at16(uint8_t bank, uint16_t addr) { return cart16(cart_offset(bank, addr)); }
static DataRef ref(uint8_t bank, uint16_t addr) { DataRef r = { cart_offset(bank, addr) }; return r; }

/* ---- kind $38 (83:BE01): its probes at $80:D1D6; the traces read it there
   and never $83:D1D6 ---- */
DataRef k38_probes(void) { return ref(0x80, 0xD1D6); }

/* ---- kind $3F (83:B19A, DB = $80): its ride test's box by +$07 at
   $80:D299 ---- */
uint8_t k3f_test_box(uint8_t variant) { return at8(0x80, (uint16_t)(0xD299 + variant)); }

/* ---- kind $42 (83:C5DE, 83:C69A): its probe offsets at $80:D2B2 (4 bytes
   a probe), and at $80:D2B6 a word by facing, the x offset of the block
   ahead of it. Every trace reads $80:D2B2-$80:D2B9 as data; $81:D2B2 on is
   code ---- */
DataRef k42_probes(void) { return ref(0x80, 0xD2B2); }
uint16_t k42_ahead_x(uint8_t facing) { return at16(0x80, (uint16_t)(0xD2B6 + 2u * facing)); }

/* ---- kind $43 (83:B393, DB = $80): at $80:D2BA its probe list (4
   bytes), then by way the carry test's box ($80:D2BE), byte pairs of the
   piece count and the first piece's spawn point ($80:D2C1), and its
   animation ($80:D2C7) ---- */
DataRef k43_probes(void) { return ref(0x80, 0xD2BA); }
uint8_t k43_carry_box(uint8_t way) { return at8(0x80, (uint16_t)(0xD2BE + way)); }
uint8_t k43_pieces(uint8_t way) { return at8(0x80, (uint16_t)(0xD2C1 + 2u * way)); }
uint8_t k43_first_spawn_at(uint8_t way) { return at8(0x80, (uint16_t)(0xD2C2 + 2u * way)); }
uint8_t k43_anim(uint8_t way) { return at8(0x80, (uint16_t)(0xD2C7 + way)); }

/* ---- kind $57 (83:D9BA): its probes at $80:D3B8, and its picture list
   at $80:8913, which ActorPictureSend reads (the traces read
   $80:8913-$80:8914 as data and run $81:8913 as code) ---- */
DataRef k57_probes(void) { return ref(0x80, 0xD3B8); }
DataRef k57_own_pictures(void) { return ref(0x80, 0x8913); }

/* ---- kind $52's wait (83:D518): pictures at $80:D382 (00 01 02 03 0A),
   zones at $80:D387 (24 24 28 2C 30); the traces read $80:D382-D38B as
   data; $83:D382 is Kind52Start's code ---- */
uint8_t k52_hurt_picture(uint8_t i) { return at8(0x80, (uint16_t)(0xD382 + i)); }
uint8_t k52_hurt_zone(uint8_t i) { return at8(0x80, (uint16_t)(0xD387 + i)); }

/* ---- kind $5B (83:DF14, its wait 83:DFCD): the delays at $80:D3F8 + its
   +$07 (the record's comment says bank $83, but $83:D3F8-D3FC is kind
   $52's way table and $83:D3FD on its code; cov_kind5b_fall's trace read
   $80:D3FA and $80:D404 and none of $83:D3F8 up). The sounds at $80:D3FC
   + x & $0F; the delays' entries 4 up are the same bytes ---- */
uint8_t k5b_delay(uint8_t variant) { return at8(0x80, (uint16_t)(0xD3F8 + variant)); }
uint8_t k5b_step_sound(uint8_t at) { return at8(0x80, (uint16_t)(0xD3FC + (at & 0x0F))); }

/* ---- kind $6D (83:ECD4, DB = $80): its probe list at $80:D667 ---- */
DataRef k6d_probes(void) { return ref(0x80, 0xD667); }

/* ---- kind $76 (84:8FBA): its picture list, the word it puts at +$29
   ($891B), which ActorPictureSend reads in bank $80: the traces read
   $80:891B-$80:891E as data and run $81:891B as code ---- */
DataRef k76_pictures(void) { return ref(0x80, 0x891B); }

/* ---- kinds $99, $9A (84:A3B7, 84:A446): read $80:D90E-D94D and
   $80:D96E: 32 animation bytes at $80:D90E, then 32 words of offsets at
   $80:D92E; kind $9A's probes at $80:D96E ---- */
DataRef k9a_probes(void) { return ref(0x80, 0xD96E); }
uint8_t k99_anim(uint8_t piece) { return at8(0x80, (uint16_t)(0xD90E + piece)); }
uint16_t k99_spread(uint8_t piece) { return at16(0x80, (uint16_t)(0xD92E + 2u * (piece & 0x1F))); }

/* ---- kind $A2 (84:8249): every trace that ran it read $80:D501 and none
   $84:D501 (its record's DB = $84 is wrong) ---- */
uint8_t ka2_heading(uint8_t at) { return at8(0x80, (uint16_t)(0xD501 + at)); }
