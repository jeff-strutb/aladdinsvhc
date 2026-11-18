/*
 * kind61.c: actor kinds $61 and $64, pads that bounce Aladdin: when his
 * test box meets one he is sent on in his bounce code ($D77C) from its
 * top, kind $61 also setting his x to its own; while he is hurt it pushes
 * him aside instead.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorHitsTestBoxIfActive(void);
void abi_PlayerPushAside(void);
void abi_SoundQueueAdd(void);
void abi_Kind64Start(void);

#define PAD_KEEP_X 0x41u
#define PAD_WAIT 0x40u
#define PAD_145A 0x145Au
#define PAD_LEVEL 0x035Eu
#define PAD_TAKEN 0x1427u
#define PAD_141C 0x141Cu
#define PAD_HURT 0x141Bu
#define PAD_BOUNCE_CODE 0xD77Cu
#define PAD_SOUND 0x12
#define PAD_144E 0x144Eu
#define PAD_V_0001 0x0001u

/* Kind64Start past its own setting ($83:E4F4, Kind64Pad): both kinds */
static void pad(void)
{
    lda8_v(0x80);
    tsb8(ea_dp(ACTOR_09));
    stz8(ea_dp(PAD_WAIT));
    for (;;) {
        actor_yield(0x83E4FAu);
        lda8(ea_abs(PAD_145A));
        if (IF_N)
            continue;
        lda8(ea_abs(PAD_LEVEL));
        cmp8_v(0x0A);
        if (IF_Z) {
            lda8(ea_abs(PAD_TAKEN));
            if (!IF_Z)
                continue;
            lda8(ea_dp(ACTOR_09));
            and8_v(0x40);
            if (IF_Z)
                continue;
        }
        lda8(ea_dp(PAD_WAIT));
        if (!IF_Z) {
            MEM8(dec, ea_dp(PAD_WAIT));
            continue;
        }
        abi_ActorHitsTestBoxIfActive();
        if (IF_C)
            continue;
        lda8_v(0x01);
        sta8(ea_abs(PAD_141C));
        lda8(ea_abs(PAD_HURT));
        if (!IF_Z) {
            /* the JSR's return on the stack while it runs: it pushes P there */
            call_frame(0xE55B, abi_PlayerPushAside);
            continue;
        }
        lda8_v(0x14);
        sta8(ea_dp(PAD_WAIT));
        lda8_v(PAD_SOUND);
        abi_SoundQueueAdd();
        op_rep(0x21);
        lda16_v(PAD_BOUNCE_CODE);
        sta16(ea_abs(PLAYER + ACTOR_03));
        actor_yield(0x83E53Cu);
        lda16(ea_dp(ACTOR_Y + 1));
        sbc16_v(0x0007);
        sta16(ea_abs(PLAYER + ACTOR_Y + 1));
        ldx8(ea_dp(PAD_KEEP_X));
        if (IF_Z) {
            lda16(ea_dp(ACTOR_X + 1));
            sta16(ea_abs(PLAYER + ACTOR_X + 1));
        }
        op_sep(0x20);
        lda8(ea_abs(PAD_V_0001));
        sta8(ea_abs(PAD_144E));
    }
}

/*
 * @routine 83:E4EC Kind61Start
 * @actor
 * WHAT IT DOES: Actor kind $61: the pad that bounces Aladdin and puts him
 * at its x (PAD_KEEP_X 0), then on as Kind64Start.
 * HOW: PAD_KEEP_X 0; on into Kind64Start past its own setting
 * ($83:E4F4).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $61 in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind61Start(void)
{
    stz8(ea_dp(PAD_KEEP_X));
    pad();
}

/*
 * @routine 83:E4F0 Kind64Start
 * @actor
 * WHAT IT DOES: Actor kind $64 (and $61 from $83:E4F4): a pad. Each frame,
 * unless bouncing is off ($145A negative), or in level $0A while Aladdin
 * is taken or it cannot be hit, or it is still waiting: when his test box
 * meets it, $141C set; hurt, he is pushed aside (PlayerPushAside); else
 * $14 frames of wait, sound $12, his code $D77C, and next frame his y its
 * y less 8 (his x its x for kind $61), $144E the byte at $0001.
 * HOW: PAD_KEEP_X 1. ACTOR_09 bit 7 set, PAD_WAIT 0. Each frame (BRK):
 * PAD_145A negative, again; level $0A: PAD_TAKEN nonzero or ACTOR_09 bit
 * 6 clear, again; PAD_WAIT nonzero: down, again; ActorHitsTestBoxIfActive
 * C set, again. PAD_141C 1; PAD_HURT nonzero: PlayerPushAside, again.
 * PAD_WAIT $14, SoundQueueAdd $12; 16-bit with C clear: his code
 * $D77C; BRK; his y = its y - 7 - 1; PAD_KEEP_X 0: his x = its x; 8-bit:
 * PAD_144E = the byte at $0001; again.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $64 in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind64Start(void)
{
    lda8_v(0x01);
    sta8(ea_dp(PAD_KEEP_X));
    pad();
}
