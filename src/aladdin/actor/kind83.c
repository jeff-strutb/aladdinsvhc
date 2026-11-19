/*
 * kind83.c: actor kind $83, a thing that flies up out of the carpet
 * levels' scenery to a height and leaves a child of kind $5E there.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_ActorCreateChild8(void);
void abi_ActorEndPlaced(void);
void abi_ActorFall(void);
void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_AnimStart(void);

#define K83_16 0x16u            /* word: $D836 */
#define K83_NO_CHILD 0x42u      /* nonzero (the late variant): no child at the end */
#define K83_TOP_SPEED 0x45u     /* the most its vertical speed's fraction may reach */
#define K83_LINE 0x4Au          /* the y (low byte) it goes on until: $C8, or $FF for the late variant */
#define K83_DELAYS 0xD83Au      /* by $07: the frames before it shows (in the bank DB names) */
#define K83_TOP_SPEEDS 0xD83Du  /* by $07: K83_TOP_SPEED */
#define K83_PRESETS 0xD840u     /* by $07: its motion preset */
#define PLAYER_X (PLAYER + ACTOR_X + 1)  /* Aladdin's x, whole pixels (word) */
#define V_1A10 0x1A10u          /* nonzero: it falls instead of flying on */
#define KIND_5E 0x5Eu

/*
 * @routine 84:9538 Kind83Start
 * @actor
 * WHAT IT DOES: Actor kind $83: by its $07, hidden for a while, then it
 * flies off by its motion preset, its vertical speed held to its top
 * speed, until its y reaches $C8; it falls instead once $1A10 is set;
 * then it leaves a child of kind $5E and ends. With $07 bit 4 set (late)
 * it waits until Aladdin is past it, flies until y $FF, and leaves no
 * child.
 * HOW: ACTOR_09 bits $90 set; K83_16 $D836; AnimStart 0; K83_NO_CHILD 0.
 * $07 bit 4: $07 AND $0F, K83_NO_CHILD up, each frame (BRK) until its x
 * is below PLAYER_X, K83_LINE $FF. Else ACTOR_08 bit 3 cleared, COP
 * K83_DELAYS by $07, the bit set again, K83_LINE $C8. Facing and flip 0;
 * K83_TOP_SPEED and the preset (ActorMotionPreset) by $07. Each frame:
 * ActorFly; y at K83_LINE or past it, on; $1A10 set: gravity 8 and each
 * frame ActorFall until y reaches K83_LINE; else the speed's fraction
 * held at K83_TOP_SPEED (gravity 0 when it was over). At the end, unless
 * K83_NO_CHILD: each frame ActorCreateChild8 of kind $5E ($21 0) until
 * one is made. ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM and that holds its tables (bank $84); reached by
 * the actor loop's jump for kind $83 in state $0C (the table at $83:8DAB,
 * through the stub at $83:92D8). Exit: never returns; ends in
 * ActorEndPlaced.
 */
void abi_Kind83Start(void)
{
    uint8_t v, line, y;
    uint16_t x;

    a8_load((uint8_t)(rd8(dpa(ACTOR_09)) | 0x90));
    wr8(dpa(ACTOR_09), (uint8_t)cpu.a);
    op_rep(0x20);
    a16_load(0xD836);
    dp16_set(K83_16, 0xD836);
    op_sep(0x20);
    a8_load(0x00);
    jsl_frame(0x849549u);
    abi_AnimStart();
    cpu.s = (uint16_t)(cpu.s + 3);
    wr8(dpa(K83_NO_CHILD), 0);
    v = rd8(dpa(ACTOR_07));
    a8_load(v);
    flag_set(P_Z, !(v & 0x10));
    if (v & 0x10) {
        /* late: once Aladdin is past it */
        a8_load((uint8_t)(v & 0x0F));
        wr8(dpa(ACTOR_07), (uint8_t)cpu.a);
        (void)inc8_at(dpa(K83_NO_CHILD));
        do {
            actor_yield(0x84955Bu);
            op_rep(0x20);
            x = dp16(ACTOR_X + 1);
            a16_load(x);
            cmp16_of(x, rd16(dba((uint16_t)PLAYER_X)));
            op_sep(0x20);
        } while (cpu.p & P_C);
        a8_load(0xFF);
        wr8(dpa(K83_LINE), 0xFF);
    } else {
        a8_load(0x08);
        trb_at(dpa(ACTOR_08), 0x08);
        x8_load(rd8(dpa(ACTOR_07)));
        a8_load(rd8(dba((uint16_t)(K83_DELAYS + cpu.x))));
        actor_wait(0x849577u);
        a8_load(0x08);
        tsb_at(dpa(ACTOR_08), 0x08);
        a8_load(0xC8);
        wr8(dpa(K83_LINE), 0xC8);
    }
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    x8_load(rd8(dpa(ACTOR_07)));
    a8_load(rd8(dba((uint16_t)(K83_TOP_SPEEDS + cpu.x))));
    wr8(dpa(K83_TOP_SPEED), (uint8_t)cpu.a);
    y8_load(rd8(dba((uint16_t)(K83_PRESETS + cpu.x))));
    jsl_frame(0x84958Fu);
    abi_ActorMotionPreset();
    cpu.s = (uint16_t)(cpu.s + 3);

    /* flying, to its line */
    for (;;) {
        actor_yield(0x849593u);
        jsl_frame(0x849595u);
        abi_ActorFly();
        cpu.s = (uint16_t)(cpu.s + 3);
        y = rd8(dpa(ACTOR_Y + 1));
        line = rd8(dpa(K83_LINE));
        a8_load(y);
        cmp8_of(y, line);
        if (y >= line)
            goto there;
        v = rd8(dba(V_1A10));
        a8_load(v);
        if (v)
            break;
        v = rd8(dpa(K83_TOP_SPEED));
        a8_load(v);
        cmp8_of(v, rd8(dpa(ACTOR_SPEED_Y)));
        if (v < rd8(dpa(ACTOR_SPEED_Y))) {
            wr8(dpa(ACTOR_SPEED_Y), v);
            wr8(dpa(ACTOR_ACCEL_Y), 0);
        }
    }
    /* falling, to its line */
    a8_load(0x08);
    wr8(dpa(ACTOR_ACCEL_Y), 0x08);
    do {
        actor_yield(0x8495B4u);
        jsl_frame(0x8495B6u);
        abi_ActorFall();
        cpu.s = (uint16_t)(cpu.s + 3);
        y = rd8(dpa(ACTOR_Y + 1));
        line = rd8(dpa(K83_LINE));
        a8_load(y);
        cmp8_of(y, line);
    } while (y < line);
there:
    /* its child, unless late */
    for (;;) {
        v = rd8(dpa(K83_NO_CHILD));
        a8_load(v);
        if (v)
            break;
        actor_yield(0x8495C4u);
        wr8(dpa(ACTOR_SPAWN_AT), 0);
        a8_load(KIND_5E);
        jsl_frame(0x8495CAu);
        abi_ActorCreateChild8();
        cpu.s = (uint16_t)(cpu.s + 3);
        if (!(cpu.p & P_C))
            break;
    }
    abi_ActorEndPlaced();
}
