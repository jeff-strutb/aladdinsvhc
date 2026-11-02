/*
 * actor_state_8c.c: state $8C (with bit 7 of the byte at $08 set and of
 * the byte at $09 cleared) for the actor's own record, and
 * ActorStateSet8C for callers in other banks.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorStateSet8C(void);

/*
 * @routine 83:8254 ActorStateSet8CSelf
 * WHAT IT DOES: Puts the actor into state $8C, with bit 7 of its byte at
 * $08 set and of its byte at $09 cleared: ActorStateSet8C for the record
 * the direct page is.
 * HOW: ASL then LSR clears bit 7 of $09; ORA sets bit 7 of $08; then the
 * state byte.
 * Entry: M 8-bit; D = the record. Exit: A low = $8C; N set, Z clear, C
 * clear. A high, X, Y and the other flags unchanged.
 */
void abi_ActorStateSet8CSelf(void)
{
    MEM8(asl, ea_dp(ACTOR_09));
    MEM8(lsr, ea_dp(ACTOR_09));
    lda8(ea_dp(ACTOR_08));
    ora8_v(0x80);
    sta8(ea_dp(ACTOR_08));
    lda8_v(0x8C);
    sta8(ea_dp(ACTOR_STATE));
}

/*
 * @routine 83:8263 ActorStateSet8CLong
 * WHAT IT DOES: ActorStateSet8C for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorStateSet8C; called by JSL. Exit: as ActorStateSet8C.
 */
void abi_ActorStateSet8CLong(void)
{
    abi_ActorStateSet8C();
}
