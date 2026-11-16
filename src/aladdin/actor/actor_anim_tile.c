/*
 * actor_anim_tile.c: a bank $A5 actor's animation chosen by the tile it
 * stands on and its speed.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"
#include "actor_anim.h"
#include "wram_player.h"

void abi_ActorAnimChange(void);

#define TILE_BEHAVIOUR V_0012            /* the tile's behaviour: bit 5 its own animation */
#define SPEED_OFF 0x002Cu                /* its speed less $1C0 (word, data bank) */

/*
 * @routine A5:E9D2 ActorAnimByTile
 * WHAT IT DOES: Picks the actor's animation: $16 on a tile with behaviour
 * bit 5, else by its speed: 0 standing, 1 or 2 moving by the speed's sign
 * once it is far from $1C0, else 1 unless already 2 or 4.
 * HOW: Bit 5 of $0012: ActorAnimChange $16 (A5:EA13). Speed 0:
 * ActorAnimChange 0. Else the speed less $1C0 at $002C: outside -16 to
 * 15, 1 when its high byte is negative, else 2; inside, nothing when the
 * animation ($2D) is 4 or 2, else 1.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; called by JSR. Exit:
 * as ActorAnimChange, or M 8-bit.
 */
void abi_ActorAnimByTile(void)
{
    uint8_t tile = rd8(dba(TILE_BEHAVIOUR)), anim;
    uint16_t speed, off;

    a8_load(tile);
    flag_set(P_Z, !(tile & 0x20));               /* BIT #$20 */
    if (tile & 0x20) {
        a8_load(0x16);
        abi_ActorAnimChange();
        return;
    }
    op_rep(0x20);
    speed = dp16(ACTOR_SPEED_X);
    a16_load(speed);
    if (!speed) {
        op_sep(0x20);
        a8_load(0x00);
        abi_ActorAnimChange();
        return;
    }
    flag_set(P_C, 1);
    off = op_sbc16(speed, 0x01C0, &cpu.p);
    wr16(dba(SPEED_OFF), off);
    flag_set(P_C, 0);
    cpu.a = op_adc16(off, 0x0010, &cpu.p);
    cmp16_of(cpu.a, 0x0020);
    op_sep(0x20);
    if (cpu.a >= 0x0020) {
        /* far from $1C0: 1 below it, 2 above */
        a8_load(0x01);
        x8_load(rd8(dba(SPEED_OFF + 1)));
        if (!(cpu.x & 0x80))
            a8_load(0x02);
        abi_ActorAnimChange();
        return;
    }
    anim = rd8(dpa(ACTOR_ANIM));
    x8_load(0x04);
    cmp8_of(0x04, anim);
    if (anim == 0x04)
        return;
    a8_load(0x01);
    x8_load(0x02);
    cmp8_of(0x02, anim);
    if (anim != 0x02)
        abi_ActorAnimChange();
}
