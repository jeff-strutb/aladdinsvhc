/*
 * kind_a1_a6.c: actor kind $A1's start (it bounces about at random) and
 * its state $4C (hit: it flies off), and kind $A6
 * (it stays until far above or below the point actors are kept in range
 * of). Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorEndPlaced(void);
void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_ActorBouncesOrHurtsPlayerLong(void);
void abi_ActorDownedByShotLong(void);
void abi_SoundQueueAdd(void);
void abi_ActorClampAboveFloor12(void);
void abi_RandomFromList(void);

#define RANGE_Y 0x1A46u  /* the y of the point actors are kept in range of (word) */
#define KIND_A1_HITS 0x41u       /* counted up each time Aladdin comes down on it */
#define KIND_A1_SAFE 0x1B2Bu     /* nonzero: it goes straight to state $8C, untested */
#define ACTOR_END_PLACED 0x8396CAu  /* ActorEndPlaced, entered at the actor loop's level */

/*
 * @routine 84:A9FC KindA1Hit
 * @actor
 * WHAT IT DOES: Actor kind $A1 in state $4C (hit): it flies off (motion
 * $1F), after 8 frames when its $41 is set, until it can no longer be hit,
 * and ends.
 * HOW: Animation 0, $08 bit 4 set; $41 nonzero: COP 8; ActorMotionPreset
 * $1F; each frame ActorFly while bit 6 of $09 is set; then ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $A1 in state $4C (through the stub at
 * $83:9412). Exit: never returns; ends in ActorEndPlaced.
 */
void abi_KindA1Hit(void)
{
    uint8_t wait, flags;

    a8_load(0x00);
    abi_AnimStart();
    wr8(dpa(ACTOR_08), (uint8_t)(rd8(dpa(ACTOR_08)) | 0x10));
    wait = rd8(dpa(ACTOR_41));
    a8_load(wait);
    if (wait) {
        a8_load(0x08);
        actor_wait(0x84AA0Eu);
    }
    /* it falls away on motion $1F until it is off the screen */
    y8_load(0x1F);
    abi_ActorMotionPreset();
    do {
        actor_yield(0x84AA16u);
        abi_ActorFly();
        flags = rd8(dpa(ACTOR_09));       /* BIT: N, V from it, Z from A AND it */
        flag_set(P_N, flags & 0x80);
        flag_set(P_V, flags & 0x40);
        flag_set(P_Z, !((uint8_t)cpu.a & flags));
    } while (flags & 0x40);
    abi_ActorEndPlaced();
}


/*
 * @routine 84:AC3F KindA6Stay
 * @actor
 * WHAT IT DOES: Actor kind $A6: shown (animation 0, flipped by its $07)
 * where it was placed until it is 256 or more pixels above or below the
 * point actors are kept in range of, then ends.
 * HOW: $08 = 3, AnimStart 0, flip = $07; each frame (BRK first) RANGE_Y
 * less its y plus $0100, unsigned below $0200: again; else ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $A6 in state $0C (through the stub at
 * $83:943E). Exit: never returns; ends in ActorEndPlaced.
 */
void abi_KindA6Stay(void)
{
    uint8_t flip;
    uint16_t d;

    a8_load(0x03);
    wr8(dpa(ACTOR_08), 0x03);
    a8_load(0x00);
    abi_AnimStart();
    flip = rd8(dpa(ACTOR_07));
    a8_load(flip);
    wr8(dpa(ACTOR_FLIP), flip);
    op_rep(P_M);
    /* until it is 256 pixels or more above or below the range point */
    do {
        actor_yield(0x84AC4Fu);
        flag_set(P_C, 1);
        d = op_sbc16(rd16(dba(RANGE_Y)), dp16(ACTOR_Y + 1), &cpu.p);
        flag_set(P_C, 0);
        d = op_adc16(d, 0x0100, &cpu.p);
        cpu.a = d;
        cmp16_of(d, 0x0200);
    } while (d < 0x0200);
    op_sep(P_M);
    abi_ActorEndPlaced();
}

#define KIND_A1_LONG 0x40u      /* set once it has taken the long bounce ($6A) */
#define BOUNCE_LONG 0x6Au       /* the long bounce, taken once; after it $68 */
#define BOUNCE_AFTER 0x68u

/*
 * @routine 84:A97F KindA1Start
 * @actor
 * WHAT IT DOES: Actor kind $A1: it bounces about: each time it lands
 * (kept 12 above the floor line), sound $4D and a bounce picked at random
 * from list $20; the long bounce ($6A) only once, then $68 in its place.
 * HOW: ACTOR_08 = 3, ACTOR_09 = $D0, facing 1, KIND_A1_LONG and KIND_A1_HITS 0;
 * AnimStart 0; ActorMotionPreset $6B. Each frame (BRK): ActorFly,
 * ActorClampAboveFloor12; landed (C set): SoundQueueAdd $4D;
 * RandomFromList with X = $40; $6A with KIND_A1_LONG set: $68 ($84:A9B6,
 * which no script meets); $6A else: KIND_A1_LONG up; ActorMotionPreset
 * with it.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; reached by
 * the actor loop's jump for kind $A1 in state $0C (the table at $83:8DAB,
 * through the stub at $83:9409). Exit: never returns.
 */
void abi_KindA1Start(void)
{
    uint8_t pick;

    a8_load(0x03);
    wr8(dpa(ACTOR_08), 0x03);
    a8_load(0xD0);
    wr8(dpa(ACTOR_09), 0xD0);
    a8_load(0x01);
    wr8(dpa(ACTOR_FACING), 0x01);
    wr8(dpa(KIND_A1_LONG), 0);
    wr8(dpa(KIND_A1_HITS), 0);
    a8_load(0x00);
    abi_AnimStart();
    y8_load(0x6B);
    abi_ActorMotionPreset();
    for (;;) {
        actor_yield(0x84A99Bu);
        abi_ActorFly();
        abi_ActorClampAboveFloor12();
        if (!(cpu.p & P_C))
            continue;
        a8_load(0x4D);
        abi_SoundQueueAdd();
        x8_load(0x40);
        abi_RandomFromList();
        pick = (uint8_t)cpu.a;
        cmp8_of(pick, BOUNCE_LONG);
        if (pick == BOUNCE_LONG) {
            x8_load(rd8(dpa(KIND_A1_LONG)));
            if ((uint8_t)cpu.x)
                a8_load(BOUNCE_AFTER);
            else
                (void)inc8_at(dpa(KIND_A1_LONG));
        }
        y8_load((uint8_t)cpu.a);
        abi_ActorMotionPreset();
    }
}


/*
 * @routine 84:A9C7 KindA1Touch
 * WHAT IT DOES: What actors of kind $A1 do each frame they wait: animate;
 * once it can no longer be hit it ends; else Aladdin touching it is hurt,
 * or coming down on it is stood on it, which counts a hit, plays sound $0F
 * and sends it to state $8C; not stood on, a shot can down it. With
 * KIND_A1_SAFE set it goes to state $8C at once.
 * HOW: AnimStep; BIT ACTOR_09: bit 6 clear, its JSL's and the loop's JSR
 * return addresses dropped (five pulls) and on into ActorEndPlaced at the
 * loop's level. KIND_A1_SAFE zero: ActorBouncesOrHurtsPlayerLong, A - 1
 * nonzero: ActorDownedByShotLong and back; zero: KIND_A1_HITS up and
 * SoundQueueAdd $0F. Then bit 7 of ACTOR_09 cleared (ASL, LSR) and state
 * $8C.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSL from the kind's stub ($83:940D) through the
 * actor loop's wait table at $83:904F. Exit: ended: never returns. Shot
 * test: as ActorDownedByShotLong. Else M and X 8-bit; A low = $8C, N set,
 * Z clear, C clear.
 */
void abi_KindA1Touch(void)
{
    uint8_t flags, safe, got = 0;

    abi_AnimStep();
    flags = rd8(dpa(ACTOR_09));
    bit8_of((uint8_t)cpu.a, flags);
    if (!(flags & 0x40)) {
        /* it can no longer be hit: out of its call and the loop's, ended */
        for (int i = 0; i < 5; i++)
            got = op_pull8();
        a8_load(got);
        actor_leave(ACTOR_END_PLACED);
    }
    safe = rd8(dba(KIND_A1_SAFE));
    a8_load(safe);
    if (!safe) {
        abi_ActorBouncesOrHurtsPlayerLong();
        got = (uint8_t)(cpu.a - 1);
        a8_load(got);
        if (got) {
            abi_ActorDownedByShotLong();
            return;
        }
        (void)inc8_at(dpa(KIND_A1_HITS));
        a8_load(0x0F);
        abi_SoundQueueAdd();
    }
    /* to state $8C: bit 7 of $09 cleared, so this work stops */
    flags = (uint8_t)(rd8(dpa(ACTOR_09)) & 0x7F);
    wr8(dpa(ACTOR_09), flags);
    flag_set(P_C, 0);
    a8_load(0x8C);
    wr8(dpa(ACTOR_STATE), 0x8C);
}
