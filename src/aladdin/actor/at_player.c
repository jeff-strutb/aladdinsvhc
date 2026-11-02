/*
 * at_player.c: an actor kept at Aladdin's place, and actor kind $12,
 * kept there until the player's box is one of three.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram_player.h"

void abi_ActorMotionPreset(void);
void abi_ActorMove(void);
void abi_ActorAtPlayer(void);
void abi_ActorEnd(void);

#define PLAYER_2F (PLAYER + 0x2Fu)  /* picks the motion of an actor kept at his place */
#define AT_PLAYER_MOTIONS 0xCA03u   /* the motion presets, by his $2F (bytes, the DB bank) */
#define KIND12_11 0x11u             /* kind $12: set to $FF at its start */

/*
 * @routine 81:EC02 ActorAtPlayer
 * WHAT IT DOES: Puts the actor at Aladdin's place and moves it by the
 * motion his $2F picks.
 * HOW: His x and y pixels copied into the actor's; the motion preset from
 * the table at $CA03 by his $2F; then ActorMotionPreset and ActorMove.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM, the table's bank; called by JSR. Exit: as ActorMove.
 */
void abi_ActorAtPlayer(void)
{
    uint16_t y = rd16(dba(PLAYER + ACTOR_Y + 1));

    dp16_set(ACTOR_X + 1, rd16(dba(PLAYER + ACTOR_X + 1)));
    dp16_set(ACTOR_Y + 1, y);
    cpu.a = y;
    x8_load(rd8(dba(PLAYER_2F)));
    y8_load(rd8(dba(AT_PLAYER_MOTIONS) + cpu.x));
    abi_ActorMotionPreset();
    abi_ActorMove();
}

/*
 * @routine 81:EBDE Kind12AtPlayer
 * @actor
 * WHAT IT DOES: Actor kind $12: kept at Aladdin's place, facing as he
 * does, until the box his code wants (PLAYER_BOX_WANTED) is 3, 6 or $12;
 * then it ends.
 * HOW: ActorAtPlayer; $11 = $FF; facing and flip = his flip ($08F0).
 * Each frame: ActorAtPlayer, BRK, then PLAYER_BOX_WANTED compared with 3,
 * 6 and $12; on a match ActorEnd (by the JML after ActorAtPlayer, at
 * $81:EC1F).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; reached by
 * the actor loop's jump for kind $12 in state $0C (the table at
 * $83:8CAB, through the stub at $83:921A). Exit: never returns.
 */
void abi_Kind12AtPlayer(void)
{
    uint8_t facing, box;

    abi_ActorAtPlayer();
    wr8(dpa(KIND12_11), 0xFF);
    facing = rd8(dba(PLAYER + ACTOR_FLIP));
    a8_load(facing);
    wr8(dpa(ACTOR_FACING), facing);
    wr8(dpa(ACTOR_FLIP), facing);
    for (;;) {
        abi_ActorAtPlayer();
        actor_yield(0x81EBEFu);
        box = rd8(dba(PLAYER_BOX_WANTED));
        a8_load(box);
        cmp8_of(box, 0x03);
        if (box == 0x03)
            break;
        cmp8_of(box, 0x06);
        if (box == 0x06)
            break;
        cmp8_of(box, 0x12);
        if (box == 0x12)
            break;
    }
    abi_ActorEnd();
}
