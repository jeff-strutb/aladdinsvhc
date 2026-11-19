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
void abi_ActorFall(void);
void abi_ActorMotionReset(void);
void abi_SoundQueueAdd(void);
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
#define KIND79_HOLD_CODE 0xD6F8u /* the player's code while an actor holds him */
#define KIND79_HOLD_AT 0x145Du   /* where the player is held: x, y (words) */
#define KIND79_BOX_WANTED 0x1448u /* the player's box wanted: $07-$0F while he holds on */
#define KIND79_HOLDER 0x0912u    /* the player's record's $37: the record of what holds him (word) */
#define KIND79_DIP 0x6C          /* its vertical speed's entry as it dips with him */
#define KIND79_DIP_GRAVITY 0x04  /* its gravity as it dips */
#define KIND79_LET_GO_WAIT 0x10  /* frames before it can be taken up again */

void abi_ActorFallAgainst(void);
void abi_ActorSpeedYSetGravity(void);
void abi_Kind79Held(void);
void abi_Kind79PlayerToIt(void);

/* Kind79Rise once taken ($83:F593): the player held at it while it dips
   and comes back up, then carried as it rises; back to the rise (the
   caller's loop) when it is let go. */
static void kind79_taken(void)
{
    uint8_t hi;
    uint16_t v;

    a8_load(KIND79_HOLD_CODE & 0xFF);
    wr8(dba(PLAYER_RESUME), KIND79_HOLD_CODE & 0xFF);
    a8_load(KIND79_HOLD_CODE >> 8);
    wr8(dba((uint16_t)(PLAYER_RESUME + 1)), KIND79_HOLD_CODE >> 8);
    op_rep(0x21);
    a16_load(dp16(ACTOR_X + 1));
    wr16(dba(KIND79_HOLD_AT), cpu.a);
    a16_load(dp16(ACTOR_Y + 1));
    flag_set(P_C, 0);
    a16_load(op_adc16(cpu.a, 0x002B, &cpu.p));
    wr16(dba((uint16_t)(KIND79_HOLD_AT + 2)), cpu.a);
    op_sep(0x20);
    actor_yield(0x83F5AFu);
    a8_load(0x02);
    abi_AnimStart();
    abi_Kind79PlayerToIt();
    a8_load(KIND79_DIP_GRAVITY);
    y8_load(KIND79_DIP);
    abi_ActorSpeedYSetGravity();
dip:
    op_sep(0x20);
dipping:
    actor_yield(0x83F5C4u);
    abi_ActorFallAgainst();
    abi_Kind79Held();
    if (cpu.p & P_C)
        abi_Kind79PlayerToIt();
    hi = rd8(dpa(ACTOR_SPEED_Y + 2));
    a8_load(hi);
    if (!(hi & 0x80))
        goto dip;
    op_rep(0x20);
    v = dp16(ACTOR_SPEED_Y + 1);
    a16_load(v);
    cmp16_of(v, 0xFFA0);
    op_sep(0x20);
    if (v < 0xFFA0)
        goto dipping;
    y8_load(KIND79_RISE);
    abi_ActorSpeedYSet();
    a8_load(rd8(dpa(KIND79_TAKEN)));
    if (!(uint8_t)cpu.a)
        return;
    for (;;) {
        actor_yield(0x83F5EBu);
        abi_ActorStepYBack();
        abi_Kind79Held();
        if (!(cpu.p & P_C))
            return;
        abi_Kind79PlayerToIt();
    }
}

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
 * ActorStepYBack, until KIND79_TAKEN is set. Taken: the player's code
 * KIND79_HOLD_CODE, KIND79_HOLD_AT its x and its y + $2B; BRK; animation
 * 2, Kind79PlayerToIt, ActorSpeedYSetGravity (A 4, Y $6C). Each frame
 * (BRK) ActorFallAgainst and Kind79Held (C set: Kind79PlayerToIt) while
 * it falls or rises faster than $FFA0; ActorSpeedYSet $6F; KIND79_TAKEN
 * cleared (let go): the rise again. Else each frame (BRK) ActorStepYBack
 * and Kind79Held: C set, Kind79PlayerToIt; clear (let go), the rise again.
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
            kind79_taken();
    }
}

/*
 * @routine 83:F5FD Kind79PlayerToIt
 * WHAT IT DOES: Puts Aladdin at actor kind $79's place: its x, and its y
 * plus $2B.
 * HOW: 16-bit (C clear): the actor's x pixels to the player's; its y
 * pixels + $2B to the player's.
 * Entry: M 8-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSR. Exit: M 8-bit; A = the player's new y (16
 * bits); N, V, Z, C from the addition.
 */
void abi_Kind79PlayerToIt(void)
{
    op_rep(0x21);
    a16_load(dp16(ACTOR_X + 1));
    wr16(dba(PLAYER + ACTOR_X + 1), cpu.a);
    a16_load(dp16(ACTOR_Y + 1));
    flag_set(P_C, 0);
    a16_load(op_adc16(cpu.a, 0x002B, &cpu.p));
    wr16(dba(PLAYER + ACTOR_Y + 1), cpu.a);
    op_sep(0x20);
}

/*
 * @routine 83:F610 Kind79Held
 * WHAT IT DOES: Tells whether Aladdin still holds on to actor kind $79
 * (his box wanted $07-$0F): if so it is marked as what holds him; if not
 * it lets him go: animation 0, no longer taken, nothing holds him, and 16
 * frames before it can be taken again.
 * HOW: KIND79_BOX_WANTED from $07 to $0F: KIND79_HOLDER = D, C set. Else
 * AnimStart 0, KIND79_TAKEN 0, KIND79_HOLDER 0 (byte by byte),
 * KIND79_WAIT $10, C clear.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR. Exit: M 8-bit, X 8-bit; C set
 * while he holds on.
 */
void abi_Kind79Held(void)
{
    uint8_t box = rd8(dba(KIND79_BOX_WANTED));

    x8_load(box);
    cmp8_of(box, 0x07);
    if (box >= 0x07) {
        cmp8_of(box, 0x10);
        if (box < 0x10) {
            op_rep(0x20);
            a16_load(cpu.d);                    /* TDC */
            wr16(dba(KIND79_HOLDER), cpu.a);
            op_sep(0x20);
            flag_set(P_C, 1);
            return;
        }
    }
    a8_load(0x00);
    jsl_frame(0x83F627u);
    abi_AnimStart();
    cpu.s = (uint16_t)(cpu.s + 3);
    wr8(dpa(KIND79_TAKEN), 0x00);
    wr8(dba(KIND79_HOLDER), 0x00);
    wr8(dba((uint16_t)(KIND79_HOLDER + 1)), 0x00);
    a8_load(KIND79_LET_GO_WAIT);
    wr8(dpa(KIND79_WAIT), KIND79_LET_GO_WAIT);
    flag_set(P_C, 0);
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
        jsl_frame(0x83F6A0u);
        abi_ActorZoneHasTestPointLong();
        cpu.s = (uint16_t)(cpu.s + 3);
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
        call_frame(0xF6BD, abi_ActorDownedByShot);
        if (!(cpu.p & P_C))
            (void)inc8_at(dpa(KIND79_43));
        call_frame(0xF6C4, abi_PlayerLandsOnActor);
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
    jsl_frame(0x83F6E2u);
    abi_AnimStep();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    uint16_t r;

    op_rep(0x20);
    flag_set(P_C, 1);
    r = rd16(dba(RANGE_X));
    a16_load(op_sbc16(r, dp16(ACTOR_X + 1), &cpu.p));
    flag_set(P_C, 0);
    a16_load(op_adc16(cpu.a, 0x00D0, &cpu.p));
    cmp16_of(cpu.a, 0x01A0);
    op_sep(0x20);
    if (cpu.p & P_C) {
        call_frame(0xF64E, abi_Kind79SlotClear);
        abi_ActorEndDropReturn();
        return;
    }
    op_rep(0x20);
    flag_set(P_C, 1);
    r = rd16(dba(RANGE_Y));
    a16_load(op_sbc16(r, dp16(ACTOR_Y + 1), &cpu.p));
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

/*
 * @routine 83:F6E9 Kind79EndIfCarry
 * WHAT IT DOES: Ends actor kind $79 for good when C is set: its slot
 * cleared, its caller's return dropped, then ActorEndPlaced. C clear, it
 * returns. Nothing in the game calls it (no JSR, JSL, JMP, pointer or
 * word in the ROM names it): unit_bank83_keep_paths runs it with C clear.
 * HOW: BCC to an RTS. Else Kind79SlotClear, then ActorEndDropReturn
 * (JMP).
 * @unreachable 83:F6EB-F6F0 The end: it pulls a caller's return that no
 * caller pushes: no JSR, JSL, JMP, branch, pointer or word in the ROM
 * names 83:F6E9, and a unit call at it would pull the stub's own return
 * and go into the actor loop with no actor running (master, 2026-11-19).
 * Entry: M 8-bit, X 8-bit; C set to end it; D = the actor's record; DB a bank
 * whose $0000-$1FFF is work RAM; called by JSR from the actor's code at
 * its own level. Exit: C clear: everything unchanged; else never
 * returns.
 */
void abi_Kind79EndIfCarry(void)
{
    if (!(cpu.p & P_C))
        return;
    call_frame(0xF6ED, abi_Kind79SlotClear);
    abi_ActorEndDropReturn();
}

/*
 * @routine 83:F6F2 Kind79Second
 * @actor
 * WHAT IT DOES: Actor kind $79 in a state with bit 7 set (landed on, or
 * hit): no longer taken up; landed on (not shot, not up past $0032) it
 * sinks a little with a sound; shot while it carried Aladdin with health
 * left, his code goes on at $CDD2; then it is hit (ACTOR_09 bit 7), poses,
 * sounds $2D, and after 6 frames falls away for good.
 * HOW: $41 up. KIND79_LOW and KIND79_43 both zero: SoundQueueAdd $0F,
 * animation 1, ActorSpeedYSetGravity (A 4, Y $6C), 8 frames (BRK) of
 * ActorFallAgainst. Else KIND79_TAKEN and HEALTH nonzero:
 * PLAYER_RESUME $CDD2, KIND79_TAKEN 0; ACTOR_09 bit 7 set. Then animation
 * 3, SoundQueueAdd $2D, COP 6, ActorMotionReset $30, ActorFall each frame
 * (BRK) for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $79 in a state with bit 7 set (the table
 * at $83:8DFD). Exit: never returns.
 */
void abi_Kind79Second(void)
{
    uint8_t v;

    (void)inc8_at(dpa(ACTOR_41));
    v = rd8(dpa(KIND79_LOW));
    a8_load(v);
    if (!v) {
        v = rd8(dpa(KIND79_43));
        a8_load(v);
        if (!v) {
            /* landed on: a sound, and it sinks 8 frames */
            a8_load(0x0F);
            abi_SoundQueueAdd();
            a8_load(0x01);
            abi_AnimStart();
            a8_load(KIND79_DIP_GRAVITY);
            y8_load(KIND79_DIP);
            jsl_frame(0x83F727u);
            abi_ActorSpeedYSetGravity();
            cpu.s = (uint16_t)(cpu.s + 3);
            a8_load(0x08);
            wr8(dpa(0x40), 0x08);
            do {
                actor_yield(0x83F72Fu);
                jsl_frame(0x83F731u);
                abi_ActorFallAgainst();
                cpu.s = (uint16_t)(cpu.s + 3);
            } while (dec8_at(dpa(0x40)));
            goto pose;
        }
    }
    v = rd8(dpa(KIND79_TAKEN));
    a8_load(v);
    if (v) {
        v = rd8(dba(HEALTH));
        a8_load(v);
        if (v) {
            a8_load(0xD2);
            wr8(dba(PLAYER_RESUME), 0xD2);
            a8_load(0xCD);
            wr8(dba((uint16_t)(PLAYER_RESUME + 1)), 0xCD);
            wr8(dpa(KIND79_TAKEN), 0x00);
        }
    }
    a8_load(0x80);
    tsb_at(dpa(ACTOR_09), 0x80);
pose:
    a8_load(0x03);
    abi_AnimStart();
    a8_load(0x2D);
    abi_SoundQueueAdd();
    a8_load(0x06);
    actor_wait(0x83F747u);
    a8_load(0x30);
    jsl_frame(0x83F74Bu);
    abi_ActorMotionReset();
    cpu.s = (uint16_t)(cpu.s + 3);
    for (;;) {
        actor_yield(0x83F74Fu);
        jsl_frame(0x83F751u);
        abi_ActorFall();
        cpu.s = (uint16_t)(cpu.s + 3);
    }
}
