/*
 * kind53.c: actor kind $53, a thing another actor holds, then lets drop:
 * it falls, bounces along the ground and breaks. Drafted by cdraft.py;
 * named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorFall(void);
void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_ActorMotionReset(void);
void abi_ActorMove(void);
void abi_ActorOutOfGround(void);
void abi_ActorOutOfWallProbe4(void);
void abi_ActorSpeedYSet(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);

#define ACTOR_END_PLACED 0x8396CAu
#define KIND53_16 0x16u           /* word: $D38C */
#define KIND53_LET_GO 0x45u       /* nonzero: its holder has let it go (copied from the holder's) */
#define KIND53_BOUNCES 0x40u      /* bounces left; then frames left breaking */
#define KIND53_COUNTER 0x4Cu      /* word: the record counting its kind's live ones at $4B */
#define KIND53_HOLDER 0x4Eu       /* word: the record of the actor holding it */
#define COUNTER_LIVE 0x4Bu        /* in the counting record: how many are live */
/* By its holder's animation frame, 4 bytes each: where it is held, x and
   y pixels from the holder's (words). */
#define KIND53_HELD_AT 0xD394u

/* The break: its breaking animation (sound $24 when it can be hit), $28
   frames of it, one less live in the counting record, and its end. */
static void kind53_break(void)
{
    lda8_v(0x08);
    abi_AnimStart();
    lda8(ea_dp(ACTOR_09));
    and8_v(0x40);
    if (IF_Z) goto quiet;
    lda8_v(0x24);
    abi_SoundQueueAdd();
quiet:
    lda8_v(0x28);
    sta8(ea_dp(KIND53_BOUNCES));
breaking:
    actor_yield(0x83D6C0u);
    abi_AnimStep();
    MEM8(dec, ea_dp(KIND53_BOUNCES));
    if (!IF_Z) goto breaking;
    op_rep(0x10);
    ldx16(ea_dp(KIND53_COUNTER));
    MEM8(dec, ea_absx(COUNTER_LIVE));
    op_sep(0x10);
    jump_routine(ACTOR_END_PLACED);
}

/* One bounce's flight: on until it is back on the ground, turned round
   at each wall. */
static void kind53_fly(uint32_t brk_at)
{
    for (;;) {
        actor_yield(brk_at);
        abi_ActorFly();
        abi_ActorOutOfWallProbe4();
        if (IF_C) {
            lda8(ea_dp(ACTOR_FACING));
            eor8_v(0x01);
            sta8(ea_dp(ACTOR_FACING));
            sta8(ea_dp(ACTOR_FLIP));
        }
        abi_ActorOutOfGround();
        if (!IF_Z) return;
    }
}

/*
 * @routine 83:D566 Kind53Held
 * @actor
 * WHAT IT DOES: Actor kind $53 started new: held by another actor, at the
 * place its holder's animation frame gives, until the holder lets it go;
 * then it falls to the ground, bounces 8 times along it (turned round at
 * walls) and breaks.
 * HOW: $08 = 3, $09 = $90, $16 = $D38C, its look $0600, facing left,
 * animation 7. Each frame while $45 is 0: the holder's ($4E) facing and
 * $45 copied, its position the holder's plus the offset in $83:D394 by
 * the holder's frame. Let go: ActorMotionReset ($10), facing the flip's
 * bit 0; ActorFall each frame to the ground; sound $30, ActorMotionPreset
 * $42 and a flight; then 8 times: sound $30 when it can be hit,
 * ActorSpeedYSet $4E and a flight. Bit 7 of $09 cleared, then the break
 * Kind53Thrown ends in.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for state $0C. Exit: never returns; ends in
 * ActorEndPlaced.
 */
void abi_Kind53Held(void)
{
    lda8_v(0x03);
    sta8(ea_dp(ACTOR_08));
    lda8_v(0x90);
    sta8(ea_dp(ACTOR_09));
    op_rep(0x20);
    lda16_v(0xD38C);
    sta16(ea_dp(KIND53_16));
    lda16_v(0x0600);
    sta16(ea_dp(ACTOR_LOOK));
    op_sep(0x20);
    stz8(ea_dp(KIND53_LET_GO));
    lda8_v(0x01);
    sta8(ea_dp(ACTOR_FACING));
    sta8(ea_dp(ACTOR_FLIP));
    lda8_v(0x07);
    abi_AnimStart();
held:
    actor_yield(0x83D58Au);
    lda8(ea_dp(KIND53_LET_GO));
    if (!IF_Z) goto let_go;
    op_rep(0x10);
    ldx16(ea_dp(KIND53_HOLDER));
    lda8(ea_absx(ACTOR_FACING));
    sta8(ea_dp(ACTOR_FACING));
    lda8(ea_absx(ACTOR_FLIP));
    sta8(ea_dp(ACTOR_FLIP));
    lda8(ea_absx(ACTOR_FRAME));
    op_sep(0x10);
    ACC8(asl);
    ACC8(asl);
    tax8();
    op_rep(0x20);
    lda16(ea_absx(KIND53_HELD_AT));
    sta16(ea_dp(ACTOR_X + 1));
    lda16(ea_absx(KIND53_HELD_AT + 2));
    sta16(ea_dp(ACTOR_Y + 1));
    op_rep(0x11);
    ldx16(ea_dp(KIND53_HOLDER));
    lda16(ea_absx(ACTOR_X + 1));
    adc16_v(rd16(ea_dp(ACTOR_X + 1)));
    sta16(ea_dp(ACTOR_X + 1));
    clc();
    lda16(ea_absx(ACTOR_Y + 1));
    adc16_v(rd16(ea_dp(ACTOR_Y + 1)));
    sta16(ea_dp(ACTOR_Y + 1));
    op_sep(0x20);
    lda8(ea_absx(KIND53_LET_GO));
    sta8(ea_dp(KIND53_LET_GO));
    op_sep(0x10);
    if (IF_Z) goto held;
let_go:
    lda8_v(0x10);
    abi_ActorMotionReset();
    lda8(ea_dp(ACTOR_FLIP));
    and8_v(0x01);
    sta8(ea_dp(ACTOR_FACING));
    sta8(ea_dp(ACTOR_FLIP));
falling:
    actor_yield(0x83D5DEu);
    abi_ActorFall();
    abi_ActorOutOfGround();
    if (IF_Z) goto falling;
    lda8_v(0x30);
    abi_SoundQueueAdd();
    ldy8_v(0x42);
    abi_ActorMotionPreset();
    kind53_fly(0x83D5F6u);
    lda8_v(0x08);
    sta8(ea_dp(KIND53_BOUNCES));
bounce:
    lda8(ea_dp(ACTOR_09));
    and8_v(0x40);
    if (IF_Z) goto silent;
    lda8_v(0x30);
    abi_SoundQueueAdd();
silent:
    ldy8_v(0x4E);
    abi_ActorSpeedYSet();
    kind53_fly(0x83D626u);
    MEM8(dec, ea_dp(KIND53_BOUNCES));
    if (!IF_Z) goto bounce;
    lda8_v(0x80);
    trb8(ea_dp(ACTOR_09));
    kind53_break();
}

/*
 * @routine 83:D694 Kind53Thrown
 * @actor
 * WHAT IT DOES: Actor kind $53 started in state $4C: thrown; it flies
 * until it reaches the ground, then breaks.
 * HOW: ActorMotionPreset $4B; each frame AnimStep and ActorMove until
 * ActorOutOfGround. The break: animation 8, sound $24 when it can be hit,
 * $28 frames of animation, the count at $4B of the record at $4C down by
 * one, then ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for state $4C. Exit: never returns; ends in
 * ActorEndPlaced.
 */
void abi_Kind53Thrown(void)
{
    ldy8_v(0x4B);
    abi_ActorMotionPreset();
flying:
    actor_yield(0x83D69Au);
    abi_AnimStep();
    abi_ActorMove();
    abi_ActorOutOfGround();
    if (IF_Z) goto flying;
    kind53_break();
}
