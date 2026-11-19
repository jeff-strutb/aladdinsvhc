/*
 * kind34.c: actor kind $34, a short-lived figure that animates in place
 * or flies off, by its $07, then ends. Drafted by cdraft.py; named and
 * described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorEndPlaced(void);
void abi_ActorFly(void);
void abi_ActorSpeedXClear(void);
void abi_ActorSpeedYSet(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_Kind34Part0(void);
void abi_Kind34Part1(void);
void abi_Kind34Part2(void);

#define KIND34_COUNT (KIND_COUNT + 0x34)  /* kind $34's count of live actors, up as one starts */
#define KIND34_PARTS 0xBBB9u      /* in bank $83: the parts' code, by $07 (words) */
#define K34_FRAMES 0x40u          /* frames left */
#define K34_SPEED_UP 0x09         /* ActorSpeedYSet's entry for part 1 */

/*
 * @routine 83:BB97 Kind34Start
 * @actor
 * WHAT IT DOES: Actor kind $34, a short-lived figure: counts itself in
 * KIND34_COUNT (its kind's count of live actors), starts animation 6,
 * and by its $07 plays one of three parts for 21 frames before it ends:
 * part 0 animates in place, part 1 flies off upward, part 2 ends at
 * once.
 * HOW: KIND34_COUNT up; ACTOR_09 = $10; the palette byte ($2C) = 4;
 * AnimStart 6; K34_FRAMES = $15; then JMP through KIND34_PARTS by $07
 * times 2: Kind34Part0, Kind34Part1, Kind34Part2.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $34
 * in state $0C (the table at $83:8CAB). Exit: never returns.
 */
void abi_Kind34Start(void)
{
    lda8(KIND34_COUNT);
    ACC8(inc);
    sta8(KIND34_COUNT);
    lda8_v(0x10);
    sta8(ea_dp(ACTOR_09));
    lda8_v(0x04);
    sta8(ea_dp(ACTOR_LOOK + 1));
    lda8_v(0x06);
    jsl_frame(0x83BBAAu);
    abi_AnimStart();
    cpu.s = (uint16_t)(cpu.s + 3);
    lda8_v(0x15);
    sta8(ea_dp(K34_FRAMES));
    lda8(ea_dp(ACTOR_07));
    ACC8(asl);
    tax8();
    switch (cpu.x) {
    case 0: abi_Kind34Part0(); break;
    case 2: abi_Kind34Part1(); break;
    case 4: abi_Kind34Part2(); break;
    default: jump_routine(0x830000u | rd16(0x830000u | (uint16_t)(KIND34_PARTS + cpu.x))); break;
    }
}

/*
 * @routine 83:BBBF Kind34Part0
 * @actor
 * WHAT IT DOES: Kind $34's part 0: animates in place for K34_FRAMES
 * frames, then ends.
 * HOW: Each frame (BRK first) AnimStep, K34_FRAMES down; at 0,
 * ActorEndPlaced. No script runs it: written from its bytes.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by a jump from Kind34Start. Exit: never
 * returns.
 */
void abi_Kind34Part0(void)
{
    do {
        actor_yield(0x83BBBFu);
        jsl_frame(0x83BBC1u);
        abi_AnimStep();
        cpu.s = (uint16_t)(cpu.s + 3);
        MEM8(dec, ea_dp(K34_FRAMES));
    } while (!IF_Z);
    abi_ActorEndPlaced();
}

/*
 * @routine 83:BBCD Kind34Part1
 * @actor
 * WHAT IT DOES: Kind $34's part 1: flies off upward, animating, for
 * K34_FRAMES frames, then ends.
 * HOW: ActorSpeedXClear; ActorSpeedYSet with entry 9; ACTOR_ACCEL_Y = 2;
 * each frame (BRK first) AnimStep, ActorFly, K34_FRAMES down; at 0,
 * ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80 (the speed
 * table's bank); reached by a jump from Kind34Start. Exit: never returns.
 */
void abi_Kind34Part1(void)
{
    jsl_frame(0x83BBCDu);
    abi_ActorSpeedXClear();
    cpu.s = (uint16_t)(cpu.s + 3);
    ldy8_v(K34_SPEED_UP);
    jsl_frame(0x83BBD3u);
    abi_ActorSpeedYSet();
    cpu.s = (uint16_t)(cpu.s + 3);
    lda8_v(0x02);
    sta8(ea_dp(ACTOR_ACCEL_Y));
    do {
        actor_yield(0x83BBDBu);
        jsl_frame(0x83BBDDu);
        abi_AnimStep();
        cpu.s = (uint16_t)(cpu.s + 3);
        jsl_frame(0x83BBE1u);
        abi_ActorFly();
        cpu.s = (uint16_t)(cpu.s + 3);
        MEM8(dec, ea_dp(K34_FRAMES));
    } while (!IF_Z);
    abi_ActorEndPlaced();
}

/*
 * @routine 83:BBED Kind34Part2
 * @actor
 * WHAT IT DOES: Kind $34's part 2: ends at once.
 * HOW: JML ActorEndPlaced. No script runs it: written from its bytes.
 * Entry: D = the actor's record; reached by a jump from Kind34Start.
 * Exit: never returns.
 */
void abi_Kind34Part2(void)
{
    abi_ActorEndPlaced();
}
