/*
 * actor_setup.c: giving actors their starting values: from a placement
 * entry, from tables by an index the actor holds, and down the list of
 * actors one owns.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "actor.h"
#include "wram.h"

/* In the bank DB names: placement entries of 5 bytes (a byte whose low 5
   bits go to ACTOR_07, then the x and y pixels in the low 14 bits of a
   word each); words and bytes by ACTOR_40 ($D6EB, $D70B); stream starts
   by ACTOR_15 (two words, 4 bytes an entry: $D71D, $D71F) and stream
   addresses by X ($D725). */
#define PLACEMENTS 0x8A7Bu
#define BY_40_WORDS 0xD6EBu
#define BY_40_BYTES 0xD70Bu
#define STREAM_STARTS 0xD71Du
#define STREAMS 0xD725u
#define PLACED_FLAGS 0x7EC600u     /* a byte per placement: bit 0 set once placed */

#define ACTOR_X_PIXEL (ACTOR_X + 1)
#define ACTOR_Y_PIXEL (ACTOR_Y + 1)

static uint32_t in_db(uint32_t addr)
{
    return (uint32_t)cpu.db << 16 | addr;
}

static uint32_t dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

/*
 * @routine 84:8D41 ActorListStart
 * WHAT IT DOES: Starts every actor in the list this actor owns: each gets
 * the pair X, Y (in its bytes at $03-$04 and at $35-$36), state 1, its
 * counters at $18, $48, $49 and $4C cleared, and at $44 a value that steps
 * by A down the list from A.
 * HOW: The inputs go to scratch bytes; with 16-bit X and Y the walk
 * follows each record's link from this actor's own, starting from the
 * first linked record whatever it is, until a link of 0.
 * Entry: M 8-bit, X 8-bit; A = the first value and the step; D = the
 * actor; DB a bank whose $0000-$1FFF is work RAM. Exit: A low = the value
 * after the last; A high = D's high byte; X = Y = 0; N clear, Z set; V
 * and C from the last addition; M and X 8-bit. The other flags unchanged.
 */
void abi_ActorListStart(void)
{
    uint8_t a = (uint8_t)cpu.a;
    uint16_t x;

    wr8(SCRATCH + 8, (uint8_t)cpu.x);
    wr8(SCRATCH + 9, (uint8_t)cpu.y);
    wr8(SCRATCH + 10, a);
    wr8(SCRATCH, a);
    op_rep(P_M | P_X);
    cpu.a = cpu.d;
    x = rd16(in_db((uint32_t)cpu.d + ACTOR_NEXT));
    op_sep(P_M);
    do {
        a = rd8(SCRATCH + 8);
        wr8(in_db((uint32_t)x + ACTOR_03), a);
        wr8(in_db((uint32_t)x + ACTOR_35), a);
        a = rd8(SCRATCH + 9);
        wr8(in_db((uint32_t)x + ACTOR_04), a);
        wr8(in_db((uint32_t)x + ACTOR_36), a);
        wr8(in_db((uint32_t)x + ACTOR_18), 0);
        wr8(in_db((uint32_t)x + ACTOR_4C), 0);
        wr8(in_db((uint32_t)x + ACTOR_48), 0);
        wr8(in_db((uint32_t)x + ACTOR_49), 0);
        wr8(in_db((uint32_t)x + ACTOR_01), 1);
        a = rd8(SCRATCH);
        wr8(in_db((uint32_t)x + ACTOR_44), a);
        cpu.p &= (uint8_t)~P_C;
        a = op_adc8(a, rd8(SCRATCH + 10), &cpu.p);
        wr8(SCRATCH, a);
        x = rd16(in_db((uint32_t)x + ACTOR_NEXT));
        cpu.p = nz16(cpu.p, x);
    } while (x != 0);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.x = 0;
    cpu.y = 0;
    op_sep(P_X);
}

/*
 * @routine 84:8D8F ActorStartBy40
 * WHAT IT DOES: Starts the actor from its index at $40: bytes $08 and $09
 * to 2 and $A0, its word at $42 from a word table less $0100, its byte at
 * $41 from a byte table, and its bytes at $46-$49, $4C and $18 cleared.
 * HOW: The index picks a word entry (doubled) and a byte entry, in the
 * bank DB names.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB the tables' bank. Exit: A
 * low = the byte entry, N and Z from it; A high = the word's high byte
 * less 1 (with the borrow); V and C from the subtraction; X = twice the
 * index, Y = the index. The other flags unchanged.
 */
void abi_ActorStartBy40(void)
{
    uint8_t i, a;

    wr8(dp(ACTOR_08), 0x02);
    wr8(dp(ACTOR_09), 0xA0);
    i = rd8(dp(ACTOR_40));
    cpu.y = i;
    cpu.x = (uint8_t)(i << 1);
    op_rep(P_M);
    cpu.a = rd16(in_db(BY_40_WORDS + (uint8_t)cpu.x));
    op_sub16(0x0100);
    wr16(dp(ACTOR_42), cpu.a);
    op_sep(P_M);
    a = rd8(in_db(BY_40_BYTES + i));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = nz8(cpu.p, a);
    wr8(dp(ACTOR_41), a);
    wr8(dp(ACTOR_46), 0);
    wr8(dp(ACTOR_47), 0);
    wr8(dp(ACTOR_48), 0);
    wr8(dp(ACTOR_49), 0);
    wr8(dp(ACTOR_4C), 0);
    wr8(dp(ACTOR_18), 0);
}

/*
 * @routine 84:8DBB ActorStreamStart
 * WHAT IT DOES: Starts the actor on byte stream X: its horizontal speed
 * and the word at $1C from the entry its byte at $15 picks, then from the
 * stream's head a byte for $45, $46 and $47, a count less one for $4D, and
 * a word for $48 (EORed with the speed's low word, plus the $1C word);
 * the stream goes on from the byte after, and the counter at $4C steps.
 * HOW: The entry is 4 bytes; the stream's address comes from a word table
 * by X, read with 16-bit Y, in the bank DB names.
 * Entry: M 8-bit, X 8-bit; X = the stream's offset; D = the actor; DB the
 * tables' and streams' bank. Exit: A = the $48 word; M 8-bit; N and Z
 * from the counter's new value; V and C from the addition; Y = the
 * stream position's low byte. X and the other flags unchanged.
 */
void abi_ActorStreamStart(void)
{
    uint8_t i = (uint8_t)(rd8(dp(ACTOR_15)) << 2), a;
    uint16_t y;

    cpu.y = i;
    op_rep(P_M);
    wr16(dp(ACTOR_SPEED_X), rd16(in_db(STREAM_STARTS + i)));
    wr16(dp(ACTOR_ACCEL_X), rd16(in_db(STREAM_STARTS + 2 + i)));
    op_sep(P_M);
    op_rep(P_X);
    y = rd16(in_db(STREAMS + (uint8_t)cpu.x));
    a = rd8(in_db(y));
    wr8(dp(ACTOR_45), a);
    wr8(dp(ACTOR_46), a);
    wr8(dp(ACTOR_47), a);
    y++;
    a = (uint8_t)(rd8(in_db(y)) - 1);
    wr8(dp(ACTOR_4D), a);
    y++;
    op_rep(P_M);
    cpu.a = rd16(in_db(y));
    cpu.a ^= rd16(dp(ACTOR_SPEED_X));
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(cpu.a, rd16(dp(ACTOR_ACCEL_X)), &cpu.p);
    wr16(dp(ACTOR_48), cpu.a);
    op_sep(P_M);
    y = (uint16_t)(y + 2);
    wr16(dp(ACTOR_STREAM), y);
    cpu.y = y;
    op_sep(P_X);
    a = (uint8_t)(rd8_rmw(dp(ACTOR_4C)) + 1);
    wr8(dp(ACTOR_4C), a);
    cpu.p = nz8(cpu.p, a);
}

/*
 * @routine 82:A906 ActorPlace
 * WHAT IT DOES: Places the actor whose record X points at from placement
 * entry Y: the entry's low 5 bits to its byte at $07, its x and y pixels,
 * state $0C; marks the placement as placed and keeps its index (the
 * caller's direct-page word at $12) in the actor's byte at $34.
 * HOW: The pixels are the low 14 bits of the entry's words. The placed
 * flag is the entry's first byte with bit 0 set, at $7E:C600 plus the
 * index.
 * Entry: M 8-bit, X 16-bit; X = the actor, Y = the entry's offset; D the
 * caller's (its word at $12 the index); DB the entries' bank, whose
 * $0000-$1FFF is work RAM. Exit: A low = the index's low byte; A high =
 * the y word's high byte (14 bits); N and Z from X. X, Y and the other
 * flags unchanged.
 */
void abi_ActorPlace(void)
{
    uint16_t x = cpu.x, y = cpu.y, i;
    uint8_t first = rd8(in_db(PLACEMENTS + (uint32_t)y));

    wr8(in_db((uint32_t)x + ACTOR_07), (uint8_t)(first & 0x1F));
    op_rep(P_M);
    wr16(in_db((uint32_t)x + ACTOR_X_PIXEL), (uint16_t)(rd16(in_db(PLACEMENTS + 1 + (uint32_t)y)) & 0x3FFF));
    cpu.a = (uint16_t)(rd16(in_db(PLACEMENTS + 3 + (uint32_t)y)) & 0x3FFF);
    wr16(in_db((uint32_t)x + ACTOR_Y_PIXEL), cpu.a);
    op_sep(P_M);
    wr8(in_db((uint32_t)x + ACTOR_STATE), 0x0C);
    i = rd16(dp(ACTOR_12));
    wr8(PLACED_FLAGS + i, (uint8_t)(first | 0x01));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | (uint8_t)i);
    cpu.p = nz16(cpu.p, x);
    wr8(in_db((uint32_t)x + ACTOR_34), (uint8_t)i);
}
