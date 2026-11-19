/*
 * kind6c.c: actor kind $6C, a spot that takes Aladdin in: when he comes
 * within reach it gives him the code at $D6F8 of his bank and holds him at
 * its place until he is done, again and again.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "wram_video.h"
#include "actor.h"

void abi_ActorInReachOfPlayer(void);

#define V_145A 0x145Au          /* negative: the spot waits */
#define PLAYER_STATE_8C_COUNT 0x141Bu /* counted up on entering state $8C (nonzero: it waits) */
#define HELD_X 0x145Du          /* where the player is held: x, y (words) */
#define HELD_Y 0x145Fu
#define V_1427 0x1427u          /* nonzero while the player is still taken */
#define PLAYER_TAKEN_CODE 0xD6F8u /* the player's code while a spot has him */
#define LEVEL_10 0x0Au          /* the level where the spot must be hittable ($09 bit 6) to act */

/*
 * @routine 83:E55E Kind6CStart
 * @actor
 * WHAT IT DOES: Actor kind $6C: each frame, unless V_145A is negative, a
 * state $8C is counted, or (in level $0A) it is not hittable, it checks
 * whether Aladdin is in reach; when he is, his code goes to $D6F8, the
 * spot's place is kept for him, and the next frame he is put there; once
 * V_1427 is 0 again it waits 16 frames and starts over.
 * HOW: ACTOR_09 bit 7 set. Each frame (BRK): V_145A negative, again;
 * LEVEL $0A with ACTOR_09 bit 6 clear, again; PLAYER_STATE_8C_COUNT
 * nonzero, again; ActorInReachOfPlayer C set, again. The player's code
 * word $D6F8; HELD_X, HELD_Y = its x, y; BRK; the player's x, y = its x,
 * y; BRK until V_1427 is 0; COP 16; over.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $6C
 * in state $0C (the table at $83:8CAB). Exit: never returns.
 */
void abi_Kind6CStart(void)
{
    uint8_t v;

    for (;;) {
        a8_load(0x80);
        tsb_at(dpa(ACTOR_09), 0x80);
        for (;;) {
            actor_yield(0x83E562u);
            v = rd8(dba(V_145A));
            a8_load(v);
            if (v & 0x80)
                continue;
            v = rd8(dba(LEVEL));
            a8_load(v);
            cmp8_of(v, LEVEL_10);
            if (v == LEVEL_10) {
                v = (uint8_t)(rd8(dpa(ACTOR_09)) & 0x40);
                a8_load(v);
                if (!v)
                    continue;
            }
            v = rd8(dba(PLAYER_STATE_8C_COUNT));
            a8_load(v);
            if (v)
                continue;
            jsl_frame(0x83E57Bu);
            abi_ActorInReachOfPlayer();
            cpu.s = (uint16_t)(cpu.s + 3);
            if (!(cpu.p & P_C))
                break;
        }
        /* he is taken: his code, and the spot's place kept for him */
        a8_load(0xF8);
        wr8(dba((uint16_t)(PLAYER + ACTOR_03)), 0xF8);
        a8_load(0xD6);
        wr8(dba((uint16_t)(PLAYER + ACTOR_03 + 1)), 0xD6);
        op_rep(0x20);
        a16_load(dp16(ACTOR_X + 1));
        wr16(dba(HELD_X), cpu.a);
        a16_load(dp16(ACTOR_Y + 1));
        wr16(dba(HELD_Y), cpu.a);
        op_sep(0x20);
        actor_yield(0x83E599u);
        op_rep(0x20);
        a16_load(dp16(ACTOR_X + 1));
        wr16(dba((uint16_t)(PLAYER + ACTOR_X + 1)), cpu.a);
        a16_load(dp16(ACTOR_Y + 1));
        wr16(dba((uint16_t)(PLAYER + ACTOR_Y + 1)), cpu.a);
        op_sep(0x20);
        do {
            actor_yield(0x83E5A9u);
            x8_load(rd8(dba(V_1427)));
        } while ((uint8_t)cpu.x);
        a8_load(0x10);
        actor_wait(0x83E5B2u);
    }
}
