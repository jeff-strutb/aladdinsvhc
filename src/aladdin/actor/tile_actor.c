/*
 * tile_actor.c: an actor drawn as tiles of the level's map, changed
 * through the tile change queue as it moves. Drafted by cdraft.py; named
 * and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_TileChangeAdd(void);

/* In the data bank: per kind ($47) the offset of its entries; 4 bytes an
   entry: the tile, and the tile 16 pixels below (0: none) */
#define TILE_ACTOR_TILES 0xD633u
#define EMPTY_TILE 0x7F80u          /* the tile put while $19 is 0 */

/*
 * @routine 83:E7F0 TileActorDraw
 * WHAT IT DOES: Puts the actor into the level's map as tiles at its tile
 * spot ($42, $44): the tile for its kind, frame and flip, and the tile 16
 * pixels below when it has one; the empty tile in place of each while its
 * $19 is 0.
 * HOW: The entry: ($4A xor $19) times 2, or its flip ($15), times 4, plus
 * the kind's offset from the table at its $47. TileChangeAdd at ($42, $44)
 * with the first tile; the second nonzero: TileChangeAdd at ($42, $44 +
 * 16).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSR. Exit: M 8-bit, X 8-bit; as TileChangeAdd, or A low = 0 and Z set
 * when there is no second tile.
 */
void abi_TileActorDraw(void)
{
    ldx8(ea_dp(0x47));  /* 83E7F0 ldx */
    lda8(ea_dp(0x4A));  /* 83E7F2 lda */
    eor8_v(rd8(ea_dp(0x19)));  /* 83E7F4 eor */
    ACC8(asl);  /* 83E7F6 asl */
    ora8_v(rd8(ea_dp(0x15)));  /* 83E7F7 ora */
    ACC8(asl);  /* 83E7F9 asl */
    ACC8(asl);  /* 83E7FA asl */
    clc();  /* 83E7FB clc */
    adc8_v(rd8(ea_absx(TILE_ACTOR_TILES)));  /* 83E7FC adc */
    op_rep(0x30);  /* 83E7FF rep */
    and16_v(0x00FF);  /* 83E801 and */
    tax16();  /* 83E804 tax */
    phx16();  /* 83E805 phx */
    lda16(ea_absx(TILE_ACTOR_TILES));  /* 83E806 lda */
    ldx16(ea_dp(0x19));  /* 83E809 ldx */
    if (!IF_Z) goto L_83E810;  /* 83E80B bne */
    lda16_v(EMPTY_TILE);  /* 83E80D lda */
L_83E810:
    ldx16(ea_dp(0x42));  /* 83E810 ldx */
    ldy16(ea_dp(0x44));  /* 83E812 ldy */
    abi_TileChangeAdd();  /* 83E814 jsl */
    plx16();  /* 83E818 plx */
    lda16(ea_absx(TILE_ACTOR_TILES + 2));  /* 83E819 lda */
    if (IF_Z) goto L_83E834;  /* 83E81C beq */
    ldx16(ea_dp(0x19));  /* 83E81E ldx */
    if (!IF_Z) goto L_83E825;  /* 83E820 bne */
    lda16_v(EMPTY_TILE);  /* 83E822 lda */
L_83E825:
    pha16();  /* 83E825 pha */
    ldx16(ea_dp(0x42));  /* 83E826 ldx */
    clc();  /* 83E828 clc */
    lda16(ea_dp(0x44));  /* 83E829 lda */
    adc16_v(0x0010);  /* 83E82B adc */
    tay16();  /* 83E82E tay */
    pla16();  /* 83E82F pla */
    abi_TileChangeAdd();  /* 83E830 jsl */
L_83E834:
    op_sep(0x30);  /* 83E834 sep */
    return;  /* 83E836 rts */
}

