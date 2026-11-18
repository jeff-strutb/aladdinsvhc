/*
 * kind77.c: actor kind $77, a platform that waits until Aladdin is on
 * it, then speeds up to the right until it reaches x $0630, carrying him
 * while he stays on.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_ActorSpeedXSetAccel(void);
void abi_AnimStart(void);
void abi_ActorSolidForPlayer(void);
void abi_ActorHoldPlayer(void);
void abi_ActorAccelerateX(void);
void abi_ActorMoveRight(void);

#define K77_ON 0x40u                /* nonzero while Aladdin stands on it this frame */
#define K77_SPEED_CAP 0x41u         /* word: it speeds up while its speed is below this */
#define K77_END_X 0x43u             /* word: it moves while its x pixels are below this */
#define K77_13 0x13u                /* 4 at its start */
#define K77_BOX 0x28u               /* ActorSolidForPlayer's box offset */
#define K77_LAST_X 0x000Cu          /* its x pixels word as he got on or last moved with it */
#define K77_LAST_Y 0x000Eu          /* and its y pixels word */
#define PLAYER_X 0x08E7u            /* Aladdin's x pixels word */
#define PLAYER_Y 0x08EAu            /* Aladdin's y pixels word */

/* JSL at site to C that may push: its return on the stack while it runs. */
static void jsl_at(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/* Aladdin's position word at at, plus the platform's move along its own
   word own since kept (LDA own, SEC, SBC kept, CLC, ADC at, STA at). */
static void carry(uint16_t own, uint16_t kept, uint16_t at)
{
    uint16_t v = dp16(own);

    flag_set(P_C, 1);
    v = op_sbc16(v, rd16(dba(kept)), &cpu.p);
    flag_set(P_C, 0);
    v = op_adc16(v, rd16(dba(at)), &cpu.p);
    cpu.a = v;
    wr16(dba(at), v);
}

/*
 * @routine 83:F4AA Kind77Start
 * @actor
 * WHAT IT DOES: Actor kind $77: a solid platform (box $28) that Aladdin
 * stands on and that holds him as ActorHoldPlayer does; once he is on it,
 * each frame it speeds up to the right (acceleration $10, from speed $48,
 * up to $00C0) until its x reaches $0630, and while he is on it he moves
 * with it.
 * HOW: K77_SPEED_CAP $00C0, K77_END_X $0630; ActorSpeedXSetAccel A = $10,
 * Y = $48; ACTOR_08 bit 1 and ACTOR_09 bits $90 set; AnimStart 0; K77_13
 * 4. Each frame (BRK first) ActorSolidForPlayer Y = $28 until C clear,
 * ActorHoldPlayer between. Then each frame: K77_ON = (ActorSolidForPlayer
 * C clear), and on: its x, y kept at K77_LAST_X, K77_LAST_Y; x below
 * K77_END_X: speed below K77_SPEED_CAP: ActorAccelerateX; ActorMoveRight;
 * K77_ON: Aladdin's x and y + its move since; ActorHoldPlayer; BRK.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $77 in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind77Start(void)
{
    int on = 1;
    uint16_t v, m;

    a8_load(0xC0);
    wr8(dpa(K77_SPEED_CAP), 0xC0);
    wr8(dpa(K77_SPEED_CAP + 1), 0);
    a8_load(0x06);
    wr8(dpa(K77_END_X + 1), 0x06);
    a8_load(0x30);
    wr8(dpa(K77_END_X), 0x30);
    a8_load(0x10);
    y8_load(0x48);
    jsl_at(0x83F4BCu, abi_ActorSpeedXSetAccel);
    a8_load(0x02);
    tsb_at(dpa(ACTOR_08), 0x02);
    a8_load(0x90);
    tsb_at(dpa(ACTOR_09), 0x90);
    a8_load(0x00);
    jsl_at(0x83F4CAu, abi_AnimStart);
    a8_load(0x04);
    wr8(dpa(K77_13), 0x04);

    /* until Aladdin is on it */
    for (;;) {
        actor_yield(0x83F4D2u);
        y8_load(K77_BOX);
        call_frame(0xF4D8u, abi_ActorSolidForPlayer);
        if (!(cpu.p & P_C))
            break;
        call_frame(0xF4DDu, abi_ActorHoldPlayer);
    }

    for (;;) {
        if (!on) {
            actor_yield(0x83F4E0u);
            wr8(dpa(K77_ON), 0);
            y8_load(K77_BOX);
            call_frame(0xF4E8u, abi_ActorSolidForPlayer);
            on = !(cpu.p & P_C);
        }
        if (on) {                            /* he is on: where it is now, kept */
            (void)inc8_at(dpa(K77_ON));
            op_rep(0x20);
            v = dp16(ACTOR_X + 1);
            a16_load(v);
            wr16(dba(K77_LAST_X), v);
            v = dp16(ACTOR_Y + 1);
            a16_load(v);
            wr16(dba(K77_LAST_Y), v);
            op_sep(0x20);
        }
        on = 0;

        /* to the right, faster up to its cap, until its end x */
        op_rep(0x20);
        v = dp16(ACTOR_X + 1);
        a16_load(v);
        m = dp16(K77_END_X);
        cmp16_of(v, m);
        if (!(cpu.p & P_C)) {
            v = dp16(ACTOR_SPEED_X);
            a16_load(v);
            m = dp16(K77_SPEED_CAP);
            cmp16_of(v, m);
            op_sep(0x20);
            if (!(cpu.p & P_C))
                jsl_at(0x83F50Bu, abi_ActorAccelerateX);
            jsl_at(0x83F50Fu, abi_ActorMoveRight);
        }
        op_sep(0x20);

        /* Aladdin moved with it while he is on */
        a8_load(rd8(dpa(K77_ON)));
        if ((uint8_t)cpu.a) {
            op_rep(0x20);
            carry(ACTOR_X + 1, K77_LAST_X, PLAYER_X);
            carry(ACTOR_Y + 1, K77_LAST_Y, PLAYER_Y);
            op_sep(0x20);
        }
        call_frame(0xF539u, abi_ActorHoldPlayer);
    }
}
