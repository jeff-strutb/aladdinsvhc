/*
 * player_state.c: putting Aladdin into state $8C, the state the bank-$83
 * player code enters from several others.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_ActorSideOfPlayer(void);

/* Counted by the state changes below; their purpose is not yet
   established. */
#define V_0347 0x000347u           /* bit 1 set on entering state $8C */
#define V_0367 0x000367u           /* counted down on entering state $8C */
#define V_141B 0x00141Bu           /* counted up on entering state $8C */
#define V_1424 0x001424u

/* LDA #$8C, STA state; INC $141B; DEC $0367; bit 7 of the player's byte
   at $08 and bit 1 of $0347 set: A low the new $0347, N and Z from it. */
static void enter_8c(void)
{
    uint8_t a;

    wr8(PLAYER + ACTOR_STATE, 0x8C);
    wr8(V_141B, (uint8_t)(rd8_rmw(V_141B) + 1));
    wr8(V_0367, (uint8_t)(rd8_rmw(V_0367) - 1));
    wr8(PLAYER + ACTOR_08, (uint8_t)(rd8(PLAYER + ACTOR_08) | 0x80));
    a = (uint8_t)(rd8(V_0347) | 0x02);
    wr8(V_0347, a);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = nz8(cpu.p, a);
}

/*
 * @routine 83:8008 PlayerEnter8C
 * WHAT IT DOES: Puts Aladdin into state $8C.
 * HOW: The state byte, then $141B counted up and $0367 down, bit 7 of the
 * player's byte at $08 and bit 1 of $0347 set.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: A low =
 * the new $0347; N and Z from it. A high, X, Y and the other flags
 * unchanged.
 */
void abi_PlayerEnter8C(void)
{
    enter_8c();
}

/*
 * @routine 83:8024 PlayerEnter8CFacing
 * WHAT IT DOES: Unless bit 7 of the player's byte at $08 is already set,
 * puts Aladdin into state $8C turned away from the actor (D).
 * HOW: As PlayerEnter8C, then ActorSideOfPlayer's facing, flipped, is
 * the player's.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: already set: A low = the byte, N set, Z clear. Entered:
 * A low = 0, N clear, Z set; X = ActorSideOfPlayer's facing; C from its
 * compare. A high = the player's x high byte. Y and the other flags
 * unchanged.
 */
void abi_PlayerEnter8CFacing(void)
{
    uint8_t a = rd8(PLAYER + ACTOR_08);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = nz8(cpu.p, a);
    if (a & 0x80)
        return;
    enter_8c();
    abi_ActorSideOfPlayer();
    a = (uint8_t)(((uint8_t)cpu.a & 0x01) ^ 0x01);
    wr8(PLAYER + ACTOR_FACING, a);
    cpu.a &= 0xFF00;
    cpu.p = nz8(cpu.p, 0);
}

/*
 * @routine 83:8056 PlayerEnter8CFrom1
 * WHAT IT DOES: Puts Aladdin into state $8C with $0367 set to 1 first
 * (so it ends at 0) and $1424 counted up.
 * HOW: The two stores, then on into PlayerEnter8C.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: as
 * PlayerEnter8C.
 */
void abi_PlayerEnter8CFrom1(void)
{
    wr8(V_0367, 0x01);
    wr8(V_1424, (uint8_t)(rd8_rmw(V_1424) + 1));
    enter_8c();
}
