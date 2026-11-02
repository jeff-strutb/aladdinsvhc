/*
 * spawn_group.c: actors that spawn children around themselves: a ring of
 * five, a pair, a copy of themselves, a child of a set kind.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorSpawnSameLook(void);

/* Spawns a child of kind A at the actor's spawn point when a record is
   free: A = 0 (Z set) when it did, C set and A nonzero when none was. */
void abi_ActorCreateChild8(void);

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
    a8_load(0x05);
    wr8(dpa(ACTOR_40), 0x05);
    x8_load(0x00);
    op_rep(P_M);
    a16_load(dp16(ACTOR_X + 1));           /* its place and $07 kept */
    pha16();
    a16_load(dp16(ACTOR_Y + 1));
    pha16();
    a16_load(dp16(ACTOR_07));
    pha16();
    for (;;) {
        /* the next point of the ring, from the last, and a kind $34 there */
        op_rep(0x21);
        cpu.a = op_adc16(rd16(dba(RING_OFFSETS) + cpu.x), dp16(ACTOR_X + 1), &cpu.p);
        dp16_set(ACTOR_X + 1, cpu.a);
        flag_set(P_C, 0);
        cpu.a = op_adc16(rd16(dba(RING_OFFSETS + 2) + cpu.x), dp16(ACTOR_Y + 1), &cpu.p);
        dp16_set(ACTOR_Y + 1, cpu.a);
        op_sep(P_M);
        wr8(dpa(ACTOR_43), (uint8_t)cpu.x);
        a8_load(0x01);
        wr8(dpa(ACTOR_07), 0x01);
        wr8(dpa(ACTOR_SPAWN_AT), 0);
        a8_load(RING_KIND);
        abi_ActorCreateChild8();
        if (!(cpu.p & P_Z))
            break;
        x8_load((uint8_t)(rd8(dpa(ACTOR_43)) + 4));
        if (!dec8_at(dpa(ACTOR_40)))
            break;
    }
    op_rep(P_M);
    pla16();
    dp16_set(ACTOR_07, cpu.a);
    pla16();
    dp16_set(ACTOR_Y + 1, cpu.a);
    pla16();
    dp16_set(ACTOR_X + 1, cpu.a);
    op_sep(P_M);
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
    uint8_t v;

    inc8_at(dpa(ACTOR_07));
    inc8_at(dpa(ACTOR_07));
    a8_load(0x46);
    wr8(dpa(ACTOR_SPAWN_AT), 0x46);
    a8_load(rd8(dpa(ACTOR_06)));
    abi_ActorSpawnSameLook();
    if (!(cpu.p & P_N)) {                 /* the second, with bit 2 of $07 */
        a8_load(0x04);
        tsb_at(dpa(ACTOR_07), 0x04);
        a8_load(0x48);
        wr8(dpa(ACTOR_SPAWN_AT), 0x48);
        a8_load(rd8(dpa(ACTOR_06)));
        abi_ActorSpawnSameLook();
    }
    v = rd8(dpa(ACTOR_07)) & 0x01;
    a8_load(v);
    wr8(dpa(ACTOR_07), v);
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
    a8_load(0x4E);
    wr8(dpa(ACTOR_SPAWN_AT), 0x4E);
    a8_load(0x5E);
    abi_ActorCreateChild8();
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
    a8_load(0x08);
    tsb_at(dpa(ACTOR_07), 0x08);          /* the copy's $07 has bit 3 */
    a8_load(0x00);
    wr8(dpa(ACTOR_SPAWN_AT), 0x00);
    a8_load(rd8(dpa(ACTOR_06)));
    abi_ActorSpawnSameLook();
    a8_load(0x08);
    trb_at(dpa(ACTOR_07), 0x08);
}
