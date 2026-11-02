/*
 * map_probe.c: an actor looks at the level map around itself: the block a
 * few pixels ahead of it along its facing and below it, and whether the
 * ground between it and the player is clear to see along.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"

void abi_ActorSideOfPlayer(void);

/* The map test: the class of the level map's block at the point SCRATCH
   (x, word), SCRATCH + 2 (y, word); A = X = it, 8-bit registers. */
void abi_TileAtProbeLong(void);
void abi_PlayerInFrontBox(void);
/* Whether the player is within the actor's vertical reach (box X): C set
   when not. */
#define PLAYER_IN_REACH_Y 0x81AEFDu
/* In bank $80 (DB), x offsets ahead of the actor: a word for facing right,
   then one for facing left, an entry each 4 bytes. */
#define AHEAD_OFFSETS 0xD062u
/* In bank $80 (DB): 5 pixels ahead, facing right then left (words). */
#define AHEAD_5 0xD176u
#define STEPS_LEFT ACTOR_STREAM   /* $4A: here, the 16-pixel steps left to test */

/* The point (x from the table entry, y SCRATCH + 2 pixels below) to
   scratch, then the tile there (TileAtProbeLong). */
static void map_at(uint32_t ahead)
{
    op_rep(0x21);
    cpu.a = op_adc16(rd16(ahead), dp16(ACTOR_X + 1), &cpu.p);
    wr16(dba(SCRATCH), cpu.a);
    flag_set(P_C, 0);
    cpu.a = op_adc16(dp16(ACTOR_Y + 1), rd16(dba(SCRATCH + 2)), &cpu.p);
    wr16(dba(SCRATCH + 2), cpu.a);
    abi_TileAtProbeLong();
}

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
    uint8_t facing = rd8(dpa(ACTOR_FACING));

    /* the entry for its facing (the ASL's carry goes into the add) */
    flag_set(P_C, facing & 0x80);
    cpu.x = op_adc8((uint8_t)(facing << 1), rd8(dba(SCRATCH)), &cpu.p);
    map_at(dba(AHEAD_OFFSETS) + cpu.x);
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
    a8_load(0x1C);
    wr8(dba(SCRATCH + 2), 0x1C);
    a8_load(0x04);
    wr8(dba(SCRATCH), 0x04);
    wr8(dba(SCRATCH + 3), 0);
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
    a8_load(0x0C);
    wr8(dba(SCRATCH + 2), 0x0C);
    wr8(dba(SCRATCH + 3), 0);
    cpu.x = (uint8_t)(rd8(dpa(ACTOR_FACING)) << 1);
    map_at(dba(AHEAD_5) + cpu.x);
}


/*
 * @routine 83:9A46 ActorSeesPlayer
 * WHAT IT DOES: Tells whether the actor sees the player: C clear when the
 * player is within its vertical reach and the map between them is clear,
 * and then it turns to face the player.
 * HOW: The reach test at $81:AEFD (box 0) first. Unless bit 4 of $07 is
 * set, the map is tested every 16 pixels from the actor toward the
 * player, at the actor's height (12 lower when bit 3 of $07 is clear),
 * for as many steps as 16-pixel spans between them; a block there (or no
 * span at all) and it does not see. Seen: ActorSideOfPlayer, as both its
 * facing and its flip.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80. Exit: C clear: seen,
 * A low byte = the facing; C set: not seen. M 8-bit, X 8-bit; $48, $4A,
 * $4C and SCRATCH up as the tests leave them.
 */
void abi_ActorSeesPlayer(void)
{
    uint16_t d, steps;
    uint8_t v;

    x8_load(0x00);
    abi_PlayerInFrontBox();
    if (cpu.p & P_C)
        goto not_seen;
    v = rd8(dpa(ACTOR_07)) & 0x10;
    a8_load(v);
    if (v)                                /* seen through walls */
        goto seen;
    /* each 16 pixels between it and him (at its height, or 12 below)
       must be an empty block */
    op_rep(P_M);
    x8_load(0x00);
    flag_set(P_C, 1);
    d = op_sbc16(dp16(ACTOR_X + 1), rd16(dba(PLAYER + ACTOR_X + 1)), &cpu.p);
    if (d & 0x8000) {
        d = (uint16_t)((d ^ 0xFFFF) + 1);
        x8_load(0x01);
    }
    flag_set(P_C, (d >> 3) & 1);
    steps = (uint16_t)(d >> 4);
    a16_load(steps);
    if (!steps)
        goto not_seen;
    dp16_set(STEPS_LEFT, steps);
    a16_load(0xFFF0);
    cmp8_of((uint8_t)cpu.x, 0x00);
    if (cpu.x)
        a16_load(0x0010);
    dp16_set(ACTOR_48, cpu.a);
    dp16_set(ACTOR_4C, 0);
    do {
        op_rep(0x21);
        cpu.a = op_adc16(dp16(ACTOR_4C), dp16(ACTOR_48), &cpu.p);
        dp16_set(ACTOR_4C, cpu.a);
        flag_set(P_C, 0);
        cpu.a = op_adc16(dp16(ACTOR_X + 1), dp16(ACTOR_4C), &cpu.p);
        wr16(dba(SCRATCH), cpu.a);
        cpu.a = dp16(ACTOR_Y + 1);
        wr16(dba(SCRATCH + 2), cpu.a);
        a16_load(dp16(ACTOR_07) & 0x0008);
        if (!cpu.a) {
            flag_set(P_C, 0);
            cpu.a = op_adc16(dp16(ACTOR_Y + 1), 0x000C, &cpu.p);
            wr16(dba(SCRATCH + 2), cpu.a);
        }
        abi_TileAtProbeLong();
        if (!(cpu.p & P_Z))
            goto not_seen;
    } while (dec8_at(dpa(STEPS_LEFT)));
seen:
    abi_ActorSideOfPlayer();
    wr8(dpa(ACTOR_FACING), (uint8_t)cpu.a);
    wr8(dpa(ACTOR_FLIP), (uint8_t)cpu.a);
    flag_set(P_C, 0);
    op_sep(P_M);
    return;
not_seen:
    flag_set(P_C, 1);
    op_sep(P_M);
}
