/*
 * unpack.c: setting up the unpacking of packed data (graphics) into bank
 * $7F.
 *
 * An unpack reads its source through the long pointer at $65 (offset 0
 * in the source's bank, the offset itself in Y) and writes 8 bytes a
 * group to bank $7F at X; $69 counts the groups. A group is a flag byte
 * and a fill byte, then a byte for each set flag bit: bit 7 first, a set
 * bit takes the next literal byte, a clear one the fill byte. The source
 * runs on across banks: past $FFFF at $8000 of the next.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "cpu_ops.h"
#include "native.h"
void abi_UnpackSetBegin(void);
void abi_UnpackBegin(void);
void abi_UnpackGroup(void);
void abi_UnpackSet(void);
void abi_AnimSetGraphicsUnpack(void);
void abi_LevelGraphicsLoad(void);
#include "wram.h"
#include "wram_video.h"
#include "hw.h"

#define UNPACK_SOURCE 0x65u        /* direct page (D = 0): the source's pointer (long) */
#define UNPACK_FLAGS 0x0Cu         /* direct page: the group's flag byte, shifted out bit by bit */
#define UNPACK_FILL 0x0Du          /* direct page: the group's fill byte */
#define UNPACK_GROUPS 0x69u        /* direct page: the 8-byte groups left (word) */
#define UNPACK_NEXT 0x006Bu        /* where the next unpack goes in bank $7F (word) */
#define LEVEL_GFX 0xDF98u          /* bank $81: per level a word, its graphics list's offset from here: a count, 7-byte VRAM parts (source offset, bank, length, VRAM address), then a count and 5-byte kept parts (source offset, bank, length) */
#define LEVEL_GFX_LEFT 0x1Eu       /* direct page: parts left */
#define ANIM_GFX_ENTRIES 0x82AC18u     /* per animation set, a word: its graphics entry's offset from ANIM_GFX_PARTS */
#define ANIM_GFX_PARTS 0x82AC58u /* per entry: length (low 14 bits), source offset (words), bank */
#define ANIM_GFX_AT 0x7FE500u  /* per animation set, where its graphics went in bank $7F (word) */
#define UNPACK_SETS 0xF039u        /* bank $80: per set, 7 bytes: destination, source offset (words), bank, length (word) */

/* A word of the data bank. */
static uint16_t db16(uint16_t addr) { return rd16(dba(addr)); }

/* P as PLP leaves it (X and Y cut to a byte when it has X 8-bit). */
static void p_restore(uint8_t p)
{
    cpu.p = 0;
    op_sep(p);
}

/* A byte's length to 8-byte groups, rounded up (C clear, as the LSRs
   leave it). */
static uint16_t groups_of(uint16_t length)
{
    uint16_t g;

    flag_set(P_C, 0);
    g = (uint16_t)((op_adc16(length, 0x0007, &cpu.p) & 0xFFF8) >> 3);
    cpu.a = g;
    flag_set(P_C, 0);
    return g;
}

/*
 * @routine 81:BB3D UnpackBegin
 * WHAT IT DOES: Starts an unpack into bank $7F: the source offset in Y
 * and the data bank at $7F, and says whether there is anything to unpack.
 * HOW: Y = A; DB = $7F; A = the group count's two bytes ORed.
 * Entry: M 16-bit, X 16-bit; A = the source offset; D = 0; $65-$67 = the
 * source's bank pointer; $69 = the 8-byte groups to unpack (word); called
 * by JSR. Exit: M 8-bit; Y = the source offset; DB = $7F; Z set when
 * there are no groups, N from the OR; X, C, V unchanged.
 */
void abi_UnpackBegin(void)
{
    cpu.y = cpu.a;
    op_sep(P_M);
    cpu.db = 0x7F;
    a8_load((uint8_t)(rd8(dpa(UNPACK_GROUPS)) | rd8(dpa(UNPACK_GROUPS + 1))));
}

/*
 * @routine 81:B92D LevelGraphicsLoad
 * WHAT IT DOES: Loads the level's graphics: the parts that go to VRAM
 * now, then the parts kept unpacked in bank $7F.
 * HOW: VMAIN = $80; with P and D kept and D = 0, 16-bit: the level's list
 * (a word at $81:DF98 + level * 2) gives a count; for each 7-byte entry
 * the part (source offset, bank, length) is unpacked to $7F:0000
 * (UnpackBegin, UnpackGroup) and copied to VRAM at the entry's address by
 * DMA channel 0. Then the next destination ($6B) = $5800, and the count
 * after the list gives 5-byte entries, each unpacked to the next
 * destination on. The count kept at $1E. With no parts, it ends (through
 * the JMP at $81:B92A).
 * Entry: M 8-bit; DB = $81; called by JSL. Exit: P and D as at entry; A,
 * X, Y as the last part left them; DB as at entry.
 */
void abi_LevelGraphicsLoad(void)
{
    uint8_t p, inner, db;
    uint16_t d, x, n;

    a8_load(0x80);
    wr8(dba((uint16_t)VMAIN), 0x80);
    p = cpu.p;
    d = cpu.d;
    op_rep(0x30);
    cpu.d = 0;
    /* the level's list */
    cpu.y = (uint16_t)(db16(LEVEL) << 1);
    x = db16((uint16_t)(LEVEL_GFX + cpu.y));
    cpu.x = x;
    a16_load(n = db16((uint16_t)(LEVEL_GFX + x)) & 0x00FF);
    if (!n)
        goto out;
    dp16_set(LEVEL_GFX_LEFT, n);
    /* the VRAM parts: each unpacked to $7F:0000 and copied by DMA */
    do {
        dp16_set(UNPACK_SOURCE, 0);
        dp16_set(UNPACK_SOURCE + 2, db16((uint16_t)(LEVEL_GFX + 3 + x)));
        dp16_set(UNPACK_GROUPS, groups_of(db16((uint16_t)(LEVEL_GFX + 4 + x))));
        a16_load(db16((uint16_t)(LEVEL_GFX + 1 + x)));
        inner = cpu.p;
        db = cpu.db;
        cpu.x = 0;
        abi_UnpackBegin();
        if (!(cpu.p & P_Z))
            do
                abi_UnpackGroup();
            while (!(cpu.p & P_Z));
        cpu.db = db;
        p_restore(inner);
        cpu.x = x;
        wr16(dba((uint16_t)VMADDL), db16((uint16_t)(LEVEL_GFX + 6 + x)));
        wr16(dba((uint16_t)DMAP0), 0x1801);
        wr16(dba((uint16_t)A1T0L), 0x0000);
        wr16(dba((uint16_t)A1B0), 0x007F);
        cpu.a = db16((uint16_t)(LEVEL_GFX + 4 + x));
        wr16(dba((uint16_t)DAS0L), cpu.a);
        op_sep(P_M);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
        wr8(dba((uint16_t)MDMAEN), 0x01);
        op_rep(P_M);
        flag_set(P_C, 0);
        cpu.x = cpu.a = x = op_adc16(x, 0x0007, &cpu.p);
    } while (dec16_at(dpa(LEVEL_GFX_LEFT)));
    /* the kept parts: unpacked one after another from $7F:5800 */
    wr16(dba(UNPACK_NEXT), 0x5800);
    a16_load(n = db16((uint16_t)(LEVEL_GFX + 1 + x)) & 0x00FF);
    if (!n)
        goto out;
    dp16_set(LEVEL_GFX_LEFT, n);
    do {
        dp16_set(UNPACK_SOURCE, 0);
        dp16_set(UNPACK_SOURCE + 2, db16((uint16_t)(LEVEL_GFX + 4 + x)));
        dp16_set(UNPACK_GROUPS, groups_of(db16((uint16_t)(LEVEL_GFX + 5 + x))));
        db = cpu.db;
        inner = cpu.p;
        a16_load(db16((uint16_t)(LEVEL_GFX + 2 + x)));
        abi_UnpackBegin();
        cpu.x = dp16(UNPACK_NEXT);
        do
            abi_UnpackGroup();
        while (!(cpu.p & P_Z));
        dp16_set(UNPACK_NEXT, cpu.x);
        p_restore(inner);
        cpu.db = db;
        flag_set(P_C, 0);
        cpu.x = cpu.a = x = op_adc16(x, 0x0005, &cpu.p);
    } while (dec16_at(dpa(LEVEL_GFX_LEFT)));
out:
    cpu.d = d;
    p_restore(p);
}

/*
 * @routine 81:BA9E AnimSetGraphicsUnpack
 * WHAT IT DOES: Unpacks animation set A's graphics into bank $7F where the
 * last unpack stopped, and notes where they went.
 * HOW: With X, Y, D, A, DB and P kept on the stack: the next destination
 * ($6B-$6D) is copied to $7F:E500 + A * 2; the set's entry (a word at
 * $82:AC18 + A * 2) gives its length (low 14 bits, in 8-byte groups), its
 * source offset and bank; when the length is not 0, UnpackBegin and
 * UnpackGroup until no groups are left, from the next destination on,
 * which moves past them.
 * Entry: A low byte = the set; called by JSR. Exit: A low byte, X, Y
 * (low bytes), D, DB and P as at entry; $6B = past the set's graphics.
 */
void abi_AnimSetGraphicsUnpack(void)
{
    uint8_t x8 = (uint8_t)cpu.x, y8 = (uint8_t)cpu.y, a8 = (uint8_t)cpu.a, db = cpu.db, p = cpu.p;
    uint16_t d = cpu.d, set, x, length;

    op_rep(0x30);
    /* where the set's graphics go: the next place */
    set = (uint16_t)((cpu.a & 0x00FF) << 1);
    wr16(ANIM_GFX_AT + set, rd16(UNPACK_NEXT));
    wr16(ANIM_GFX_AT + 1 + set, rd16(UNPACK_NEXT + 1));
    cpu.d = 0;
    x = rd16(ANIM_GFX_ENTRIES + set);
    cpu.x = x;
    a16_load(length = rd16(ANIM_GFX_PARTS + x));
    if (length) {
        dp16_set(UNPACK_GROUPS, (uint16_t)((length & 0x3FFF) >> 3));
        dp16_set(UNPACK_SOURCE, 0);
        dp16_set(UNPACK_SOURCE + 2, rd16(ANIM_GFX_PARTS + 4 + x));
        a16_load(rd16(ANIM_GFX_PARTS + 2 + x));
        abi_UnpackBegin();
        cpu.x = dp16(UNPACK_NEXT);
        do
            abi_UnpackGroup();
        while (!(cpu.p & P_Z));
        dp16_set(UNPACK_NEXT, cpu.x);
    }
    p_restore(p);
    cpu.db = db;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a8);
    cpu.d = d;
    cpu.y = y8;
    x8_load(x8);
}


/*
 * @routine 81:BB04 UnpackSetBegin
 * WHAT IT DOES: Sets up the unpacking of packed set A (the graphics sets
 * $81:BAF1 unpacks) into bank $7F.
 * HOW: Entry A of the 7-byte table at $80:F039 (A * 7, counting in scratch
 * $0C, with DB = $80 and D = 0): X = where it goes in bank $7F, the source
 * pointer at $65 = its bank (+4) with offset 0, $69 = its length (+5)
 * rounded up to 8-byte groups; then the source offset (+2) goes on into
 * UnpackBegin.
 * Entry: M 16-bit, X 16-bit; A low byte = the set; called by JSR. Exit:
 * as UnpackBegin; X = where the set goes; D = 0.
 */
void abi_UnpackSetBegin(void)
{
    uint16_t set = cpu.a & 0x00FF, y;

    wr16(dba(SCRATCH), set);
    op_sep(P_M);
    cpu.db = 0x80;
    op_rep(0x30);
    /* the set's 7-byte entry */
    set = db16(SCRATCH);
    flag_set(P_C, 1);
    y = op_sbc16((uint16_t)(set << 3), set, &cpu.p);
    cpu.y = y;
    cpu.d = 0;
    cpu.x = db16((uint16_t)(UNPACK_SETS + y));
    dp16_set(UNPACK_SOURCE, 0);
    dp16_set(UNPACK_SOURCE + 2, db16((uint16_t)(UNPACK_SETS + 4 + y)));
    dp16_set(UNPACK_GROUPS, groups_of(db16((uint16_t)(UNPACK_SETS + 5 + y))));
    a16_load(db16((uint16_t)(UNPACK_SETS + 2 + y)));
    abi_UnpackBegin();
}

/* The next source byte: past $FFFF the source goes on at $8000 of the
   next bank. */
static uint8_t source_byte(uint16_t *y)
{
    uint8_t v = rd8(dp_long_y(UNPACK_SOURCE, *y));

    if (!++*y) {
        *y = 0x8000;
        inc8_at(dpa(UNPACK_SOURCE + 2));
    }
    return v;
}

/*
 * @routine 81:BB49 UnpackGroup
 * WHAT IT DOES: Unpacks one 8-byte group of a packed asset to $7F:X and
 * counts it off.
 * HOW: Takes the flag and fill bytes from the source, then for each of the
 * 8 bytes shifts the flags: a set bit stores the next source byte, a clear
 * one the fill byte. Each source step past $FFFF goes on at $8000 of the
 * next bank. X moves on 8; the groups left go down by one.
 * Entry: M 8-bit, X 16-bit; D = 0; DB = $7F; X = the destination; Y = the
 * source offset in the bank at UNPACK_SOURCE + 2; called by JSR
 * (UnpackSet). Exit: X = the next destination, A = X (16 bits, from
 * TXA/ADC), Y the next source offset; N and Z from the groups-left
 * decrement (Z set: none left); C from the ADC; M 8-bit, X 16-bit.
 */
void abi_UnpackGroup(void)
{
    uint16_t y = cpu.y, x = cpu.x;
    uint8_t flags, fill, v = 0;
    int i;

    /* a flag byte and a fill byte; then 8 bytes, each a source byte when
       its flag (from the top) is set, else the fill */
    flags = source_byte(&y);
    fill = source_byte(&y);
    wr8(dpa(UNPACK_FILL), fill);
    for (i = 0; i < 8; i++) {
        v = flags & 0x80 ? source_byte(&y) : fill;
        flags <<= 1;
        wr8(dba(0x0000) + (uint16_t)(x + i), v);
    }
    wr8(dpa(UNPACK_FLAGS), 0);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    op_rep(P_M);
    flag_set(P_C, 0);
    cpu.a = op_adc16(x, 0x0008, &cpu.p);
    cpu.x = cpu.a;
    cpu.y = y;
    dec16_at(dpa(UNPACK_GROUPS));
    op_sep(P_M);
}

