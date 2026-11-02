/*
 * map_probe.c: an actor looks at the level map around itself: the block a
 * few pixels ahead of it along its facing and below it, and whether the
 * ground between it and the player is clear to see along.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"

void abi_ActorSideOfPlayer(void);

/* The map test: the class of the level map's block at the point SCRATCH
   (x, word), SCRATCH + 2 (y, word); A = X = it, 8-bit registers. */
#define MAP_BLOCK_AT 0x81B82Bu
/* Whether the player is within the actor's vertical reach (box X): C set
   when not. */
#define PLAYER_IN_REACH_Y 0x81AEFDu
/* In bank $80 (DB), x offsets ahead of the actor: a word for facing right,
   then one for facing left, an entry each 4 bytes. */
#define AHEAD_OFFSETS 0xD062u
/* In bank $80 (DB): 5 pixels ahead, facing right then left (words). */
#define AHEAD_5 0xD176u
#define STEPS_LEFT ACTOR_STREAM   /* $4A: here, the 16-pixel steps left to test */

/*
 * @routine 83:9A15 ActorMapAtOffset
 * WHAT IT DOES: Gives the class of the level map's block at a point ahead
 * of the actor (along its facing) and below it.
 * HOW: The x offset is the word of the ahead table (bank $80, $D062) by
 * the entry in SCRATCH and the facing; it and the y offset in SCRATCH + 2
 * are added to the actor's pixels, and the map test at $81:B82B looks the
 * point up.
 * Entry: M 8-bit, X 8-bit; SCRATCH (byte) = the entry's offset, SCRATCH + 2
 * (word) = pixels below the actor; D = the actor; DB = $80. Exit: as the
 * map test at $81:B82B leaves them: M and X 8-bit, A low byte = X = the
 * block's class, N and Z from it; SCRATCH and SCRATCH + 2 = the point.
 */
void abi_ActorMapAtOffset(void)
{
    lda8(ea_dp(ACTOR_FACING));
    ACC8(asl);
    adc8_v(rd8(ea_abs(SCRATCH)));
    tax8();
    op_rep(0x21);
    lda16(ea_absx(AHEAD_OFFSETS));
    adc16_v(rd16(ea_dp(ACTOR_X + 1)));
    sta16(ea_abs(SCRATCH));
    lda16(ea_dp(ACTOR_Y + 1));
    clc();
    adc16_v(rd16(ea_abs(SCRATCH + 2)));
    sta16(ea_abs(SCRATCH + 2));
    call_routine(MAP_BLOCK_AT, 1);
}

/*
 * @routine 83:9A01 ActorMapAhead7Down28
 * WHAT IT DOES: Gives the class of the map's block 7 pixels ahead of the
 * actor and 28 below it.
 * HOW: Entry 4 of the ahead table and 28 below, then on into
 * ActorMapAtOffset.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80. Exit: as
 * ActorMapAtOffset.
 */
void abi_ActorMapAhead7Down28(void)
{
    lda8_v(0x1C);
    sta8(ea_abs(SCRATCH + 2));
    lda8_v(0x04);
    sta8(ea_abs(SCRATCH));
    stz8(ea_abs(SCRATCH + 3));
    abi_ActorMapAtOffset();
}

/*
 * @routine 83:B5F6 ActorMapAhead5Down12
 * WHAT IT DOES: Gives the class of the map's block 5 pixels ahead of the
 * actor and 12 below it.
 * HOW: As ActorMapAtOffset, with its own table of offsets (bank $80,
 * $D176) and 12 below.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80. Exit: as the map test
 * at $81:B82B leaves them: M and X 8-bit, A low byte = X = the block's
 * class, N and Z from it; SCRATCH and SCRATCH + 2 = the point.
 */
void abi_ActorMapAhead5Down12(void)
{
    lda8_v(0x0C);
    sta8(ea_abs(SCRATCH + 2));
    stz8(ea_abs(SCRATCH + 3));
    lda8(ea_dp(ACTOR_FACING));
    ACC8(asl);
    tax8();
    op_rep(0x21);
    lda16(ea_absx(AHEAD_5));
    adc16_v(rd16(ea_dp(ACTOR_X + 1)));
    sta16(ea_abs(SCRATCH));
    lda16(ea_dp(ACTOR_Y + 1));
    clc();
    adc16_v(rd16(ea_abs(SCRATCH + 2)));
    sta16(ea_abs(SCRATCH + 2));
    call_routine(MAP_BLOCK_AT, 1);
}

