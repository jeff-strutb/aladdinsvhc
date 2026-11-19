/*
 * kind66.c: actor kind $66 (stage a2 s1 and others) by its $07: platforms
 * that swing up and down, rise when signalled, or carry Aladdin up and
 * down holding him, a gate that waits for its parent to end, or the
 * holder of kind $6C. Its wait is Kind66WaitWork (kind_waits_84.c).
 * Written instruction for instruction from the assembly (kind66.asm),
 * every JSR and JSL with its return frame.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram_player.h"

void abi_ActorEndOutOfRangeLong(void);
void abi_ActorEndPlaced(void);
void abi_ActorSpawn(void);
void abi_ActorSpeedYSet(void);
void abi_ActorStepY(void);
void abi_ActorStepYBack(void);
void abi_ActorZoneHasTestPointLong(void);
void abi_AnimStart(void);
void abi_Kind6CStart(void);
void abi_SoundQueueAdd(void);
void abi_Kind66Gate(void);
void abi_Kind66MarkTop(void);
void abi_Kind66PlayerHeldAt(void);
void abi_Kind66PlayerOnTop(void);
void abi_Kind66Spawn(void);
void abi_Kind66TellOthers(void);
void abi_Kind66Wobble(void);

#define K66_COUNT ACTOR_40      /* frames left of a stroke */
#define K66_PERIOD ACTOR_41     /* a stroke's frames */
#define K66_TOP ACTOR_42        /* its y pixels as it starts (word) */
#define K66_GO ACTOR_44         /* nonzero: signalled to move (set by Kind66TellOthers) */
#define K66_SIGNAL ACTOR_45     /* what Kind66TellOthers sets in its siblings' K66_GO */
#define K66_LOW ACTOR_46        /* its lowest y pixels: K66_TOP less $38 (word) */
#define K66_STEP ACTOR_48       /* Kind66Wobble's step */
#define K66_PARENT ACTOR_49     /* the record of the actor that spawned it (word) */
#define K66_HELD 0x4Bu          /* nonzero: no touch or shot tests (Kind66WaitWork) */
#define K66_PAUSE ACTOR_4C      /* frames the lift waits before it takes the player again */
#define K66_ZONE 0x78           /* the lift's zone for the player (ActorZoneHasTestPointLong's Y) */
#define V_141B 0x141Bu
#define V_1427 0x1427u          /* nonzero: no landing on actors */
#define NO_LANDING 0x145Au      /* nonzero: frames with no hold on a ledge; negative: none */
#define PLAYER_CODE (PLAYER + ACTOR_03)    /* the player's resume address (word) */
#define PLAYER_X (PLAYER + ACTOR_X + 1)    /* his x pixels (word) */
#define PLAYER_Y (PLAYER + ACTOR_Y + 1)    /* his y pixels (word) */
#define KIND_COUNT 0x7EFF00u    /* by kind, how many such actors are live */

/* A JSL at `site`: its return address on the stack while the callee runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 84:839E Kind66Start
 * @actor
 * WHAT IT DOES: Actor kind $66, by its $07: bit 4, kind $6C's holder
 * (pose 3, its touches off, on as Kind6CStart); bit 3, a gate
 * (Kind66Gate); bit 2, a lift that carries Aladdin down while he stands on
 * it and back up; bit 1, a platform that rises out of reach while
 * signalled and comes back; else a platform swinging up and down for good
 * (bit 0: the shorter swing).
 * HOW: $08 bits 0-1 and $09 bits $90 set (TSB); K66_HELD 0. Bit 4:
 * AnimStart 3, K66_HELD up, JML Kind6CStart. Bit 3: on at Kind66Gate.
 * K66_GO 0. Bit 2 (the lift): Kind66Spawn X = $0B at spawn place $5A; $09
 * bit 7 off; AnimStart 1; Kind66MarkTop; K66_SIGNAL and K66_PAUSE 0. Each
 * frame (BRK first) ActorEndOutOfRangeLong X = 0; K66_PAUSE: down, the
 * next frame; V_141B set or NO_LANDING negative: the next frame;
 * ActorZoneHasTestPointLong Y = K66_ZONE, C set: the next frame. Taken
 * (no script runs it): the player's code word $D9C5 (PlayerSwitchHang),
 * Kind66PlayerHeldAt, BRK, Kind66PlayerOnTop, COP 4; each frame (BRK
 * first) while V_1427 is set: its y pixels 2 lower (16-bit), past
 * K66_TOP + $10 sound $15, K66_SIGNAL up, Kind66TellOthers, BRK until
 * V_1427 clears; else Kind66PlayerOnTop. Then K66_SIGNAL 0,
 * Kind66TellOthers; each frame (BRK first) up a pixel until at K66_TOP;
 * K66_PAUSE $10; again. Else AnimStart 0, K66_STEP 0. Bit 1 (the rise):
 * Kind66Spawn X = $0A at $58; ActorSpeedYSet Y = $48; Kind66MarkTop; each
 * frame (BRK) until K66_GO; then each frame (BRK first) while K66_GO and
 * its y at or below K66_LOW: Kind66Wobble, ActorStepYBack; K66_GO 0: COP
 * $38, then each frame (BRK first) Kind66Wobble, ActorStepY until at
 * K66_TOP (K66_GO set again: COP $18, back up). Else (the swing, no script
 * runs it): Kind66Spawn X = $0A at $56; Y $48, X $80 (bit 0 shifted out
 * set: Y $4B, X $40) into K66_COUNT and K66_PERIOD; ActorSpeedYSet; then
 * for good K66_COUNT frames (BRK first) Kind66Wobble and ActorStepYBack,
 * COP $0C, K66_PERIOD frames of Kind66Wobble and ActorStepY, COP $0C.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop for kind $66 in state $0C (the stub at $83:944A). Exit:
 * never returns.
 */
void abi_Kind66Start(void)
{
    lda8_v(0x03);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0x90);
    tsb8(ea_dp(ACTOR_09));
    stz8(ea_dp(K66_HELD));
    lda8(ea_dp(ACTOR_07));
    bit8_i(0x10);
    if (IF_Z)
        goto l83BA;
    lda8_v(0x03);
    jsl_call(0x8483B0u, abi_AnimStart);
    MEM8(inc, ea_dp(K66_HELD));
    abi_Kind6CStart();
    return;
l83BA:
    and8_v(0x08);
    if (IF_Z)
        goto l83C1;
    abi_Kind66Gate();
    return;
l83C1:
    stz8(ea_dp(K66_GO));
    lda8(ea_dp(ACTOR_07));
    and8_v(0x04);
    if (IF_Z)
        goto l83CC;
    goto l847D;
l83CC:
    lda8_v(0x00);
    jsl_call(0x8483CEu, abi_AnimStart);
    stz8(ea_dp(K66_STEP));
    lda8(ea_dp(ACTOR_07));
    and8_v(0x02);
    if (!IF_Z)
        goto l8423;
    lda8_v(0x56);
    sta8(ea_dp(ACTOR_SPAWN_AT));
    ldx8_v(0x0A);
    call_frame(0x83E2, abi_Kind66Spawn);
    ldy8_v(0x48);
    ldx8_v(0x80);
    MEM8(lsr, ea_dp(ACTOR_07));
    if (!IF_C)
        goto l83EF;
    ldy8_v(0x4B);
    ldx8_v(0x40);
l83EF:
    stx8(ea_dp(K66_COUNT));
    stx8(ea_dp(K66_PERIOD));
    jsl_call(0x8483F3u, abi_ActorSpeedYSet);
l83F7:
    actor_yield(0x8483F7u);
    call_frame(0x83FB, abi_Kind66Wobble);
    jsl_call(0x8483FCu, abi_ActorStepYBack);
    MEM8(dec, ea_dp(K66_COUNT));
    if (!IF_Z)
        goto l83F7;
    lda8_v(0x0C);
    actor_wait(0x848406u);
    lda8(ea_dp(K66_PERIOD));
    sta8(ea_dp(K66_COUNT));
l840C:
    actor_yield(0x84840Cu);
    call_frame(0x8410, abi_Kind66Wobble);
    jsl_call(0x848411u, abi_ActorStepY);
    MEM8(dec, ea_dp(K66_COUNT));
    if (!IF_Z)
        goto l840C;
    lda8_v(0x0C);
    actor_wait(0x84841Bu);
    lda8(ea_dp(K66_PERIOD));
    sta8(ea_dp(K66_COUNT));
    goto l83F7;
l8423:
    lda8_v(0x58);
    sta8(ea_dp(ACTOR_SPAWN_AT));
    ldx8_v(0x0A);
    call_frame(0x842B, abi_Kind66Spawn);
    ldy8_v(0x48);
    jsl_call(0x84842Eu, abi_ActorSpeedYSet);
    call_frame(0x8434, abi_Kind66MarkTop);
l8435:
    actor_yield(0x848435u);
    lda8(ea_dp(K66_GO));
    if (IF_Z)
        goto l8435;
l843B:
    actor_yield(0x84843Bu);
    lda8(ea_dp(K66_GO));
    if (IF_Z)
        goto l845A;
    op_rep(0x20);
    lda16(ea_dp(ACTOR_Y + 1));
    cmp16_v(rd16(ea_dp(K66_LOW)));
    op_sep(0x20);
    if (!IF_C)
        goto l8454;
    call_frame(0x844D, abi_Kind66Wobble);
    jsl_call(0x84844Eu, abi_ActorStepYBack);
    goto l843B;
l8454:
    actor_yield(0x848454u);
    lda8(ea_dp(K66_GO));
    if (!IF_Z)
        goto l8454;
l845A:
    lda8_v(0x38);
    actor_wait(0x84845Cu);
l845E:
    actor_yield(0x84845Eu);
    lda8(ea_dp(K66_GO));
    if (!IF_Z)
        goto l8477;
    call_frame(0x8466, abi_Kind66Wobble);
    jsl_call(0x848467u, abi_ActorStepY);
    op_rep(0x20);
    lda16(ea_dp(ACTOR_Y + 1));
    cmp16_v(rd16(ea_dp(K66_TOP)));
    op_sep(0x20);
    if (!IF_C)
        goto l845E;
    goto l8435;
l8477:
    lda8_v(0x18);
    actor_wait(0x848479u);
    goto l843B;
l847D:
    lda8_v(0x5A);
    sta8(ea_dp(ACTOR_SPAWN_AT));
    ldx8_v(0x0B);
    call_frame(0x8485, abi_Kind66Spawn);
    lda8_v(0x80);
    trb8(ea_dp(ACTOR_09));
    lda8_v(0x01);
    jsl_call(0x84848Cu, abi_AnimStart);
    call_frame(0x8492, abi_Kind66MarkTop);
    stz8(ea_dp(K66_SIGNAL));
    stz8(ea_dp(K66_PAUSE));
l8497:
    actor_yield(0x848497u);
    ldx8_v(0x00);
    jsl_call(0x84849Bu, abi_ActorEndOutOfRangeLong);
    lda8(ea_dp(K66_PAUSE));
    if (IF_Z)
        goto l84A7;
    MEM8(dec, ea_dp(K66_PAUSE));
    goto l8497;
l84A7:
    lda8(ea_abs(V_141B));
    if (!IF_Z)
        goto l8497;
    lda8(ea_abs(NO_LANDING));
    if (IF_N)
        goto l8497;
    ldy8_v(K66_ZONE);
    jsl_call(0x8484B3u, abi_ActorZoneHasTestPointLong);
    if (IF_C)
        goto l8497;
    lda8_v(0xC5);
    sta8(ea_abs(PLAYER_CODE));
    lda8_v(0xD9);
    sta8(ea_abs(PLAYER_CODE + 1));
    call_frame(0x84C5, abi_Kind66PlayerHeldAt);
    actor_yield(0x8484C6u);
    call_frame(0x84CA, abi_Kind66PlayerOnTop);
    lda8_v(0x04);
    actor_wait(0x8484CDu);
l84CF:
    actor_yield(0x8484CFu);
    lda8(ea_abs(V_1427));
    if (IF_Z)
        goto l84FE;
    op_rep(0x21);
    MEM16(inc, ea_dp(ACTOR_Y + 1));
    MEM16(inc, ea_dp(ACTOR_Y + 1));
    lda16(ea_dp(K66_TOP));
    adc16_v(0x0010);
    cmp16_v(rd16(ea_dp(ACTOR_Y + 1)));
    op_sep(0x20);
    php();
    call_frame(0x84E8, abi_Kind66PlayerOnTop);
    plp();
    if (IF_C)
        goto l84CF;
    lda8_v(0x15);
    jsl_call(0x8484EEu, abi_SoundQueueAdd);
    MEM8(inc, ea_dp(K66_SIGNAL));
    call_frame(0x84F6, abi_Kind66TellOthers);
l84F7:
    actor_yield(0x8484F7u);
    lda8(ea_abs(V_1427));
    if (!IF_Z)
        goto l84F7;
l84FE:
    stz8(ea_dp(K66_SIGNAL));
    call_frame(0x8502, abi_Kind66TellOthers);
l8503:
    actor_yield(0x848503u);
    op_rep(0x20);
    lda16(ea_dp(ACTOR_Y + 1));
    cmp16_v(rd16(ea_dp(K66_TOP)));
    if (IF_Z)
        goto l8513;
    MEM16(dec, ea_dp(ACTOR_Y + 1));
    op_sep(0x20);
    goto l8503;
l8513:
    op_sep(0x20);
    lda8_v(0x10);
    sta8(ea_dp(K66_PAUSE));
    goto l8497;
}


/*
 * @routine 84:851C Kind66PlayerOnTop
 * WHAT IT DOES: Puts Aladdin on the lift: his x its x, his y its y + $32.
 * HOW: 16-bit (REP #$21, carry clear): PLAYER_X = its x pixels, PLAYER_Y =
 * its y pixels + $32.
 * Entry: M 8-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSR from Kind66Start's lift. Exit: M 8-bit; A low
 * byte = the y's low byte; C, N, Z from the addition.
 */
void abi_Kind66PlayerOnTop(void)
{
    op_rep(0x21);
    lda16(ea_dp(ACTOR_X + 1));
    sta16(ea_abs(PLAYER_X));
    lda16(ea_dp(ACTOR_Y + 1));
    adc16_v(0x0032);
    sta16(ea_abs(PLAYER_Y));
    op_sep(0x20);
    return;
}


/*
 * @routine 84:852E Kind66PlayerHeldAt
 * WHAT IT DOES: Keeps the place Aladdin hangs from the lift at: its x, its
 * y + $32.
 * HOW: 16-bit (REP #$21, carry clear): PLAYER_POS_SAVED = its x pixels, +2
 * = its y pixels + $32.
 * Entry: M 8-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSR from Kind66Start's lift. Exit: as
 * Kind66PlayerOnTop.
 */
void abi_Kind66PlayerHeldAt(void)
{
    op_rep(0x21);
    lda16(ea_dp(ACTOR_X + 1));
    sta16(ea_abs(PLAYER_POS_SAVED));
    lda16(ea_dp(ACTOR_Y + 1));
    adc16_v(0x0032);
    sta16(ea_abs(PLAYER_POS_SAVED + 2));
    op_sep(0x20);
    return;
}


/*
 * @routine 84:8540 Kind66TellOthers
 * WHAT IT DOES: Tells every live actor of its kind its K66_SIGNAL (their
 * K66_GO), so the rising platforms move with the lift.
 * HOW: X 16-bit from $0B5B by $50 while below $141C (the pool's
 * records): state nonzero and kind = its kind: their K66_GO = K66_SIGNAL.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR from Kind66Start's lift. Exit: M
 * and X 8-bit; X = $1C (the end's low byte); C set.
 */
void abi_Kind66TellOthers(void)
{
    op_rep(0x10);
    ldx16_v(0x0B5B);
l8545:
    lda8(ea_absx(ACTOR_STATE));
    if (IF_Z)
        goto l8556;
    lda8(ea_absx(ACTOR_06));
    cmp8_v(rd8(ea_dp(ACTOR_06)));
    if (!IF_Z)
        goto l8556;
    lda8(ea_dp(K66_SIGNAL));
    sta8(ea_absx(K66_GO));
l8556:
    op_rep(0x21);
    txa16();
    adc16_v(0x0050);
    tax16();
    cpx16_v(0x141C);
    op_sep(0x20);
    if (!IF_C)
        goto l8545;
    op_sep(0x10);
    return;
}


/*
 * @routine 84:8567 Kind66MarkTop
 * WHAT IT DOES: Keeps its height as it starts (K66_TOP) and its lowest
 * ($38 above, K66_LOW).
 * HOW: 16-bit (REP #$21): K66_TOP = its y pixels; less $37 with the carry
 * clear (so $38) to K66_LOW.
 * Entry: M 8-bit; D = the actor's record; called by JSR from Kind66Start.
 * Exit: M 8-bit; A = K66_LOW (16 bits); flags from the subtraction.
 */
void abi_Kind66MarkTop(void)
{
    op_rep(0x21);
    lda16(ea_dp(ACTOR_Y + 1));
    sta16(ea_dp(K66_TOP));
    sbc16_v(0x0037);
    sta16(ea_dp(K66_LOW));
    op_sep(0x20);
    return;
}


/*
 * @routine 84:8575 Kind66Wobble
 * WHAT IT DOES: A platform's wobble as it moves: K66_STEP counts 1, 2, 3:
 * at 1 nothing, at 2 its y a pixel up, from 3 on still for good ($FF);
 * the count wrapping to 0 moves it a pixel down.
 * HOW: X 16-bit = its y pixels; K66_STEP up: 0, X up; 1, nothing; 2, X
 * down; else K66_STEP $FF; X stored as its y pixels where it moved. No
 * script runs it.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR from
 * Kind66Start's swing and rise. Exit: M and X 8-bit; A = K66_STEP less
 * what was taken off it; flags from that.
 */
void abi_Kind66Wobble(void)
{
    op_rep(0x10);
    ldx16(ea_dp(ACTOR_Y + 1));
    MEM8(inc, ea_dp(K66_STEP));
    lda8(ea_dp(K66_STEP));
    if (IF_Z)
        goto l858B;
    ACC8(dec);
    if (IF_Z)
        goto l8591;
    ACC8(dec);
    if (IF_Z)
        goto l858E;
    lda8_v(0xFF);
    sta8(ea_dp(K66_STEP));
    goto l8591;
l858B:
    inx16();
    goto l858F;
l858E:
    dex16();
l858F:
    stx16(ea_dp(ACTOR_Y + 1));
l8591:
    op_sep(0x10);
    return;
}


/*
 * @routine 84:8594 Kind66Gate
 * @actor
 * WHAT IT DOES: Kind $66 with $07 bit 3: a piece that stays (pose 2)
 * until the actor that spawned it has ended, then ends.
 * HOW: $08 = 2; $09 bit 7 off (TRB); AnimStart 2; each frame (BRK first)
 * until the state of the record at K66_PARENT (X 16-bit) is 0; JML
 * ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the jump from Kind66Start. Exit: never returns.
 */
void abi_Kind66Gate(void)
{
    lda8_v(0x02);
    sta8(ea_dp(ACTOR_08));
    lda8_v(0x80);
    trb8(ea_dp(ACTOR_09));
    lda8_v(0x02);
    jsl_call(0x84859Eu, abi_AnimStart);
l85A2:
    actor_yield(0x8485A2u);
    op_rep(0x10);
    ldx16(ea_dp(K66_PARENT));
    lda8(ea_absx(ACTOR_STATE));
    op_sep(0x10);
    if (!IF_Z)
        goto l85A2;
    abi_ActorEndPlaced();
    return;
}


/*
 * @routine 84:85B3 Kind66Spawn
 * WHAT IT DOES: Spawns a child of its kind with $07 = X at its spawn
 * place, with its look and itself as the child's parent, and counts the
 * kind's live actors up.
 * HOW: Its $07 pushed, $07 = X; ActorSpawn with A = its kind ($06); N set
 * (none): skip. Else (16-bit) the child's look = its look, the child's
 * K66_PARENT = D; 8-bit: KIND_COUNT,X (X = its kind) up. $07 pulled back.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = the child's $07;
 * ACTOR_SPAWN_AT set; DB = $80; called by JSR from Kind66Start. Exit: M
 * and X 8-bit when spawned (else as ActorSpawn left them); A = its $07.
 */
void abi_Kind66Spawn(void)
{
    lda8(ea_dp(ACTOR_07));
    pha8();
    stx8(ea_dp(ACTOR_07));
    lda8(ea_dp(ACTOR_06));
    jsl_call(0x8485BAu, abi_ActorSpawn);
    if (IF_N)
        goto l85D8;
    op_rep(0x20);
    lda16(ea_dp(ACTOR_LOOK));
    sta16(ea_absx(ACTOR_LOOK));
    tdc();
    sta16(ea_absx(K66_PARENT));
    op_sep(0x30);
    ldx8(ea_dp(ACTOR_06));
    lda8(ea_longx(KIND_COUNT));
    ACC8(inc);
    sta8(ea_longx(KIND_COUNT));
l85D8:
    pla8();
    sta8(ea_dp(ACTOR_07));
    return;
}
