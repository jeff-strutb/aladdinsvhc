/*
 * kind69.c: actor kind $69 (also kind $68's second state): a figure that,
 * on a cue, opens a circle wipe round itself and hands its place to the
 * level's escort, then follows the escort's facing and motion with its
 * poses; and kind $54's per-frame work.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorCircleWipeStep(void);
void abi_ActorEndIfLeftBehind(void);
void abi_ActorHurtsPlayerOnTouch(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_TaskCreate(void);

#define CUE_BE 0x00BEu          /* the scene's cue: nonzero, the wipe starts */
#define WIPE_STEPS 0x40u        /* steps of the wipe left */
#define K69_HANDED 0x42u        /* set once the escort has its place */
#define WIPE_RADIUS 0x45u       /* the wipe's radius (word; ActorCircleWipeStep's) */
#define K69_16 0x16u            /* word: $D657 */
#define SUB_LAYERS 0x039Au      /* layers on the sub screen (as kind76.c names it) */
#define WINDOW_03A6 0x03A6u     /* the windows' settings, $03A6-$03AE */
#define ESCORT_CODE (ACTOR_ESCORT + ACTOR_03)
#define ESCORT_07 (ACTOR_ESCORT + ACTOR_07)
#define ESCORT_X (ACTOR_ESCORT + ACTOR_X + 1)
#define ESCORT_Y (ACTOR_ESCORT + ACTOR_Y + 1)
#define ESCORT_FLIP (ACTOR_ESCORT + ACTOR_FLIP)
#define ESCORT_SPEED_X (ACTOR_ESCORT + ACTOR_SPEED_X)
#define ESCORT_AT_PLACE 0xE554u /* the escort's code once it is given a place (as camera_trigger.c) */

/*
 * @routine 83:E9F7 Kind69Start
 * @actor
 * WHAT IT DOES: Actor kind $69 (and $68 in its second state): faces
 * left and waits for cue $BE; then a circle wipe opens round it over 32
 * steps of 3 frames (window effect 2 started, colour math adding to the
 * backdrop), and the level's escort takes its place ($07 3, its code at
 * $E554, at its x and y); from then on each frame it faces as the escort
 * does and shows pose 1 while the escort moves, pose 0 while it stands.
 * HOW: ACTOR_09 bits $30 set; K69_16 $D657, WIPE_RADIUS 0; facing and
 * flip 1; K69_HANDED 0. Each frame (BRK) until CUE_BE. WIPE_STEPS $20;
 * SUB_LAYERS 3, CGWSEL 2, CGADSUB $20, TMW 3, $03AA $22, $03AB and WOBJSEL
 * 0; ActorCircleWipeStep; TaskCreate (slot 6, parameter 4,
 * WindowEffectTask); COP 2; ACTOR_09 bit 7 set, AnimStart 0; COP 3 and
 * ActorCircleWipeStep until WIPE_STEPS counts down. K69_HANDED up,
 * ACTOR_09 bits 4-5 = 1; HDMAEN, SUB_LAYERS, TMW and $03AA 0; the escort's
 * $07 3, code $E554, x and y its own. Each frame: facing and flip the
 * escort's flip; the pose (1 if the escort's speed x is nonzero) doubled
 * against ACTOR_ANIM: changed, AnimStart.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $69
 * in state $0C (the table at $83:8CAB) and for kind $68 with state bit 7
 * set (the table at $83:8DFD). Exit: never returns.
 */
void abi_Kind69Start(void)
{
    uint8_t pose, sx;

    a8_load(0x30);
    tsb_at(dpa(ACTOR_09), 0x30);
    op_rep(0x20);
    a16_load(0xD657);
    dp16_set(K69_16, 0xD657);
    dp16_set(WIPE_RADIUS, 0);
    op_sep(0x20);
    a8_load(0x01);
    wr8(dpa(ACTOR_FACING), 0x01);
    wr8(dpa(ACTOR_FLIP), 0x01);
    wr8(dpa(K69_HANDED), 0);
    do {
        actor_yield(0x83EA0Eu);
        a8_load(rd8(dba(CUE_BE)));
    } while (!(uint8_t)cpu.a);

    /* the wipe opens round it */
    a8_load(0x20);
    wr8(dpa(WIPE_STEPS), 0x20);
    a8_load(0x03);
    wr8(dba(SUB_LAYERS), 0x03);
    a8_load(0x02);
    wr8(dba(CGWSEL_SHADOW), 0x02);
    a8_load(0x20);
    wr8(dba(CGADSUB_SHADOW), 0x20);
    a8_load(0x03);
    wr8(dba(TMW_SHADOW), 0x03);
    a8_load(0x22);
    wr8(dba(WINDOW_03A6 + 4), 0x22);
    wr8(dba(WINDOW_03A6 + 5), 0);
    wr8(dba(WOBJSEL_SHADOW), 0);
    abi_ActorCircleWipeStep();
    jsl_frame(0x83EA3Bu); abi_TaskCreate(); cpu.unwound = 0;
    a8_load(0x02);
    actor_wait(0x83EA46u);
    a8_load(0x80);
    tsb_at(dpa(ACTOR_09), 0x80);
    a8_load(0x00);
    abi_AnimStart();
    do {
        a8_load(0x03);
        actor_wait(0x83EA54u);
        abi_ActorCircleWipeStep();
    } while (dec8_at(dpa(WIPE_STEPS)));

    /* the escort takes its place */
    (void)inc8_at(dpa(K69_HANDED));
    a8_load((uint8_t)((rd8(dpa(ACTOR_09)) & 0xCF) | 0x10));
    wr8(dpa(ACTOR_09), (uint8_t)cpu.a);
    wr8(dba(HDMAEN_SHADOW), 0);
    wr8(dba(SUB_LAYERS), 0);
    wr8(dba(TMW_SHADOW), 0);
    wr8(dba(WINDOW_03A6 + 4), 0);
    a8_load(0x03);
    wr8(dba((uint16_t)ESCORT_07), 0x03);
    op_rep(0x20);
    a16_load(ESCORT_AT_PLACE);
    wr16(dba((uint16_t)ESCORT_CODE), ESCORT_AT_PLACE);
    a16_load(dp16(ACTOR_X + 1));
    wr16(dba((uint16_t)ESCORT_X), cpu.a);
    a16_load(dp16(ACTOR_Y + 1));
    wr16(dba((uint16_t)ESCORT_Y), cpu.a);
    op_sep(0x20);

    /* then it follows the escort's facing and motion */
    for (;;) {
        actor_yield(0x83EA8Cu);
        a8_load(rd8(dba((uint16_t)ESCORT_FLIP)));
        wr8(dpa(ACTOR_FACING), (uint8_t)cpu.a);
        wr8(dpa(ACTOR_FLIP), (uint8_t)cpu.a);
        sx = rd8(dba((uint16_t)ESCORT_SPEED_X));
        a8_load((uint8_t)(sx | rd8(dba((uint16_t)(ESCORT_SPEED_X + 1)))));
        pose = (uint8_t)cpu.a ? 0x01 : 0x00;
        a8_load(pose);
        flag_set(P_C, 0);               /* the ASL's: bit 7 of 0 or 1 */
        a8_load((uint8_t)(pose << 1));
        cmp8_of((uint8_t)(pose << 1), rd8(dpa(ACTOR_ANIM)));
        if ((uint8_t)(pose << 1) == rd8(dpa(ACTOR_ANIM)))
            continue;
        flag_set(P_C, 0);               /* the LSR's: bit 0 of a doubled value */
        a8_load(pose);
        abi_AnimStart();
    }
}

/*
 * @routine 83:D7E7 Kind54Wait
 * WHAT IT DOES: What actors of kind $54 do each frame they wait: animate,
 * hurt Aladdin on touch unless its $07 bit 3 is set, and end once left
 * behind.
 * HOW: AnimStep; $07 AND 8 zero: ActorHurtsPlayerOnTouch; then on into
 * ActorEndIfLeftBehind (JMP).
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
 * the actor loop (83:94D0) by its kind (the wait table at $83:8F4F).
 * Exit: as ActorEndIfLeftBehind.
 */
void abi_Kind54Wait(void)
{
    uint8_t v;

    abi_AnimStep();
    v = (uint8_t)(rd8(dpa(ACTOR_07)) & 0x08);
    a8_load(v);
    if (!v)
        abi_ActorHurtsPlayerOnTouch();
    abi_ActorEndIfLeftBehind();
}
