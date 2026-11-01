/*
 * object_fields.c: small steps object code shares that set fields of the
 * object's record, reached through the direct page (D = the record).
 */
#include "machine.h"
#include "cpu_ops.h"
#include "object.h"
#include "wram_state.h"

/* The object being worked on, as some bank-$84 code keeps it (word). */
#define OBJECT_CURRENT 0x001B14u

/* Tables in the bank DB names: two bytes for OBJ_08's low bits, by the
   frame's parity ($84:D6B9); three-byte values for OBJ_19 and OBJ_1D
   ($84:D8D2 for one family of objects, $A5:CFB7 and $A5:CFCF for
   another). */
#define PARITY_BITS 0xD6B9u
#define TRIPLES_84 0xD8D2u
#define TRIPLES_A5_19 0xCFB7u
#define TRIPLES_A5_1D 0xCFCFu

static uint32_t in_db(uint32_t addr)
{
    return (uint32_t)cpu.db << 16 | addr;
}

/* LDA abs,Y / STA dp, three times: the three bytes from `table` + Y to
   the direct page at `off`; A low the last, N and Z from it. */
static void triple_from(uint32_t table, uint8_t off)
{
    uint8_t a = 0;
    int i;

    for (i = 0; i < 3; i++) {
        a = rd8(in_db(table + i + (uint8_t)cpu.y));
        wr8(dp((uint8_t)(off + i)), a);
    }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = nz8(cpu.p, a);
}

/*
 * @routine 83:824B ObjectMarkCC
 * WHAT IT DOES: Sets the record's first byte to $CC.
 * HOW: LDA #$CC, STA to direct page 0.
 * Entry: M 8-bit; D = the record. Exit: A low = $CC; N set, Z clear. A
 * high, X, Y and the other flags unchanged.
 */
void abi_ObjectMarkCC(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xCC);
    cpu.p = nz8(cpu.p, 0xCC);
    wr8(dp(OBJ_00), 0xCC);
}

/*
 * @routine 83:9A34 ObjectCopy0CTo40
 * WHAT IT DOES: Copies the record's word at $0C to its word at $40.
 * HOW: A byte at a time, low then high.
 * Entry: M 8-bit; D = the record. Exit: A low = the high byte copied; N
 * and Z from it. A high, X, Y and the other flags unchanged.
 */
void abi_ObjectCopy0CTo40(void)
{
    uint8_t a;

    a = rd8(dp(OBJ_0C));
    wr8(dp(OBJ_40), a);
    a = rd8(dp(OBJ_0C + 1));
    wr8(dp(OBJ_40 + 1), a);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = nz8(cpu.p, a);
}

/*
 * @routine 83:9A3D ObjectSet19To010000
 * WHAT IT DOES: Sets the record's three bytes at $19 to 0, 1, 0.
 * HOW: STZ, LDA #1 and STA, STZ.
 * Entry: M 8-bit; D = the record. Exit: A low = 1; N and Z clear. A high,
 * X, Y and the other flags unchanged.
 */
void abi_ObjectSet19To010000(void)
{
    wr8(dp(OBJ_19), 0);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 1);
    cpu.p = nz8(cpu.p, 1);
    wr8(dp(OBJ_19 + 1), 1);
    wr8(dp(OBJ_19 + 2), 0);
}

/*
 * @routine 83:9BFA ObjectSet44To40And80
 * WHAT IT DOES: Sets the record's words at $44 and $46 to $0040 and
 * $0080.
 * HOW: $40 to $44, shifted left to $80 for $46, then both high bytes 0.
 * Entry: M 8-bit; D = the record. Exit: A low = $80; N set, Z and C
 * clear. A high, X, Y and the other flags unchanged.
 */
void abi_ObjectSet44To40And80(void)
{
    wr8(dp(OBJ_44), 0x40);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x80);
    cpu.p = (uint8_t)(nz8(cpu.p, 0x80) & ~P_C);
    wr8(dp(OBJ_46), 0x80);
    wr8(dp(OBJ_45), 0);
    wr8(dp(OBJ_47), 0);
}

/*
 * @routine 84:8CD2 ObjectParityBitsSet
 * WHAT IT DOES: Sets the low two bits of the record's byte at $08 from a
 * two-entry table by whether the frame count is odd or even, so the
 * object alternates between two settings frame by frame.
 * HOW: The frame count's bit 0 picks the table byte, ORed into the byte
 * with its low two bits cleared.
 * Entry: M 8-bit, X 8-bit; D = the record; DB the table's bank ($84,
 * with work RAM at $0000-$1FFF). Exit: A low = the new byte; X = the
 * parity (0 or 1); N and Z from the byte. A high, Y and the other flags
 * unchanged.
 */
void abi_ObjectParityBitsSet(void)
{
    uint8_t x = (uint8_t)(rd8(FRAME_COUNT) & 1), a;

    cpu.x = x;
    a = (uint8_t)(rd8(dp(OBJ_08)) & 0xFC);
    a |= rd8(in_db(PARITY_BITS + x));
    wr8(dp(OBJ_08), a);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = nz8(cpu.p, a);
}

/*
 * @routine 84:9ED0 Object19FromTable84
 * WHAT IT DOES: Loads the record's three bytes at $19 from the table at
 * $D8D2, entry Y.
 * HOW: Three byte copies.
 * Entry: M 8-bit, X 8-bit; Y = the entry's offset; D = the record; DB
 * the table's bank ($84). Exit: A low = the third byte; N and Z from it. A high, X, Y and
 * the other flags unchanged.
 */
void abi_Object19FromTable84(void)
{
    triple_from(TRIPLES_84, OBJ_19);
}

/*
 * @routine 84:9EE0 Object1DFromTable84
 * WHAT IT DOES: Loads the record's three bytes at $1D from the table at
 * $D8D2, entry Y.
 * HOW: Three byte copies.
 * Entry: M 8-bit, X 8-bit; Y = the entry's offset; D = the record; DB
 * the table's bank ($84). Exit: A low = the third byte; N and Z from it. A high, X, Y and
 * the other flags unchanged.
 */
void abi_Object1DFromTable84(void)
{
    triple_from(TRIPLES_84, OBJ_1D);
}

/*
 * @routine A5:EA93 Object19FromTableA5
 * WHAT IT DOES: Loads the record's three bytes at $19 from the table at
 * $CFB7, entry Y.
 * HOW: Three byte copies.
 * Entry: M 8-bit, X 8-bit; Y = the entry's offset; D = the record; DB
 * the table's bank ($A5). Exit: A low = the third byte; N and Z from it. A high, X, Y and
 * the other flags unchanged.
 */
void abi_Object19FromTableA5(void)
{
    triple_from(TRIPLES_A5_19, OBJ_19);
}

/*
 * @routine A5:EAA3 Object1DFromTableA5
 * WHAT IT DOES: Loads the record's three bytes at $1D from the table at
 * $CFCF, entry Y.
 * HOW: Three byte copies.
 * Entry: M 8-bit, X 8-bit; Y = the entry's offset; D = the record; DB
 * the table's bank ($A5). Exit: A low = the third byte; N and Z from it. A high, X, Y and
 * the other flags unchanged.
 */
void abi_Object1DFromTableA5(void)
{
    triple_from(TRIPLES_A5_1D, OBJ_1D);
}

/*
 * @routine 83:9BE5 ObjectSet44By07
 * WHAT IT DOES: Sets the record's words at $44 and $46 to $0020 and
 * $0040, or to $0028 and $0050 when bit 1 of its byte at $07 is set.
 * HOW: The value in X, doubled through A for $46; both high bytes 0.
 * Entry: M 8-bit, X 8-bit; D = the record. Exit: X = $20 or $28; A low =
 * twice it; N, Z and C clear. A high, Y and the other flags unchanged.
 */
void abi_ObjectSet44By07(void)
{
    uint8_t x = (rd8(dp(OBJ_07)) & 0x02) ? 0x28 : 0x20;

    cpu.x = x;
    wr8(dp(OBJ_44), x);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | (uint8_t)(x << 1));
    cpu.p = (uint8_t)(nz8(cpu.p, (uint8_t)(x << 1)) & ~P_C);
    wr8(dp(OBJ_46), (uint8_t)(x << 1));
    wr8(dp(OBJ_45), 0);
    wr8(dp(OBJ_47), 0);
}

/*
 * @routine 83:A167 ObjectSet1DTo20
 * WHAT IT DOES: Clears the record's three bytes at $1D and sets its bytes
 * at $20, $47 and $48 to 8, $28 and 1.
 * HOW: Three STZs and three stores, in that order.
 * Entry: M 8-bit; D = the record. Exit: A low = 1; N and Z clear. A high,
 * X, Y and the other flags unchanged.
 */
void abi_ObjectSet1DTo20(void)
{
    wr8(dp(OBJ_1D), 0);
    wr8(dp(OBJ_1D + 1), 0);
    wr8(dp(OBJ_1D + 2), 0);
    wr8(dp(OBJ_20), 0x08);
    wr8(dp(OBJ_47), 0x28);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 1);
    cpu.p = nz8(cpu.p, 1);
    wr8(dp(OBJ_48), 1);
}

/*
 * @routine 83:8267 ObjectStateSet8C
 * WHAT IT DOES: Puts the object whose record Y points at into state $8C,
 * with bit 7 of its byte at $08 set and of its byte at $09 cleared.
 * HOW: ORA and AND on the two bytes, then the state byte.
 * Entry: M 8-bit, X 16-bit; Y = the record (in the bank DB names, whose
 * $0000-$1FFF is work RAM). Exit: A low = $8C; N set, Z clear. A high, X,
 * Y and the other flags unchanged.
 */
void abi_ObjectStateSet8C(void)
{
    uint16_t y = cpu.y;

    wr8(in_db((uint32_t)y + OBJ_08), (uint8_t)(rd8(in_db((uint32_t)y + OBJ_08)) | 0x80));
    wr8(in_db((uint32_t)y + OBJ_09), (uint8_t)(rd8(in_db((uint32_t)y + OBJ_09)) & 0x7F));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x8C);
    cpu.p = nz8(cpu.p, 0x8C);
    wr8(in_db((uint32_t)y + OBJ_00), 0x8C);
}

/*
 * @routine 83:B61A ObjectCap1D
 * WHAT IT DOES: Limits the low word of the record's value at $1D to
 * $0600 while the value is not negative (bit 7 of $1F clear), clearing
 * its byte at $20 when it does.
 * HOW: Compares $0600 with the word (16-bit A); stores it and clears $20
 * when the word is larger.
 * Entry: M 8-bit; D = the record. Exit: negative: A low = the byte at
 * $1F, N set, Z clear. Otherwise A = $0600; N, Z and C from comparing
 * $0600 with the word as it was. M 8-bit; X, Y and the other flags
 * unchanged.
 */
void abi_ObjectCap1D(void)
{
    uint8_t a = rd8(dp(OBJ_1D + 2));

    cpu.p = nz8(cpu.p, a);
    if (a & 0x80) {
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
        return;
    }
    op_rep(P_M);
    cpu.a = 0x0600;
    op_cmp16(rd16(dp(OBJ_1D)));
    if (!(cpu.p & P_C)) {
        wr16(dp(OBJ_1D), 0x0600);
        op_sep(P_M);
        wr8(dp(OBJ_20), 0);
    }
    op_sep(P_M);
}

/*
 * @routine 84:8CE2 ObjectLayerByHeight
 * WHAT IT DOES: Sets bits 4-5 of the current object's byte at $09 to 1
 * when its Y position is less than 128, else to 2.
 * HOW: The record is the one $1B14 points at; with 16-bit X and Y, the
 * byte's two bits are cleared and the value ORed in.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: A low = the new byte, N and Z from it; C set when the Y position
 * is 128 or more; X = the record's low byte, Y = the position's low byte.
 * A high and the other flags unchanged.
 */
void abi_ObjectLayerByHeight(void)
{
    uint16_t x, y;
    uint8_t a;

    op_rep(P_X);
    x = rd16(OBJECT_CURRENT);
    cpu.x = x;
    wr8(in_db((uint32_t)x + OBJ_09), (uint8_t)(rd8(in_db((uint32_t)x + OBJ_09)) & 0xCF));
    y = rd16(in_db((uint32_t)x + OBJ_0F));
    cpu.y = y;
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (y >= 0x0080 ? P_C : 0));
    a = (uint8_t)((y >= 0x0080 ? 0x20 : 0x10) | rd8(in_db((uint32_t)x + OBJ_09)));
    wr8(in_db((uint32_t)x + OBJ_09), a);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = nz8(cpu.p, a);
    op_sep(P_X);
}

/*
 * @routine 84:8A52 ObjectListFindFree
 * WHAT IT DOES: Walks a list of records from the one the record's word at
 * $37 points at, to the first whose byte at $18 is 0.
 * HOW: With 16-bit X and Y each record's $18 byte is read; when not 0 the
 * walk goes on to the record its own $37 word links, until a link of 0.
 * Entry: M 8-bit, X 8-bit; D = the record; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: found: C clear, Z set, A low = 0, X = the record's low
 * byte. End of the list: C and Z set, N clear, X = 0, A low = the last $18
 * byte read. Y = the last link read, low byte, or unchanged when none was
 * read. A high and the other flags unchanged.
 */
void abi_ObjectListFindFree(void)
{
    uint16_t x;
    uint8_t a;

    op_rep(P_X);
    cpu.p &= (uint8_t)~P_C;
    x = rd16(dp(OBJ_37));
    for (;;) {
        a = rd8(in_db((uint32_t)x + OBJ_18));
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
        cpu.p = nz8(cpu.p, a);
        if (a == 0)
            break;
        cpu.y = rd16(in_db((uint32_t)x + OBJ_37));
        x = cpu.y;
        cpu.p = nz16(cpu.p, x);
        if (x == 0) {
            cpu.p |= P_C;
            break;
        }
    }
    cpu.x = x;
    op_sep(P_X);
}
