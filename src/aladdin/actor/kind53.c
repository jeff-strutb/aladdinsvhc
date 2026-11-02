/*
 * kind53.c: actor kind $53, a thing another actor holds, then lets drop:
 * it falls, bounces along the ground and breaks. Drafted by cdraft.py;
 * named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
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

void abi_ActorEndPlaced(void);
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
/* It breaks: animation 8 (with sound $24 when on the screen) for $28
   frames, its kind's live count down one, and it ends. */
static void kind53_break(void)
{
    uint8_t shown;

    a8_load(0x08);
    abi_AnimStart();
    shown = rd8(dpa(ACTOR_09)) & 0x40;
    a8_load(shown);
    if (shown) {
        a8_load(0x24);
        abi_SoundQueueAdd();
    }
    a8_load(0x28);
    wr8(dpa(KIND53_BOUNCES), 0x28);
    do {
        actor_yield(0x83D6C0u);
        abi_AnimStep();
    } while (dec8_at(dpa(KIND53_BOUNCES)));
    op_rep(P_X);
    cpu.x = dp16(KIND53_COUNTER);
    dec8_at(dba(COUNTER_LIVE) + cpu.x);
    op_sep(P_X);
    abi_ActorEndPlaced();
}

/* One bounce's flight: on until it is back on the ground, turned round
   at each wall. */
/* Each frame (BRK at `brk_at` first): flight, turned back from walls,
   until it is in the ground. */
static void kind53_fly(uint32_t brk_at)
{
    for (;;) {
        actor_yield(brk_at);
        abi_ActorFly();
        abi_ActorOutOfWallProbe4();
        if (cpu.p & P_C) {
            a8_load((uint8_t)(rd8(dpa(ACTOR_FACING)) ^ 0x01));
            wr8(dpa(ACTOR_FACING), (uint8_t)cpu.a);
            wr8(dpa(ACTOR_FLIP), (uint8_t)cpu.a);
        }
        abi_ActorOutOfGround();
        if (!(cpu.p & P_Z))
            return;
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
    uint32_t holder;
    uint8_t v;

    a8_load(0x03);
    wr8(dpa(ACTOR_08), 0x03);
    a8_load(0x90);
    wr8(dpa(ACTOR_09), 0x90);
    dp16_set(KIND53_16, 0xD38C);
    dp16_set(ACTOR_LOOK, 0x0600);
    cpu.a = 0x0600;
    wr8(dpa(KIND53_LET_GO), 0);
    a8_load(0x01);
    wr8(dpa(ACTOR_FACING), 0x01);
    wr8(dpa(ACTOR_FLIP), 0x01);
    a8_load(0x07);
    abi_AnimStart();
    /* held: at its holder's hand for the holder's frame, facing its way */
    for (;;) {
        actor_yield(0x83D58Au);
        v = rd8(dpa(KIND53_LET_GO));
        a8_load(v);
        if (v)
            break;
        op_rep(P_X);
        cpu.x = dp16(KIND53_HOLDER);
        holder = dba(0) + cpu.x;
        a8_load(rd8(holder + ACTOR_FACING));
        wr8(dpa(ACTOR_FACING), (uint8_t)cpu.a);
        a8_load(rd8(holder + ACTOR_FLIP));
        wr8(dpa(ACTOR_FLIP), (uint8_t)cpu.a);
        v = rd8(holder + ACTOR_FRAME);
        a8_load(v);
        op_sep(P_X);
        cpu.x = (uint8_t)(v << 2);
        op_rep(P_M);
        dp16_set(ACTOR_X + 1, rd16(dba(KIND53_HELD_AT) + cpu.x));
        dp16_set(ACTOR_Y + 1, rd16(dba(KIND53_HELD_AT + 2) + cpu.x));
        op_rep(0x11);
        cpu.x = dp16(KIND53_HOLDER);
        holder = dba(0) + cpu.x;
        cpu.a = op_adc16(rd16(holder + ACTOR_X + 1), dp16(ACTOR_X + 1), &cpu.p);
        dp16_set(ACTOR_X + 1, cpu.a);
        flag_set(P_C, 0);
        cpu.a = op_adc16(rd16(holder + ACTOR_Y + 1), dp16(ACTOR_Y + 1), &cpu.p);
        dp16_set(ACTOR_Y + 1, cpu.a);
        op_sep(P_M);
        v = rd8(holder + KIND53_LET_GO);
        a8_load(v);
        wr8(dpa(KIND53_LET_GO), v);
        op_sep(P_X);
        if (v)
            break;
    }
    /* let go: it falls to the ground, then bounces 8 times, then breaks */
    a8_load(0x10);
    abi_ActorMotionReset();
    a8_load(rd8(dpa(ACTOR_FLIP)) & 0x01);
    wr8(dpa(ACTOR_FACING), (uint8_t)cpu.a);
    wr8(dpa(ACTOR_FLIP), (uint8_t)cpu.a);
    do {
        actor_yield(0x83D5DEu);
        abi_ActorFall();
        abi_ActorOutOfGround();
    } while (cpu.p & P_Z);
    a8_load(0x30);
    abi_SoundQueueAdd();
    y8_load(0x42);
    abi_ActorMotionPreset();
    kind53_fly(0x83D5F6u);
    a8_load(0x08);
    wr8(dpa(KIND53_BOUNCES), 0x08);
    do {
        v = rd8(dpa(ACTOR_09)) & 0x40;
        a8_load(v);
        if (v) {
            a8_load(0x30);
            abi_SoundQueueAdd();
        }
        y8_load(0x4E);
        abi_ActorSpeedYSet();
        kind53_fly(0x83D626u);
    } while (dec8_at(dpa(KIND53_BOUNCES)));
    a8_load(0x80);
    trb_at(dpa(ACTOR_09), 0x80);
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
    y8_load(0x4B);
    abi_ActorMotionPreset();
    do {
        actor_yield(0x83D69Au);
        abi_AnimStep();
        abi_ActorMove();
        abi_ActorOutOfGround();
    } while (cpu.p & P_Z);
    kind53_break();
}
