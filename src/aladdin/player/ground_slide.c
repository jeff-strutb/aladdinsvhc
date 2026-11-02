/*
 * ground_slide.c: ground that slides Aladdin along it: tiles whose
 * behaviour has bit 4 set, in the levels where the slide replaces the push
 * (ground_push.c). Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorStepX(void);
void abi_GroundBehindBit4(void);

#define GROUND_PUSHED 0x1429u  /* 2 while the player stands on ground that pushes, 1 while he slides on it, else 0 */

/*
 * @routine 81:CEF4 PlayerGroundSlide
 * WHAT IT DOES: On ground that slides (tile behaviour bit 4), turns
 * Aladdin the way the tile's bit 7 says and moves him along it by half a
 * pixel a frame, marking him sliding.
 * HOW: GROUND_PUSHED cleared; GroundBehindBit4 with probe $18: zero, out
 * (the RTS at $81:CEF3). Else facing = 1 (bit 7 clear, C clear) or 0;
 * GROUND_PUSHED = 1; speed x = $80 (24-bit); ActorStepX.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from the player's code (81:CD0A). Exit: as ActorStepX, or as
 * GroundBehindBit4 with Z set.
 */
void abi_PlayerGroundSlide(void)
{
    wr8(dba(GROUND_PUSHED), 0);
    a8_load(0x18);
    wr8(dpa(ACTOR_18), 0x18);
    abi_GroundBehindBit4();
    if (cpu.p & P_Z)
        return;
    /* pushing ground: he slides at half a pixel a frame, its way */
    if (cpu.p & P_C) {
        wr8(dpa(ACTOR_FACING), 0);
    } else {
        a8_load(0x01);
        wr8(dpa(ACTOR_FACING), 0x01);
    }
    a8_load(0x01);
    wr8(dba(GROUND_PUSHED), 0x01);
    a8_load(0x80);
    wr8(dpa(ACTOR_SPEED_X), 0x80);
    wr8(dpa(ACTOR_SPEED_X + 1), 0);
    wr8(dpa(ACTOR_SPEED_X + 2), 0);
    abi_ActorStepX();
}
