/*
 * ground_push_marks.c: actor kind $60 (and $5F in state $4C): the mark at
 * Aladdin's feet while the ground pushes him or he slides on it, shown
 * only then. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_AnimStart(void);

#define GROUND_PUSHED 0x1429u  /* 2 while the player stands on ground that pushes, 1 while he slides on it, else 0 */
#define PLAYER_X 0x08E7u  /* the player's x pixels (word) */
#define PLAYER_Y 0x08EAu  /* the player's y pixels (word) */
#define MARK_ANIM 0x41u  /* the animation it shows */
#define MARK_ANIM_WANTED 0x40u  /* the animation it is to show */

/*
 * @routine 83:E480 GroundPushMark
 * @actor
 * WHAT IT DOES: Actor kind $60 started new (and kind $5F in state $4C): a
 * mark that follows Aladdin's feet (his x, 24 below his y) for good; shown
 * animated only while GROUND_PUSHED is set: animation 9 while the ground
 * pushes him (2), $0A while he slides (1).
 * HOW: $09 bits $90 set, its look $0600; $40 and $41 = $0A, AnimStart $0A;
 * $08 bit 0 set, bit 3 (animated) cleared. Each frame (BRK first): its
 * position from the player's; GROUND_PUSHED zero: bit 3 of $08 cleared;
 * else set, the animation wanted 9 (2) or $0A (1), started when it is not
 * the one shown ($41).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $60 in state $0C and kind $5F in state
 * $4C. Exit: never returns.
 */
void abi_GroundPushMark(void)
{
    uint8_t pushed, anim;

    wr8(dpa(ACTOR_09), (uint8_t)(rd8(dpa(ACTOR_09)) | 0x90));
    dp16_set(ACTOR_LOOK, 0x0600);
    cpu.a = 0x0600;
    a8_load(0x0A);
    wr8(dpa(MARK_ANIM_WANTED), 0x0A);
    wr8(dpa(MARK_ANIM), 0x0A);
    abi_AnimStart();
    a8_load((uint8_t)((rd8(dpa(ACTOR_08)) | 0x01) & 0xF7));
    wr8(dpa(ACTOR_08), (uint8_t)cpu.a);
    for (;;) {
        actor_yield(0x83E4A1u);
        /* at the player's feet; shown (bit 3) while the ground pushes him */
        dp16_set(ACTOR_X + 1, rd16(dba(PLAYER_X)));
        flag_set(P_C, 0);
        cpu.a = op_adc16(rd16(dba(PLAYER_Y)), 0x0018, &cpu.p);
        dp16_set(ACTOR_Y + 1, cpu.a);
        pushed = rd8(dba(GROUND_PUSHED));
        a8_load(pushed);
        if (!pushed) {
            a8_load((uint8_t)(rd8(dpa(ACTOR_08)) & 0xF7));
            wr8(dpa(ACTOR_08), (uint8_t)cpu.a);
            continue;
        }
        a8_load((uint8_t)(rd8(dpa(ACTOR_08)) | 0x08));
        wr8(dpa(ACTOR_08), (uint8_t)cpu.a);
        anim = pushed == 1 ? 0x0A : 0x09;   /* sliding, pushed */
        a8_load(anim);
        wr8(dpa(MARK_ANIM_WANTED), anim);
        cmp8_of(anim, rd8(dpa(MARK_ANIM)));
        if (anim == rd8(dpa(MARK_ANIM)))
            continue;
        wr8(dpa(MARK_ANIM), anim);
        abi_AnimStart();
    }
}
