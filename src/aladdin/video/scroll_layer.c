/*
 * scroll_layer.c: keeping each scroll layer's tilemap ahead of its scroll.
 *
 * A layer's tilemap in VRAM holds a little more than the screen. As the
 * layer scrolls, the column or row of tiles about to come into view is
 * drawn from the level's map: a map of blocks at $7F:F000, each block 16
 * by 16 metatiles, each metatile 2 by 2 tiles. Each layer record keeps four
 * edges (left, right, top, bottom), each a place in the tilemap and the
 * same place in the map, and steps them a tile at a time as it draws.
 * The direct page is the layer's record ($1738 or $188E) throughout.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"
#include "wram_video.h"

/* The step, in pixels, the drawn position makes for a column or row
   queued: +8 (right or down), -8 (left or up) */
#define LAYER_DRAW_STEP 0x80C666u

/* The level table: per level a word offset (from here) to its header */
#define LEVEL_TABLE 0x80E460u

/* Per layer, 7 bytes: the map base's high byte, the tilemap base's high
   byte, the tilemap mask (word), the screen flip's high byte, the tiles a
   column and a row draw */
#define LAYER_SETUP 0x80C648u

/* Per edge, where it starts from the scroll: X, Y (words): left, right,
   top, bottom */
#define EDGE_PLACE 0x80C656u

/* Per side, the edge a column (right, left) or a row (bottom, top) is
   drawn at */
#define SIDE_EDGE 0x80C66Au

/* Per side, that edge's bit in the edge mask: columns (right, left), then
   rows (bottom, top) */
#define SIDE_EDGE_BIT 0x80C66Eu

/* A layer record's size: the backdrop's follows the play field's */
#define LAYER_SIZE 0x0156u

/* The level's block map */
#define BLOCK_MAP 0x7FF000u


/* An edge: 10 bytes at a layer record's start (left 0, right 10, top 20,
   bottom 30) */
enum {
    EDGE_TILEMAP = 0,   /* tilemap word: column 0-31, row bits 5-9, screen bit 10 */
    EDGE_MAP = 2,       /* the block's offset in the block map (2 across, $40 down) */
    EDGE_BLOCK = 4,     /* the metatile's offset in the block (2 across, $20 down) */
    EDGE_HALF = 8,      /* the tile in the metatile: 2 the right half, 4 the bottom */
};

static uint16_t dp(uint16_t off)
{
    return (uint16_t)(cpu.d + off);
}

static void set_a8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* BIT dp with an 8-bit accumulator: N and V from memory, Z from A AND it */
static int bit8_dp(uint16_t off)
{
    uint8_t m = rd8(dp(off));

    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_V | P_Z)) | (m & (P_N | P_V)) |
                      (((uint8_t)cpu.a & m) ? 0 : P_Z));
    return ((uint8_t)cpu.a & m) != 0;
}

/* BIT # (8 or 16-bit): Z only */
static int bit_imm(uint16_t m)
{
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((cpu.a & m) ? 0 : P_Z));
    return (cpu.a & m) != 0;
}

static void inc8_dp(uint16_t off)
{
    uint8_t v = (uint8_t)(rd8_rmw(dp(off)) + 1);

    wr8(dp(off), v);
    cpu.p = nz8(cpu.p, v);
}

static void dec8_dp(uint16_t off)
{
    uint8_t v = (uint8_t)(rd8_rmw(dp(off)) - 1);

    wr8(dp(off), v);
    cpu.p = nz8(cpu.p, v);
}

static void inc16_dp(uint16_t off)
{
    uint16_t v = (uint16_t)(rd16_rmw(dp(off)) + 1);

    wr16_rmw(dp(off), v);
    cpu.p = nz16(cpu.p, v);
}

static void dec16_dp(uint16_t off)
{
    uint16_t v = (uint16_t)(rd16_rmw(dp(off)) - 1);

    wr16_rmw(dp(off), v);
    cpu.p = nz16(cpu.p, v);
}

static void op_ror16(void)
{
    uint8_t c = (uint8_t)(cpu.a & 1);

    cpu.a = (uint16_t)(cpu.a >> 1 | (cpu.p & P_C) << 15);
    cpu.p = (uint8_t)(nz16((uint8_t)(cpu.p & ~P_C), cpu.a) | c);
}

static void op_ora16(uint16_t m)
{
    cpu.a |= m;
    cpu.p = nz16(cpu.p, cpu.a);
}

/* LDA dp, EOR #, STA dp, 8-bit */
static void flip8_dp(uint16_t off, uint8_t bits)
{
    set_a8(rd8(dp(off)));
    set_a8((uint8_t)cpu.a ^ bits);
    wr8(dp(off), (uint8_t)cpu.a);
}

/* The same, 16-bit */
static void flip16_dp(uint16_t off, uint16_t bits)
{
    op_lda16(dp(off));
    cpu.a ^= bits;
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(dp(off), cpu.a);
}

/*
 * @routine 81:C032 LevelScrollLimits
 * WHAT IT DOES: Sets how far the play field may scroll, right and down,
 * from the level's size.
 * HOW: Follows the level table to the level's header, then to its size
 * record; its two size bytes, in screens of 256 pixels, become the scroll
 * limits X and Y (the byte times 256).
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: M and X 8-bit; A = the Y limit; X = the size record's offset; N
 * and Z from the Y limit; C from doubling the level number; Y and the
 * other flags unchanged.
 */
void abi_LevelScrollLimits(void)
{
    op_rep(0x30);
    op_lda16(LEVEL);
    op_asl16();
    op_tax16();
    op_lda16(LEVEL_TABLE + cpu.x);
    op_tax16();
    op_lda16(LEVEL_TABLE + cpu.x);
    op_tax16();
    op_lda16(LEVEL_TABLE - 1 + cpu.x);
    op_and16(0xFF00);
    wr16(SCROLL_LIMIT_X, cpu.a);
    op_lda16(LEVEL_TABLE + cpu.x);
    op_and16(0xFF00);
    wr16(SCROLL_LIMIT_Y, cpu.a);
    op_sep(0x30);
}

/* LayerQueueColumn and LayerQueueRow: `scroll` and `drawn` the layer's
   fields, `queue` the queue's entries, `tail` its write index. */
static void layer_queue(uint16_t scroll, uint16_t drawn, uint32_t queue, uint32_t tail)
{
    cpu.y = 0;
    cpu.p = nz8(cpu.p, 0);
    op_lda16(dp(scroll));
    op_sub16(rd16(dp(drawn)));
    if (cpu.p & P_N) {
        cpu.y = 1;
        cpu.p = nz8(cpu.p, 1);
        cpu.a ^= 0xFFFF;
        cpu.p = nz16(cpu.p, cpu.a);
        cpu.a++;
        cpu.p = nz16(cpu.p, cpu.a);
    }
    op_cmp16(6);
    if (!(cpu.p & P_C))
        return;
    cpu.x = rd8(tail);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    op_lda16_imm(cpu.d);
    wr16(queue + cpu.x, cpu.a);
    op_lda16_imm(cpu.y);
    wr16(queue + 2 + cpu.x, cpu.a);
    op_txa16();
    op_add16(4);
    op_and16(0x001F);
    wr16(tail, cpu.a);
    op_lda16_imm(cpu.y);
    op_asl16();
    cpu.y = (uint8_t)cpu.a;
    cpu.p = nz8(cpu.p, (uint8_t)cpu.y);
    op_lda16(LAYER_DRAW_STEP + cpu.y);
    op_add16(rd16(dp(drawn)));
    wr16(dp(drawn), cpu.a);
}

/*
 * @routine 81:C276 LayerQueueColumn
 * WHAT IT DOES: When the layer has scrolled 6 pixels or more across from
 * where its tiles are drawn to, queues the next column of tiles to draw
 * (on the side it is scrolling to) and moves the drawn position 8 pixels
 * that way.
 * HOW: Compares the scroll X with the drawn X. Far enough: the layer
 * (its direct page) and the side (0 right, 1 left) go in the column
 * queue, a ring of 8, and the drawn X steps by 8.
 * Entry: M 16-bit, X 8-bit; D = the layer's record; DB = $80.
 * Exit: nothing queued: Y = the side; A = the distance; N, Z, C from
 * comparing it with 6; X unchanged. Queued: A = the new drawn X; X = the
 * entry's index; Y = the side times 2; N, V, Z, C from the add. The other
 * flags unchanged.
 */
void abi_LayerQueueColumn(void)
{
    layer_queue(LAYER_SCROLL_X, LAYER_DRAWN_X, COLUMN_QUEUE, COLUMN_QUEUE_TAIL);
}

/*
 * @routine 81:C2AB LayerQueueRow
 * WHAT IT DOES: When the layer has scrolled 6 pixels or more up or down
 * from where its tiles are drawn to, queues the next row of tiles to draw
 * (on the side it is scrolling to) and moves the drawn position 8 pixels
 * that way.
 * HOW: Compares the scroll Y with the drawn Y. Far enough: the layer
 * (its direct page) and the side (0 down, 1 up) go in the row queue, a
 * ring of 8, and the drawn Y steps by 8.
 * Entry: M 16-bit, X 8-bit; D = the layer's record; DB = $80.
 * Exit: nothing queued: Y = the side; A = the distance; N, Z, C from
 * comparing it with 6; X unchanged. Queued: A = the new drawn Y; X = the
 * entry's index; Y = the side times 2; N, V, Z, C from the add. The other
 * flags unchanged.
 */
void abi_LayerQueueRow(void)
{
    layer_queue(LAYER_SCROLL_Y, LAYER_DRAWN_Y, ROW_QUEUE, ROW_QUEUE_TAIL);
}

/*
 * @routine 81:C498 EdgeStepLeft
 * WHAT IT DOES: Moves a layer's edge one tile left, when the layer draws
 * that edge.
 * HOW: When A's bit is set in the layer's edge mask, the tilemap column
 * steps back, from 0 to 31 of the other screen; the tile in the metatile
 * flips between halves, and from the left half on to the right half of
 * the metatile to its left: the block offset steps back, from its first
 * column to its last of the block to the left.
 * Entry: M 8-bit, X 8-bit; A = the edge's bit; X = the edge's offset in
 * the record; D = the layer's record. Exit: A, N, Z as the last
 * instructions leave them; V from the mask's bit 6; C from comparing
 * the tilemap column with 31 when the edge is drawn, else unchanged; X
 * and Y unchanged.
 */
void abi_EdgeStepLeft(void)
{
    uint16_t x = cpu.x;

    if (!bit8_dp(LAYER_EDGES))
        return;
    dec8_dp(x + EDGE_TILEMAP);
    set_a8(rd8(dp(x + EDGE_TILEMAP)));
    set_a8((uint8_t)cpu.a & 0x1F);
    cpu.p = cmp8_flags(cpu.p, (uint8_t)cpu.a, 0x1F);
    if ((uint8_t)cpu.a == 0x1F) {
        inc8_dp(x + EDGE_TILEMAP);
        set_a8(rd8(dp(x + EDGE_TILEMAP)));
        set_a8((uint8_t)cpu.a | 0x1F);
        wr8(dp(x + EDGE_TILEMAP), (uint8_t)cpu.a);
        flip8_dp(x + EDGE_TILEMAP + 1, 0x04);
    }
    flip8_dp(x + EDGE_HALF, 0x02);
    if (!bit_imm(0x02))
        return;
    set_a8(rd8(dp(x + EDGE_BLOCK)));
    if (bit_imm(0x1E)) {
        set_a8((uint8_t)((uint8_t)cpu.a - 1));
        set_a8((uint8_t)((uint8_t)cpu.a - 1));
        wr8(dp(x + EDGE_BLOCK), (uint8_t)cpu.a);
        return;
    }
    set_a8((uint8_t)cpu.a | 0x1E);
    wr8(dp(x + EDGE_BLOCK), (uint8_t)cpu.a);
    dec16_dp(x + EDGE_MAP);
    dec16_dp(x + EDGE_MAP);
}

/*
 * @routine 81:C4DA EdgeStepRight
 * WHAT IT DOES: Moves a layer's edge one tile right.
 * HOW: The tilemap column steps on, from 31 to 0 of the other screen; the
 * tile in the metatile flips between halves, and from the right half on
 * to the left half of the metatile to its right: the block offset steps
 * on, from its last column to its first of the block to the right.
 * Entry: M 8-bit, X 8-bit; X = the edge's offset in the record; D = the
 * layer's record. Exit: A, Y, N, Z as the last instructions leave them;
 * C from comparing the block offset's column bits with $1E when it
 * reached the left half, else unchanged; X and V unchanged.
 */
void abi_EdgeStepRight(void)
{
    uint16_t x = cpu.x;

    inc8_dp(x + EDGE_TILEMAP);
    set_a8(rd8(dp(x + EDGE_TILEMAP)));
    set_a8((uint8_t)cpu.a & 0x1F);
    if ((uint8_t)cpu.a == 0) {
        dec8_dp(x + EDGE_TILEMAP);
        set_a8(rd8(dp(x + EDGE_TILEMAP)));
        set_a8((uint8_t)cpu.a & 0xE0);
        wr8(dp(x + EDGE_TILEMAP), (uint8_t)cpu.a);
        flip8_dp(x + EDGE_TILEMAP + 1, 0x04);
    }
    flip8_dp(x + EDGE_HALF, 0x02);
    if (bit_imm(0x02))
        return;
    set_a8(rd8(dp(x + EDGE_BLOCK)));
    cpu.y = (uint8_t)cpu.a;
    cpu.p = nz8(cpu.p, (uint8_t)cpu.y);
    set_a8((uint8_t)cpu.a & 0x1E);
    cpu.p = cmp8_flags(cpu.p, (uint8_t)cpu.a, 0x1E);
    if ((uint8_t)cpu.a != 0x1E) {
        cpu.y = (uint8_t)(cpu.y + 1);
        cpu.p = nz8(cpu.p, (uint8_t)cpu.y);
        cpu.y = (uint8_t)(cpu.y + 1);
        cpu.p = nz8(cpu.p, (uint8_t)cpu.y);
        wr8(dp(x + EDGE_BLOCK), (uint8_t)cpu.y);
        return;
    }
    set_a8((uint8_t)cpu.y);
    set_a8((uint8_t)cpu.a & 0xE0);
    wr8(dp(x + EDGE_BLOCK), (uint8_t)cpu.a);
    inc16_dp(x + EDGE_MAP);
    inc16_dp(x + EDGE_MAP);
}

/*
 * @routine 81:C4D6 EdgeStepRightIfDrawn
 * WHAT IT DOES: Moves a layer's edge one tile right, when the layer draws
 * that edge.
 * HOW: EdgeStepRight, when A's bit is set in the layer's edge mask.
 * Entry: M 8-bit, X 8-bit; A = the edge's bit; X = the edge's offset in
 * the record; D = the layer's record. Exit: not drawn: N and V from the
 * mask, Z set, the rest unchanged; else as EdgeStepRight.
 */
void abi_EdgeStepRightIfDrawn(void)
{
    if (!bit8_dp(LAYER_EDGES))
        return;
    abi_EdgeStepRight();
}

/*
 * @routine 81:C516 EdgeStepUp
 * WHAT IT DOES: Moves a layer's edge one tile up, when the layer draws
 * that edge.
 * HOW: When A's bit is set in the layer's edge mask, the tilemap row
 * steps back, from 0 to 31 of the other screen (the layer's screen bits
 * flipped); the tile in the metatile flips between halves, and on to the
 * metatile above: the block offset steps back a row, from its first to
 * its last of the block above.
 * Entry: M 8-bit, X 8-bit; A = the edge's bit; X = the edge's offset in
 * the record; D = the layer's record. Exit: M and X 8-bit; A, Y and the
 * flags as the last instructions leave them; X unchanged.
 */
void abi_EdgeStepUp(void)
{
    uint16_t x = cpu.x;

    if (!bit8_dp(LAYER_EDGES))
        goto done;
    op_rep(0x30);
    op_lda16(dp(x + EDGE_TILEMAP));
    op_tay16();
    op_sub16(0x0020);
    wr16(dp(x + EDGE_TILEMAP), cpu.a);
    op_and16(0x03E0);
    op_cmp16(0x03E0);
    if (cpu.a == 0x03E0) {
        cpu.a = cpu.y;
        cpu.a ^= rd16(dp(LAYER_SCREEN_FLIP));
        cpu.a |= 0x03E0;
        cpu.p = nz16(cpu.p, cpu.a);
        wr16(dp(x + EDGE_TILEMAP), cpu.a);
    }
    flip16_dp(x + EDGE_HALF, 0x0004);
    if (!bit_imm(0x0004))
        goto done;
    op_lda16(dp(x + EDGE_BLOCK));
    op_sub16(0x0020);
    op_and16(0x01FE);
    wr16(dp(x + EDGE_BLOCK), cpu.a);
    op_cmp16(0x01E0);
    if (!(cpu.p & P_C))
        goto done;
    op_lda16(dp(x + EDGE_MAP));
    op_sub16(0x0040);
    wr16(dp(x + EDGE_MAP), cpu.a);
done:
    op_sep(0x30);
}

/*
 * @routine 81:C55C EdgeStepDown
 * WHAT IT DOES: Moves a layer's edge one tile down, when the layer draws
 * that edge.
 * HOW: When A's bit is set in the layer's edge mask, the tilemap row
 * steps on, from 31 to 0 of the other screen (the layer's screen bits
 * flipped); the tile in the metatile flips between halves, and on to the
 * metatile below: the block offset steps on a row, from its last to its
 * first of the block below.
 * Entry: M 8-bit, X 8-bit; A = the edge's bit; X = the edge's offset in
 * the record; D = the layer's record. Exit: not drawn: N and V from the
 * mask, Z set, the rest unchanged. Else M and X 8-bit; A, Y and the flags
 * as the last instructions leave them; X unchanged.
 */
void abi_EdgeStepDown(void)
{
    uint16_t x = cpu.x;

    if (!bit8_dp(LAYER_EDGES))
        return;
    op_rep(0x30);
    op_lda16(dp(x + EDGE_TILEMAP));
    op_tay16();
    op_add16(0x0020);
    wr16(dp(x + EDGE_TILEMAP), cpu.a);
    op_and16(0x03E0);
    if (cpu.a == 0) {
        cpu.a = cpu.y;
        cpu.a ^= rd16(dp(LAYER_SCREEN_FLIP));
        cpu.a &= 0xFC1F;
        cpu.p = nz16(cpu.p, cpu.a);
        wr16(dp(x + EDGE_TILEMAP), cpu.a);
    }
    flip16_dp(x + EDGE_HALF, 0x0004);
    if (bit_imm(0x0004))
        goto done;
    op_lda16(dp(x + EDGE_BLOCK));
    op_add16(0x0020);
    op_and16(0x01FE);
    wr16(dp(x + EDGE_BLOCK), cpu.a);
    op_cmp16(0x0020);
    if (cpu.p & P_C)
        goto done;
    op_lda16(dp(x + EDGE_MAP));
    op_add16(0x0040);
    wr16(dp(x + EDGE_MAP), cpu.a);
done:
    op_sep(0x30);
}

/* The edges in the order the steps take them, each its bit in the edge
   mask and its offset in the record: left, right, top, bottom */
static const uint8_t edge_bit[4] = { 0x02, 0x01, 0x08, 0x04 };
static const uint8_t edge_at[4] = { 0, 10, 20, 30 };

static void step_edges(void (*step)(void))
{
    int i;

    for (i = 0; i < 4; i++) {
        set_a8(edge_bit[i]);
        cpu.x = edge_at[i];
        cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
        step();
    }
}

/*
 * @routine 81:C41F LayerStepAcross
 * WHAT IT DOES: After a column is drawn, moves the layer's four edges a
 * tile the way the layer scrolled: right or left.
 * HOW: By the column's side (0 right): EdgeStepRightIfDrawn or
 * EdgeStepLeft for the left, right, top and bottom edges in turn.
 * Entry: M 8-bit, X 8-bit; D = the layer's record. Exit: as the last
 * step leaves them (X = 30, the bottom edge's offset).
 */
void abi_LayerStepAcross(void)
{
    cpu.x = rd8(dp(LAYER_COLUMN_SIDE));
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    step_edges(cpu.x == 0 ? abi_EdgeStepRightIfDrawn : abi_EdgeStepLeft);
}

/*
 * @routine 81:C45C LayerStepUpDown
 * WHAT IT DOES: After a row is drawn, moves the layer's four edges a tile
 * the way the layer scrolled: down or up.
 * HOW: By the row's side (0 down): EdgeStepDown or EdgeStepUp for the
 * left, right, top and bottom edges in turn.
 * Entry: M 8-bit, X 8-bit; D = the layer's record. Exit: as the last
 * step leaves them (X = 30, the bottom edge's offset).
 */
void abi_LayerStepUpDown(void)
{
    cpu.x = rd8(dp(LAYER_ROW_SIDE));
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    step_edges(cpu.x == 0 ? abi_EdgeStepDown : abi_EdgeStepUp);
}

/*
 * @routine 81:C0F7 LayerSetup
 * WHAT IT DOES: Sets a scroll layer up for a level: where its tilemap and
 * block map are, how long its columns and rows are, where its metatiles
 * come from, and that it draws all four edges.
 * HOW: Copies the layer's 7 bytes from the setup table into its record,
 * then fixed values: block lists in bank $7F, metatiles at $8000 up in
 * the level's metatile bank, edge mask $0F.
 * Entry: M 16-bit, X 8-bit; A = the layer's record; X = its offset in the
 * setup table (0 or 7); DB = $80. Exit: D = the record; Y = $0F; N and Z
 * from it; A, X and the other flags unchanged.
 */
void abi_LayerSetup(void)
{
    uint16_t x = cpu.x;

    op_lda16_imm(cpu.a);                 /* TCD: N, Z from it */
    cpu.d = cpu.a;
    cpu.y = rd8(LAYER_SETUP + x);
    wr8(dp(LAYER_MAP_BASE + 1), (uint8_t)cpu.y);
    cpu.y = rd8(LAYER_SETUP + 1 + x);
    wr8(dp(LAYER_TILEMAP_BASE + 1), (uint8_t)cpu.y);
    op_lda16(LAYER_SETUP + 2 + x);
    wr16(dp(LAYER_TILEMAP_MASK), cpu.a);
    cpu.y = rd8(LAYER_SETUP + 4 + x);
    wr8(dp(LAYER_SCREEN_FLIP + 1), (uint8_t)cpu.y);
    cpu.y = rd8(LAYER_SETUP + 5 + x);
    wr8(dp(LAYER_COLUMN_LENGTH), (uint8_t)cpu.y);
    cpu.y = rd8(LAYER_SETUP + 6 + x);
    wr8(dp(LAYER_ROW_LENGTH), (uint8_t)cpu.y);
    cpu.y = 0x7F;
    wr8(dp(LAYER_BLOCK_PTR + 2), 0x7F);
    cpu.y = 0x80;
    wr8(dp(LAYER_METATILE_PTR + 1), 0x80);
    cpu.y = rd8(LEVEL_METATILE_BANK);
    wr8(dp(LAYER_METATILE_PTR + 2), (uint8_t)cpu.y);
    cpu.y = 0x0F;
    cpu.p = nz8(cpu.p, 0x0F);
    wr8(dp(LAYER_EDGES), 0x0F);
}

/*
 * @routine 81:C177 TilemapWord
 * WHAT IT DOES: Gives the tilemap word, in a layer's 64 by 32 tilemap,
 * of the tile at a pixel position.
 * HOW: Column from X bits 3-7, row from Y bits 3-7, the second screen
 * (bit 10) from X bit 8, over the base $4800.
 * Entry: M 16-bit, X 16-bit; A = X position; Y = Y position; DB a bank
 * whose $0000-$1FFF is work RAM. Exit: A = the word; N and Z from it; C
 * from the last shift; $0014 = the column and row part; X, Y and the
 * other flags unchanged.
 */
void abi_TilemapWord(void)
{
    uint16_t px = cpu.a;

    op_and16(0x00FF);
    op_lsr16();
    op_lsr16();
    op_lsr16();
    wr16((SCRATCH + 8), cpu.a);
    op_lda16_imm(cpu.y);
    op_and16(0x00F8);
    op_asl16();
    op_asl16();
    op_ora16(rd16((SCRATCH + 8)));
    wr16((SCRATCH + 8), cpu.a);
    op_lda16_imm(px);
    op_xba();
    op_lsr16();
    op_lda16_imm(cpu.y);
    op_xba();
    op_ror16();
    op_ror16();
    op_lsr16();
    op_lsr16();
    op_lsr16();
    op_lsr16();
    op_and16(0x0400);
    op_ora16(0x4800);
    op_ora16(rd16((SCRATCH + 8)));
}

/*
 * @routine 81:C1A2 EdgeMapPlace
 * WHAT IT DOES: Puts a layer's edge at a pixel position in the level's
 * map: its block, its metatile in the block and its tile in the metatile.
 * HOW: The block from the position's high bytes (one block over and one
 * down: the map has a border), plus the layer's map base; the metatile
 * from bits 4-7; the tile's half from bit 3.
 * Entry: M 16-bit, X 16-bit; A = X position; Y = Y position; X = the
 * edge's offset in the record; D = the layer's record; DB a bank whose
 * $0000-$1FFF is work RAM. Exit: A = the half; $000C, $000E = the
 * position; $0010 = the X half; X, Y and P unchanged.
 */
void abi_EdgeMapPlace(void)
{
    uint8_t p = cpu.p;
    uint16_t x = cpu.x;

    wr16(SCRATCH, cpu.a);
    wr16((SCRATCH + 2), cpu.y);
    op_xba();
    cpu.a++;
    cpu.p = nz16(cpu.p, cpu.a);
    op_asl16();
    op_and16(0x00FE);
    wr16((SCRATCH + 4), cpu.a);
    op_lda16_imm(cpu.y);
    op_add16(0x0100);
    op_and16(0xFF00);
    op_lsr16();
    op_lsr16();
    op_add16(rd16((SCRATCH + 4)));
    op_add16(rd16(dp(LAYER_MAP_BASE)));
    wr16(dp(x + EDGE_MAP), cpu.a);
    op_lda16(SCRATCH);
    op_lsr16();
    op_lsr16();
    op_lsr16();
    op_and16(0x001E);
    wr16((SCRATCH + 4), cpu.a);
    op_lda16((SCRATCH + 2));
    op_asl16();
    op_and16(0x01E0);
    op_ora16(rd16((SCRATCH + 4)));
    wr16(dp(x + EDGE_BLOCK), cpu.a);
    op_lda16(SCRATCH);
    op_lsr16();
    op_lsr16();
    op_and16(0x0002);
    wr16((SCRATCH + 4), cpu.a);
    op_lda16((SCRATCH + 2));
    op_lsr16();
    op_and16(0x0004);
    op_ora16(rd16((SCRATCH + 4)));
    wr16(dp(x + EDGE_HALF), cpu.a);
    op_plp(p);
}

/*
 * @routine 81:C163 EdgePlaceAt
 * WHAT IT DOES: Puts a layer's edge at a pixel position: its tilemap word
 * and its place in the level's map.
 * HOW: TilemapWord, kept to the layer's mask and given its base bits;
 * then EdgeMapPlace.
 * Entry: M 16-bit, X 16-bit; A = X position; Y = Y position; X = the
 * edge's offset in the record; D = the layer's record; DB = $80.
 * Exit: as EdgeMapPlace, with N and Z from Y.
 */
void abi_EdgePlaceAt(void)
{
    uint16_t px = cpu.a, py = cpu.y;

    abi_TilemapWord();
    op_and16(rd16(dp(LAYER_TILEMAP_MASK)));
    op_ora16(rd16(dp(LAYER_TILEMAP_BASE)));
    wr16(dp(cpu.x + EDGE_TILEMAP), cpu.a);
    op_lda16_imm(px);
    cpu.y = py;
    cpu.p = nz16(cpu.p, py);
    abi_EdgeMapPlace();
}

/*
 * @routine 81:C146 EdgePlace
 * WHAT IT DOES: Puts one of a layer's edges where it starts, from the
 * layer's scroll: just off the screen on its side.
 * HOW: The scroll plus the edge's start from the edge place table, then
 * EdgePlaceAt.
 * Entry: M 16-bit, X 16-bit; A = the edge's offset in the record; X = its
 * offset in the edge place table; D = the layer's record; DB = $80.
 * Exit: X = the next edge's table offset (4 on); N and Z from it; A, Y
 * as EdgePlaceAt leaves them; $001C = the edge's offset.
 */
void abi_EdgePlace(void)
{
    uint16_t x = cpu.x;

    wr16((SCRATCH + 16), cpu.a);
    op_lda16(dp(LAYER_SCROLL_Y));
    op_add16(rd16(EDGE_PLACE + 2 + x));
    op_tay16();
    op_lda16(dp(LAYER_SCROLL_X));
    op_add16(rd16(EDGE_PLACE + x));
    cpu.x = rd16((SCRATCH + 16));
    cpu.p = nz16(cpu.p, cpu.x);
    abi_EdgePlaceAt();
    cpu.x = (uint16_t)(x + 4);
    cpu.p = nz16(cpu.p, cpu.x);
}

/*
 * @routine 81:C128 LayerPlaceEdges
 * WHAT IT DOES: Puts a layer's four edges where they start, from its
 * scroll.
 * HOW: EdgePlace for the left, right, top and bottom edges.
 * Entry: M 16-bit, X 8-bit; A = the layer's record; X = 0; DB = $80.
 * Exit: M 16-bit, X 8-bit; D = the record; X = 16; A, Y as the last
 * EdgePlace leaves them; N and Z from X.
 */
void abi_LayerPlaceEdges(void)
{
    int i;

    op_rep(0x30);
    op_lda16_imm(cpu.a);                 /* TCD */
    cpu.d = cpu.a;
    for (i = 0; i < 4; i++) {
        op_lda16_imm(edge_at[i]);
        abi_EdgePlace();
    }
    op_sep(0x10);
}

/*
 * @routine 81:C0B6 LayersSetup
 * WHAT IT DOES: Sets both scroll layers up for the level and puts their
 * edges where they start.
 * HOW: LayerSetup and LayerPlaceEdges for the play field and the
 * backdrop. In levels 0, 2, 3, $14 and $15 the backdrop's rows are 68
 * tiles long.
 * Entry: M 8-bit, X 8-bit; DB = $80. Exit: M and X 8-bit; D unchanged, N
 * and Z from it; A, X, Y as the last instructions leave them; C from the
 * last level compare.
 */
void abi_LayersSetup(void)
{
    static const uint8_t wide[4] = { 0x15, 0x14, 0x02, 0x03 };
    uint16_t d = cpu.d;
    uint8_t level;
    int i, hit;

    op_rep(0x20);
    op_lda16_imm(LAYER0);
    cpu.x = 0;
    cpu.p = nz8(cpu.p, 0);
    abi_LayerSetup();
    op_lda16_imm(LAYER1);
    cpu.x = 7;
    cpu.p = nz8(cpu.p, 7);
    abi_LayerSetup();
    op_lda16_imm(LAYER0);
    cpu.x = 0;
    cpu.p = nz8(cpu.p, 0);
    abi_LayerPlaceEdges();
    op_lda16_imm(LAYER1);
    cpu.x = 0;
    cpu.p = nz8(cpu.p, 0);
    abi_LayerPlaceEdges();
    op_sep(0x30);
    op_lda8(LEVEL);
    level = (uint8_t)cpu.a;
    hit = level == 0;
    for (i = 0; i < 4 && !hit; i++) {
        cpu.p = cmp8_flags(cpu.p, level, wide[i]);
        hit = level == wide[i];
    }
    if (hit) {
        set_a8(0x44);
        wr8(LAYER1 + LAYER_ROW_LENGTH, 0x44);
    }
    cpu.d = d;
    cpu.p = nz16(cpu.p, d);
}

/* A tile of the level's map: the metatile list of the block at `map` in
   the block map (an empty block, a negative entry, takes list 0), and the
   tile at `half` of the metatile at `block` in it. */
static void map_block(uint16_t map)
{
    op_lda16(BLOCK_MAP + map);
    if (cpu.p & P_N)
        op_lda16_imm(0);
    wr16(dp(LAYER_BLOCK_PTR), cpu.a);
}

static void map_metatile(void)
{
    op_lda16((rd24(dp(LAYER_BLOCK_PTR)) + cpu.y) & 0xFFFFFF);
    op_ora16(0x8000);
    wr16(dp(LAYER_METATILE_PTR), cpu.a);
}

static void map_tile(void)
{
    op_lda16((rd24(dp(LAYER_METATILE_PTR)) + cpu.y) & 0xFFFFFF);
}

/*
 * @routine 81:C2E0 LayerDrawColumn
 * WHAT IT DOES: Draws the column of tiles coming into view on a side of a
 * layer, from the level's map, ready to be copied to the tilemap.
 * HOW: From the side's edge (the right or the left), down the map: each
 * block's metatile list, each metatile's tile in the edge's column half,
 * the top and bottom halves in turn, into the layer's column buffer from
 * the edge's tilemap row on, wrapping at 32, for the layer's column
 * length. Then the tilemap word to copy it to and its parts (two when
 * longer than 32 tiles).
 * Entry: M either, X 8-bit; Y = the side (0 right, 1 left); D = the
 * layer's record; DB = $80. Exit: M and X 8-bit; A = the parts; X = the
 * edge's offset; Y = the column length; N, Z, C from comparing it with
 * 33 (N and Z from the parts when 2); $000C-$0015 as the walk leaves them.
 */
void abi_LayerDrawColumn(void)
{
    uint16_t edge;

    cpu.x = rd8(SIDE_EDGE + cpu.y);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    op_rep(0x30);
    edge = cpu.x;
    cpu.p &= (uint8_t)~P_C;
    op_lda16(dp(edge + EDGE_MAP));
    wr16(SCRATCH, cpu.a);
    op_lda16(dp(edge + EDGE_BLOCK));
    wr16((SCRATCH + 2), cpu.a);
    op_lda16(dp(edge + EDGE_HALF));
    wr16((SCRATCH + 6), cpu.a);
    op_lda16(dp(LAYER_COLUMN_LENGTH));
    wr16((SCRATCH + 8), cpu.a);
    op_lda16(dp(edge + EDGE_TILEMAP));
    op_and16(0xFBFF);
    op_lsr16();
    op_lsr16();
    op_lsr16();
    op_lsr16();
    op_lsr16();
    if (bit_imm(0x0040))
        op_ora16(0x0020);
    op_asl16();
    op_and16(0x003F);
    op_tay16();
    cpu.x = rd16(SCRATCH);
    cpu.p = nz16(cpu.p, cpu.x);
    for (;;) {                                   /* each block down */
        map_block(cpu.x);
        cpu.x = cpu.y;
        cpu.p = nz16(cpu.p, cpu.x);
        cpu.y = rd16((SCRATCH + 2));
        cpu.p = nz16(cpu.p, cpu.y);
        for (;;) {                               /* each metatile down */
            map_metatile();
            cpu.y = rd16((SCRATCH + 6));
            cpu.p = nz16(cpu.p, cpu.y);
            for (;;) {                           /* its two tiles */
                uint16_t left;

                map_tile();
                wr16(dp(LAYER_COLUMN_BUF + cpu.x), cpu.a);
                left = (uint16_t)(rd16_rmw((SCRATCH + 8)) - 1);
                wr16_rmw((SCRATCH + 8), left);
                cpu.p = nz16(cpu.p, left);
                if (left == 0)
                    goto drawn;
                cpu.x = (uint16_t)(cpu.x + 2);
                cpu.p = nz16(cpu.p, cpu.x);
                op_txa16();
                op_and16(0x003F);
                op_tax16();
                op_lda16_imm(cpu.y);
                cpu.a ^= 0x0004;
                cpu.p = nz16(cpu.p, cpu.a);
                op_tay16();
                if (!bit_imm(0x0004))
                    break;
            }
            wr16((SCRATCH + 6), cpu.a);
            op_lda16((SCRATCH + 2));
            op_add16(0x0020);
            op_and16(0x01FE);
            op_tay16();
            wr16((SCRATCH + 2), cpu.a);
            op_cmp16(0x0020);
            if (!(cpu.p & P_C))
                break;
        }
        cpu.y = cpu.x;
        cpu.p = nz16(cpu.p, cpu.y);
        op_lda16(SCRATCH);
        cpu.a = op_adc16(cpu.a, 0x0040, &cpu.p);
        wr16(SCRATCH, cpu.a);
        op_tax16();
    }
drawn:
    cpu.x = edge;
    cpu.p = nz16(cpu.p, cpu.x);
    op_lda16(dp(edge + EDGE_TILEMAP));
    op_and16(0x041F);
    op_ora16(rd16(dp(LAYER_TILEMAP_BASE)));
    wr16(dp(LAYER_COLUMN_DEST), cpu.a);
    op_sep(0x30);
    set_a8(1);
    cpu.y = rd8(dp(LAYER_COLUMN_LENGTH));
    cpu.p = cmp8_flags(cpu.p, (uint8_t)cpu.y, 0x21);
    if (cpu.p & P_C)
        set_a8(2);
    wr8(dp(LAYER_COLUMN_PARTS), (uint8_t)cpu.a);
}

/*
 * @routine 81:C383 LayerDrawRow
 * WHAT IT DOES: Draws the row of tiles coming into view on a side of a
 * layer, from the level's map, ready to be copied to the tilemap.
 * HOW: From the side's edge (the bottom or the top), across the map: each
 * block's metatile list, each metatile's tile in the edge's row half, the
 * left and right halves in turn, into the layer's row buffer from the
 * edge's tilemap column on (both screens), wrapping at 64, for the
 * layer's row length. Then the tilemap word to copy it to, and 2 parts.
 * Entry: M 8-bit, X 8-bit; Y = the side (0 bottom, 1 top); D = the
 * layer's record; DB = $80. Exit: M and X 8-bit; A = 2; N and Z from
 * it; X = the edge's offset; Y and C as the walk leaves them;
 * $000C-$0015 as the walk leaves them.
 */
void abi_LayerDrawRow(void)
{
    uint16_t edge;

    cpu.x = rd8(SIDE_EDGE + 2 + cpu.y);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    op_rep(0x30);
    edge = cpu.x;
    cpu.p &= (uint8_t)~P_C;
    op_lda16(dp(edge + EDGE_MAP));
    wr16(SCRATCH, cpu.a);
    op_lda16(dp(edge + EDGE_BLOCK));
    wr16((SCRATCH + 2), cpu.a);
    op_lda16(dp(edge + EDGE_HALF));
    wr16((SCRATCH + 6), cpu.a);
    cpu.y = rd16(dp(LAYER_ROW_LENGTH));
    cpu.p = nz16(cpu.p, cpu.y);
    wr16((SCRATCH + 8), cpu.y);
    op_lda16(dp(edge + EDGE_TILEMAP));
    op_and16(0x041F);
    if (bit_imm(0x0400))
        op_ora16(0x0020);
    op_and16(0x007F);
    op_asl16();
    op_tay16();
    cpu.x = rd16(SCRATCH);
    cpu.p = nz16(cpu.p, cpu.x);
    for (;;) {                                   /* each block across */
        map_block(cpu.x);
        cpu.x = cpu.y;
        cpu.p = nz16(cpu.p, cpu.x);
        cpu.y = rd16((SCRATCH + 2));
        cpu.p = nz16(cpu.p, cpu.y);
        for (;;) {                               /* each metatile across */
            map_metatile();
            cpu.y = rd16((SCRATCH + 6));
            cpu.p = nz16(cpu.p, cpu.y);
            for (;;) {                           /* its two tiles */
                uint16_t left;

                map_tile();
                wr16(dp(LAYER_ROW_BUF + cpu.x), cpu.a);
                left = (uint16_t)(rd16_rmw((SCRATCH + 8)) - 1);
                wr16_rmw((SCRATCH + 8), left);
                cpu.p = nz16(cpu.p, left);
                if (left == 0)
                    goto drawn;
                cpu.x = (uint16_t)(cpu.x + 2);
                cpu.p = nz16(cpu.p, cpu.x);
                op_txa16();
                op_and16(0x007F);
                op_tax16();
                op_lda16_imm(cpu.y);
                cpu.a ^= 0x0002;
                cpu.p = nz16(cpu.p, cpu.a);
                op_tay16();
                if (!bit_imm(0x0002))
                    break;
            }
            wr16((SCRATCH + 6), cpu.a);
            op_inc16_mem((SCRATCH + 2));
            op_inc16_mem((SCRATCH + 2));
            op_lda16((SCRATCH + 2));
            op_tay16();
            if (!bit_imm(0x001E))
                break;
        }
        op_sub16(0x0020);
        wr16((SCRATCH + 2), cpu.a);
        cpu.y = cpu.x;
        cpu.p = nz16(cpu.p, cpu.y);
        op_inc16_mem(SCRATCH);
        op_inc16_mem(SCRATCH);
        cpu.x = rd16(SCRATCH);
        cpu.p = nz16(cpu.p, cpu.x);
    }
drawn:
    cpu.x = edge;
    cpu.p = nz16(cpu.p, cpu.x);
    op_lda16(dp(edge + EDGE_TILEMAP));
    op_and16(0x0BE0);
    op_and16(rd16(dp(LAYER_TILEMAP_MASK)));
    op_ora16(rd16(dp(LAYER_TILEMAP_BASE)));
    wr16(dp(LAYER_ROW_DEST), cpu.a);
    op_sep(0x30);
    set_a8(2);
    wr8(dp(LAYER_ROW_PARTS), 2);
}

/* Draws every queued column or row: `head`, `tail` the queue's indexes,
   `side` the record's side field, `bits` the side's edge bits, `draw` and
   `step` the draw and the edges' step. */
static void draw_queued(uint32_t queue, uint32_t head, uint32_t tail, uint16_t side,
                        uint32_t bits, void (*draw)(void), void (*step)(void))
{
    for (;;) {
        cpu.x = rd8(head);
        cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
        cpu.p = cmp8_flags(cpu.p, (uint8_t)cpu.x, rd8(tail));
        if ((uint8_t)cpu.x == rd8(tail))
            return;
        set_a8(rd8(queue + 1 + cpu.x));
        op_xba();
        set_a8(rd8(queue + cpu.x));
        op_lda16_imm(cpu.a);                     /* TCD */
        cpu.d = cpu.a;
        cpu.y = rd8(queue + 2 + cpu.x);
        cpu.p = nz8(cpu.p, (uint8_t)cpu.y);
        wr8(dp(side), (uint8_t)cpu.y);
        cpu.p &= (uint8_t)~P_C;
        set_a8((uint8_t)cpu.x);
        set_a8(op_adc8((uint8_t)cpu.a, 4, &cpu.p));
        set_a8((uint8_t)cpu.a & 0x1F);
        wr8(head, (uint8_t)cpu.a);
        set_a8(rd8(bits + cpu.y));
        if (bit8_dp(LAYER_EDGES)) {
            draw();
            step();
        }
    }
}

/*
 * @routine 81:C1FA LayersScroll
 * WHAT IT DOES: Once a frame, keeps both scroll layers' tilemaps ahead of
 * their scroll: queues the columns and rows coming into view, then draws
 * them and moves the layers' edges on.
 * HOW: For each layer that draws an edge (or has a column to copy):
 * LayerQueueColumn and LayerQueueRow. Then each queued column, of a side
 * whose edge its layer draws: LayerDrawColumn and LayerStepAcross; then
 * each queued row the same way, with LayerDrawRow and LayerStepUpDown.
 * Entry: M 8-bit, X 8-bit; DB = $80. Exit: P and D as at entry; A, X, Y
 * as the last instructions leave them.
 */
void abi_LayersScroll(void)
{
    uint8_t p = cpu.p;
    uint16_t d = cpu.d;

    op_rep(0x20);
    op_lda16_imm(LAYER0);
    cpu.d = cpu.a;
    do {
        op_lda16(dp(LAYER_EDGES));
        if (cpu.a != 0) {
            abi_LayerQueueColumn();
            abi_LayerQueueRow();
        }
        op_lda16_imm(cpu.d);                     /* TDC */
        op_add16(LAYER_SIZE);
        cpu.d = cpu.a;
        op_cmp16(LAYER1 + LAYER_SIZE);
    } while (cpu.a != LAYER1 + LAYER_SIZE);
    op_sep(0x20);
    draw_queued(COLUMN_QUEUE, COLUMN_QUEUE_HEAD, COLUMN_QUEUE_TAIL, LAYER_COLUMN_SIDE,
                SIDE_EDGE_BIT, abi_LayerDrawColumn, abi_LayerStepAcross);
    draw_queued(ROW_QUEUE, ROW_QUEUE_HEAD, ROW_QUEUE_TAIL, LAYER_ROW_SIDE,
                SIDE_EDGE_BIT + 2, abi_LayerDrawRow, abi_LayerStepUpDown);
    cpu.d = d;
    op_plp(p);
}
