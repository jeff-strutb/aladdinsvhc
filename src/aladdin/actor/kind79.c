/*
 * kind79.c: actor kind $79, which starts below the point actors are kept
 * in range of and rises until it is taken up (its $42 set), then carries
 * the player (code not yet reached by any script).
 */
#include "machine.h"
#include "cpu_ops.h"
#include "native.h"
#include "actor.h"
#include "actor_anim.h"

void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_ActorSpeedYSet(void);
void abi_ActorStepYBack(void);
void abi_ActorZoneHasTestPointLong(void);
void abi_ActorDownedByShot(void);
void abi_PlayerLandsOnActor(void);
void abi_ActorEndDropReturn(void);
void abi_Kind79InRange(void);
void abi_Kind79SlotClear(void);

#define RANGE_Y 0x1A46u      /* the y of the point actors are kept in range of (word) */
#define KIND79_FIRST_Y 0x45u /* the first of a pair: its y as it was placed (word) */
#define KIND79_TAKEN 0x42u   /* nonzero: it is taken up, and goes on */
#define KIND79_43 0x43u      /* counted up each frame a shot reaches it */
#define KIND79_SLOT 0x44u    /* its offset in SLOT_WORDS */
#define KIND79_WAIT 0x47u    /* frames left before it can be taken up (counted down) */
#define KIND79_LOW 0x49u     /* nonzero: it came up past y $0032 (hit state each frame) */
#define RANGE_X 0x1A44u      /* the x of the point actors are kept in range of (word) */
#define SLOT_WORDS 0x1C77u   /* a word per kind $79 slot, cleared as it leaves */
#define PLAYER_STATE_8C_COUNT 0x141Bu /* counted up on entering state $8C */
#define V_141C 0x141Cu       /* zero: no taking hold */
#define HEALTH 0x0367u       /* the player's health */
#define PLAYER_RESUME 0x08DEu /* the player's resume address (its record's $03, word) */
#define KIND79_ZONE 0x80     /* the zone it is taken up by */
#define KIND79_RISE 0x6F     /* its vertical speed's entry in the speed table */
/* Not yet filed (no script reaches it): the part after it is taken up,
   which carries the player. */
#define KIND79_TAKEN_ON 0x83F593u

/*
 * @routine 83:F541 Kind79Rise
 * @actor
 * WHAT IT DOES: Actor kind $79: starts 32 pixels (an even $07: the first
 * of a pair, which marks its $07 odd) or 112 pixels (an odd $07) below
 * the point actors are kept in range of, and rises each frame until it
 * is taken up.
 * HOW: Even $07: $07 up, its y kept in KIND79_FIRST_Y, its y RANGE_Y plus
 * $20, ACTOR_09 = $90, ACTOR_12 = 8. Odd: its y RANGE_Y plus $70. Then
 * ACTOR_08 = 2, animation 0, ActorSpeedYSet $6F, $41, KIND79_TAKEN,
 * KIND79_43 and ACTOR_49 cleared; each frame (BRK first)
 * ActorStepYBack, until KIND79_TAKEN is set; then on at KIND79_TAKEN_ON,
 * which no script reaches yet: its C is not written, and it runs as the
 * original's (jump_routine).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $79 in state $0C (the table at
 * $83:8CAB). Exit: never returns.
 */
void abi_Kind79Rise(void)
{
    uint8_t pair = rd8(dpa(ACTOR_07));
    uint16_t y;

    if (!(pair & 1)) {
        /* the first of a pair: its $07 made odd, its place kept */
        wr8(dpa(ACTOR_07), (uint8_t)(pair + 1));
        dp16_set(KIND79_FIRST_Y, dp16(ACTOR_Y + 1));
        flag_set(P_C, 0);
        y = op_adc16(rd16(dba(RANGE_Y)), 0x0020, &cpu.p);
        dp16_set(ACTOR_Y + 1, y);
        wr8(dpa(ACTOR_09), 0x90);
        wr8(dpa(ACTOR_12), 0x08);
    } else {
        flag_set(P_C, 0);
        y = op_adc16(rd16(dba(RANGE_Y)), 0x0070, &cpu.p);
        dp16_set(ACTOR_Y + 1, y);
    }
    wr8(dpa(ACTOR_08), 0x02);
    cpu.a = (uint16_t)(y & 0xFF00);       /* animation 0; A's high byte the sum's */
    nz8_of(0);
    abi_AnimStart();
    y8_load(KIND79_RISE);
    abi_ActorSpeedYSet();
    wr8(dpa(ACTOR_41), 0);
    wr8(dpa(KIND79_TAKEN), 0);
    wr8(dpa(KIND79_43), 0);
    wr8(dpa(ACTOR_49), 0);
    for (;;) {
        actor_yield(0x83F589u);
        abi_ActorStepYBack();
        uint8_t taken = rd8(dpa(KIND79_TAKEN));
        a8_load(taken);
        if (taken)
            break;
    }
    jump_routine(KIND79_TAKEN_ON);
}

/*
 * @routine 83:F68C Kind79Wait
 * WHAT IT DOES: What actor kind $79 runs each frame it waits: its wait
 * counted down; unless its $41 is set, it is taken up (KIND79_TAKEN up)
 * when Aladdin's test box is in its zone $80 and he can take hold (not in
 * state $8C, V_141C set, health left, its wait over); it counts the shots
 * that reach it; it goes to state $8C when he lands on it, once it has
 * come up past y $0032 (its $07 bit 1), and every frame after that; then
 * its animation steps, and on into Kind79InRange.
 * HOW: KIND79_WAIT nonzero: down. ACTOR_41 zero: KIND79_LOW set, state
 * |= $8C; Y = $80, ActorZoneHasTestPointLong, C clear and
 * PLAYER_STATE_8C_COUNT 0, V_141C, HEALTH nonzero, KIND79_WAIT 0:
 * KIND79_TAKEN up; ActorDownedByShot C clear: KIND79_43 up;
 * PlayerLandsOnActor Z clear: state |= $8C; $07 bit 1 with its y
 * (16-bit) below $0032: state |= $8C, KIND79_LOW up. Then AnimStep and on
 * into Kind79InRange (JMP).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM, the zones' and hit boxes' bank; called by JSR
 * through the actor loop's wait table ($83:8F4F, kind $79). Exit: as
 * Kind79InRange.
 */
void abi_Kind79Wait(void)
{
    a8_load(rd8(dpa(KIND79_WAIT)));
    if ((uint8_t)cpu.a)
        (void)dec8_at(dpa(KIND79_WAIT));
    a8_load(rd8(dpa(ACTOR_41)));
    if (!(uint8_t)cpu.a) {
        a8_load(rd8(dpa(KIND79_LOW)));
        if ((uint8_t)cpu.a) {
            a8_load(0x8C);
            tsb_at(dpa(ACTOR_STATE), 0x8C);
        }
        y8_load(KIND79_ZONE);
        abi_ActorZoneHasTestPointLong();
        if (!(cpu.p & P_C)) {
            a8_load(rd8(dba(PLAYER_STATE_8C_COUNT)));
            if (!(uint8_t)cpu.a) {
                a8_load(rd8(dba(V_141C)));
                if ((uint8_t)cpu.a) {
                    a8_load(rd8(dba(HEALTH)));
                    if ((uint8_t)cpu.a) {
                        a8_load(rd8(dpa(KIND79_WAIT)));
                        if (!(uint8_t)cpu.a)
                            (void)inc8_at(dpa(KIND79_TAKEN));
                    }
                }
            }
        }
        abi_ActorDownedByShot();
        if (!(cpu.p & P_C))
            (void)inc8_at(dpa(KIND79_43));
        abi_PlayerLandsOnActor();
        if (!(cpu.p & P_Z)) {
            a8_load(0x8C);
            tsb_at(dpa(ACTOR_STATE), 0x8C);
        }
        a8_load((uint8_t)(rd8(dpa(ACTOR_07)) & 0x02));
        if ((uint8_t)cpu.a) {
            op_rep(0x20);
            a16_load(dp16(ACTOR_Y + 1));
            cmp16_of(cpu.a, 0x0032);
            op_sep(0x20);
            if (!(cpu.p & P_C)) {
                a8_load(0x8C);
                tsb_at(dpa(ACTOR_STATE), 0x8C);
                (void)inc8_at(dpa(KIND79_LOW));
            }
        }
    }
    abi_AnimStep();
    abi_Kind79InRange();
}

/*
 * @routine 83:F639 Kind79InRange
 * WHAT IT DOES: Keeps actor kind $79 near the point actors are kept in
 * range of: more than $D0 pixels to either side, its slot is cleared and
 * it ends; from $98 pixels below to $B0 above, nothing; out of that
 * height it starts again (state $0C), and if it had been taken up while
 * Aladdin has health left, his code goes on at $CDD2.
 * HOW: 16-bit: RANGE_X less its x, plus $D0, $01A0 or more:
 * Kind79SlotClear, then ActorEndDropReturn (JMP): the actor loop's call
 * dropped, the actor ended. Else RANGE_Y less its y, plus $98, below
 * $0148: return. Else KIND79_TAKEN and HEALTH nonzero: PLAYER_RESUME =
 * $CDD2; state = its low nibble | $0C.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by a jump from Kind79Wait, one JSR
 * (the actor loop's) above the actor loop's level. Exit: M 8-bit; A =
 * the new state, or the height test's difference (16 bits) with N, Z and
 * C (clear) from its compare; or as ActorEndDropReturn.
 */
void abi_Kind79InRange(void)
{
    uint8_t st;

    op_rep(0x20);
    flag_set(P_C, 1);
    a16_load(op_sbc16(rd16(dba(RANGE_X)), dp16(ACTOR_X + 1), &cpu.p));
    flag_set(P_C, 0);
    a16_load(op_adc16(cpu.a, 0x00D0, &cpu.p));
    cmp16_of(cpu.a, 0x01A0);
    op_sep(0x20);
    if (cpu.p & P_C) {
        abi_Kind79SlotClear();
        abi_ActorEndDropReturn();
        return;
    }
    op_rep(0x20);
    flag_set(P_C, 1);
    a16_load(op_sbc16(rd16(dba(RANGE_Y)), dp16(ACTOR_Y + 1), &cpu.p));
    flag_set(P_C, 0);
    a16_load(op_adc16(cpu.a, 0x0098, &cpu.p));
    cmp16_of(cpu.a, 0x0148);
    op_sep(0x20);
    if (!(cpu.p & P_C))
        return;
    a8_load(rd8(dpa(KIND79_TAKEN)));
    if ((uint8_t)cpu.a) {
        a8_load(rd8(dba(HEALTH)));
        if ((uint8_t)cpu.a) {
            a8_load(0xD2);
            wr8(dba(PLAYER_RESUME), 0xD2);
            a8_load(0xCD);
            wr8(dba((uint16_t)(PLAYER_RESUME + 1)), 0xCD);
        }
    }
    st = (uint8_t)((rd8(dpa(ACTOR_STATE)) & 0x0F) | 0x0C);
    a8_load(st);
    wr8(dpa(ACTOR_STATE), st);
    op_sep(0x20);
}

/*
 * @routine 83:F683 Kind79SlotClear
 * WHAT IT DOES: Clears actor kind $79's word in SLOT_WORDS as it leaves.
 * HOW: X = KIND79_SLOT; SLOT_WORDS + X and the byte after it 0.
 * Entry: X 8-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSR. Exit: X = KIND79_SLOT, N and Z from it.
 */
void abi_Kind79SlotClear(void)
{
    x8_load(rd8(dpa(KIND79_SLOT)));
    wr8(dba((uint16_t)(SLOT_WORDS + cpu.x)), 0);
    wr8(dba((uint16_t)(SLOT_WORDS + 1 + cpu.x)), 0);
}
