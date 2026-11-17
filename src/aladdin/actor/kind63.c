/*
 * kind63.c: actor kind $63, which watches for Aladdin to come within
 * reach below it and throws at him (two children of hit box $A2 each
 * round), its per-frame work while it waits, and its knock-away when
 * hit (its second state).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_ActorShotHitFace(void);
void abi_ActorStompOrHurtLong(void);
void abi_AnimStep(void);
void abi_ActorPictureSend(void);
void abi_ActorFreeOutOfRangeLong(void);
void abi_AnimStart(void);
void abi_ActorNearPlayer(void);
void abi_ActorSpawnA2(void);
void abi_ActorSpawnPairA3(void);
void abi_ActorPriorityClear(void);
void abi_ActorKnockedAway(void);

#define KIND63_HIT 0x43u     /* nonzero while it shows being hit: no shot test then */
#define KIND63_GONE 0x47u    /* counted up each frame it is out of range */
#define KIND63_OUT 0x42u     /* its throws still flying (ActorSpawnA2 counts them) */
#define KIND63_OUT_HIT 0x46u /* KIND63_OUT as a shot found it (ActorShotHitFace) */
#define KIND63_AIM 0x44u     /* how far below it he is: 2 near, 1 farther, 0 farthest */
#define KIND63_HEALTH 0x11u
#define KIND63_16 0x16u      /* word: $D4E5 */
#define KIND63_28 0x28u      /* $FF */
#define KIND63_29 0x29u      /* word: $891D */
#define SIDE 0x002Cu         /* its x compared with his, rotated in: bit 0 set when he is left of it (scratch, data bank) */
#define PLAYER_X_AT (PLAYER + ACTOR_X + 1)
#define PLAYER_Y_AT (PLAYER + ACTOR_Y + 1)
#define REACH_Y 0xD4EDu      /* by its $07 (times 2): added to its y before his is taken off (word) */
#define AIM_NEAR 0xD4F7u     /* by its $07 (times 2): below this, aim 2 */
#define AIM_MID 0xD4F8u      /* then below this, aim 1; else 0 */

/* Its watch, its two throws and its wait for them are one loop, entered
   at any of the three. */
enum { AT_WATCH, AT_THROW_AGAIN, AT_WAIT_THROWS };
static void kind63_from(int at);

/*
 * @routine 84:81FC Kind63Work
 * WHAT IT DOES: What actors of kind $63 do each frame they wait: unless
 * already hit, a shot from the second pool turns it to face Aladdin and
 * hits it; Aladdin is hurt by it or stomps it; it animates and shows its
 * own picture; and out of range by box $28 it ends, its $47 counted up.
 * HOW: KIND63_HIT zero: ActorShotHitFace. Then ActorStompOrHurtLong,
 * AnimStep, ActorPictureSend, ActorFreeOutOfRangeLong with X = $28; C
 * set (out of range): KIND63_GONE up by one.
 * Entry: M 8-bit, X 8-bit; D = the actor; called by JSL from the kind's
 * stub ($83:924D) through the actor loop's wait table at $83:8F4F. Exit:
 * M and X 8-bit; C as ActorFreeOutOfRangeLong left it; in range the
 * registers it left, else N and Z from the count.
 */
void abi_Kind63Work(void)
{
    uint8_t hit = rd8(dpa(KIND63_HIT));

    a8_load(hit);
    if (!hit)
        abi_ActorShotHitFace();
    abi_ActorStompOrHurtLong();
    abi_AnimStep();
    abi_ActorPictureSend();
    x8_load(0x28);
    abi_ActorFreeOutOfRangeLong();
    if (cpu.p & P_C)
        (void)inc8_at(dpa(KIND63_GONE));
}

/*
 * @routine 84:8080 Kind63Start
 * @actor
 * WHAT IT DOES: Actor kind $63's start: shown facing right with one hit
 * of health and nothing thrown yet, then on to watch for Aladdin.
 * HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set; KIND63_28 = $FF,
 * KIND63_16 = $D4E5, KIND63_29 = $891D; KIND63_OUT, KIND63_HIT and
 * KIND63_GONE 0; health 1; facing and flip 0; on into Kind63Watch.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the actor
 * loop's jump for kind $63 in state $0C (the table at $83:8CAB, through the
 * stub at $83:9249). Exit: never returns.
 */
void abi_Kind63Start(void)
{
    uint8_t flags;

    a8_load(0x02);
    tsb_at(dpa(ACTOR_08), 0x02);
    flags = (uint8_t)(rd8(dpa(ACTOR_09)) | 0x90);
    a8_load(flags);
    wr8(dpa(ACTOR_09), flags);
    a8_load(0xFF);
    wr8(dpa(KIND63_28), 0xFF);
    op_rep(0x20);
    a16_load(0xD4E5);
    dp16_set(KIND63_16, 0xD4E5);
    a16_load(0x891D);
    dp16_set(KIND63_29, 0x891D);
    op_sep(0x20);
    wr8(dpa(KIND63_OUT), 0);
    wr8(dpa(KIND63_HIT), 0);
    wr8(dpa(KIND63_GONE), 0);
    a8_load(0x01);
    wr8(dpa(KIND63_HEALTH), 0x01);
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    kind63_from(AT_WATCH);
}

/*
 * @routine 84:80AA Kind63Watch
 * @actor
 * WHAT IT DOES: Kind $63 watches (animation 0) until Aladdin is near and
 * within reach below it, then turns toward him, picks how far to throw,
 * and throws once (animation 4, a child of hit box $A2).
 * HOW: AnimStart 0, COP $10; each frame (BRK first) ActorNearPlayer with
 * Y = $30 until C clear; then its x against his, rotated into SIDE; its
 * y plus REACH_Y by its $07, less his: a borrow, watch on. KIND63_AIM = 2,
 * 1 or 0 by that against AIM_NEAR and AIM_MID; facing and flip = bit 0
 * of SIDE. COP 8, AnimStart 4, COP $10, ActorSpawnA2 with A = 1, COP 8;
 * on into Kind63ThrowAgain.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached from
 * Kind63Start, Kind63WaitThrows or Kind63ShotHit. Exit: never returns.
 */
void abi_Kind63Watch(void)
{
    kind63_from(AT_WATCH);
}

/*
 * @routine 84:810B Kind63ThrowAgain
 * @actor
 * WHAT IT DOES: Kind $63's second throw: a pause (animation 1), then
 * animation 5 and a child of hit box $A2.
 * HOW: AnimStart 1, COP $28, AnimStart 5, COP $10, ActorSpawnA2 with A =
 * 0, COP 8; on into Kind63WaitThrows.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached from
 * Kind63Watch or Kind63ShotHit. Exit: never returns.
 */
void abi_Kind63ThrowAgain(void)
{
    kind63_from(AT_THROW_AGAIN);
}

/*
 * @routine 84:8128 Kind63WaitThrows
 * @actor
 * WHAT IT DOES: Kind $63 waits for its throws to be gone (animation 2
 * while two fly, 1 while one does), then watches again.
 * HOW: AnimStart 2; each frame (BRK first) until KIND63_OUT is below 2;
 * AnimStart 1; each frame until it is 0; then Kind63Watch.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached from
 * Kind63ThrowAgain or Kind63ShotHit. Exit: never returns.
 */
void abi_Kind63WaitThrows(void)
{
    kind63_from(AT_WAIT_THROWS);
}

static void kind63_from(int at)
{
    uint8_t out, i, aim, low;
    uint16_t ax, d;

    for (;;) {
        if (at == AT_WATCH) {
            a8_load(0x00);
            abi_AnimStart();
            a8_load(0x10);
            actor_wait(0x8480B2u);
            for (;;) {
                actor_yield(0x8480B4u);
                y8_load(0x30);
                abi_ActorNearPlayer();
                if (cpu.p & P_C)
                    continue;                    /* not near */
                i = (uint8_t)(rd8(dpa(ACTOR_07)) << 1);
                a8_load(i);
                flag_set(P_C, rd8(dpa(ACTOR_07)) & 0x80);
                x8_load(i);
                op_rep(0x20);
                ax = dp16(ACTOR_X + 1);
                cmp16_of(ax, rd16(dba(PLAYER_X_AT)));
                cpu.a = (uint16_t)(ax << 1 | ((cpu.p & P_C) ? 1 : 0));
                flag_set(P_C, ax & 0x8000);
                nz16_of(cpu.a);
                wr16(dba(SIDE), cpu.a);
                flag_set(P_C, 0);
                d = dp16(ACTOR_Y + 1);
                d = op_adc16(d, rd16(dba((uint16_t)(REACH_Y + i))), &cpu.p);
                flag_set(P_C, 1);
                cpu.a = op_sbc16(d, rd16(dba(PLAYER_Y_AT)), &cpu.p);
                op_sep(0x20);
                if (cpu.p & P_C)
                    break;                       /* within reach below it */
            }
            /* how far: 2, 1 or 0 by the limits for its $07 */
            low = (uint8_t)cpu.a;
            aim = 0x02;
            y8_load(aim);
            cmp8_of(low, rd8(dba((uint16_t)(AIM_NEAR + i))));
            if (low >= rd8(dba((uint16_t)(AIM_NEAR + i)))) {
                y8_load(--aim);
                cmp8_of(low, rd8(dba((uint16_t)(AIM_MID + i))));
                if (low >= rd8(dba((uint16_t)(AIM_MID + i))))
                    y8_load(--aim);
            }
            wr8(dpa(KIND63_AIM), aim);
            low = (uint8_t)(rd8(dba(SIDE)) & 0x01);
            a8_load(low);
            wr8(dpa(ACTOR_FACING), low);
            wr8(dpa(ACTOR_FLIP), low);
            a8_load(0x08);
            actor_wait(0x8480F6u);
            a8_load(0x04);
            abi_AnimStart();
            a8_load(0x10);
            actor_wait(0x848100u);
            a8_load(0x01);
            abi_ActorSpawnA2();
            a8_load(0x08);
            actor_wait(0x848109u);
            at = AT_THROW_AGAIN;
        }
        if (at == AT_THROW_AGAIN) {
            a8_load(0x01);
            abi_AnimStart();
            a8_load(0x28);
            actor_wait(0x848113u);
            a8_load(0x05);
            abi_AnimStart();
            a8_load(0x10);
            actor_wait(0x84811Du);
            a8_load(0x00);
            abi_ActorSpawnA2();
            a8_load(0x08);
            actor_wait(0x848126u);
        }
        /* AT_WAIT_THROWS */
        a8_load(0x02);
        abi_AnimStart();
        do {
            actor_yield(0x84812Eu);
            out = rd8(dpa(KIND63_OUT));
            a8_load(out);
            cmp8_of(out, 0x02);
        } while (out >= 0x02);
        a8_load(0x01);
        abi_AnimStart();
        do {
            actor_yield(0x84813Cu);
            out = rd8(dpa(KIND63_OUT));
            a8_load(out);
        } while (out);
        at = AT_WATCH;
    }
}

/*
 * @routine 84:81CD Kind63ShotHit
 * @actor
 * WHAT IT DOES: Kind $63 hit by a shot (state $CC): with nothing thrown
 * when it was hit, it throws a pair (animation 6) and watches again; else
 * it shows the hit (animation 3) and goes on waiting for its throws, or
 * throws again when one is left.
 * HOW: KIND63_OUT_HIT zero: AnimStart 6, ActorSpawnPairA3, COP $18,
 * Kind63Watch. Else KIND63_HIT up, AnimStart 3, COP $18, KIND63_HIT 0, a
 * yield; KIND63_OUT less 1 zero: Kind63ThrowAgain, else Kind63WaitThrows.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the actor
 * loop's jump for kind $63 in state $CC (the table at $83:90A1, through
 * the stub at $83:9256). Exit: never returns.
 */
void abi_Kind63ShotHit(void)
{
    uint8_t n = rd8(dpa(KIND63_OUT_HIT));

    a8_load(n);
    if (!n) {
        a8_load(0x06);
        abi_AnimStart();
        abi_ActorSpawnPairA3();
        a8_load(0x18);
        actor_wait(0x8481DCu);
        kind63_from(AT_WATCH);
    }
    (void)inc8_at(dpa(KIND63_HIT));
    a8_load(0x03);
    abi_AnimStart();
    a8_load(0x18);
    actor_wait(0x8481EBu);
    wr8(dpa(KIND63_HIT), 0);
    actor_yield(0x8481EFu);
    n = (uint8_t)(rd8(dpa(KIND63_OUT)) - 1);
    a8_load(n);
    kind63_from(n ? AT_WAIT_THROWS : AT_THROW_AGAIN);
}

/*
 * @routine 84:81BE Kind63Knocked
 * @actor
 * WHAT IT DOES: Actor kind $63 in its second state (hit): KIND63_GONE
 * counted up, its sprite priority cleared, and it is knocked away with
 * animation 8, sound $0F and motion preset 3.
 * HOW: KIND63_GONE up; ActorPriorityClear; A = 8, X = $0F, Y = 3; on into
 * ActorKnockedAway (JML).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the actor
 * loop's jump for kind $63 with state bit 7 set (the table at $83:8DFD,
 * through the stub at $83:9252). Exit: as ActorKnockedAway.
 */
void abi_Kind63Knocked(void)
{
    (void)inc8_at(dpa(KIND63_GONE));
    abi_ActorPriorityClear();
    a8_load(0x08);
    x8_load(0x0F);
    y8_load(0x03);
    abi_ActorKnockedAway();
}

