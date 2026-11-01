/*
 * placement_index.c: the indexes a level's actor placements get when it
 * starts: how many fall in each 32-pixel band and where each band's run
 * begins, so the placements near the camera are found without a search.
 *
 * The placements are 6-byte entries after a count byte, in the bank DB
 * names; the tables are 256 bytes each from $7E:C100.
 */
#include "machine.h"
#include "cpu_ops.h"

void abi_ShiftRight5(void);

#define ENTRY_WORD_2 0x8A7Du       /* the word at byte 2 of entry Y */
#define ENTRY_WORD_4 0x8A7Fu       /* the word at byte 4 of entry Y */
#define BAND_COUNT 0x7EC100u       /* per band: placements in it (or per entry: its band) */
#define BAND_FIRST 0x7EC200u       /* per band: the first placement's number */
#define BAND_RUNS 0x7EC300u        /* per band: placements counted in the second pass */
#define BAND_STARTS 0x7EC400u      /* per band: where its run starts in BAND_ORDER */
#define BAND_ORDER 0x7EC500u       /* placement numbers, band by band */

/* Direct-page words the two passes use. */
#define DP_LEFT 0x0C
#define DP_LAST 0x0E
#define DP_NEXT 0x0F
#define DP_TOP 0x10
#define DP_BAND 0x11
#define DP_START 0x12

static uint32_t in_db(uint32_t addr)
{
    return (uint32_t)cpu.db << 16 | addr;
}

static uint32_t dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

static void set_a8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
}

/* REP #$21; TYA; ADC #6; TAY: on to the next entry, C clear first. */
static void next_entry(void)
{
    op_rep(P_M | P_C);
    cpu.a = cpu.y;
    cpu.a = op_adc16(cpu.a, 6, &cpu.p);
    cpu.y = cpu.a;
}

/* DEC of the direct-page word: N and Z from the result. */
static uint16_t dec_dp16(uint8_t off)
{
    uint16_t v = (uint16_t)(rd16_rmw(dp(off)) - 1);

    wr16_rmw(dp(off), v);
    cpu.p = nz16(cpu.p, v);
    return v;
}

/*
 * @routine 82:A984 PlacementsByColumn
 * WHAT IT DOES: Counts A placements from entry Y into their columns: each
 * entry's column (the word at its byte 2 over 32, 10 bits) gets one more
 * in the count table, and a column met for the first time since the last
 * gets the placement's number in the first-of-band table. Columns from
 * 255 up are not counted.
 * HOW: The direct page's word at $0C counts down the entries; its byte at
 * $0E is the last column met and $0F the placement number, stepped for
 * each placement counted. 8-bit A for the table steps.
 * Entry: M 16-bit, X 16-bit; A = the count, Y = the first entry's offset;
 * D a page whose $0C-$0F it may use ($0E and $0F set by the caller); DB
 * the entries' bank. Exit: A = Y = the offset past the last entry; X the
 * last column counted; N clear, Z set; V and C from the last step; M
 * 16-bit. The other flags unchanged.
 */
void abi_PlacementsByColumn(void)
{
    uint8_t a;

    wr16(dp(DP_LEFT), cpu.a);
    do {
        cpu.a = rd16(in_db(ENTRY_WORD_2 + (uint32_t)cpu.y));
        cpu.p = nz16(cpu.p, cpu.a);
        abi_ShiftRight5();
        op_and16(0x03FF);
        op_cmp16(0x00FF);
        if (!(cpu.p & P_C)) {
            cpu.x = cpu.a;
            op_sep(P_M);
            a = (uint8_t)(rd8(BAND_COUNT + cpu.x) + 1);
            wr8(BAND_COUNT + cpu.x, a);
            a = (uint8_t)cpu.x;
            set_a8(a);
            cpu.p = cmp8_flags(cpu.p, a, rd8(dp(DP_LAST)));
            if (!(cpu.p & P_Z)) {
                wr8(dp(DP_LAST), a);
                a = rd8(dp(DP_NEXT));
                set_a8(a);
                wr8(BAND_FIRST + cpu.x, a);
            }
            a = (uint8_t)(rd8_rmw(dp(DP_NEXT)) + 1);
            wr8(dp(DP_NEXT), a);
        }
        next_entry();
    } while (dec_dp16(DP_LEFT) != 0);
}

/*
 * @routine 82:A9BB PlacementsByRow
 * WHAT IT DOES: Indexes A placements from entry Y by row: each entry's row
 * (the word at its byte 4 over 32, 11 bits, kept only below 255) goes in
 * a per-entry table; then row by row the placements in it are listed,
 * from the last placement to the first, the row's count stepped for each,
 * and each row that has any gets its run's start in the list.
 * HOW: Two passes. The direct page's word at $0C counts down the
 * entries; $0E is the count, $10 the last entry's number, $11 the row
 * searched and $0F the list's length, $12 the last start written.
 * Entry: M 16-bit, X 16-bit; A = the count (1 up), Y = the first entry's
 * offset; D a page whose $0C-$12 it may use ($11 0 for the first row);
 * DB the entries' bank. Exit: A low = the row after the last searched, A
 * high as the first pass left it; X = Y = the list's length; N, Z and C
 * from comparing it with the count (Z set); V from the first pass; M and
 * X 16-bit. The other flags unchanged.
 */
void abi_PlacementsByRow(void)
{
    uint8_t a, x, y;
    uint16_t n = cpu.a;

    wr16(dp(DP_LEFT), n);
    wr16(dp(DP_LAST), n);
    cpu.a = (uint16_t)(n - 1);
    wr16(dp(DP_TOP), cpu.a);
    cpu.x = 0;
    do {
        cpu.a = rd16(in_db(ENTRY_WORD_4 + (uint32_t)cpu.y));
        cpu.p = nz16(cpu.p, cpu.a);
        abi_ShiftRight5();
        op_and16(0x07FF);
        op_cmp16(0x00FF);
        if (!(cpu.p & P_C)) {
            op_sep(P_M);
            wr8(BAND_COUNT + cpu.x, (uint8_t)cpu.a);
        }
        next_entry();
        cpu.x = (uint16_t)(cpu.x + 1);
    } while (dec_dp16(DP_LEFT) != 0);
    op_sep(P_M | P_X);
    set_a8(0);
    wr8(dp(DP_START), 0);
    do {
        a = (uint8_t)cpu.a;
        x = rd8(dp(DP_TOP));
        do {
            cpu.p = cmp8_flags(cpu.p, a, rd8(BAND_COUNT + x));
            if (cpu.p & P_Z) {
                y = x;
                wr8(BAND_RUNS + a, (uint8_t)(rd8(BAND_RUNS + a) + 1));
                wr8(BAND_ORDER + rd8(dp(DP_NEXT)), y);
                wr8(dp(DP_NEXT), (uint8_t)(rd8_rmw(dp(DP_NEXT)) + 1));
                x = y;
            }
            x--;
            cpu.p = cmp8_flags(cpu.p, x, 0xFF);
        } while (!(cpu.p & P_Z));
        cpu.x = a;
        y = rd8(dp(DP_NEXT));
        cpu.y = y;
        cpu.p = cmp8_flags(cpu.p, y, rd8(dp(DP_START)));
        if (!(cpu.p & P_Z)) {
            a = rd8(dp(DP_START));
            wr8(dp(DP_START), y);
            wr8(BAND_STARTS + (uint8_t)cpu.x, a);
        }
        a = (uint8_t)(rd8_rmw(dp(DP_BAND)) + 1);
        wr8(dp(DP_BAND), a);
        a = rd8(dp(DP_BAND));
        set_a8(a);
        x = rd8(dp(DP_NEXT));
        cpu.x = x;
        cpu.p = cmp8_flags(cpu.p, x, rd8(dp(DP_LAST)));
    } while (!(cpu.p & P_Z));
    op_rep(P_M | P_X);
}
