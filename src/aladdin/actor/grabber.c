/*
 * grabber.c: actor kinds $46 and $62, a thing that catches Aladdin,
 * holds and shakes him, lets him fall, breaks into pieces and falls
 * itself, then bounces him. Drafted by cdraft.py; named and described by
 * hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorBouncesPlayer(void);
void abi_ActorFall(void);
void abi_ActorFallTileTest(void);
void abi_ActorFlyOffEnd(void);
void abi_ActorMotionReset(void);
void abi_ActorSpawnSameLook(void);
void abi_ActorZoneHasTestPointLong(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_GrabberStart(void);
void abi_PlayerToActor(void);
void abi_Point145DToActor(void);
void abi_SoundQueueAdd(void);

#define GRAB_FRAMES 0x40u         /* frames, or shake steps, left */
#define GRAB_KIND 0x42u           /* 0: kind $46; 1: kind $62 */
#define GRAB_DROPPED 0x43u        /* nonzero: straight to the shake, the player not moved */
#define GRAB_HELD 0x44u           /* nonzero: it holds the player */
#define GRAB_SHAKE_AT 0xD2CAu     /* in bank $80: by step (1-8), the offset in GRAB_SHAKES */
#define GRAB_SHAKES 0xD2D3u       /* in bank $80: x steps of the shake (words) */
#define GRAB_V_141B 0x141Bu       /* nonzero: it does not catch */
#define GRAB_V_145A 0x145Au       /* negative: it does not catch */
#define GRAB_V_1427 0x1427u       /* zero: it lets go */
#define GRAB_PLAYER_FLIP (PLAYER + ACTOR_FLIP)
#define GRAB_PLAYER_RESUME (PLAYER + ACTOR_03) /* the player's resume address (word) */
#define GRAB_PLAYER_X (PLAYER + ACTOR_X + 1)   /* his x pixels (word) */
#define GRAB_HOLD_CODE 0xD6F8     /* where it sends the player's code while it holds him */
#define GRAB_FALL_CODE 0xCDD2     /* PlayerFall: where it sends it as it lets go */
#define GRAB_ZONE 0x6C            /* ActorZoneHasTestPointLong's zone */
#define GRAB_LAND_Y 0x00A8        /* kind $62 falls on from these y pixels down */

/* ActorSpawnSameLook with its hit box, each child counted. */
static int spawn_counted(void)
{
    lda8(ea_dp(ACTOR_06));
    abi_ActorSpawnSameLook();
    if (IF_N)
        return 0;
    ldx8(ea_dp(ACTOR_06));
    lda8(ea_longx(KIND_COUNT));
    ACC8(inc);
    sta8(ea_longx(KIND_COUNT));
    return 1;
}

/*
 * @routine 83:C761 Kind46Start
 * @actor
 * WHAT IT DOES: Actor kind $46: GrabberStart with GRAB_KIND 0.
 * HOW: GRAB_KIND cleared, on into GrabberStart.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $46 in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind46Start(void)
{
    stz8(ea_dp(GRAB_KIND));
    abi_GrabberStart();
}

/*
 * @routine 83:C763 GrabberStart
 * @actor
 * WHAT IT DOES: The code of actor kinds $46 and $62 (GRAB_KIND: 0 for
 * $46, 1 for $62), a thing that catches Aladdin: it waits until he comes
 * into its zone, holds him (his code sent to $D6F8, his place kept at
 * it) for 48 frames, shakes for 8 steps of 2 frames, lets him fall (his
 * code sent to PlayerFall) if he is still held, breaks into two pieces
 * and falls; when it lands it breaks into three more, settles on the
 * tile below, and from then on bounces him when he comes down on it,
 * with a sound. Kind $62's stays hittable, makes no landing sound, and
 * after landing keeps falling and animating; with $07 bit 7 set it flies
 * off at once (ActorFlyOffEnd).
 * HOW: GRAB_DROPPED and GRAB_HELD 0; ACTOR_08 bit 1 and ACTOR_09 bits
 * $90 set (TSB). The facing ($15) from $07 bit 0; AnimStart 0. Each frame
 * (BRK first) until caught: GRAB_DROPPED nonzero goes straight to the
 * shake; on when V_141B is 0, V_145A not negative and
 * ActorZoneHasTestPointLong (Y = $6C) says C clear. Caught: its $14 = 1
 * when the player's $15 is 0, else 0; the player's resume address =
 * $D6F8; GRAB_HELD up; Point145DToActor; BRK; PlayerToActor; 48 frames
 * (BRK each), GRAB_HELD cleared when V_1427 is 0. The shake, GRAB_FRAMES
 * 8 down to 1: its x pixels plus the word of GRAB_SHAKES at the offset
 * GRAB_SHAKE_AT gives (16-bit, C clear); unless GRAB_DROPPED: V_1427 0
 * clears GRAB_HELD, else with GRAB_HELD the player's x = the same; COP 2.
 * Then with V_1427 and GRAB_HELD the player's resume address = $CDD2
 * (PlayerFall). Its $15 = its $14; kind $46: ACTOR_09 bit 7 cleared;
 * AnimStart 2; ActorMotionReset with gravity $38; $07 = $81, ACTOR_SPAWN_AT
 * $40: ActorSpawnSameLook with its hit box ($06), each child counted in
 * KIND_COUNT by $06, $07 down, while $07 is negative. Each frame (BRK
 * first): AnimStep, ActorFall, (kind $62: on at y pixels $A8 and below)
 * ActorFallTileTest until Z (the tile below of kind 2). Landed: sound $18
 * for kind $46 when ACTOR_09 bit 6 is set. $07 = $84, ACTOR_SPAWN_AT $42:
 * children as before, its $15 flipped while $07 is at least $84, $07 down
 * until $81. Its y pixels set to (y + 10) AND $FFF0 less 9. Kind $62:
 * each frame (BRK first) AnimStep, ActorFall, for good. Kind $46:
 * AnimStart 1, ACTOR_08 low bits 2; each frame (BRK first)
 * ActorBouncesPlayer, and on a bounce sound $12. No script runs kind
 * $62, GRAB_DROPPED set or the hold's GRAB_HELD clear: written from their
 * bytes.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80 (the shake
 * tables' bank); GRAB_KIND set; reached from Kind46Start (falling in) or
 * Kind62Start (a jump). Exit: never returns.
 */
void abi_GrabberStart(void)
{
    stz8(ea_dp(GRAB_DROPPED));
    stz8(ea_dp(GRAB_HELD));
    lda8_v(0x02);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0x90);
    tsb8(ea_dp(ACTOR_09));
    lda8(ea_dp(ACTOR_07));
    if (IF_N) {
        abi_ActorFlyOffEnd();
        return;
    }
    lda8(ea_dp(ACTOR_07));
    and8_v(0x01);
    sta8(ea_dp(ACTOR_FLIP));
    lda8_v(0x00);
    abi_AnimStart();

    /* until it catches him */
    for (;;) {
        actor_yield(0x83C782u);
        lda8(ea_dp(GRAB_DROPPED));
        if (!IF_Z)
            goto shake;
        lda8(ea_abs(GRAB_V_141B));
        if (!IF_Z)
            continue;
        lda8(ea_abs(GRAB_V_145A));
        if (IF_N)
            continue;
        ldy8_v(GRAB_ZONE);
        abi_ActorZoneHasTestPointLong();
        if (!IF_C)
            break;
    }
    stz8(ea_dp(ACTOR_FACING));
    lda8(ea_abs(GRAB_PLAYER_FLIP));
    if (IF_Z)
        MEM8(inc, ea_dp(ACTOR_FACING));
    lda8_v(GRAB_HOLD_CODE & 0xFF);
    sta8(ea_abs(GRAB_PLAYER_RESUME));
    lda8_v(GRAB_HOLD_CODE >> 8);
    sta8(ea_abs(GRAB_PLAYER_RESUME + 1));
    MEM8(inc, ea_dp(GRAB_HELD));
    abi_Point145DToActor();
    actor_yield(0x83C7B4u);
    abi_PlayerToActor();
    lda8_v(0x30);
    sta8(ea_dp(GRAB_FRAMES));
    do {                                  /* holds him 48 frames */
        lda8(ea_abs(GRAB_V_1427));
        if (IF_Z)
            stz8(ea_dp(GRAB_HELD));
        actor_yield(0x83C7C4u);
        MEM8(dec, ea_dp(GRAB_FRAMES));
    } while (!IF_Z);

shake:
    lda8_v(0x08);
    sta8(ea_dp(GRAB_FRAMES));
    do {
        ldx8(ea_dp(GRAB_FRAMES));
        ldy8(ea_absx(GRAB_SHAKE_AT));
        op_rep(0x21);
        lda16(ea_dp(ACTOR_X + 1));
        adc16_v(rd16(ea_absy(GRAB_SHAKES)));
        sta16(ea_dp(ACTOR_X + 1));
        ldx8(ea_dp(GRAB_DROPPED));
        if (IF_Z) {
            ldx8(ea_abs(GRAB_V_1427));
            if (IF_Z) {
                ldx8_v(0x00);
                stx8(ea_dp(GRAB_HELD));
            } else {
                ldx8(ea_dp(GRAB_HELD));
                if (!IF_Z)
                    sta16(ea_abs(GRAB_PLAYER_X));
            }
        }
        op_sep(P_M);
        lda8_v(0x02);
        actor_wait(0x83C7F6u);
        MEM8(dec, ea_dp(GRAB_FRAMES));
    } while (!IF_Z);
    ldx8(ea_abs(GRAB_V_1427));
    if (!IF_Z) {
        ldx8(ea_dp(GRAB_HELD));
        if (!IF_Z) {                      /* lets him fall */
            lda8_v(GRAB_FALL_CODE & 0xFF);
            sta8(ea_abs(GRAB_PLAYER_RESUME));
            lda8_v(GRAB_FALL_CODE >> 8);
            sta8(ea_abs(GRAB_PLAYER_RESUME + 1));
        }
    }

    /* breaks into two and falls */
    lda8(ea_dp(ACTOR_FACING));
    sta8(ea_dp(ACTOR_FLIP));
    lda8(ea_dp(GRAB_KIND));
    if (IF_Z) {
        lda8_v(0x80);
        trb8(ea_dp(ACTOR_09));
    }
    lda8_v(0x02);
    abi_AnimStart();
    lda8_v(0x38);
    abi_ActorMotionReset();
    lda8_v(0x81);
    sta8(ea_dp(ACTOR_07));
    lda8_v(0x40);
    sta8(ea_dp(ACTOR_SPAWN_AT));
    while (spawn_counted()) {
        MEM8(dec, ea_dp(ACTOR_07));
        if (!IF_N)
            break;
    }
    for (;;) {
        actor_yield(0x83C846u);
        abi_AnimStep();
        abi_ActorFall();
        lda8(ea_dp(GRAB_KIND));
        if (!IF_Z) {
            lda8(ea_dp(ACTOR_Y + 1));
            cmp8_v(GRAB_LAND_Y);
            if (IF_C)
                goto landed;
        }
        push16(0xC85C);
        abi_ActorFallTileTest();
        cpu.s = (uint16_t)(cpu.s + 2);
        if (IF_Z)
            break;
    }
    lda8(ea_dp(ACTOR_09));
    and8_v(0x40);
    if (!IF_Z) {
        lda8(ea_dp(GRAB_KIND));
        if (IF_Z) {
            lda8_v(0x18);
            abi_SoundQueueAdd();
        }
    }

landed:
    /* three more pieces, and it settles on the tile */
    lda8_v(0x84);
    sta8(ea_dp(ACTOR_07));
    lda8_v(0x42);
    sta8(ea_dp(ACTOR_SPAWN_AT));
    while (spawn_counted()) {
        lda8(ea_dp(ACTOR_07));
        cmp8_v(0x84);
        if (IF_C) {
            lda8(ea_dp(ACTOR_FLIP));
            eor8_v(0x01);
            sta8(ea_dp(ACTOR_FLIP));
        }
        MEM8(dec, ea_dp(ACTOR_07));
        lda8(ea_dp(ACTOR_07));
        cmp8_v(0x81);
        if (IF_Z)
            break;
    }
    op_rep(0x21);
    lda16(ea_dp(ACTOR_Y + 1));
    adc16_v(0x000A);
    and16_v(0xFFF0);
    sec();
    sbc16_v(0x0009);
    sta16(ea_dp(ACTOR_Y + 1));
    op_sep(P_M);
    lda8(ea_dp(GRAB_KIND));
    if (!IF_Z) {
        for (;;) {                        /* kind $62: falls on */
            actor_yield(0x83C8D1u);
            abi_AnimStep();
            abi_ActorFall();
        }
    }
    lda8_v(0x01);
    abi_AnimStart();
    lda8(ea_dp(ACTOR_08));
    and8_v(0xFC);
    ora8_v(0x02);
    sta8(ea_dp(ACTOR_08));
    for (;;) {
        actor_yield(0x83C8C2u);
        abi_ActorBouncesPlayer();
        if (IF_Z)
            continue;
        lda8_v(0x12);
        abi_SoundQueueAdd();
    }
}

/*
 * @routine 83:C95F Kind62Start
 * @actor
 * WHAT IT DOES: Actor kind $62: GrabberStart with GRAB_KIND 1.
 * HOW: GRAB_KIND = 1, JMP GrabberStart. No script runs it: written from
 * its bytes.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $62 in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind62Start(void)
{
    lda8_v(0x01);
    sta8(ea_dp(GRAB_KIND));
    abi_GrabberStart();
}
