/*
 * parallax.c: the backdrop's scroll, from the play field's.
 *
 * Each frame the scroll update ends in one of a table of handlers, chosen
 * by the level ($083F), that set the backdrop layer's scroll from the play
 * field's: at the same speed, at half speed, or not at all, so the
 * backdrop looks farther away.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "wram.h"
#include "wram_video.h"
#include "actor.h"

/*
 * @routine 81:C7A5 ParallaxNone
 * WHAT IT DOES: Leaves the backdrop where it is: for levels whose backdrop
 * does not scroll with the play field.
 * HOW: Returns at once (the shared return of ParallaxHalf).
 * Entry: any. Exit: everything unchanged.
 */
void abi_ParallaxNone(void)
{
}

/*
 * @routine 81:C797 ParallaxHalf
 * WHAT IT DOES: Scrolls the backdrop at half the play field's speed, both
 * ways, so it looks far away.
 * HOW: The backdrop's scroll X and Y are the play field's, halved.
 * Entry: M 16-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: A = the
 * play field's scroll Y halved; N clear, Z from it, C the bit shifted out;
 * X, Y and the other flags unchanged.
 */
void abi_ParallaxHalf(void)
{
    op_lda16(LAYER0 + LAYER_SCROLL_X);
    op_lsr16();
    wr16(LAYER1 + LAYER_SCROLL_X, cpu.a);
    op_lda16(LAYER0 + LAYER_SCROLL_Y);
    op_lsr16();
    wr16(LAYER1 + LAYER_SCROLL_Y, cpu.a);
    abi_ParallaxNone();
}

/*
 * @routine 81:C7A6 ParallaxFull
 * WHAT IT DOES: Scrolls the backdrop with the play field, at its speed.
 * HOW: The backdrop's scroll X and Y are the play field's.
 * Entry: M 16-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: A = the
 * play field's scroll Y; N and Z from it; X, Y and the other flags
 * unchanged.
 */
void abi_ParallaxFull(void)
{
    op_lda16(LAYER0 + LAYER_SCROLL_X);
    wr16(LAYER1 + LAYER_SCROLL_X, cpu.a);
    op_lda16(LAYER0 + LAYER_SCROLL_Y);
    wr16(LAYER1 + LAYER_SCROLL_Y, cpu.a);
}

/*
 * @routine 81:C7D4 ParallaxHalfX
 * WHAT IT DOES: Scrolls the backdrop at half the play field's speed across
 * and at its full speed up and down.
 * HOW: The backdrop's scroll X is the play field's halved, its scroll Y
 * the play field's.
 * Entry: M 16-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: A = the
 * play field's scroll Y; N and Z from it; C the bit shifted out of scroll
 * X; X, Y and the other flags unchanged.
 */
void abi_ParallaxHalfX(void)
{
    op_lda16(LAYER0 + LAYER_SCROLL_X);
    op_lsr16();
    wr16(LAYER1 + LAYER_SCROLL_X, cpu.a);
    op_lda16(LAYER0 + LAYER_SCROLL_Y);
    wr16(LAYER1 + LAYER_SCROLL_Y, cpu.a);
}

/*
 * @routine 81:C7E7 ParallaxDrift
 * WHAT IT DOES: Scrolls a backdrop of 11 horizontal bands, each drifting
 * at its own speed, one band (the tenth) fixed to the play field.
 * HOW: Each band's position steps by its speed; the tenth's becomes the
 * play field's scroll X. The positions go into the bands' HDMA table, and
 * the first band's into the backdrop's scroll.
 * Entry: M 16-bit, X 16-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: M 16-bit; A = the first band's position; X = 44, Y = 33; $000C =
 * 0; N and Z from DB; DB unchanged; C from the last step's high word.
 */
void abi_ParallaxDrift(void)
{
    uint16_t x, y, n;
    uint8_t db = cpu.db;

    cpu.x = 0;
    cpu.p = nz16(cpu.p, 0);
    cpu.y = 0x000B;
    cpu.p = nz16(cpu.p, 0x000B);
    do {
        x = cpu.x;
        op_lda16(BAND_POS + x);
        op_add16(rd16(BAND_SPEED + x));
        wr16(BAND_POS + x, cpu.a);
        op_lda16(BAND_POS + 2 + x);
        cpu.a = op_adc16(cpu.a, 0, &cpu.p);
        wr16(BAND_POS + 2 + x, cpu.a);
        cpu.x = (uint16_t)(x + 4);
        cpu.p = nz16(cpu.p, cpu.x);
        cpu.y--;
        cpu.p = nz16(cpu.p, cpu.y);
    } while (cpu.y != 0);
    op_lda16(LAYER0 + LAYER_SCROLL_X);
    wr16(BAND_POS + 9 * 4 + 1, cpu.a);
    cpu.x = 0;
    cpu.p = nz16(cpu.p, 0);
    cpu.y = 0;
    cpu.p = nz16(cpu.p, 0);
    op_sep(0x20);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x7E);
    cpu.p = nz8(cpu.p, 0x7E);
    cpu.db = 0x7E;
    op_rep(0x20);
    op_lda16_imm(0x000B);
    wr16(SCRATCH, cpu.a);
    do {
        x = cpu.x;
        y = cpu.y;
        op_lda16(BAND_POS + 1 + x);
        wr16(BAND_HDMA + 1 + y, cpu.a);
        cpu.x = (uint16_t)(x + 4);
        cpu.p = nz16(cpu.p, cpu.x);
        cpu.y = (uint16_t)(y + 3);
        cpu.p = nz16(cpu.p, cpu.y);
        n = (uint16_t)(rd16(SCRATCH) - 1);
        cpu.a = n;
        cpu.p = nz16(cpu.p, n);
        wr16(SCRATCH, n);
    } while (n != 0);
    op_lda16(BAND_POS + 1);
    wr16(0x7E0000u | BG2_X, cpu.a);     /* DB $7E */
    cpu.db = db;
    cpu.p = nz8(cpu.p, db);
}

/* The handlers by PARALLAX_KIND (words, this bank), and the backdrop
   bands' sets: per set its bands' count (times 2, words), where its edges
   start in SBAND_EDGES (words); SBAND_EDGES (its first two words those
   starts) lists each set's lines (y in the level) where a band starts,
   closed by $7FFF; the two HDMA tables
   BandTable writes in turn (words). All in bank $80 but the first. */
#define PARALLAX_HANDLERS 0x81C781u
#define SBAND_SETS_COUNT 0x80C602u
#define SBAND_EDGES 0x80C606u
#define SBAND_TABLES 0x80C628u
#define PARALLAX_KIND 0x083Fu       /* the level's handler, times 2 (word) */
#define BACKDROP_STAGE 0x0841u      /* PlayField3Stages's stage, times 2 (word) */
#define PLAYER_X_AT (PLAYER + ACTOR_X + 1) /* the player's x, whole pixels (word) */
#define STAGE1_X 0x0F10u            /* the player's x where stage 0 can end */
#define STAGE1_SCROLL 0x0F80u       /* the scroll X past which it ends at once */
#define WIDE_X 0x1000u              /* past this scroll X the backdrop moves at half speed */

void abi_BackdropFollowOffsetY(void);
void abi_PlayerOnFirmGround(void);

/*
 * @routine 81:C772 ParallaxUpdate
 * WHAT IT DOES: Sets the backdrop's scroll from the play field's in the
 * way the level asks: runs the level's parallax handler.
 * HOW: JSR through PARALLAX_HANDLERS by PARALLAX_KIND; then 8-bit
 * registers.
 * Entry: M 16-bit, X 16-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: as the handler leaves A and the flags; M and X 8-bit (X and Y's
 * high bytes cleared).
 */
void abi_ParallaxUpdate(void)
{
    cpu.x = rd16(dba(PARALLAX_KIND));
    nz16_of(cpu.x);
    call_routine(0x810000u | rd16(0x810000u | (uint16_t)(PARALLAX_HANDLERS + cpu.x)), 0);
    op_sep(0x30);
}

/*
 * @routine 81:C77B ParallaxUpdateLong
 * WHAT IT DOES: ParallaxUpdate for callers in other banks.
 * HOW: 16-bit registers, ParallaxUpdate, RTL.
 * Entry: DB a bank whose $0000-$1FFF is work RAM; called by JSL.
 * Exit: as ParallaxUpdate.
 */
void abi_ParallaxUpdateLong(void)
{
    op_rep(0x30);
    abi_ParallaxUpdate();
}

/* One band set's scrolls, SBAND_SCROLLS by band, top first (bank DB = $7E).
   Both add DRIFT_STEP to the drift, a 24-bit position (fraction byte
   first) the top bands carry on their own. */
static void drift_on(void)
{
    flag_set(P_C, 0);
    wr16(BAND_DRIFT, op_adc16(rd16(BAND_DRIFT), 0x00C0, &cpu.p));
    wr16(BAND_DRIFT + 2, op_adc16(rd16(BAND_DRIFT + 2), 0, &cpu.p));
}

/* Set 0: every band below the top at the scroll X halved once more per
   band upward; the top two at the drift plus the scroll X, halved and
   quartered. */
static void scrolls_halving(void)
{
    uint16_t sx = rd16(LAYER0 + LAYER_SCROLL_X), y, v;

    y = (uint16_t)(rd16(dba((uint16_t)SBAND_COUNT)) << 1);
    v = sx;
    do {
        flag_set(P_C, v & 1);
        v >>= 1;
        wr16(dba((uint16_t)(SBAND_SCROLLS - 2 + y)), v);
        y -= 2;
    } while (y != 0);
    cpu.y = 0;
    drift_on();
    flag_set(P_C, 0);
    v = op_adc16(rd16(BAND_DRIFT + 1), sx, &cpu.p);
    flag_set(P_C, v & 1);
    v >>= 1;
    wr16(dba((uint16_t)SBAND_SCROLLS), v);
    flag_set(P_C, v & 1);
    v >>= 1;
    wr16(dba((uint16_t)(SBAND_SCROLLS + 2)), v);
    cpu.a = v;
    nz16_of(v);
}

/* Set 2: the drift, held back by as far as the play field moved right
   this frame, plus the scroll X for the top band, halved for each of the
   next three; then the scroll X halved, and three quarters of it. */
static void scrolls_drifting(void)
{
    uint16_t sx = rd16(LAYER0 + LAYER_SCROLL_X), d, v;

    drift_on();
    flag_set(P_C, 1);
    d = op_sbc16(sx, rd16(SCROLL_X_BEFORE), &cpu.p);
    if (d & 0x8000)
        d = 0;
    wr16(SCRATCH, d);
    flag_set(P_C, 1);
    wr16(BAND_DRIFT + 1, op_sbc16(rd16(BAND_DRIFT + 1), rd16(SCRATCH), &cpu.p));
    flag_set(P_C, 0);
    v = op_adc16(rd16(BAND_DRIFT + 1), sx, &cpu.p);
    wr16(dba((uint16_t)SBAND_SCROLLS), v);
    v >>= 1;
    wr16(dba((uint16_t)(SBAND_SCROLLS + 2)), v);
    v >>= 1;
    wr16(dba((uint16_t)(SBAND_SCROLLS + 4)), v);
    v >>= 1;
    wr16(dba((uint16_t)(SBAND_SCROLLS + 6)), v);
    wr16(dba((uint16_t)(SBAND_SCROLLS + 8)), (uint16_t)(sx >> 1));
    wr16(SCRATCH, (uint16_t)(sx >> 2));
    flag_set(P_C, 0);
    cpu.a = op_adc16(sx, rd16(SCRATCH), &cpu.p);
    wr16(dba((uint16_t)(SBAND_SCROLLS + 10)), cpu.a);
}

/*
 * @routine 81:C948 BandScrolls
 * WHAT IT DOES: Works out each backdrop band's scroll X for this frame, by
 * the band set: set 0 halves the scroll once more per band upward, with
 * two drifting bands of cloud on top; set 2 has four drifting bands that
 * the play field's moves right hold back, over two at half and three
 * quarters of its speed.
 * HOW: JMP through its own table by SBAND_SET. Both add $C0 to BAND_DRIFT (24
 * bits, from its fraction byte). Set 0: SBAND_SCROLLS from the bottom band
 * up, the scroll X shifted right once more each; the top two, BAND_DRIFT's
 * pixels plus the scroll X, shifted right once and twice. Set 2: BAND_DRIFT's
 * pixels less the scroll X's move right this frame (none if it moved
 * left), plus the scroll X, shifted right 0-3 times for the top four;
 * then the scroll X halved, and plus its quarter.
 * Entry: M 16-bit, X 16-bit; DB = $7E; SBAND_SET 0 or 2; SBAND_COUNT the
 * set's bands. Exit: set 0: A = the second band's scroll, N clear, Z from
 * it, C the bit shifted out, V from the drift's sum; X = 0, Y = 0. Set
 * 2: A = the last band's scroll; N, Z, C, V from its add; X = 2; Y as at
 * entry; SCRATCH = the scroll X's quarter.
 */
void abi_BandScrolls(void)
{
    cpu.x = rd16(dba((uint16_t)SBAND_SET));
    nz16_of(cpu.x);
    if (cpu.x == 0)
        scrolls_halving();
    else if (cpu.x == 2)
        scrolls_drifting();
    else
        jump_routine(0x810000u | rd16(0x810000u | (uint16_t)(0xC94Eu + cpu.x)));
}

/* BandTable from 81:C84B, with set: HDMA channel 4's table for the
   backdrop's bands, from the screen's top line down. */
static void band_table(uint16_t set)
{
    uint8_t db = cpu.db;
    uint16_t sy, at, base, idx, k, left, n;

    wr16(SBAND_SET, set);
    cpu.db = 0x7E;
    sy = rd16(LAYER0 + LAYER_SCROLL_Y);
    wr16(LAYER1 + LAYER_SCROLL_Y, sy);
    flag_set(P_C, 0);
    wr16(dba((uint16_t)SBAND_LINE), op_adc16(sy, 8, &cpu.p));
    cpu.x = rd16(dba((uint16_t)SBAND_SET));
    wr16(dba((uint16_t)SBAND_COUNT), (uint16_t)(rd16(SBAND_SETS_COUNT + cpu.x) >> 1));
    abi_BandScrolls();

    /* the other of the two tables than last frame's */
    cpu.x = rd16(SBAND_BUFFER) ^ 2;
    wr16(SBAND_BUFFER, cpu.x);
    base = rd16(SBAND_TABLES + cpu.x);
    at = base;
    left = 0x00E0;
    wr16(dba((uint16_t)SBAND_LINES_LEFT), left);
    wr16(dba((uint16_t)SBAND_EDGE), rd16(SBAND_EDGES + rd16(dba((uint16_t)SBAND_SET))));
    wr16(dba((uint16_t)SBAND_AT), 0);
    for (;;) {
        idx = rd16(dba((uint16_t)SBAND_EDGE));
        cpu.x = idx;
        if (rd16(dba((uint16_t)SBAND_LINE)) >= rd16(SBAND_EDGES + idx)
                && rd16(dba((uint16_t)SBAND_LINE)) < rd16(SBAND_EDGES + 2 + idx)) {
            /* the band the line is in: its lines to the next edge, at most
               the lines left (one if none: no script meets that) */
            flag_set(P_C, 1);
            n = op_sbc16(rd16(SBAND_EDGES + 2 + idx), rd16(dba((uint16_t)SBAND_LINE)), &cpu.p);
            if (n == 0)
                n = 1;
            if (n >= rd16(dba((uint16_t)SBAND_LINES_LEFT)))
                n = rd16(dba((uint16_t)SBAND_LINES_LEFT));
            wr16(dba((uint16_t)SBAND_LINES), n);
            k = rd16(dba((uint16_t)SBAND_AT));
            if (n < 0x80) {
                wr16(dba((uint16_t)(at + 3)), n);
                wr16(dba((uint16_t)(at + 4)), rd16(dba((uint16_t)(SBAND_SCROLLS + k))));
                flag_set(P_C, 0);
                at = op_adc16(at, 3, &cpu.p);
            } else {
                /* an entry holds 127 lines at most: two */
                flag_set(P_C, 1);
                wr16(dba((uint16_t)(at + 3)), op_sbc16(n, 0x007F, &cpu.p));
                wr16(dba((uint16_t)(at + 6)), 0x007F);
                wr16(dba((uint16_t)(at + 4)), rd16(dba((uint16_t)(SBAND_SCROLLS + k))));
                wr16(dba((uint16_t)(at + 7)), rd16(dba((uint16_t)(SBAND_SCROLLS + k))));
                flag_set(P_C, 0);
                at = op_adc16(at, 6, &cpu.p);
            }
            cpu.x = idx;
            wr16(dba((uint16_t)SBAND_LINE), rd16(SBAND_EDGES + 2 + idx));
            flag_set(P_C, 0);
            left = op_sbc16(rd16(dba((uint16_t)SBAND_LINES_LEFT)), rd16(dba((uint16_t)SBAND_LINES)), &cpu.p);
            if (!(cpu.p & P_C))
                break;              /* the screen is full */
            wr16(dba((uint16_t)SBAND_LINES_LEFT), (uint16_t)(left + 1));
        }
        wr16(dba((uint16_t)SBAND_EDGE), (uint16_t)(idx + 2));
        wr16(dba((uint16_t)SBAND_AT), (uint16_t)(rd16(dba((uint16_t)SBAND_AT)) + 2));
        if (dec16_at(dba((uint16_t)SBAND_COUNT)) == 0)
            break;
    }
    /* the end mark; ahead of the bands, one line at the top band's
       scroll, which also goes to BG2's */
    wr16(dba((uint16_t)(at + 3)), 0);
    cpu.y = base;
    wr16(dba(base), 0x0001);
    n = rd16(dba((uint16_t)(base + 4)));
    wr16(dba((uint16_t)(base + 1)), n);
    wr16(BG2_X, n);
    n = rd16(LAYER0 + LAYER_SCROLL_X);
    flag_set(P_C, n & 1);
    cpu.a = (uint16_t)(n >> 1);
    wr16(LAYER1 + LAYER_SCROLL_X, cpu.a);
    cpu.db = db;
    nz8_of(db);
    wr16(dba(HDMA_TABLE_AT), base);
}

/*
 * @routine 81:C848 BandTable
 * WHAT IT DOES: Scrolls a backdrop of horizontal bands, each at its own
 * speed (band set 0): writes the bands' HDMA table for this frame.
 * HOW: SBAND_SET = the set; the backdrop's scroll Y the play field's;
 * SBAND_LINE = the scroll Y + 8, SBAND_COUNT the set's bands; BandScrolls.
 * Then in the other of SBAND_TABLES than last frame's (SBAND_BUFFER), after
 * a 3-byte head: for each band the screen's line reaches, an entry of its
 * lines (to its lower edge, at most the 224 left; two entries past 127)
 * and its scroll; a 0 to end. The head: one line at the top band's
 * scroll, which BG2's scroll X takes too. The backdrop's scroll X is the
 * play field's halved; HDMA_TABLE_AT (channel 4) the table.
 * Entry: M 16-bit, X 16-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: A = the backdrop's scroll X, C the bit shifted out; N and Z from
 * DB (PLB); V from the last add or subtract; X = the last band's edge
 * index; Y = the table; DB unchanged.
 */
void abi_BandTable(void)
{
    band_table(0);
}

/*
 * @routine 81:C7E2 BandTableSet2
 * WHAT IT DOES: BandTable for band set 2: four drifting bands over two.
 * HOW: Into BandTable past its first load, with A = 2.
 * Entry: as BandTable. Exit: as BandTable.
 */
void abi_BandTableSet2(void)
{
    band_table(2);
}

/*
 * @routine 81:CA55 BackdropWideX
 * WHAT IT DOES: PlayField3Stages's x for the backdrop in its later
 * stages: the play field's scroll X up to WIDE_X, past it half as fast.
 * HOW: The play field's scroll X, compared with WIDE_X; at or past it,
 * what is past it halved and WIDE_X added back. The result is the
 * backdrop's scroll X.
 * Entry: M 16-bit, X 16-bit; DB = $80 (work RAM); called by JSR from
 * PlayField3Stages. Exit: A = the backdrop's scroll X; below WIDE_X, C
 * clear and N, Z from the compare; else C, N, V, Z from the add.
 */
void abi_BackdropWideX(void)
{
    uint16_t sx = rd16(dba(LAYER0 + LAYER_SCROLL_X));

    flag_set(P_C, 1);
    cmp16_of(sx, WIDE_X);
    if (sx >= WIDE_X) {
        cpu.a = op_sbc16(sx, WIDE_X, &cpu.p);
        flag_set(P_C, cpu.a & 1);
        cpu.a >>= 1;
        flag_set(P_C, 0);
        cpu.a = op_adc16(cpu.a, WIDE_X, &cpu.p);
    } else {
        cpu.a = sx;
    }
    wr16(dba(LAYER1 + LAYER_SCROLL_X), cpu.a);
}

/*
 * @routine 81:C9F2 PlayField3Stages
 * WHAT IT DOES: The backdrop of a level that changes as the player goes:
 * stage 0 follows the play field with an offset down until the player
 * reaches its end (x $0F10) on firm ground, or the scroll passes $0F80;
 * stage 1 eases the backdrop's y back to 0, at most 8 a frame; stage 2
 * scrolls it across with the play field, at half speed past $1000.
 * HOW: JMP through its own table by BACKDROP_STAGE. Stage 0:
 * BackdropFollowOffsetY; with the player's x at STAGE1_X or more, the
 * scroll X at STAGE1_SCROLL or more, or PlayerOnFirmGround (8-bit) C
 * set, BACKDROP_STAGE + 2. Stage 1: with the backdrop's scroll Y 0, the
 * next stage; else it less itself clamped to -8..8, then the x as stage
 * 2. Stage 2: the backdrop's scroll X the play field's, past WIDE_X half
 * of what is past it. Only stage 0 up to its x test is run by a script.
 * Entry: M 16-bit, X 16-bit; DB = $80 (work RAM). Exit: M 16-bit, X
 * 16-bit; A and flags as the last step left them.
 */
void abi_PlayField3Stages(void)
{
    uint16_t v, d;

    cpu.x = rd16(dba(BACKDROP_STAGE));
    nz16_of(cpu.x);
    switch (cpu.x) {
    case 0:
        abi_BackdropFollowOffsetY();
        v = rd16(dba((uint16_t)PLAYER_X_AT));
        cpu.a = v;
        cmp16_of(v, STAGE1_X);
        if (v < STAGE1_X)
            return;
        v = rd16(dba(LAYER0 + LAYER_SCROLL_X));
        cpu.a = v;
        cmp16_of(v, STAGE1_SCROLL);
        if (v < STAGE1_SCROLL) {
            op_sep(0x30);
            abi_PlayerOnFirmGround();
            op_rep(0x30);
            if (!(cpu.p & P_C))
                return;
        }
        (void)inc16_at(dba(BACKDROP_STAGE));
        (void)inc16_at(dba(BACKDROP_STAGE));
        return;
    case 2:
        v = rd16(dba(LAYER1 + LAYER_SCROLL_Y));
        cpu.a = v;
        nz16_of(v);
        if (v == 0) {
            (void)inc16_at(dba(BACKDROP_STAGE));
            (void)inc16_at(dba(BACKDROP_STAGE));
            return;
        }
        if (!(v & 0x8000)) {
            cmp16_of(v, 8);
            d = v < 8 ? v : 8;
        } else {
            cmp16_of(v, 0xFFF8);
            d = v >= 0xFFF8 ? v : 0xFFF8;
        }
        d = (uint16_t)((d ^ 0xFFFF) + 1);
        flag_set(P_C, 0);
        wr16(dba(LAYER1 + LAYER_SCROLL_Y), op_adc16(d, rd16(dba(LAYER1 + LAYER_SCROLL_Y)), &cpu.p));
        abi_BackdropWideX();
        return;
    case 4:
        abi_BackdropWideX();
        return;
    default:
        jump_routine(0x810000u | rd16(0x810000u | (uint16_t)(0xC9F8u + cpu.x)));
    }
}
