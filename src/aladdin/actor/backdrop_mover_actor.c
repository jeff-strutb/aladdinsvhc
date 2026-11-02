/*
 * backdrop_mover_actor.c: actor kind $7C, the backdrop mover (helpers in
 * backdrop_mover.c), and kind $A3, a shot that steps out of its sender and
 * flies off. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorEndPlaced(void);
void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_AnimStart(void);
void abi_BackdropOffsetStep(void);
void abi_MoverSound(void);

#define MOVER_FRAMES 0x40u  /* word: frames left of its move ($0333) */
#define MOVER_HUM 0x42u  /* frames until its hum is queued again */
#define MOVER_43 0x43u
#define A3_ANIMS 0xD507u  /* by $07, its animation (in the bank DB names) */
#define A3_STEPS 0xD509u  /* by $07, how far across it steps out (words) */

/*
 * @routine 84:9044 BackdropMover
 * @actor
 * WHAT IT DOES: Actor kind $7C: moves the backdrop layer against the play
 * field one way for $0333 frames with a hum every $30, stops with a clunk,
 * waits, and goes back the other way, for good.
 * HOW: Position cleared, $09 = $80, $42 and $43 cleared. Each move: $40 =
 * $0333 (word); each frame (BRK first) $42 down, past zero: MoverSound $3F
 * and $42 = $30; BackdropOffsetStep until it counts $40 out (Z set). Then
 * MoverSound $0D, COP $3F, the facing flipped.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $7C in state $0C (through the stub at
 * $83:928E). Exit: never returns.
 */
void abi_BackdropMover(void)
{
    dp16_set(ACTOR_X, 0);
    dp16_set(ACTOR_X + 2, 0);
    dp16_set(ACTOR_Y + 1, 0);
    a8_load(0x80);
    wr8(dpa(ACTOR_09), 0x80);
    wr8(dpa(MOVER_HUM), 0);
    wr8(dpa(MOVER_43), 0);
    for (;;) {
        /* $0333 frames moving the backdrop its way, humming every $30 */
        a8_load(0x33);
        wr8(dpa(ACTOR_40), 0x33);
        a8_load(0x03);
        wr8(dpa(ACTOR_41), 0x03);
        do {
            actor_yield(0x84905Eu);
            if (dec8_at(dpa(MOVER_HUM)) & 0x80) {
                a8_load(0x3F);
                abi_MoverSound();
                a8_load(0x30);
                wr8(dpa(MOVER_HUM), 0x30);
            }
            abi_BackdropOffsetStep();
        } while (!(cpu.p & P_Z));
        /* its stop's sound, a wait, then the other way */
        a8_load(0x0D);
        abi_MoverSound();
        a8_load(0x3F);
        actor_wait(0x849079u);
        a8_load((uint8_t)(rd8(dpa(ACTOR_FACING)) ^ 0x01));
        wr8(dpa(ACTOR_FACING), (uint8_t)cpu.a);
    }
}


/*
 * @routine 84:8349 KindA3Shot
 * @actor
 * WHAT IT DOES: Actor kind $A3: a shot facing right; shown, it waits 8
 * frames, steps out across by A3_STEPS, waits 8 more, then flies off
 * (motion $6C) until it can no longer be hit, and ends.
 * HOW: $08 bit 1, $09 bit 7 set, its look $0400, flip and facing 0 (the
 * look's low byte), animation A3_ANIMS by $07; COP 8; x pixels plus
 * A3_STEPS by $07; COP 8; ActorMotionPreset $6C; each frame ActorFly while
 * bit 6 of $09 is set; then ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $A3 in state $0C (through the stub at
 * $83:9423). Exit: never returns; ends in ActorEndPlaced.
 */
void abi_KindA3Shot(void)
{
    uint8_t variant, flags;

    a8_load((uint8_t)(rd8(dpa(ACTOR_08)) | 0x02));
    wr8(dpa(ACTOR_08), (uint8_t)cpu.a);
    a8_load((uint8_t)(rd8(dpa(ACTOR_09)) | 0x80));
    wr8(dpa(ACTOR_09), (uint8_t)cpu.a);
    dp16_set(ACTOR_LOOK, 0x0400);
    cpu.a = 0x0400;
    wr8(dpa(ACTOR_FLIP), 0);
    variant = rd8(dpa(ACTOR_07));
    x8_load(variant);
    wr8(dpa(ACTOR_FACING), 0x00);         /* A's low byte, 0 */
    a8_load(rd8(dba(A3_ANIMS) + cpu.x));
    abi_AnimStart();
    a8_load(0x08);
    actor_wait(0x84836Du);
    /* it steps out across by its variant's step, then flies */
    cpu.x = (uint8_t)(variant << 1);
    flag_set(P_C, 0);
    cpu.a = op_adc16(rd16(dba(A3_STEPS) + cpu.x), dp16(ACTOR_X + 1), &cpu.p);
    dp16_set(ACTOR_X + 1, cpu.a);
    a8_load(0x08);
    actor_wait(0x848381u);
    y8_load(0x6C);
    abi_ActorMotionPreset();
    do {
        actor_yield(0x848389u);
        abi_ActorFly();
        flags = rd8(dpa(ACTOR_09));
        a8_load(flags);
        flag_set(P_Z, !(flags & 0x40));
    } while (flags & 0x40);
    abi_ActorEndPlaced();
}
