/*
 * level_actor_graphics.c: the graphics of the actor kinds a level places:
 * marked from its placement list, then unpacked into bank $7F. Drafted by
 * cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor_anim.h"
#include "wram_video.h"

void abi_AnimSetGraphicsUnpack(void);
void abi_AnimSetsFill(void);

#define PLACEMENTS 0x8A7Au  /* bank $82: by level, its placement list's offset (word); there a count, then 6-byte entries (+1 the kind, +2 bits 5-6 its variant) */
#define GFX_ENTRIES 0xAC18u  /* bank $82: per kind, its graphics entry's offset from GFX_PARTS (word) */
#define GFX_PARTS 0xAC58u  /* bank $82: per entry: length (low 14 bits; bits 14-15 the extra tile slots), ..., +5 by variant a byte */
#define KIND_GFX 0x7FE500u  /* per kind: where its graphics went (word), then three more tables to $7F:EAFF */
#define KIND_VARIANT_BYTE 0x7FEA00u  /* per kind: its variant's byte from its graphics entry, 0 until set */
#define VARIANT 0x000Eu  /* the placement's variant (word) */
#define UNPACK_NEXT 0x006Bu  /* where the next unpack goes in bank $7F (word) */

/* The data bank to $82. */
static void bank_82(void)
{
    a8_load(0x82);
    pha8();
    plb();
}

/*
 * @routine 81:B9F4 LevelActorGraphicsLoad
 * WHAT IT DOES: Loads the graphics of every actor kind the level places:
 * the kinds are marked from the level's placement list, with each kind's
 * extra tile slots and its variant's byte, then each marked kind's
 * graphics are unpacked into bank $7F from $7000 on.
 * HOW: With DB = $82: AnimSetsFill 0; $7F:E500-$7F:EAFF cleared. The
 * level's list (PLACEMENTS by LEVEL): for each of its count's entries, the
 * kind's flag in ANIM_SET_OF_TYPE = 1, its CELL_EXTRA_SLOTS = bits 6-7 of
 * byte 1 of its GFX_PARTS entry, and its KIND_VARIANT_BYTE, while 0, = the
 * entry's byte 5 + the variant (bits 5-6 of the placement's byte 2). Then
 * UNPACK_NEXT = $7000 and AnimSetGraphicsUnpack for each kind 0-255
 * marked.
 * Entry: M 8-bit, X 8-bit; DB any; called by JSL (81:86BA). Exit: M and X
 * 8-bit; X = 0; DB as at entry, N and Z from it (PLB); A, Y as the last
 * unpack left them.
 */
void abi_LevelActorGraphicsLoad(void)
{
    uint8_t count, kind, v;

    phb();
    bank_82();
    a8_load(0x00);
    abi_AnimSetsFill();
    op_rep(P_X);
    a8_load(0x00);
    for (int x = 0x05FF; x >= 0; x--)     /* every kind's tables cleared */
        wr8(KIND_GFX + (uint32_t)x, 0x00);
    cpu.x = 0xFFFF;
    nz16_of(0xFFFF);
    /* each placement of the level: its kind's animation set wanted, its
       extra tile slots, and (first time) its variant's byte */
    cpu.a = (uint16_t)(rd8(dba(LEVEL)) << 1);
    cpu.x = cpu.a;
    cpu.y = rd16(dba(PLACEMENTS) + cpu.x);
    count = rd8(dba(PLACEMENTS) + cpu.y);
    a8_load(count);
    if (count) {
        wr8(dba(SCRATCH), count);
        do {
            kind = rd8(dba(PLACEMENTS + 1) + cpu.y);
            cpu.a = kind;
            cpu.x = kind;
            a8_load(0x01);
            wr8(ANIM_SET_OF_TYPE + kind, 0x01);
            v = (uint8_t)((rd8(dba(PLACEMENTS + 2) + cpu.y) >> 5) & 0x03);
            a8_load(v);
            wr8(dba(VARIANT), v);
            wr8(dba(VARIANT + 1), 0);
            phy16();
            phb();
            bank_82();
            cpu.y = rd16(dba(GFX_ENTRIES) + (uint16_t)(kind << 1));
            v = (uint8_t)(rd8(dba(GFX_PARTS + 1) + cpu.y) >> 6);
            a8_load(v);
            wr8(CELL_EXTRA_SLOTS + kind, v);
            v = rd8(KIND_VARIANT_BYTE + kind);
            a8_load(v);
            if (!v) {
                flag_set(P_C, 0);
                cpu.y = op_adc16(cpu.y, rd16(dba(VARIANT)), &cpu.p);
                a8_load(rd8(dba(GFX_PARTS + 5) + cpu.y));
                wr8(KIND_VARIANT_BYTE + kind, (uint8_t)cpu.a);
            }
            op_rep(P_M);
            plb();
            pla16();
            flag_set(P_C, 0);
            cpu.a = op_adc16(cpu.a, 0x0006, &cpu.p);
            cpu.y = cpu.a;
            op_sep(P_M);
        } while (dec8_at(dba(SCRATCH)));
    }
    /* the wanted sets' graphics unpacked from $7F:7000 */
    cpu.x = 0x7000;
    wr16(dba(UNPACK_NEXT), 0x7000);
    op_sep(0x30);
    x8_load(0x00);
    do {
        v = rd8(ANIM_SET_OF_TYPE + cpu.x);
        a8_load(v);
        if (v) {
            a8_load((uint8_t)cpu.x);
            abi_AnimSetGraphicsUnpack();
        }
        x8_load((uint8_t)(cpu.x + 1));
    } while (cpu.x);
    plb();
}
