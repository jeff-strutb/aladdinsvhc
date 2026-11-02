/*
 * actor_steps.c: small steps of some actors' code: rising after a delay,
 * a height that follows the animation's frame, a flag each of eight
 * actors sets once, and a test picked by X.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorAccelerateY(void);
void abi_ActorStepYBack(void);

/* In bank $80 (DB): the y change for each frame of the animation
   (words), by ACTOR_FRAME. */
#define FRAME_Y_STEPS 0xD673u
/* In bank $80 (DB): a bit mask for each of 8 (by the low 3 bits of $07). */
#define BIT_MASKS 0xD316u
#define V_0359 0x000359u          /* eight flags, set by the actors that use these */
/* Jump tables in the code's bank: 4 cases by bits 3-4 of $07; the tests
   picked by X. */
#define FLAG_CASES 0xCF29u
#define TESTS_BY_X 0xFBA9u

/*
 * @routine 83:A17A ActorRiseAfterDelay
 * WHAT IT DOES: Counts its delay ($47, a word) down; once it is out, the
 * actor moves up by its vertical speed, and the speed grows by its
 * gravity while its whole part is below 2.
 * HOW: DEC on the word while it is not 0; else ActorStepYBack, then
 * ActorAccelerateY when the speed's whole low byte is below 2.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: counting: M 8-bit; A low byte = $47 OR $48 before; N
 * and Z from the decrement. Else as ActorStepYBack (speed 2 up: A = the
 * speed's whole low byte, C set, N and Z from its compare with 2) or
 * ActorAccelerateY leaves them. X, Y unchanged.
 */
void abi_ActorRiseAfterDelay(void)
{
    uint8_t delay = (uint8_t)(rd8(dpa(ACTOR_47)) | rd8(dpa(ACTOR_48))), speed;

    a8_load(delay);
    if (delay) {                          /* still waiting: the count down */
        uint16_t n = (uint16_t)(dp16(ACTOR_47) - 1);
        dp16_set(ACTOR_47, n);
        nz16_of(n);
        return;
    }
    abi_ActorStepYBack();
    speed = rd8(dpa(ACTOR_SPEED_Y + 1));
    a8_load(speed);
    cmp8_of(speed, 0x02);
    if (speed < 0x02)                     /* speeding up to 2 pixels a frame */
        abi_ActorAccelerateY();
}

/*
 * @routine 83:F031 ActorYByFrame
 * WHAT IT DOES: When the actor's animation frame has changed, moves it up
 * or down by that frame's step, so its height follows the animation.
 * HOW: The frame is compared with the one kept in $49; when new, the
 * word of the frame-steps table (bank $80, $D673) by it is added to the
 * y pixels; then $49 = the frame.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80. Exit: Y = the frame;
 * same frame: Z set, C set, the rest unchanged. New: M 8-bit; A = the new
 * y pixels (16 bits); X = frame * 2; N, V, Z, C from the addition.
 */
void abi_ActorYByFrame(void)
{
    uint8_t frame = rd8(dpa(ACTOR_FRAME)), was = rd8(dpa(ACTOR_49));

    y8_load(frame);
    cmp8_of(frame, was);
    if (frame != was) {                   /* a new frame: its y step */
        cpu.x = (uint8_t)(frame << 1);
        a8_load((uint8_t)cpu.x);
        flag_set(P_C, 0);
        cpu.a = op_adc16(dp16(ACTOR_Y + 1), rd16(dba(FRAME_Y_STEPS) + cpu.x), &cpu.p);
        dp16_set(ACTOR_Y + 1, cpu.a);
    }
    wr8(dpa(ACTOR_49), frame);
}



