/*
 * kind_7f.c: actor kind $7F: a backdrop figure that drifts across for
 * good, slower the higher its $07. Drafted by cdraft.py; named and
 * described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorMoveX(void);
void abi_AnimStart(void);

#define KIND7F_16 0x16u  /* word: $D832 */
#define KIND7F_SLOWER 0x40u  /* $07 times 16 */

/*
 * @routine 84:934B Kind7FDrift
 * @actor
 * WHAT IT DOES: Actor kind $7F: shown with the animation $07 picks (bits
 * from $07 also in $08), facing right, it drifts across at $70 less 16
 * times $07 (a fraction of a pixel a frame) for good.
 * HOW: $08 |= 2 | $07; $09 bits $90 set; $16 = $D832; AnimStart $07;
 * facing and flip 0; $40 = $07 * 16; its speed's fraction = $70 - $40;
 * each frame (BRK first) ActorMoveX.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $7F in state $0C (through the stub at
 * $83:92B5). Exit: never returns.
 */
void abi_Kind7FDrift(void)
{
    uint8_t variant = rd8(dpa(ACTOR_07)), slower, speed;

    wr8(dpa(ACTOR_08), (uint8_t)(rd8(dpa(ACTOR_08)) | 0x02 | variant));
    wr8(dpa(ACTOR_09), (uint8_t)(rd8(dpa(ACTOR_09)) | 0x90));
    dp16_set(KIND7F_16, 0xD832);
    cpu.a = 0xD832;
    a8_load(variant);
    abi_AnimStart();
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    /* right at $70 less 16 per variant (fraction pixels a frame), for good */
    slower = (uint8_t)(variant << 4);
    wr8(dpa(KIND7F_SLOWER), slower);
    flag_set(P_C, 1);
    speed = op_sbc8(0x70, slower, &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | speed);
    wr8(dpa(ACTOR_SPEED_X), speed);
    for (;;) {
        actor_yield(0x84937Bu);
        abi_ActorMoveX();
    }
}
