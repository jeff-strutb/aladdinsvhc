/*
 * tile_slots.c: four slots of animated tile graphics that map cells can
 * claim; a claimed slot's graphics are copied into the slot's place in a
 * work-RAM copy of the tiles, and counted for upload.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "actor.h"
#include "wram.h"

#define TILE_SLOT_UPLOADS 0x000844u /* tiles copied, waiting for upload */
#define TILE_SLOT_USED 0x0015E9u    /* 4 bytes: nonzero while a slot is claimed */
#define TILE_SLOT_CELL 0x0015EDu    /* 4 bytes: the low byte of the cell that claimed it */
#define MAP_CELL (SCRATCH + 16)     /* word: the map cell worked on */
#define MAP_CELL_ATTR (SCRATCH + 22) /* word: its offset in the attribute table */
#define COPY_COUNT (SCRATCH + 30)   /* word: words left to copy */
#define CELL_TILES 0x7FE9FFu        /* by cell, the word whose top 13 bits give the source tile */
#define CELL_ATTRS 0x7FE700u        /* by cell, attribute words: bits 9-11 a slot place */
#define CELL_SLOTS 0x7FEB00u        /* by cell, the slot (1-4) it holds */
#define SLOT_PLACES 0x82ABBEu       /* 8 words: where each place's tile goes in TILE_COPY */
#define SLOT_WORDS 0xAC06u          /* in the bank DB names: a word per slot (1-4) */
#define TILE_SOURCE 0x84B000u       /* the tiles' graphics, 32 bytes each */
#define TILE_COPY 0x7EFD00u         /* the work-RAM copy the slots go into */

static uint32_t in_db(uint32_t addr)
{
    return (uint32_t)cpu.db << 16 | addr;
}

/*
 * @routine 82:AB77 TileSlotFill
 * WHAT IT DOES: Copies the cell's tile graphics (32 bytes) into its slot's
 * place in the work-RAM tile copy, and counts one more for upload.
 * HOW: The cell's tile word, shifted right 3 with its low 5 bits cleared,
 * is the source's offset in $84:B000; the place comes from bits 9-11 of
 * the cell's attribute word through the table at $82:ABBE. 16 words are
 * copied with 16-bit registers, with DB $84 while they are.
 * Entry: any widths; DB a bank whose $0000-$1FFF is work RAM. Exit: A =
 * the last word copied (16 bits); X and Y their last offsets' low bytes;
 * N and Z from DB as it comes back from the stack; C from the shift; M
 * and X 8-bit. DB and the other flags unchanged.
 */
void abi_TileSlotFill(void)
{
    uint16_t x, y, n;
    uint8_t a;

    op_rep(P_M | P_X);
    x = rd16(MAP_CELL);
    cpu.a = rd16(CELL_TILES + x);
    x = rd16(MAP_CELL_ATTR);
    op_lsr16();
    op_lsr16();
    op_lsr16();
    op_and16(0xFFE0);
    y = cpu.a;
    wr16(COPY_COUNT, 0x0010);
    cpu.a = rd16(CELL_ATTRS + x);
    op_and16(0x0E00);
    op_xba();
    x = rd16(SLOT_PLACES + cpu.a);
    do {
        cpu.a = rd16(TILE_SOURCE + y);
        wr16(TILE_COPY + x, cpu.a);
        y = (uint16_t)(y + 2);
        x = (uint16_t)(x + 2);
        n = (uint16_t)(rd16_rmw(COPY_COUNT) - 1);
        wr16_rmw(COPY_COUNT, n);
    } while (n != 0);
    cpu.x = x;
    cpu.y = y;
    op_sep(P_M | P_X);
    a = (uint8_t)(rd8_rmw(TILE_SLOT_UPLOADS) + 1);
    wr8(TILE_SLOT_UPLOADS, a);
    cpu.p = nz8(cpu.p, cpu.db);
}

/*
 * @routine 82:ABCE TileSlotClaim
 * WHAT IT DOES: Claims a free animated-tile slot for the cell and the
 * actor whose record Y points at; the carry is set when all four are
 * taken.
 * HOW: Slots are tried from 3 down to 0. The claimed one is marked used
 * and keeps the cell's low byte; the slot's number (1-4) goes to the
 * actor and to the cell, and the slot's word to the actor's word at $2B
 * and to the cell's attribute word.
 * Entry: M 8-bit, X 16-bit; Y = the actor; DB a bank whose $0000-$1FFF is
 * work RAM, the slot words' bank. Exit: none free: C set, X = $FFFF, N
 * set, Z clear, A low = slot 0's used byte. Claimed: C clear, A = the
 * slot's word, X = the attribute offset, N and Z from it. M 8-bit; Y and
 * the other flags unchanged.
 */
void abi_TileSlotClaim(void)
{
    uint16_t x = 3, y = cpu.y;
    uint8_t a, n;

    for (;;) {
        a = rd8(TILE_SLOT_USED + x);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
        if (a == 0)
            break;
        x = (uint16_t)(x - 1);
        if (x & 0x8000) {
            cpu.x = x;
            cpu.p = (uint8_t)(nz16(cpu.p, x) | P_C);
            return;
        }
    }
    cpu.a = (uint16_t)(cpu.a >> 8);
    wr8(TILE_SLOT_USED + x, (uint8_t)(rd8_rmw(TILE_SLOT_USED + x) + 1));
    wr8(TILE_SLOT_CELL + x, rd8(MAP_CELL));
    n = (uint8_t)(x + 1);
    cpu.a = n;
    wr8(in_db((uint32_t)y + ACTOR_TILE_SLOT), n);
    wr8(CELL_SLOTS + rd16(MAP_CELL), n);
    op_rep(P_M);
    op_asl16();
    cpu.a = rd16(in_db(SLOT_WORDS + cpu.a));
    wr16(in_db((uint32_t)y + ACTOR_2B), cpu.a);
    x = rd16(MAP_CELL_ATTR);
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
    wr16(CELL_ATTRS + x, cpu.a);
    op_sep(P_M);
    cpu.p &= (uint8_t)~P_C;
}
