/*
 * actor_fields.c: small settings of an actor's record: its state, and
 * bytes its kind's code reads.
 */
#include "actor/actor_own.h"
#include "data/actor_a_tables.h"

/*
 * @routine 83:824B ActorStateSetCC
 * WHAT IT DOES: Puts the actor into state $CC: its kind's third entry at
 * its next turn.
 */
void ActorStateSetCC(Actor *a)
{
    a->state = 0xCC;
}

/*
 * @routine 83:8267 ActorStateSet8C
 * WHAT IT DOES: Puts the actor into state $8C (its kind's second entry at
 * its next turn), with bit 7 of its byte at $08 set and of its flags
 * cleared.
 */
void ActorStateSet8C(Actor *a)
{
    a->anim_flags |= 0x80;
    a->flags &= 0x7F;
    a->state = 0x8C;
}

/*
 * @routine 83:B61A ActorFallSpeedCap
 * WHAT IT DOES: Limits the actor's falling speed to 6 pixels a frame:
 * while it moves down (the speed's top byte not negative), a speed whose
 * fraction and low pixel byte, as a word, pass $0600 becomes $0600 (its
 * top byte kept) and its gravity is cleared.
 */
void ActorFallSpeedCap(Actor *a)
{
    uint32_t s = (uint32_t)a->speed_y;

    if (a->speed_y < 0)
        return;
    if ((s & 0xFFFF) > 0x0600) {
        a->speed_y = signed24((s & ~0xFFFFu) | 0x0600);   /* its top byte kept */
        a->accel_y = 0;
    }
}

/*
 * @routine A5:EA93 ActorSpeedXFromA5
 * WHAT IT DOES: Sets the actor's speed across from the table at $80:CFB7,
 * entry `at` (a byte offset, 3 bytes an entry).
 */
void ActorSpeedXFromA5(Actor *a, uint8_t at)
{
    a->speed_x = signed24(a5_speed_x(at));
}

/*
 * @routine A5:EAA3 ActorSpeedYFromA5
 * WHAT IT DOES: Sets the actor's speed down from the table at $80:CFCF,
 * entry `at`.
 */
void ActorSpeedYFromA5(Actor *a, uint8_t at)
{
    a->speed_y = signed24(a5_speed_y(at));
}

/*
 * @routine 84:8CD2 ActorParityBitsSet
 * WHAT IT DOES: Sets the low two bits of the actor's +$08 from a two-entry
 * table by whether the frame count is odd or even, so the actor alternates
 * between two settings frame by frame.
 */
void ActorParityBitsSet(Actor *a)
{
    a->anim_flags = (uint8_t)((a->anim_flags & 0xFC) | parity_bits(g.frame_count & 1));
}
