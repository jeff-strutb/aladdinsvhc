/*
 * kind6b.c: actor kind $6B, which the camera trigger task starts with
 * $07 = 1: shown in its first pose; with $07 nonzero it waits for the
 * task's cue $00BF, then the second cue $00BE, and walks off to the right
 * with a song, a pause and a second walk, and ends.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "actor_anim.h"

void abi_AnimStart(void);
void abi_SongPlay(void);
void abi_ActorSpeedXSet(void);
void abi_ActorMoveX(void);
void abi_ActorSnapToGround(void);

#define KIND6B_16 0x16u       /* word: $D663 */
#define CUE_BF 0x0000BFu      /* the camera trigger task's third cue: cleared as it starts */
#define CUE_BE 0x00BEu        /* the scene's cue to walk off */
#define KIND6B_FRAMES 0x40u   /* frames left of a walk */
#define KIND6B_SONG 0x31u     /* the song it starts as it walks off */
#define ACTOR_END_PLACED 0x8396CAu

/* LDA #a; JSL AnimStart at site (its C pushes: the JSL's return there). */
static void kind6b_anim(uint8_t a, uint32_t site)
{
    a8_load(a);
    jsl_frame(site);
    abi_AnimStart();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/* Each frame (BRK at brk_at) ActorMoveX and ActorSnapToGround (its JSL 6
   bytes on), until KIND6B_FRAMES is down to 0. */
static void kind6b_walk(uint32_t brk_at)
{
    do {
        actor_yield(brk_at);
        abi_ActorMoveX();
        jsl_frame(brk_at + 6);
        abi_ActorSnapToGround();
        cpu.s = (uint16_t)(cpu.s + 3);
    } while (dec8_at(dpa(KIND6B_FRAMES)));
}

/*
 * @routine 83:EC2C Kind6BWait
 * @actor
 * WHAT IT DOES: Actor kind $6B: shown facing right in animation 0. With
 * $07 zero it stays so; else it waits for CUE_BF, moves $20 pixels right
 * in animation 1, waits for CUE_BE, then after $40 frames starts the song
 * $31 and walks right ($70 frames, speed $3F), pauses ($30 frames) and
 * walks on ($60 frames), and ends.
 * HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set; ACTOR_2C bits 1-3 = 1
 * (and $F1, or 2); KIND6B_16 = $D663; facing and flip 0. $07 zero:
 * AnimStart 0, then BRK for good. Else AnimStart 0, then each frame
 * (BRK first) until CUE_BF is nonzero; its x pixels + $20; AnimStart 1;
 * ACTOR_08 bits 0-1 cleared; each frame (BRK first) until CUE_BE; COP $40;
 * SongPlay $31; AnimStart 3; KIND6B_FRAMES $70, ActorSpeedXSet $3F; each
 * frame (BRK first) ActorMoveX, ActorSnapToGround, KIND6B_FRAMES down to
 * 0; AnimStart 4, COP $30, AnimStart 3; $60 frames as before; JML
 * ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $6B in state $0C (the table at
 * $83:8CAB). Exit: never returns.
 */
void abi_Kind6BWait(void)
{
    uint8_t v, look;

    v = rd8(dpa(ACTOR_08));
    wr8(dpa(ACTOR_08), (uint8_t)(v | 0x02));
    v = rd8(dpa(ACTOR_09));
    wr8(dpa(ACTOR_09), (uint8_t)(v | 0x90));
    look = (uint8_t)((rd8(dpa(ACTOR_2C)) & 0xF1) | 0x02);
    wr8(dpa(ACTOR_2C), look);
    dp16_set(KIND6B_16, 0xD663);
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    v = rd8(dpa(ACTOR_07));
    cpu.a = 0xD600;
    nz8_of(0x00);
    jsl_frame(v ? 0x83EC59u : 0x83EC4Fu);
    abi_AnimStart();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (!v)
        for (;;)
            actor_yield(0x83EC53u);
    for (;;) {
        actor_yield(0x83EC5Du);
        uint8_t cue = rd8(dba(CUE_BF));
        a8_load(cue);
        if (cue)
            break;
    }

    /* cued: $20 pixels on, animation 1, until the cue to walk off */
    op_rep(0x20);
    flag_set(P_C, 0);
    cpu.a = op_adc16(dp16(ACTOR_X + 1), 0x0020, &cpu.p);
    dp16_set(ACTOR_X + 1, cpu.a);
    op_sep(0x20);
    kind6b_anim(0x01, 0x83EC72u);
    a8_load(0x03);
    trb_at(dpa(ACTOR_08), 0x03);
    do {
        actor_yield(0x83EC7Au);
        a8_load(rd8(dba(CUE_BE)));
    } while (!(uint8_t)cpu.a);
    a8_load(0x40);
    actor_wait(0x83EC83u);
    a8_load(KIND6B_SONG);
    jsl_frame(0x83EC87u);
    abi_SongPlay();
    cpu.s = (uint16_t)(cpu.s + 3);

    /* the walk off: $70 frames, a pause, $60 frames */
    kind6b_anim(0x03, 0x83EC8Du);
    a8_load(0x70);
    wr8(dpa(KIND6B_FRAMES), 0x70);
    y8_load(0x3F);
    jsl_frame(0x83EC97u);
    abi_ActorSpeedXSet();
    cpu.s = (uint16_t)(cpu.s + 3);
    kind6b_walk(0x83EC9Bu);
    kind6b_anim(0x04, 0x83ECABu);
    a8_load(0x30);
    actor_wait(0x83ECB1u);
    kind6b_anim(0x03, 0x83ECB5u);
    a8_load(0x60);
    wr8(dpa(KIND6B_FRAMES), 0x60);
    kind6b_walk(0x83ECBDu);
    actor_leave(ACTOR_END_PLACED);           /* JML ActorEndPlaced */
}
