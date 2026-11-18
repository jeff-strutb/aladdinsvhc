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
#include "ops65.h"
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
/* The band variables by the addressing the original uses: absolute in
   bank DB ($7E), or absolute indexed. */
#define SB(a) ea_abs((uint16_t)(a))
#define SBX(a) ea_absx((uint16_t)(a))
#define SBY(a) ea_absy((uint16_t)(a))
#define SCROLL_X_AT ea_long(LAYER0 + LAYER_SCROLL_X)

static void drift_on(void)
{
    clc();                                      /* 24 bits: + $C0 */
    lda16(ea_long(BAND_DRIFT));
    adc16_v(0x00C0);
    sta16(ea_long(BAND_DRIFT));
    lda16(ea_long(BAND_DRIFT + 2));
    adc16_v(0);
    sta16(ea_long(BAND_DRIFT + 2));
}

/* Set 0: every band below the top at the scroll X halved once more per
   band upward; the top two at the drift plus the scroll X, halved and
   quartered. */
static void scrolls_halving(void)
{
    lda16(SB(SBAND_COUNT));                     /* 81C952 */
    cpu.a = asl16_r(cpu.a);
    tay16();
    lda16(SCROLL_X_AT);
    do {
        cpu.a = lsr16_r(cpu.a);
        sta16(SBY(SBAND_SCROLLS - 2));
        dey16();
        dey16();
    } while (!IF_Z);
    drift_on();
    clc();
    lda16(ea_long(BAND_DRIFT + 1));
    adc16_v(rd16(SCROLL_X_AT));
    cpu.a = lsr16_r(cpu.a);
    sta16(SB(SBAND_SCROLLS));
    cpu.a = lsr16_r(cpu.a);
    sta16(SB(SBAND_SCROLLS + 2));
}

/* Set 2: the drift, held back by as far as the play field moved right
   this frame, plus the scroll X for the top band, halved for each of the
   next three; then the scroll X halved, and three quarters of it. */
static void scrolls_drifting(void)
{
    drift_on();                                 /* 81C98C */
    sec();                                      /* held back by the move right */
    lda16(SCROLL_X_AT);
    sbc16_v(rd16(ea_long(SCROLL_X_BEFORE)));
    if (IF_N)
        lda16_v(0);
    sta16(ea_long(SCRATCH));
    sec();
    lda16(ea_long(BAND_DRIFT + 1));
    sbc16_v(rd16(ea_long(SCRATCH)));
    sta16(ea_long(BAND_DRIFT + 1));
    clc();
    lda16(ea_long(BAND_DRIFT + 1));
    adc16_v(rd16(SCROLL_X_AT));
    sta16(SB(SBAND_SCROLLS));
    cpu.a = lsr16_r(cpu.a);
    sta16(SB(SBAND_SCROLLS + 2));
    cpu.a = lsr16_r(cpu.a);
    sta16(SB(SBAND_SCROLLS + 4));
    cpu.a = lsr16_r(cpu.a);
    sta16(SB(SBAND_SCROLLS + 6));
    lda16(SCROLL_X_AT);
    pha16();
    cpu.a = lsr16_r(cpu.a);
    sta16(SB(SBAND_SCROLLS + 8));
    cpu.a = lsr16_r(cpu.a);
    sta16(ea_long(SCRATCH));
    pla16();
    clc();
    adc16_v(rd16(ea_long(SCRATCH)));
    sta16(SB(SBAND_SCROLLS + 10));
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
    ldx16(SB(SBAND_SET));
    if (cpu.x == 0)
        scrolls_halving();
    else if (cpu.x == 2)
        scrolls_drifting();
    else
        jump_routine(0x810000u | rd16(0x810000u | (uint16_t)(0xC94Eu + cpu.x)));
}

/* BandTable from 81:C84B, with A = set: HDMA channel 4's table for the
   backdrop's bands, from the screen's top line down. Every read, write,
   push and pull where the original makes it, in its order: an interrupt
   taken inside the frame's step reads what this has written so far (the
   NMI loads channel 4's table address from HDMA_TABLE_AT). */
static void band_table(uint16_t set)
{
    lda16_v(set);
    sta16(ea_long(SBAND_SET));                  /* 81C84B */
    op_sep(0x20);
    phb();
    lda8_v(0x7E);
    pha8();
    plb();
    op_rep(0x20);
    lda16(ea_long(LAYER0 + LAYER_SCROLL_Y));
    sta16(ea_long(LAYER1 + LAYER_SCROLL_Y));
    clc();
    adc16_v(8);
    sta16(SB(SBAND_LINE));
    ldx16(SB(SBAND_SET));
    lda16(ea_longx(SBAND_SETS_COUNT));
    cpu.a = lsr16_r(cpu.a);
    sta16(SB(SBAND_COUNT));
    call_frame(0xC874, abi_BandScrolls);        /* 81C872 jsr */
    lda16(ea_long(SBAND_BUFFER));               /* the other table than last frame's */
    eor16_v(2);
    sta16(ea_long(SBAND_BUFFER));
    tax16();
    lda16(ea_longx(SBAND_TABLES));
    tay16();
    phy16();
    lda16_v(224);
    sta16(SB(SBAND_LINES_LEFT));
    ldx16(SB(SBAND_SET));
    lda16(ea_longx(SBAND_EDGES));
    sta16(SB(SBAND_EDGE));
    stz16(SB(SBAND_AT));
    for (;;) {
        ldx16(SB(SBAND_EDGE));                  /* 81C89C */
        lda16(SB(SBAND_LINE));
        cmp16_v(rd16(ea_longx(SBAND_EDGES)));
        if (IF_C) {
            cmp16_v(rd16(ea_longx(SBAND_EDGES + 2)));
            if (!IF_C) {
                /* the band the line is in: its lines to the next edge, at
                   most the lines left (one if none: no script meets that) */
                sec();
                lda16(ea_longx(SBAND_EDGES + 2));
                sbc16_v(rd16(SB(SBAND_LINE)));
                if (IF_Z)
                    lda16_v(1);
                cmp16_v(rd16(SB(SBAND_LINES_LEFT)));
                if (IF_C)
                    lda16(SB(SBAND_LINES_LEFT));
                sta16(SB(SBAND_LINES));
                ldx16(SB(SBAND_AT));
                cmp16_v(128);
                if (!IF_C) {
                    sta16(SBY(3));              /* an entry: lines, scroll */
                    lda16(SBX(SBAND_SCROLLS));
                    sta16(SBY(4));
                    clc();
                    tya16();
                    adc16_v(3);
                    tay16();
                } else {
                    sbc16_v(127);               /* past 127 lines: two entries */
                    sta16(SBY(3));
                    lda16_v(127);
                    sta16(SBY(6));
                    lda16(SBX(SBAND_SCROLLS));
                    sta16(SBY(4));
                    sta16(SBY(7));
                    clc();
                    tya16();
                    adc16_v(6);
                    tay16();
                }
                ldx16(SB(SBAND_EDGE));
                lda16(ea_longx(SBAND_EDGES + 2));
                sta16(SB(SBAND_LINE));
                clc();
                lda16(SB(SBAND_LINES_LEFT));
                sbc16_v(rd16(SB(SBAND_LINES)));
                if (!IF_C)
                    break;                      /* the screen is full */
                cpu.a = inc16_r(cpu.a);
                sta16(SB(SBAND_LINES_LEFT));
            }
        }
        MEM16(inc, SB(SBAND_EDGE));
        MEM16(inc, SB(SBAND_EDGE));
        MEM16(inc, SB(SBAND_AT));
        MEM16(inc, SB(SBAND_AT));
        MEM16(dec, SB(SBAND_COUNT));
        if (IF_Z)
            break;
    }
    lda16_v(0);                                 /* the end mark */
    sta16(SBY(3));
    ply16();
    lda16_v(1);                                 /* the head: one line at the top band's scroll */
    sta16(SBY(0));
    lda16(SBY(4));
    sta16(SBY(1));
    sta16(ea_long(BG2_X));
    lda16(ea_long(LAYER0 + LAYER_SCROLL_X));
    cpu.a = lsr16_r(cpu.a);
    sta16(ea_long(LAYER1 + LAYER_SCROLL_X));
    plb();
    sty16(ea_abs((uint16_t)HDMA_TABLE_AT));
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
