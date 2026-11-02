/*
 * kind_8f_94.c: actor kinds $8F (a figure that crosses the screen and
 * back, a song playing, its x kept for other code) and $94 (a spawner that
 * starts one more of its kind at each step of a count). Drafted by
 * cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorEndPlaced(void);
void abi_ActorMoveX(void);
void abi_ActorPictureSend(void);
void abi_ActorSpawn(void);
void abi_ActorSpeedXSet(void);
void abi_AnimStart(void);
void abi_SongPlay(void);

#define V_00A5 0x00A5u  /* the walker's x pixels (word), kept for other code */
#define V_00AD 0x00ADu  /* a count the scene steps; each new value, one more spark */
#define WALK_TIME 0x40u  /* frames left of its crossing */
#define SPARK_STEP 0x40u  /* the spawner: the count it last acted on */
#define SPARK_X 0x41u  /* the spawner: the next one's x pixels */
#define SPARK_Y 0x42u  /* the spawner: the next one's y pixels */
#define KIND8F_29 0x29u  /* word: $890B */
#define SPARK_PLACES 0xD87Cu  /* by count, x and y pixels (bytes), in the bank DB names */

/*
 * @routine 83:FA4F Kind8FWalk
 * @actor
 * WHAT IT DOES: Actor kind $8F: starts off the screen's left edge, waits,
 * plays song $27, then walks right across the screen for $56 frames, comes
 * back left $80 lower for as long, and ends; its x is kept in V_00A5 all
 * the while.
 * HOW: $09 bits $90 set, $29 = $890B, x = V_00A5 = -32; COP $3A; SongPlay
 * $27; COP 6; animation 0, ActorPictureSend, ActorSpeedXSet $5D. $56
 * frames: ActorMoveX, its x pixels to V_00A5. Facing right then: y pixels'
 * low byte = $80, facing and flip 1, again; else ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $8F in state $0C (through the stub at
 * $83:9355). Exit: never returns; ends in ActorEndPlaced.
 */
void abi_Kind8FWalk(void)
{
    uint8_t facing;

    a8_load(0x00);
    tsb_at(dpa(ACTOR_08), 0x00);
    a8_load(0x90);
    tsb_at(dpa(ACTOR_09), 0x90);
    dp16_set(KIND8F_29, 0x890B);
    dp16_set(ACTOR_X + 1, 0xFFE0);        /* from 32 left of the screen */
    wr16(dba(V_00A5), 0xFFE0);
    cpu.a = 0xFFE0;
    a8_load(0x3A);
    actor_wait(0x83FA6Au);
    a8_load(0x27);
    abi_SongPlay();
    a8_load(0x06);
    actor_wait(0x83FA74u);
    a8_load(0x00);
    abi_AnimStart();
    abi_ActorPictureSend();
    y8_load(0x5D);
    abi_ActorSpeedXSet();
    for (;;) {
        /* across for $56 frames, its x kept in V_00A5; then back once, lower */
        a8_load(0x56);
        wr8(dpa(ACTOR_40), 0x56);
        do {
            actor_yield(0x83FA8Au);
            abi_ActorMoveX();
            wr8(dba(V_00A5), rd8(dpa(ACTOR_X + 1)));
            a8_load(rd8(dpa(ACTOR_X + 2)));
            wr8(dba(V_00A5 + 1), (uint8_t)cpu.a);
        } while (dec8_at(dpa(ACTOR_40)));
        facing = rd8(dpa(ACTOR_FACING));
        a8_load(facing);
        if (facing)
            break;
        a8_load(0x80);
        wr8(dpa(ACTOR_Y + 1), 0x80);
        a8_load(0x01);
        wr8(dpa(ACTOR_FACING), 0x01);
        wr8(dpa(ACTOR_FLIP), 0x01);
    }
    abi_ActorEndPlaced();
}


/*
 * @routine 83:FABB Kind94Spark
 * @actor
 * WHAT IT DOES: Actor kind $94. With $07 zero, one spark: shown $20 frames
 * with animation 0, then ended. Else the spawner: each frame V_00AD has a
 * new value, it starts a spark (its own kind, $07 0) at the place
 * SPARK_PLACES gives for it, with its own look.
 * HOW: Spark: $08 bit 0 and $09 bits $90 set, the high bytes of x and y
 * cleared, animation 0, COP $20, ActorEndPlaced. Spawner: $40 cleared;
 * each frame (BRK first) V_00AD compared with $40; new: kept, SPARK_PLACES
 * at twice it to $41 and $42; ActorSpawn of its kind; a record (N clear):
 * its $2B and $2C, x and y pixels' low bytes from $41 and $42, $07 0.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $94 in state $0C (through the stub at
 * $83:9396). Exit: never returns; a spark ends in ActorEndPlaced.
 */
void abi_Kind94Spark(void)
{
    uint8_t count;

    a8_load(rd8(dpa(ACTOR_07)));
    if (!(uint8_t)cpu.a) {                /* a spark: shown $20 frames */
        a8_load(0x01);
        tsb_at(dpa(ACTOR_08), 0x01);
        a8_load(0x90);
        tsb_at(dpa(ACTOR_09), 0x90);
        wr8(dpa(ACTOR_X + 2), 0);
        wr8(dpa(ACTOR_Y + 2), 0);
        a8_load(0x00);
        abi_AnimStart();
        a8_load(0x20);
        actor_wait(0x83FAD3u);
        abi_ActorEndPlaced();
        return;
    }
    /* the spawner: a spark at the place for each new count */
    wr8(dpa(SPARK_STEP), 0);
    for (;;) {
        actor_yield(0x83FADBu);
        count = rd8(dba(V_00AD));
        a8_load(count);
        cmp8_of(count, rd8(dpa(SPARK_STEP)));
        if (count == rd8(dpa(SPARK_STEP)))
            continue;
        wr8(dpa(SPARK_STEP), count);
        cpu.x = (uint8_t)(count << 1);
        flag_set(P_C, count & 0x80);
        wr8(dpa(SPARK_X), rd8(dba(SPARK_PLACES) + cpu.x));
        a8_load(rd8(dba(SPARK_PLACES + 1) + cpu.x));
        wr8(dpa(SPARK_Y), (uint8_t)cpu.a);
        a8_load(rd8(dpa(ACTOR_06)));
        abi_ActorSpawn();
        if (!(cpu.p & P_N)) {
            uint32_t spark = dba(0) + cpu.x;

            a8_load(rd8(dpa(ACTOR_LOOK)));
            wr8(spark + ACTOR_LOOK, (uint8_t)cpu.a);
            a8_load(rd8(dpa(ACTOR_2C)));
            wr8(spark + ACTOR_2C, (uint8_t)cpu.a);
            a8_load(rd8(dpa(SPARK_X)));
            wr8(spark + ACTOR_X + 1, (uint8_t)cpu.a);
            a8_load(rd8(dpa(SPARK_Y)));
            wr8(spark + ACTOR_Y + 1, (uint8_t)cpu.a);
            wr8(spark + ACTOR_07, 0);
        }
        op_sep(P_X);
    }
}
