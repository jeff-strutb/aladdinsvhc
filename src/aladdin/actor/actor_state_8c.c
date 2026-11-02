/*
 * actor_state_8c.c: state $8C (with bit 7 of the byte at $08 set and of
 * the byte at $09 cleared) for the actor's own record, and
 * ActorStateSet8C for callers in other banks.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
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
    wr8(dpa(ACTOR_09), (uint8_t)(rd8(dpa(ACTOR_09)) & 0x7F));
    flag_set(P_C, 0);
    wr8(dpa(ACTOR_08), (uint8_t)(rd8(dpa(ACTOR_08)) | 0x80));
    a8_load(0x8C);
    wr8(dpa(ACTOR_STATE), 0x8C);
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
