/*
 * kind90.c: actor kind $90, a thing that flies a set path from a slot of
 * its $07 in steps, each step's flight made at once, and reports in its
 * slot how it ended: $80 when it came down onto ground, $40 when it hit a
 * wall whose probe result has bit 6 clear, 0 when its steps ran out.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "wram_player.h"
#include "actor.h"

void abi_ActorEndPlaced(void);
void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_ActorOutOfGround(void);
void abi_ActorOutOfWallProbe4(void);

#define K90_FLIGHT 0x40u        /* flights left in this step */
#define K90_STEPS 0x41u         /* steps left */
#define K90_16 0x16u            /* word: $CF8F */
#define K90_SLOTS 0x1478u       /* by $07: its slot (low four bits nonzero: go on); how it ended */
#define K90_STEP_COUNTS 0xD878u /* (data bank) by $07: K90_STEPS */
#define K90_PRESETS 0xD875u     /* by $07: its motion preset */
#define K90_FLIGHTS 0xD87Bu     /* by $07: flights a step */

/* A JSL at `site` to fn, its return address on the stack while fn runs, as
   the original's (frame_step.c's model). */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

static void report(uint8_t how)
{
    a8_load(how);
    x8_load(rd8(dpa(ACTOR_07)));
    wr8(dba((uint16_t)(K90_SLOTS + cpu.x)), how);
    abi_ActorEndPlaced();
}

/*
 * @routine A5:F4D2 Kind90Start
 * @actor
 * WHAT IT DOES: Actor kind $90: flies by the preset its $07 names, in
 * steps of several flights each made within one frame, while its slot in
 * K90_SLOTS stays live; it ends, reporting in the slot $80 when it is in
 * the ground, $40 when it meets a wall whose probe has bit 6 clear
 * ($A5:F50D, which no script meets), 0 when its steps run out.
 * HOW: ACTOR_09 bit 7 set; K90_16 $CF8F; K90_STEPS from K90_STEP_COUNTS,
 * ActorMotionPreset with K90_PRESETS, by $07. Each step: K90_FLIGHT from
 * K90_FLIGHTS; the slot's low nibble 0: ActorEndPlaced at once; that many
 * ActorFly; ActorOutOfWallProbe4 C set and V_0012 bit 6 clear: slot $40;
 * ActorOutOfGround nonzero: slot $80; else BRK and the next step, after
 * the last slot 0. Each report: the slot written, ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
 * the actor loop's jump for kind $90 in state $0C (the table at $83:8DAB,
 * through the stub at $83:9362). Exit: never returns.
 */
void abi_Kind90Start(void)
{
    uint8_t v07;

    a8_load(0x80);
    tsb_at(dpa(ACTOR_09), 0x80);
    op_rep(0x20);
    a16_load(0xCF8F);
    dp16_set(K90_16, 0xCF8F);
    op_sep(0x20);
    v07 = rd8(dpa(ACTOR_07));
    x8_load(v07);
    a8_load(rd8(dba((uint16_t)(K90_STEP_COUNTS + v07))));
    wr8(dpa(K90_STEPS), (uint8_t)cpu.a);
    y8_load(rd8(dba((uint16_t)(K90_PRESETS + v07))));
    jsl_call(0xA5F4E9u, abi_ActorMotionPreset);
    for (;;) {
        x8_load(rd8(dpa(ACTOR_07)));
        a8_load(rd8(dba((uint16_t)(K90_FLIGHTS + cpu.x))));
        wr8(dpa(K90_FLIGHT), (uint8_t)cpu.a);
        a8_load((uint8_t)(rd8(dba((uint16_t)(K90_SLOTS + cpu.x))) & 0x0F));
        if (!(uint8_t)cpu.a) {
            abi_ActorEndPlaced();
            return;
        }
        do
            jsl_call(0xA5F4FFu, abi_ActorFly);
        while (dec8_at(dpa(K90_FLIGHT)));
        jsl_call(0xA5F507u, abi_ActorOutOfWallProbe4);
        if (cpu.p & P_C) {
            a8_load(rd8(dba((uint16_t)V_0012)));
            flag_set(P_Z, !((uint8_t)cpu.a & 0x40));
            if (!((uint8_t)cpu.a & 0x40)) {
                report(0x40);
                return;
            }
        }
        jsl_call(0xA5F514u, abi_ActorOutOfGround);
        if (!(cpu.p & P_Z)) {
            report(0x80);
            return;
        }
        actor_yield(0xA5F51Au);
        if (!dec8_at(dpa(K90_STEPS)))
            break;
    }
    report(0x00);
}
