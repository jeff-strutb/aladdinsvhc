/*
 * kind4d.c: actor kind $4D, a thing that gives way when it is hit, leaps
 * and falls, then drops its children once. Drafted by cdraft.py; named
 * and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorFall(void);
void abi_ActorFlagOnce(void);
void abi_ActorSpeedYSet(void);
void abi_AnimStart(void);
void abi_SoundQueueAdd(void);

#define K4D_CHECK 0x701000u       /* the copier check's bytes: work RAM on a copier, none on the cartridge */
#define K4D_FILL 0xAA             /* what a copier's low bytes are filled with */
#define K4D_FILLED 0x0000u        /* the bytes filled, $0000-$001F */
#define K4D_FRAMES 0x40u          /* frames left */
#define K4D_HITS 0x42u            /* set when it is hit (1: the first time) */
#define K4D_PLAYER_FLIP (PLAYER + ACTOR_FLIP)

/*
 * @routine 83:CE7B Kind4DStart
 * @actor
 * WHAT IT DOES: Actor kind $4D, a thing that gives way when it is hit:
 * once K4D_HITS is set it sounds $12, waits a frame (8 on the first hit),
 * leaps up and falls for 53 frames (on the first hit facing away from the
 * player), then in pose 2 is sent up again with sound $3C, drops its
 * children once (ActorFlagOnce) and stays. First it counts the byte at
 * $70:1000 + X up and, when the count stays written (work RAM there,
 * which the cartridge does not have), fills bytes $0000-$001F with $AA:
 * a copier check.
 * HOW: The byte at K4D_CHECK + X up and compared; equal: X = $1C down to
 * 0, $AA to $0003 + X, then to $0000 and $0001. ACTOR_08 bit 1 and
 * ACTOR_09 bits $90 set (TSB); AnimStart 0; K4D_HITS 0. Each frame (BRK
 * first) until K4D_HITS is nonzero. SoundQueueAdd $12; K4D_FRAMES = 8
 * when K4D_HITS is 1, else 1; that many BRKs. AnimStart 1; ActorSpeedYSet
 * with entry $7E; ACTOR_ACCEL_Y $38; K4D_FRAMES $35; K4D_HITS 1: facing
 * and flip = the player's $15 EOR 1. Each frame (BRK first) ActorFall
 * until K4D_FRAMES runs out. AnimStart 2; ActorSpeedYSet with entry $2A;
 * SoundQueueAdd $3C; ACTOR_08 bits 0-1 set (TSB); ActorFlagOnce; BRK for
 * good. No script runs the fill: written from its bytes.
 * Entry: M 8-bit, X 8-bit; X as the actor loop's jump leaves it; D = the
 * actor's record; DB = $80; reached by the actor loop's jump for kind $4D
 * in state $0C (the table at $83:8CAB). Exit: never returns.
 */
void abi_Kind4DStart(void)
{
    lda8(ea_longx(K4D_CHECK));
    ACC8(inc);
    sta8(ea_longx(K4D_CHECK));
    cmp8_v(rd8(ea_longx(K4D_CHECK)));
    if (IF_Z) {                           /* the count stayed written: a copier */
        ldx8_v(0x1C);
        lda8_v(K4D_FILL);
        do {
            sta8(ea_absx(K4D_FILLED + 3));
            dex8();
        } while (!IF_N);
        sta8(ea_abs(K4D_FILLED));
        sta8(ea_abs(K4D_FILLED + 1));
    }
    lda8_v(0x02);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0x90);
    tsb8(ea_dp(ACTOR_09));
    lda8_v(0x00);
    abi_AnimStart();
    stz8(ea_dp(K4D_HITS));
    do
        actor_yield(0x83CEAAu);
    while (lda8(ea_dp(K4D_HITS)), IF_Z);
    lda8_v(0x12);
    abi_SoundQueueAdd();
    ldx8_v(0x01);
    lda8(ea_dp(K4D_HITS));
    ACC8(dec);
    if (IF_Z)
        ldx8_v(0x08);
    stx8(ea_dp(K4D_FRAMES));
    do {
        actor_yield(0x83CEC1u);
        MEM8(dec, ea_dp(K4D_FRAMES));
    } while (!IF_Z);
    lda8_v(0x01);
    abi_AnimStart();
    ldy8_v(0x7E);
    abi_ActorSpeedYSet();
    lda8_v(0x38);
    sta8(ea_dp(ACTOR_ACCEL_Y));
    lda8_v(0x35);
    sta8(ea_dp(K4D_FRAMES));
    lda8(ea_dp(K4D_HITS));
    ACC8(dec);
    if (IF_Z) {
        lda8(ea_abs(K4D_PLAYER_FLIP));
        eor8_v(0x01);
        sta8(ea_dp(ACTOR_FACING));
        sta8(ea_dp(ACTOR_FLIP));
    }
    do {
        actor_yield(0x83CEE9u);
        abi_ActorFall();
        MEM8(dec, ea_dp(K4D_FRAMES));
    } while (!IF_Z);
    lda8_v(0x02);
    abi_AnimStart();
    ldy8_v(0x2A);
    abi_ActorSpeedYSet();
    lda8_v(0x3C);
    abi_SoundQueueAdd();
    lda8_v(0x03);
    tsb8(ea_dp(ACTOR_08));
    abi_ActorFlagOnce();
    for (;;)
        actor_yield(0x83CF0Cu);
}
