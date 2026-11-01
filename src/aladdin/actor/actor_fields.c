/*
 * actor_fields.c: small steps actor code shares that set fields of the
 * actor's record, reached through the direct page (D = the record).
 */
#include "machine.h"
#include "cpu_ops.h"
#include "actor.h"

#define ACTOR_X_PIXEL (ACTOR_X + 1)
#define ACTOR_Y_PIXEL (ACTOR_Y + 1)

static uint32_t dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}
#include "wram_state.h"

/* The actor being worked on, as some bank-$84 code keeps it (word). */
#define ACTOR_CURRENT 0x001B14u

/* Tables in the bank DB names: two bytes for ACTOR_08's low bits, by the
   frame's parity ($84:D6B9); three-byte values for ACTOR_SPEED_X and ACTOR_SPEED_Y
   ($84:D8D2 for one family of actors, $A5:CFB7 and $A5:CFCF for
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
 * @routine 83:824B ActorStateSetCC
 * WHAT IT DOES: Puts the actor into state $CC.
 * HOW: LDA #$CC, STA to its state byte.
 * Entry: M 8-bit; D = the record. Exit: A low = $CC; N set, Z clear. A
 * high, X, Y and the other flags unchanged.
 */
void abi_ActorStateSetCC(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xCC);
    cpu.p = nz8(cpu.p, 0xCC);
    wr8(dp(ACTOR_STATE), 0xCC);
}

/*
 * @routine 83:9A34 ActorXTo40
 * WHAT IT DOES: Copies the actor's x pixel position to its word at $40.
 * HOW: A byte at a time, low then high.
 * Entry: M 8-bit; D = the record. Exit: A low = the high byte copied; N
 * and Z from it. A high, X, Y and the other flags unchanged.
 */
void abi_ActorXTo40(void)
{
    uint8_t a;

    a = rd8(dp(ACTOR_X_PIXEL));
    wr8(dp(ACTOR_40), a);
    a = rd8(dp(ACTOR_X_PIXEL + 1));
    wr8(dp(ACTOR_40 + 1), a);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = nz8(cpu.p, a);
}

/*
 * @routine 83:9A3D ActorSpeedXSet1
 * WHAT IT DOES: Sets the actor's horizontal speed to 1 pixel a frame.
 * HOW: The speed's three bytes to 0 (fraction), 1, 0.
 * Entry: M 8-bit; D = the record. Exit: A low = 1; N and Z clear. A high,
 * X, Y and the other flags unchanged.
 */
void abi_ActorSpeedXSet1(void)
{
    wr8(dp(ACTOR_SPEED_X), 0);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 1);
    cpu.p = nz8(cpu.p, 1);
    wr8(dp(ACTOR_SPEED_X + 1), 1);
    wr8(dp(ACTOR_SPEED_X + 2), 0);
}

/*
 * @routine 83:9BFA Actor44Set40And80
 * WHAT IT DOES: Sets the actor's words at $44 and $46 to $0040 and $0080.
 * HOW: $40 to $44, shifted left to $80 for $46, then both high bytes 0.
 * Entry: M 8-bit; D = the record. Exit: A low = $80; N set, Z and C
 * clear. A high, X, Y and the other flags unchanged.
 */
void abi_Actor44Set40And80(void)
{
    wr8(dp(ACTOR_44), 0x40);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x80);
    cpu.p = (uint8_t)(nz8(cpu.p, 0x80) & ~P_C);
    wr8(dp(ACTOR_46), 0x80);
    wr8(dp(ACTOR_45), 0);
    wr8(dp(ACTOR_47), 0);
}

/*
 * @routine 84:8CD2 ActorParityBitsSet
 * WHAT IT DOES: Sets the low two bits of the actor's byte at $08 from a
 * two-entry table by whether the frame count is odd or even, so the
 * actor alternates between two settings frame by frame.
 * HOW: The frame count's bit 0 picks the table byte, ORed into the byte
 * with its low two bits cleared.
 * Entry: M 8-bit, X 8-bit; D = the record; DB the table's bank ($84,
 * with work RAM at $0000-$1FFF). Exit: A low = the new byte; X = the
 * parity (0 or 1); N and Z from the byte. A high, Y and the other flags
 * unchanged.
 */
void abi_ActorParityBitsSet(void)
{
    uint8_t x = (uint8_t)(rd8(FRAME_COUNT) & 1), a;

    cpu.x = x;
    a = (uint8_t)(rd8(dp(ACTOR_08)) & 0xFC);
    a |= rd8(in_db(PARITY_BITS + x));
    wr8(dp(ACTOR_08), a);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = nz8(cpu.p, a);
}

/*
 * @routine 84:9ED0 ActorSpeedXFrom84
 * WHAT IT DOES: Sets the actor's horizontal speed from the table at $D8D2,
 * entry Y.
 * HOW: Three byte copies.
 * Entry: M 8-bit, X 8-bit; Y = the entry's offset; D = the record; DB
 * the table's bank ($84). Exit: A low = the third byte; N and Z from it. A high, X, Y and
 * the other flags unchanged.
 */
void abi_ActorSpeedXFrom84(void)
{
    triple_from(TRIPLES_84, ACTOR_SPEED_X);
}

/*
 * @routine 84:9EE0 ActorSpeedYFrom84
 * WHAT IT DOES: Sets the actor's vertical speed from the table at $D8D2,
 * entry Y.
 * HOW: Three byte copies.
 * Entry: M 8-bit, X 8-bit; Y = the entry's offset; D = the record; DB
 * the table's bank ($84). Exit: A low = the third byte; N and Z from it. A high, X, Y and
 * the other flags unchanged.
 */
void abi_ActorSpeedYFrom84(void)
{
    triple_from(TRIPLES_84, ACTOR_SPEED_Y);
}

/*
 * @routine A5:EA93 ActorSpeedXFromA5
 * WHAT IT DOES: Sets the actor's horizontal speed from the table at $CFB7,
 * entry Y.
 * HOW: Three byte copies.
 * Entry: M 8-bit, X 8-bit; Y = the entry's offset; D = the record; DB
 * the table's bank ($A5). Exit: A low = the third byte; N and Z from it. A high, X, Y and
 * the other flags unchanged.
 */
void abi_ActorSpeedXFromA5(void)
{
    triple_from(TRIPLES_A5_19, ACTOR_SPEED_X);
}

/*
 * @routine A5:EAA3 ActorSpeedYFromA5
 * WHAT IT DOES: Sets the actor's vertical speed from the table at $CFCF,
 * entry Y.
 * HOW: Three byte copies.
 * Entry: M 8-bit, X 8-bit; Y = the entry's offset; D = the record; DB
 * the table's bank ($A5). Exit: A low = the third byte; N and Z from it. A high, X, Y and
 * the other flags unchanged.
 */
void abi_ActorSpeedYFromA5(void)
{
    triple_from(TRIPLES_A5_1D, ACTOR_SPEED_Y);
}

/*
 * @routine 83:9BE5 Actor44SetBy07
 * WHAT IT DOES: Sets the actor's words at $44 and $46 to $0020 and $0040,
 * or to $0028 and $0050 when bit 1 of its byte at $07 is set.
 * HOW: The value in X, doubled through A for $46; both high bytes 0.
 * Entry: M 8-bit, X 8-bit; D = the record. Exit: X = $20 or $28; A low =
 * twice it; N, Z and C clear. A high, Y and the other flags unchanged.
 */
void abi_Actor44SetBy07(void)
{
    uint8_t x = (rd8(dp(ACTOR_07)) & 0x02) ? 0x28 : 0x20;

    cpu.x = x;
    wr8(dp(ACTOR_44), x);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | (uint8_t)(x << 1));
    cpu.p = (uint8_t)(nz8(cpu.p, (uint8_t)(x << 1)) & ~P_C);
    wr8(dp(ACTOR_46), (uint8_t)(x << 1));
    wr8(dp(ACTOR_45), 0);
    wr8(dp(ACTOR_47), 0);
}

/*
 * @routine 83:A167 ActorFallStart
 * WHAT IT DOES: Starts the actor falling from rest: no vertical speed,
 * gravity 8 (a fraction of a pixel a frame each frame), and its bytes at
 * $47 and $48 set to $28 and 1.
 * HOW: Three STZs, then the three stores, in that order.
 * Entry: M 8-bit; D = the record. Exit: A low = 1; N and Z clear. A high,
 * X, Y and the other flags unchanged.
 */
void abi_ActorFallStart(void)
{
    wr8(dp(ACTOR_SPEED_Y), 0);
    wr8(dp(ACTOR_SPEED_Y + 1), 0);
    wr8(dp(ACTOR_SPEED_Y + 2), 0);
    wr8(dp(ACTOR_ACCEL_Y), 0x08);
    wr8(dp(ACTOR_47), 0x28);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 1);
    cpu.p = nz8(cpu.p, 1);
    wr8(dp(ACTOR_48), 1);
}

/*
 * @routine 83:8267 ActorStateSet8C
 * WHAT IT DOES: Puts the actor whose record Y points at into state $8C,
 * with bit 7 of its byte at $08 set and of its byte at $09 cleared.
 * HOW: ORA and AND on the two bytes, then the state byte.
 * Entry: M 8-bit, X 16-bit; Y = the record (in the bank DB names, whose
 * $0000-$1FFF is work RAM). Exit: A low = $8C; N set, Z clear. A high, X,
 * Y and the other flags unchanged.
 */
void abi_ActorStateSet8C(void)
{
    uint16_t y = cpu.y;

    wr8(in_db((uint32_t)y + ACTOR_08), (uint8_t)(rd8(in_db((uint32_t)y + ACTOR_08)) | 0x80));
    wr8(in_db((uint32_t)y + ACTOR_09), (uint8_t)(rd8(in_db((uint32_t)y + ACTOR_09)) & 0x7F));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x8C);
    cpu.p = nz8(cpu.p, 0x8C);
    wr8(in_db((uint32_t)y + ACTOR_STATE), 0x8C);
}

/*
 * @routine 83:B61A ActorFallSpeedCap
 * WHAT IT DOES: Limits the actor's falling speed to 6 pixels a frame:
 * while it moves down (the speed's top byte not negative) a speed whose
 * fraction and low pixel byte, as a word, pass $0600 becomes $0600 and its
 * gravity is cleared.
 * HOW: Compares $0600 with the word (16-bit A); stores it and clears the
 * gravity when the word is larger.
 * Entry: M 8-bit; D = the record. Exit: negative: A low = the byte at
 * $1F, N set, Z clear. Otherwise A = $0600; N, Z and C from comparing
 * $0600 with the word as it was. M 8-bit; X, Y and the other flags
 * unchanged.
 */
void abi_ActorFallSpeedCap(void)
{
    uint8_t a = rd8(dp(ACTOR_SPEED_Y + 2));

    cpu.p = nz8(cpu.p, a);
    if (a & 0x80) {
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
        return;
    }
    op_rep(P_M);
    cpu.a = 0x0600;
    op_cmp16(rd16(dp(ACTOR_SPEED_Y)));
    if (!(cpu.p & P_C)) {
        wr16(dp(ACTOR_SPEED_Y), 0x0600);
        op_sep(P_M);
        wr8(dp(ACTOR_ACCEL_Y), 0);
    }
    op_sep(P_M);
}

/*
 * @routine 84:8CE2 ActorPriorityByHeight
 * WHAT IT DOES: Sets the sprite priority (bits 4-5 of the byte at $09) of
 * the actor $1B14 points at to 1 when its y position is less than 128,
 * else to 2.
 * HOW: With 16-bit X and Y, the byte's two bits are cleared and the value
 * ORed in.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: A low = the new byte, N and Z from it; C set when the Y position
 * is 128 or more; X = the record's low byte, Y = the position's low byte.
 * A high and the other flags unchanged.
 */
void abi_ActorPriorityByHeight(void)
{
    uint16_t x, y;
    uint8_t a;

    op_rep(P_X);
    x = rd16(ACTOR_CURRENT);
    cpu.x = x;
    wr8(in_db((uint32_t)x + ACTOR_09), (uint8_t)(rd8(in_db((uint32_t)x + ACTOR_09)) & 0xCF));
    y = rd16(in_db((uint32_t)x + ACTOR_Y_PIXEL));
    cpu.y = y;
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (y >= 0x0080 ? P_C : 0));
    a = (uint8_t)((y >= 0x0080 ? 0x20 : 0x10) | rd8(in_db((uint32_t)x + ACTOR_09)));
    wr8(in_db((uint32_t)x + ACTOR_09), a);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = nz8(cpu.p, a);
    op_sep(P_X);
}

/*
 * @routine 84:8A52 ActorListFindIdle
 * WHAT IT DOES: Walks the actor's list, from the record its link points
 * at, to the first whose byte at $18 is 0.
 * HOW: With 16-bit X and Y each record's $18 byte is read; when not 0 the
 * walk goes on through that record's own link, until a link of 0.
 * Entry: M 8-bit, X 8-bit; D = the record; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: found: C clear, Z set, A low = 0, X = the record's low
 * byte. End of the list: C and Z set, N clear, X = 0, A low = the last $18
 * byte read. Y = the last link read, low byte, or unchanged when none was
 * read. A high and the other flags unchanged.
 */
void abi_ActorListFindIdle(void)
{
    uint16_t x;
    uint8_t a;

    op_rep(P_X);
    cpu.p &= (uint8_t)~P_C;
    x = rd16(dp(ACTOR_NEXT));
    for (;;) {
        a = rd8(in_db((uint32_t)x + ACTOR_18));
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
        cpu.p = nz8(cpu.p, a);
        if (a == 0)
            break;
        cpu.y = rd16(in_db((uint32_t)x + ACTOR_NEXT));
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
