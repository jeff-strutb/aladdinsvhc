/*
 * apple_throw.c: Aladdin throws an apple: a shot (kind $10) from the
 * second pool, from his hand by his pose, one apple fewer. Drafted by
 * cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"
#include "wram_player.h"
#include "wram_video.h"

void abi_ActorAllocBLong(void);
void abi_ActorStopX(void);
void abi_SoundQueueAdd(void);

#define APPLES 0x0369u  /* apples left, two decimal digits */
#define V_0389 0x0389u  /* zero: no apple is thrown */
#define THROW_X 0xC718u  /* by pose (the wanted box, low two bits), word: the hand's x from the player's, along his facing */
#define THROW_Y 0xC720u  /* by pose, word: the hand's height above his y */
#define THROW_COURSE 0xC728u  /* by pose, word (low byte used): the shot's course ($07) */

/*
 * @routine 81:CDE9 PlayerAppleThrow
 * WHAT IT DOES: Throws an apple when one can be thrown: a shot (kind $10)
 * starts at Aladdin's hand for his pose, on the course the pose gives,
 * facing his way; one apple fewer, sound 3, and his horizontal motion
 * stopped unless $141C is set. C set when thrown.
 * HOW: V_0389 zero, APPLES zero, LEVEL $25, or no free record
 * (ActorAllocBLong, N set): out with C clear (the clc at $81:CDE7). Else
 * the record: state $0C, facing and flip his flip; the pose
 * (PLAYER_BOX_WANTED and 3) picks THROW_X (added to his x, or taken when
 * his flip is 0), THROW_Y (taken from his y) and THROW_COURSE (its $07);
 * kind $10. APPLES down by one in decimal (a low digit 0 takes 7 off);
 * SoundQueueAdd 3; V_141C zero: ActorStopX. C set.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $81; called by
 * JSR from the player's code. Exit: M 8-bit, X 8-bit; C set when thrown,
 * clear when not.
 */
void abi_PlayerAppleThrow(void)
{
    uint32_t apple;
    uint16_t pose, x, y;
    uint8_t v, facing, apples;

    /* none when throwing is off, no apples are left, or in level $25 */
    v = rd8(dba(V_0389));
    a8_load(v);
    if (!v)
        goto none;
    v = rd8(dba(APPLES));
    a8_load(v);
    if (!v)
        goto none;
    v = rd8(dba(LEVEL));
    a8_load(v);
    cmp8_of(v, 0x25);
    if (v == 0x25)
        goto none;
    abi_ActorAllocBLong();
    if (cpu.p & P_N)
        goto none;
    /* the apple starts at his hand for his pose, going his way */
    apple = dba(0) + cpu.x;
    a8_load(0x0C);
    wr8(apple + ACTOR_STATE, 0x0C);
    facing = rd8(dpa(ACTOR_FLIP));
    a8_load(facing);
    wr8(apple + ACTOR_FACING, facing);
    wr8(apple + ACTOR_FLIP, facing);
    op_rep(P_M);
    pose = rd16(dba(PLAYER_BOX_WANTED)) & 0x0003;
    cpu.a = pose;
    pha16();
    cpu.y = (uint16_t)(pose << 1);
    if (dp16(ACTOR_FLIP) & 0x00FF) {
        flag_set(P_C, 0);
        x = op_adc16(dp16(ACTOR_X + 1), rd16(dba(THROW_X) + cpu.y), &cpu.p);
    } else {
        flag_set(P_C, 1);
        x = op_sbc16(dp16(ACTOR_X + 1), rd16(dba(THROW_X) + cpu.y), &cpu.p);
    }
    wr16(apple + ACTOR_X + 1, x);
    flag_set(P_C, 1);
    y = op_sbc16(dp16(ACTOR_Y + 1), rd16(dba(THROW_Y) + cpu.y), &cpu.p);
    wr16(apple + ACTOR_Y + 1, y);
    cpu.a = y;
    op_sep(P_M);
    ply16();
    a8_load(0x10);
    wr8(apple + ACTOR_06, 0x10);
    v = rd8(dba(THROW_COURSE) + cpu.y);
    a8_load(v);
    wr8(apple + ACTOR_07, v);
    op_sep(P_X);
    /* one apple fewer (two decimal digits) */
    apples = rd8(dba(APPLES));
    a8_load(apples);
    pha8();
    nz8_of(apples & 0x0F);
    pla8();
    if (apples & 0x0F) {
        a8_load((uint8_t)(apples - 1));
    } else {
        flag_set(P_C, 1);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | op_sbc8(apples, 0x07, &cpu.p));
    }
    wr8(dba(APPLES), (uint8_t)cpu.a);
    a8_load(0x03);
    abi_SoundQueueAdd();
    v = rd8(dba(V_141C));
    a8_load(v);
    if (!v)
        abi_ActorStopX();
    flag_set(P_C, 1);
    return;
none:
    flag_set(P_C, 0);
}
