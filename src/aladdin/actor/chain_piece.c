/*
 * chain_piece.c: what the pieces of the nine-piece chain (kinds $74 and
 * $75, behind its head of kind $73) do each frame they wait.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "wram.h"
#include "actor.h"

void abi_AnimStep(void);
void abi_ActorPathStep(void);
void abi_ActorsFollowIf8(void);
void abi_ActorHurtsPlayerOnTouchLong(void);
void abi_ActorTurnsShotBackLong(void);

#define CHAIN_HEAD 0x1B14u          /* the head's record (word; child_chain.c) */
#define CHAIN_UNTOUCHABLE 0x1B2Cu   /* nonzero: no touch tests (the head's K73_UNTOUCHABLE) */
#define V_032D 0x032Du              /* the high byte of the word at $032C */
#define STATE_GONE 0x8Cu            /* the head's state once it is going (and so the pieces') */
#define PIECE_RESUME 0x35u          /* word: where the piece resumes, kept from ACTOR_03 */

/*
 * @routine 84:8A78 ChainPieceWork
 * WHAT IT DOES: What the chain's pieces (kinds $74 and $75) do each frame
 * they wait: animate, follow their path and the pieces after them, and,
 * unless the chain is untouchable, hurt Aladdin on touch on odd counts of
 * V_032D or turn his shots back on even ones; when the head goes into
 * state $8C the piece goes with it; its $08 bits 4 and 7 follow the
 * head's.
 * HOW: AnimStep, ActorPathStep, ActorsFollowIf8; CHAIN_UNTOUCHABLE zero:
 * V_032D bit 0 set ActorHurtsPlayerOnTouchLong, clear
 * ActorTurnsShotBackLong. With X 16-bit = CHAIN_HEAD: the head's state
 * $8C: the piece's state $8C, PIECE_RESUME = its $03 word, $09 bit 7
 * cleared (ASL, LSR). SCRATCH = the head's $08 AND $90; the piece's $08 =
 * its $08 AND $6F OR SCRATCH.
 * Entry: M 8-bit, X 8-bit; D = the piece's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSL from the kinds' stubs ($83:926B,
 * $83:9278) through the actor loop's wait table. Exit: M 8-bit, X 8-bit;
 * A = the new $08, N and Z from it; X = the low byte of CHAIN_HEAD's
 * record; Y as the touch tests left it, or the low byte of $03 when the
 * head went; C clear when the head went, else from comparing $8C with the
 * head's state.
 */
void abi_ChainPieceWork(void)
{
    uint16_t head;
    uint8_t untouchable, state, bits, v;

    abi_AnimStep();
    abi_ActorPathStep();
    abi_ActorsFollowIf8();
    untouchable = rd8(dba(CHAIN_UNTOUCHABLE));
    a8_load(untouchable);
    if (!untouchable) {
        a8_load((uint8_t)(rd8(dba(V_032D)) & 0x01));
        if (cpu.a & 0x01)
            abi_ActorHurtsPlayerOnTouchLong();
        else
            abi_ActorTurnsShotBackLong();
    }

    op_rep(0x10);
    head = rd16(dba(CHAIN_HEAD));
    cpu.x = head;
    a8_load(STATE_GONE);
    state = rd8(dba(head));
    cmp8_of(STATE_GONE, state);
    if (state == STATE_GONE) {
        uint16_t resume = dp16(ACTOR_03);

        wr8(dpa(ACTOR_STATE), STATE_GONE);
        cpu.y = resume;
        dp16_set(PIECE_RESUME, resume);
        wr8(dpa(ACTOR_09), (uint8_t)(rd8(dpa(ACTOR_09)) << 1));
        wr8(dpa(ACTOR_09), (uint8_t)(rd8(dpa(ACTOR_09)) >> 1));
        flag_set(P_C, 0);                   /* the LSR's: bit 0 of a doubled byte */
    }
    bits = (uint8_t)(rd8(dba((uint16_t)(head + ACTOR_08))) & 0x90);
    wr8(dba(SCRATCH), bits);
    v = (uint8_t)((rd8(dpa(ACTOR_08)) & 0x6F) | bits);
    a8_load(v);
    wr8(dpa(ACTOR_08), v);
    op_sep(0x10);
}
