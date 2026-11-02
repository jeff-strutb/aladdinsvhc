/*
 * spawn_group.c: actors that spawn children around themselves: a ring of
 * five, a pair, a copy of themselves, a child of a set kind.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorSpawnSameLook(void);

/* Spawns a child of kind A at the actor's spawn point when a record is
   free: A = 0 (Z set) when it did, C set and A nonzero when none was. */
#define SPAWN_KIND 0x81A9B1u
/* In bank $80 (DB): five x and y offsets (words, 4 bytes an entry) the
   ring's children are spawned at, each from the one before. */
#define RING_OFFSETS 0xD36Eu
#define RING_KIND 0x34
#define ACTOR_43 0x43u            /* here: the ring entry's offset */





/*
 * @routine 83:D0A3 ActorSpawnRing
 * WHAT IT DOES: Spawns up to five children of kind $34 in a ring of
 * points around the actor, each with $07 = 1, at spawn point 0 of its
 * point; stops at the first that finds no record free.
 * HOW: The actor's x, y and $07 (words) are kept on the stack; for each
 * entry of the ring table its offsets are added to x and y, $07 = 1 and
 * spawn point 0, and the spawn at $81:A9B1 made; then they are put back.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80. Exit: M 8-bit, X
 * 8-bit; A = the actor's x pixels (16 bits); $40 = the children not
 * spawned (0 when all five were); $43 = the last entry's offset; $21 = 0;
 * N and Z from x; X = the next entry's offset when all five were
 * spawned, else as the spawn that failed leaves it; Y as the spawns
 * leave it.
 */
void abi_ActorSpawnRing(void)
{
    lda8_v(0x05);
    sta8(ea_dp(ACTOR_40));
    ldx8_v(0x00);
    op_rep(0x20);
    lda16(ea_dp(ACTOR_X + 1));
    pha16();
    lda16(ea_dp(ACTOR_Y + 1));
    pha16();
    lda16(ea_dp(ACTOR_07));
    pha16();
next:
    op_rep(0x21);
    lda16(ea_absx(RING_OFFSETS));
    adc16_v(rd16(ea_dp(ACTOR_X + 1)));
    sta16(ea_dp(ACTOR_X + 1));
    clc();
    lda16(ea_absx(RING_OFFSETS + 2));
    adc16_v(rd16(ea_dp(ACTOR_Y + 1)));
    sta16(ea_dp(ACTOR_Y + 1));
    op_sep(0x20);
    stx8(ea_dp(ACTOR_43));
    lda8_v(0x01);
    sta8(ea_dp(ACTOR_07));
    stz8(ea_dp(ACTOR_SPAWN_AT));
    lda8_v(RING_KIND);
    call_routine(SPAWN_KIND, 1);
    if (!IF_Z) goto done;
    ldx8(ea_dp(ACTOR_43));
    inx8();
    inx8();
    inx8();
    inx8();
    MEM8(dec, ea_dp(ACTOR_40));
    if (!IF_Z) goto next;
done:
    op_rep(0x20);
    pla16();
    sta16(ea_dp(ACTOR_07));
    pla16();
    sta16(ea_dp(ACTOR_Y + 1));
    pla16();
    sta16(ea_dp(ACTOR_X + 1));
    op_sep(0x20);
}
/*
 * @routine 83:D332 ActorSpawnPair
 * WHAT IT DOES: Spawns two children that look like the actor, of its own
 * hit box: at spawn point $46, then (if the first found a record) at $48
 * with bit 2 of $07 set; the children take $07 + 2 as theirs. The actor's
 * $07 keeps only bit 0 after.
 * HOW: $07 up by 2 for the children; ActorSpawnSameLook twice; then $07
 * masked to bit 0.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: A low byte = $07; N and Z from it; M 8-bit; X and Y as
 * the last spawn leaves them.
 */
void abi_ActorSpawnPair(void)
{
    MEM8(inc, ea_dp(ACTOR_07));
    MEM8(inc, ea_dp(ACTOR_07));
    lda8_v(0x46);
    sta8(ea_dp(ACTOR_SPAWN_AT));
    lda8(ea_dp(ACTOR_06));
    abi_ActorSpawnSameLook();
    if (IF_N) goto done;
    lda8_v(0x04);
    tsb8(ea_dp(ACTOR_07));
    lda8_v(0x48);
    sta8(ea_dp(ACTOR_SPAWN_AT));
    lda8(ea_dp(ACTOR_06));
    abi_ActorSpawnSameLook();
done:
    lda8(ea_dp(ACTOR_07));
    and8_v(0x01);
    sta8(ea_dp(ACTOR_07));
}
/*
 * @routine 83:D7C9 ActorSpawn5E
 * WHAT IT DOES: Spawns a child of kind $5E at spawn point $4E.
 * HOW: The spawn at $81:A9B1.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80. Exit: as the spawn at
 * $81:A9B1 leaves them: spawned, A = 0, Z set, M and X 8-bit; none free,
 * C set.
 */
void abi_ActorSpawn5E(void)
{
    lda8_v(0x4E);
    sta8(ea_dp(ACTOR_SPAWN_AT));
    lda8_v(0x5E);
    call_routine(SPAWN_KIND, 1);
}
/*
 * @routine 83:D7D4 ActorSpawnCopy
 * WHAT IT DOES: Spawns a child that looks like the actor, of its own hit
 * box, at spawn point 0, with bit 3 of $07 set for it.
 * HOW: Bit 3 of $07 is set around ActorSpawnSameLook.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: A low byte = 8; N clear; Z from $07 AND 8 before it was
 * cleared; M 8-bit; X and Y as the spawn leaves them.
 */
void abi_ActorSpawnCopy(void)
{
    lda8_v(0x08);
    tsb8(ea_dp(ACTOR_07));
    lda8_v(0x00);
    sta8(ea_dp(ACTOR_SPAWN_AT));
    lda8(ea_dp(ACTOR_06));
    abi_ActorSpawnSameLook();
    lda8_v(0x08);
    trb8(ea_dp(ACTOR_07));
}
