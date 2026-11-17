/*
 * kind2d.c: actor kind $2D, a thrower: it watches for Aladdin within its
 * reach ahead of it and throws copies of its kind at him (each with $07
 * bit 7 set), which fly by their motion preset and hurt him in their
 * zone; what it does when hit; and the ROM check its start makes first.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "wram_video.h"
#include "actor.h"

void abi_ActorEndOutOfRange(void);
void abi_RandomNext(void);
void abi_ActorKnockedAway(void);
void abi_ActorKnockedAwayResumed(void);
void abi_ActorKnockedAwayResumedB(void);
void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_ActorPictureQueue(void);
void abi_ActorSideOfPlayer(void);
void abi_ActorSpawn(void);
void abi_ActorZoneHurtsPlayer(void);
void abi_AnimStart(void);
void abi_SoundQueueAdd(void);
void abi_Kind2DWatch(void);
void abi_Kind2DRest(void);
void abi_Kind2DThrown(void);
void abi_Kind2DAfterThrow(void);
static void throw_at(int again);

#define K2D_FRAMES 0x40u        /* the watch: frames left; the thrown copy: frames of a pose */
#define K2D_ARC 0x41u           /* the throw's arc (the copy's $07, bit 7 set); the copy: its pose */
#define K2D_POSE 0x43u          /* the throw's pose: 4 (level), 3 (Aladdin above) */
#define K2D_COUNT 0x45u         /* the throw's frames left */
#define K2D_FIRST 0x47u         /* the throw's frames, by $07 */
#define K2D_AT 0x48u            /* the frame of the throw the copy leaves at, by $07 */
#define K2D_AGAIN 0x49u         /* set once a $07 = 2 thrower has thrown twice */
#define K2D_4F 0x4Fu            /* 0: a thrower; 1: a thrown copy */
#define K2D_29 0x29u            /* word: $8911 */
#define K2D_REACH_X 0xD122u     /* (bank $83) by facing * 4: x offset (word), then reach (word) */
#define K2D_REACH 0xD124u
#define K2D_FRAMES_BY 0xD130u   /* by $07: K2D_FIRST */
#define K2D_AT_BY 0xD133u       /* by $07: K2D_AT */
#define K2D_NEXT_ARC 0xD136u    /* by the arc: the second throw's arc */
#define K2D_POSES 0xD12Au       /* by the copy's $07: its first pose */
#define PLAYER_X (PLAYER + ACTOR_X + 1)
#define PLAYER_Y (PLAYER + ACTOR_Y + 1)
#define SPAWN_COUNTS 0x7EFF00u  /* by hit box: copies spawned */
#define CHECK_A 0x83989Eu       /* two code bytes the ROM check reads: a NOP ($EA) at either */
#define CHECK_B 0x8398ACu       /* means the ROM was changed */

/*
 * @routine 83:A82F Kind2DStart
 * @actor
 * WHAT IT DOES: Actor kind $2D's start: first a check that two bytes of
 * the actor loop's code are not NOPs (a changed ROM: then its hit box is
 * a random one, code the retail game never runs); a thrown copy ($07
 * bit 7) goes to Kind2DThrown; a thrower shows itself, faces Aladdin and
 * rests (Kind2DRest).
 * HOW: CHECK_A or CHECK_B $EA: RandomNext, its A to $06 ($83:A83F).
 * $07 bit 7: Kind2DThrown (JMP). Else K2D_4F 0, ACTOR_08 bit 1 set,
 * K2D_29 $8911, AnimStart 0, ActorPictureQueue, BRK, ACTOR_09 bits $D0
 * set, facing and flip by ActorSideOfPlayer; Kind2DRest (JMP).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $2D in state $0C (the table at $83:8CAB)
 * and for kind $2C with state bit 7 set (the table at $83:8DFD). Exit:
 * never returns.
 */
void abi_Kind2DStart(void)
{
    a8_load(rd8(CHECK_A));
    cmp8_of((uint8_t)cpu.a, 0xEA);
    if ((uint8_t)cpu.a != 0xEA) {
        a8_load(rd8(CHECK_B));
        cmp8_of((uint8_t)cpu.a, 0xEA);
    }
    if ((uint8_t)cpu.a == 0xEA) {
        /* a changed ROM ($83:A83F, which no script runs): a random hit box */
        abi_RandomNext();
        wr8(dpa(ACTOR_06), (uint8_t)cpu.a);
    }
    a8_load(rd8(dpa(ACTOR_07)));
    if ((uint8_t)cpu.a & 0x80) {
        abi_Kind2DThrown();
        return;
    }
    wr8(dpa(K2D_4F), 0);
    a8_load(0x02);
    tsb_at(dpa(ACTOR_08), 0x02);
    a8_load(0x11);
    wr8(dpa(K2D_29), 0x11);
    a8_load(0x89);
    wr8(dpa(K2D_29 + 1), 0x89);
    a8_load(0x00);
    abi_AnimStart();
    abi_ActorPictureQueue();
    actor_yield(0x83A864u);
    a8_load(0xD0);
    tsb_at(dpa(ACTOR_09), 0xD0);
    abi_ActorSideOfPlayer();
    wr8(dpa(ACTOR_FACING), (uint8_t)cpu.a);
    wr8(dpa(ACTOR_FLIP), (uint8_t)cpu.a);
    abi_Kind2DRest();
}

/*
 * @routine 83:A875 Kind2DWatch
 * @actor
 * WHAT IT DOES: The thrower's watch: for up to 128 frames, each frame,
 * when Aladdin is near its height (from 64 above it to 96 below) and
 * within its reach ahead: pose 2, the arc by how far he is (3 more when he
 * is above it), the pose 3 or 4, a wait (1 frame, or 32 for $07 = 0),
 * then the throw. At frame 96 it turns to face him in pose 1. The time
 * up, it rests (Kind2DRest).
 * HOW: K2D_FRAMES $80. Each frame (BRK): (its y + $40 - Aladdin's y) <
 * $A0 and (its x + K2D_REACH_X - his x) < K2D_REACH, by facing * 4:
 * AnimStart 2; K2D_ARC (word) = (|its x - his x| - 16) / 32; its y - 16
 * below his: K2D_POSE 3 and K2D_ARC + 3, else 4; COP 1 ($20 with $07 0);
 * the throw ($83:A90C, in Kind2DRest's code). Else at K2D_FRAMES $20
 * facing and flip by ActorSideOfPlayer and AnimStart 1; K2D_FRAMES down;
 * at 0 Kind2DRest.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * a jump from Kind2DRest and Kind2DAfterThrow. Exit: never returns.
 */
void abi_Kind2DWatch(void)
{
    uint16_t d;
    uint8_t at, pose;

    a8_load(0x80);
    wr8(dpa(K2D_FRAMES), 0x80);
    for (;;) {
        actor_yield(0x83A879u);
        op_rep(0x21);
        d = op_adc16(dp16(ACTOR_Y + 1), 0x0040, &cpu.p);
        flag_set(P_C, 1);
        d = op_sbc16(d, rd16(dba((uint16_t)PLAYER_Y)), &cpu.p);
        a16_load(d);
        cmp16_of(d, 0x00A0);
        op_sep(0x20);
        if (!(cpu.p & P_C)) {
            at = (uint8_t)(rd8(dpa(ACTOR_FACING)) << 2);
            a8_load(at);
            x8_load(at);
            op_rep(0x21);
            d = op_adc16(dp16(ACTOR_X + 1), rd16(dba((uint16_t)(K2D_REACH_X + at))), &cpu.p);
            flag_set(P_C, 1);
            d = op_sbc16(d, rd16(dba((uint16_t)PLAYER_X)), &cpu.p);
            a16_load(d);
            cmp16_of(d, rd16(dba((uint16_t)(K2D_REACH + at))));
            op_sep(0x20);
            if (!(cpu.p & P_C)) {
                /* he is in reach: the arc and the pose, then the throw */
                a8_load(0x02);
                abi_AnimStart();
                op_rep(0x20);
                flag_set(P_C, 1);
                d = op_sbc16(dp16(ACTOR_X + 1), rd16(dba((uint16_t)PLAYER_X)), &cpu.p);
                a16_load(d);
                if (d & 0x8000) {
                    d = (uint16_t)((d ^ 0xFFFF) + 1);
                    a16_load(d);
                }
                flag_set(P_C, 1);
                d = op_sbc16(d, 0x0010, &cpu.p);
                flag_set(P_C, d & 0x10);        /* the fifth LSR's */
                a16_load((uint16_t)(d >> 5));
                dp16_set(K2D_ARC, cpu.a);
                flag_set(P_C, 1);
                d = op_sbc16(dp16(ACTOR_Y + 1), 0x0010, &cpu.p);
                a16_load(d);
                cmp16_of(d, rd16(dba((uint16_t)PLAYER_Y)));
                op_sep(0x20);
                pose = 0x04;
                y8_load(pose);
                if (!(cpu.p & P_C)) {
                    flag_set(P_C, 0);
                    a8_load(op_adc8(0x03, rd8(dpa(K2D_ARC)), &cpu.p));
                    wr8(dpa(K2D_ARC), (uint8_t)cpu.a);
                    pose = 0x03;
                    y8_load(pose);
                }
                wr8(dpa(K2D_POSE), pose);
                a8_load(0x01);
                y8_load(rd8(dpa(ACTOR_07)));
                if (!(uint8_t)cpu.y)
                    a8_load(0x20);
                actor_wait(0x83A8E4u);
                throw_at(0);
                return;
            }
        }
        a8_load(rd8(dpa(K2D_FRAMES)));
        cmp8_of((uint8_t)cpu.a, 0x20);
        if ((uint8_t)cpu.a == 0x20) {
            abi_ActorSideOfPlayer();
            wr8(dpa(ACTOR_FACING), (uint8_t)cpu.a);
            wr8(dpa(ACTOR_FLIP), (uint8_t)cpu.a);
            a8_load(0x01);
            abi_AnimStart();
        }
        if (!dec8_at(dpa(K2D_FRAMES))) {
            abi_Kind2DRest();
            return;
        }
    }
}

/* The throw ($83:A90C; again: $83:A90E, a $07 = 2 thrower's second):
   the pose, then a copy of its kind thrown on the frame K2D_AT_BY says. */
static void throw_at(int again)
{
    uint8_t v07, box;

    if (!again)
        wr8(dpa(K2D_AGAIN), 0);
    for (;;) {
        x8_load(rd8(dpa(ACTOR_07)));
        a8_load(rd8(dba((uint16_t)(K2D_FRAMES_BY + cpu.x))));
        wr8(dpa(K2D_FIRST), (uint8_t)cpu.a);
        a8_load(rd8(dba((uint16_t)(K2D_AT_BY + cpu.x))));
        wr8(dpa(K2D_AT), (uint8_t)cpu.a);
        a8_load(rd8(dpa(K2D_POSE)));
        abi_AnimStart();
        a8_load((uint8_t)(((rd8(dpa(K2D_POSE)) >> 1) & 0x02) + 0x16));
        flag_set(P_C, 0);
        wr8(dpa(ACTOR_SPAWN_AT), (uint8_t)cpu.a);
        a8_load(rd8(dpa(K2D_FIRST)));
        wr8(dpa(K2D_COUNT), (uint8_t)cpu.a);
        do {
            actor_yield(0x83A92Eu);
            a8_load(rd8(dpa(K2D_COUNT)));
            cmp8_of((uint8_t)cpu.a, rd8(dpa(K2D_AT)));
            if ((uint8_t)cpu.a != rd8(dpa(K2D_AT)))
                continue;
            /* the copy leaves: sound $11, its $07 the arc with bit 7 */
            a8_load(0x11);
            abi_SoundQueueAdd();
            v07 = rd8(dpa(ACTOR_07));
            a8_load(v07);
            op_push8(v07);
            a8_load((uint8_t)(rd8(dpa(K2D_ARC)) ^ 0x80));
            wr8(dpa(ACTOR_07), (uint8_t)cpu.a);
            a8_load(rd8(dpa(ACTOR_06)));
            abi_ActorSpawn();
            if (!(cpu.p & P_N)) {
                uint16_t child = (uint16_t)cpu.x;

                a8_load(rd8(dpa(ACTOR_LOOK)));
                wr8(dba((uint16_t)(child + ACTOR_LOOK)), (uint8_t)cpu.a);
                a8_load(rd8(dpa(ACTOR_LOOK + 1)));
                wr8(dba((uint16_t)(child + ACTOR_LOOK + 1)), (uint8_t)cpu.a);
                a8_load(0x01);
                wr8(dba((uint16_t)(child + K2D_4F)), 0x01);
                op_sep(0x10);
                box = rd8(dpa(ACTOR_06));
                x8_load(box);
                a8_load((uint8_t)(rd8(SPAWN_COUNTS + box) + 1));
                wr8(SPAWN_COUNTS + box, (uint8_t)cpu.a);
            }
            op_sep(0x10);
            a8_load(op_pull8());
            wr8(dpa(ACTOR_07), (uint8_t)cpu.a);
        } while (dec8_at(dpa(K2D_COUNT)));
        a8_load(rd8(dpa(ACTOR_07)));
        cmp8_of((uint8_t)cpu.a, 0x02);
        if ((uint8_t)cpu.a != 0x02)
            break;
        a8_load(rd8(dpa(K2D_AGAIN)));
        if ((uint8_t)cpu.a)
            break;
        x8_load(rd8(dpa(K2D_ARC)));
        a8_load(rd8(dba((uint16_t)(K2D_NEXT_ARC + cpu.x))));
        wr8(dpa(K2D_ARC), (uint8_t)cpu.a);
        (void)inc8_at(dpa(K2D_AGAIN));
    }
    abi_Kind2DAfterThrow();
}

/*
 * @routine 83:A903 Kind2DRest
 * @actor
 * WHAT IT DOES: The thrower at rest (pose 7), then on watching; its code
 * also holds the throw ($83:A90C): the pose 3 or 4, and on the frame its
 * $07 says a copy of its kind thrown (sound $11; the copy's $07 the arc
 * with bit 7 set, its look the thrower's, its $4F 1, counted in
 * SPAWN_COUNTS); a $07 = 2 thrower throws twice, the second on the arc
 * K2D_NEXT_ARC gives; then Kind2DAfterThrow.
 * HOW: AnimStart 7; Kind2DWatch (JMP). The throw: K2D_AGAIN 0; K2D_FIRST
 * and K2D_AT by $07; AnimStart K2D_POSE; ACTOR_SPAWN_AT $16 or $18 (pose
 * 4 or 3... its bit 2); K2D_COUNT frames of BRK, at K2D_AT: SoundQueueAdd
 * $11, $07 kept on the stack and set to K2D_ARC EOR $80, ActorSpawn with
 * its hit box; spawned (N clear): the child's $2B-$2C and $4F, the count
 * up; $07 back.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * a jump from Kind2DStart and Kind2DWatch. Exit: never returns.
 */
void abi_Kind2DRest(void)
{
    a8_load(0x07);
    abi_AnimStart();
    abi_Kind2DWatch();
}

/*
 * @routine 83:A988 Kind2DAfterThrow
 * @actor
 * WHAT IT DOES: After a throw: pose 7 for 72 frames, then on watching.
 * HOW: AnimStart 7; COP $48; Kind2DWatch (JMP).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached
 * from the throw ($83:A976, $83:A97A). Exit: never returns.
 */
void abi_Kind2DAfterThrow(void)
{
    a8_load(0x07);
    abi_AnimStart();
    a8_load(0x48);
    actor_wait(0x83A990u);
    abi_Kind2DWatch();
}

/*
 * @routine 83:A995 Kind2DThrown
 * @actor
 * WHAT IT DOES: A thrown copy of kind $2D: it flies by the motion preset
 * its arc names, stepping through its poses every 13 frames up to pose
 * $0E, hurting Aladdin in its zone, until it is out of range. In levels
 * 0, 3 and $0B it is drawn with sprite priority $10.
 * HOW: ACTOR_08 bit 0 set; ACTOR_09 |= $10 in levels 0, 3, $0B (0
 * otherwise); $07 AND $0F; ActorMotionPreset $17 + $07; K2D_ARC (its
 * pose) = K2D_POSES by $07, AnimStart; K2D_FRAMES $0D. Each frame (BRK):
 * below pose $0E, K2D_FRAMES down, at 0 again $0D and the next pose
 * (AnimStart); ActorFly; ActorZoneHurtsPlayer with zone (pose - 8) * 4;
 * ActorEndOutOfRange with box 0.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * a jump from Kind2DStart ($07 bit 7 set). Exit: never returns; ends
 * when out of range.
 */
void abi_Kind2DThrown(void)
{
    uint8_t lvl, pose;

    a8_load(0x01);
    tsb_at(dpa(ACTOR_08), 0x01);
    a8_load(0x00);
    lvl = rd8(dba(LEVEL));
    x8_load(lvl);
    if (lvl == 0x00 || lvl == 0x03 || lvl == 0x0B) {
        cmp8_of(lvl, lvl);
        a8_load(0x10);
    } else
        cmp8_of(lvl, 0x0B);
    tsb_at(dpa(ACTOR_09), (uint8_t)cpu.a);
    a8_load((uint8_t)(rd8(dpa(ACTOR_07)) & 0x0F));
    wr8(dpa(ACTOR_07), (uint8_t)cpu.a);
    flag_set(P_C, 0);
    a8_load(op_adc8((uint8_t)cpu.a, 0x17, &cpu.p));
    y8_load((uint8_t)cpu.a);
    abi_ActorMotionPreset();
    y8_load(rd8(dpa(ACTOR_07)));
    pose = rd8(dba((uint16_t)(K2D_POSES + cpu.y)));
    a8_load(pose);
    wr8(dpa(K2D_ARC), pose);
    abi_AnimStart();
    a8_load(0x0D);
    wr8(dpa(K2D_FRAMES), 0x0D);
    for (;;) {
        actor_yield(0x83A9C9u);
        pose = rd8(dpa(K2D_ARC));
        a8_load(pose);
        cmp8_of(pose, 0x0E);
        if (pose != 0x0E && !dec8_at(dpa(K2D_FRAMES))) {
            a8_load(0x0D);
            wr8(dpa(K2D_FRAMES), 0x0D);
            a8_load(inc8_at(dpa(K2D_ARC)));
            abi_AnimStart();
        }
        abi_ActorFly();
        flag_set(P_C, 1);
        a8_load(op_sbc8(rd8(dpa(K2D_ARC)), 0x08, &cpu.p));
        a8_load((uint8_t)(cpu.a << 2));
        y8_load((uint8_t)cpu.a);
        abi_ActorZoneHurtsPlayer();
        x8_load(0x00);
        /* the JSR's return address on the stack, which ActorEndOutOfRange
           pulls when it ends the actor */
        call_frame(0xA9F4, abi_ActorEndOutOfRange);
    }
}

/*
 * @routine 83:AA5A Kind2DHit
 * @actor
 * WHAT IT DOES: Kind $2D when hit (its state with bit 7 set): a thrown
 * copy is knocked away at once; a thrower, the first time, is knocked
 * away with sound $0F, or, while it faces left (bit 7 of its facing),
 * turned to face by bit 0 and knocked away in the air with sound 7.
 * HOW: K2D_4F set: ActorKnockedAwayResumedB with A = 5, X = 2, Y = 3
 * ($83:AA8C). Else K2D_4F = 1; facing negative ($83:AA75): facing = bit
 * 0, flip its opposite, SoundQueueAdd 7, ActorKnockedAwayResumed with A
 * = 5, X = 2, Y = $4C; else SoundQueueAdd $0F, ActorKnockedAway with A =
 * 5, X = $0F, Y = 3. Each by JMP. The first two paths no script meets
 * yet: written from their bytes.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the actor
 * loop's jump for kind $2D with state bit 7 set (the table at $83:8DFD).
 * Exit: as the routine it goes on into.
 */
void abi_Kind2DHit(void)
{
    uint8_t f;

    a8_load(rd8(dpa(K2D_4F)));
    if ((uint8_t)cpu.a) {
        a8_load(0x05);
        x8_load(0x02);
        y8_load(0x03);
        abi_ActorKnockedAwayResumedB();
        return;
    }
    a8_load(0x01);
    wr8(dpa(K2D_4F), 0x01);
    f = rd8(dpa(ACTOR_FACING));
    a8_load(f);
    if (f & 0x80) {
        a8_load((uint8_t)(f & 0x01));
        wr8(dpa(ACTOR_FACING), (uint8_t)cpu.a);
        a8_load((uint8_t)(cpu.a ^ 0x01));
        wr8(dpa(ACTOR_FLIP), (uint8_t)cpu.a);
        a8_load(0x07);
        abi_SoundQueueAdd();
        a8_load(0x05);
        x8_load(0x02);
        y8_load(0x4C);
        abi_ActorKnockedAwayResumed();
        return;
    }
    a8_load(0x0F);
    abi_SoundQueueAdd();
    a8_load(0x05);
    x8_load(0x0F);
    y8_load(0x03);
    abi_ActorKnockedAway();
}

