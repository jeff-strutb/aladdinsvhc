/*
 * kind6d.c: actor kind $6D (the pots and the like that the wait routine
 * Kind6DWait drives): its start, eight ways of moving by its $07, and what
 * it does once downed (its state with bit 7 set): knocked away, a sound,
 * and off until it is out of range.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorEndOutOfRange(void);
void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);

#define K6D_FRAMES 0x40u  /* frames left before it flies off */
#define K6D_DOWNED 0x48u  /* counted up by Kind6DWait when a shot downs it */
#define K6D_KNOCKED 0x1F  /* the motion preset it is knocked away with */
#define K6D_POSE 0x07     /* its animation once downed */
#define K6D_SOUND 0x0F    /* the sound of its fall */

#define K6D_CHECK 0x703000u   /* the copier check's bytes: work RAM on a copier, none on the cartridge */
#define K6D_PROBES 0xD667u    /* its probe list */
#define K6D_TURNS 0x41u       /* the homing way's turns left; then its turn step */
#define K6D_SPAWN_SLOT 0x21u  /* the slot byte its children are made with */
#define K6D_STEP 0x47u        /* the spawner's own $07 kept while a child is made */
#define K6D_RANGE_X 0x1A44u   /* the x of the point actors are kept in range of (word) */

void abi_ActorAccelerateX(void);
void abi_ActorAccelerateY(void);
void abi_ActorDecelerateX(void);
void abi_ActorDecelerateY(void);
void abi_ActorDirectionToPlayer(void);
void abi_ActorMotionReset(void);
void abi_ActorMoveByVector(void);
void abi_ActorMoveX(void);
void abi_ActorOutOfGround(void);
void abi_ActorOutOfWallProbe4(void);
void abi_ActorSideOfPlayer(void);
void abi_ActorSpawnSameLook(void);
void abi_ActorSpeedXSet(void);
void abi_ActorStepY(void);
void abi_ActorStepYBack(void);
void abi_ActorStopX(void);
void abi_ActorStopY(void);

void abi_Kind6DAway(void);

/* way 0 ($83:ED33): back and forth across */
static void k6d_across(void)
{
    lda8_v(0x08);
    jsl_frame(0x83ED35u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
    lda8_v(0x00);
    jsl_frame(0x83ED3Bu); abi_ActorMotionReset(); cpu.s = (uint16_t)(cpu.s + 3);
    for (;;) {
        lda8_v(0x04);
        sta8(ea_dp(ACTOR_ACCEL_X));
        lda8_v(0x60);
        sta8(ea_dp(K6D_FRAMES));
        do {
            actor_yield(0x83ED47u);
            jsl_frame(0x83ED49u); abi_ActorMoveX(); cpu.s = (uint16_t)(cpu.s + 3);
            jsl_frame(0x83ED4Du); abi_ActorAccelerateX(); cpu.s = (uint16_t)(cpu.s + 3);
            op_rep(0x20);
            lda16(ea_dp(ACTOR_SPEED_X));
            cmp16_v(0x00C0);
            op_sep(0x20);
            if (IF_C)
                stz8(ea_dp(ACTOR_ACCEL_X));
            MEM8(dec, ea_dp(K6D_FRAMES));
        } while (!IF_Z);
        lda8_v(0x04);
        sta8(ea_dp(ACTOR_ACCEL_X));
        lda8(ea_dp(ACTOR_FACING));
        eor8_v(0x01);
        sta8(ea_dp(ACTOR_FLIP));
        do {
            actor_yield(0x83ED6Cu);
            jsl_frame(0x83ED6Eu); abi_ActorMoveX(); cpu.s = (uint16_t)(cpu.s + 3);
            jsl_frame(0x83ED72u); abi_ActorDecelerateX(); cpu.s = (uint16_t)(cpu.s + 3);
        } while (!IF_N);
        lda8(ea_dp(ACTOR_FACING));
        eor8_v(0x01);
        sta8(ea_dp(ACTOR_FACING));
        lda8_v(0x18);
        actor_wait(0x83ED80u);
    }
}

/* ActorStepYBack while facing nonzero, else ActorStepY ($83:ED9A, EDC3) */
static void k6d_step_y(uint32_t brk_at)
{
    lda8(ea_dp(ACTOR_FACING));
    if (!IF_Z)
        { jsl_frame(brk_at + 6); abi_ActorStepYBack(); cpu.s = (uint16_t)(cpu.s + 3); }
    else
        { jsl_frame(brk_at + 12); abi_ActorStepY(); cpu.s = (uint16_t)(cpu.s + 3); }
}

/* way 1 ($83:ED84): up and down */
static void k6d_updown(void)
{
    lda8_v(0x08);
    jsl_frame(0x83ED86u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
    stz8(ea_dp(ACTOR_FACING));
    stz8(ea_dp(ACTOR_49));
    for (;;) {
        lda8_v(0x03);
        jsl_frame(0x83ED90u); abi_ActorMotionReset(); cpu.s = (uint16_t)(cpu.s + 3);
        lda8_v(0x60);
        sta8(ea_dp(K6D_FRAMES));
        do {
            actor_yield(0x83ED98u);
            k6d_step_y(0x83ED98u);
            jsl_frame(0x83EDA8u); abi_ActorAccelerateY(); cpu.s = (uint16_t)(cpu.s + 3);
            op_rep(0x20);
            lda16(ea_dp(ACTOR_SPEED_Y));
            cmp16_v(0x00C0);
            op_sep(0x20);
            if (IF_C)
                stz8(ea_dp(ACTOR_ACCEL_Y));
            MEM8(dec, ea_dp(K6D_FRAMES));
        } while (!IF_Z);
        lda8_v(0x03);
        sta8(ea_dp(ACTOR_ACCEL_Y));
        do {
            actor_yield(0x83EDC1u);
            k6d_step_y(0x83EDC1u);
            jsl_frame(0x83EDD1u); abi_ActorDecelerateY(); cpu.s = (uint16_t)(cpu.s + 3);
        } while (!IF_N);
        lda8(ea_dp(ACTOR_FACING));
        eor8_v(0x01);
        sta8(ea_dp(ACTOR_FACING));
    }
}

/* way 2 ($83:EDDF): a run that rises away */
static void k6d_run(void)
{
    lda8_v(0x02);
    jsl_frame(0x83EDE1u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
    lda8_v(0x04);
    jsl_frame(0x83EDE7u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
    lda8_v(0x02);
    sta8(ea_dp(ACTOR_ACCEL_X));
    ldy8_v(0x5A);
    jsl_frame(0x83EDF1u); abi_ActorSpeedXSet(); cpu.s = (uint16_t)(cpu.s + 3);
    lda8_v(0x40);
    sta8(ea_dp(K6D_FRAMES));
    do {
        actor_yield(0x83EDF9u);
        jsl_frame(0x83EDFBu); abi_ActorMoveX(); cpu.s = (uint16_t)(cpu.s + 3);
        jsl_frame(0x83EDFFu); abi_ActorAccelerateX(); cpu.s = (uint16_t)(cpu.s + 3);
        MEM8(dec, ea_dp(K6D_FRAMES));
    } while (!IF_Z);
    lda8_v(0x08);
    jsl_frame(0x83EE09u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
    stz8(ea_dp(ACTOR_ACCEL_X));
    lda8_v(0x30);
    sta8(ea_dp(ACTOR_ACCEL_Y));
    lda8_v(0x10);
    sta8(ea_dp(K6D_FRAMES));
    do {
        actor_yield(0x83EE17u);
        jsl_frame(0x83EE19u); abi_ActorStepYBack(); cpu.s = (uint16_t)(cpu.s + 3);
        jsl_frame(0x83EE1Du); abi_ActorMoveX(); cpu.s = (uint16_t)(cpu.s + 3);
        jsl_frame(0x83EE21u); abi_ActorAccelerateY(); cpu.s = (uint16_t)(cpu.s + 3);
        MEM8(dec, ea_dp(K6D_FRAMES));
    } while (!IF_Z);
    do {
        actor_yield(0x83EE29u);
        jsl_frame(0x83EE2Bu); abi_ActorStepYBack(); cpu.s = (uint16_t)(cpu.s + 3);
        jsl_frame(0x83EE2Fu); abi_ActorMoveX(); cpu.s = (uint16_t)(cpu.s + 3);
        jsl_frame(0x83EE33u); abi_ActorDecelerateY(); cpu.s = (uint16_t)(cpu.s + 3);
        lda8(ea_dp(ACTOR_SPEED_Y + 2));
    } while (!IF_N);
    abi_Kind6DAway();
}

/* ways 3 ($83:EE43) and 7 ($83:EF45): falls and hops, by preset `preset`;
   way 3 turns at walls, way 7 cuts its gravity while it falls fast */
static void k6d_hop(uint8_t preset, int seven)
{
    ldy8_v(preset);
    jsl_frame(seven ? 0x83EF47u : 0x83EE45u); abi_ActorMotionPreset(); cpu.s = (uint16_t)(cpu.s + 3);
    stz8(ea_dp(ACTOR_SPEED_Y));
    stz8(ea_dp(ACTOR_SPEED_Y + 1));
    stz8(ea_dp(ACTOR_SPEED_Y + 2));
    for (;;) {
        lda8_v(0x08);
        jsl_frame(seven ? 0x83EF5Bu : 0x83EE59u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
        do {
            actor_yield(seven ? 0x83EF5Fu : 0x83EE5Du);
            jsl_frame(seven ? 0x83EF61u : 0x83EE5Fu); abi_ActorFly(); cpu.s = (uint16_t)(cpu.s + 3);
            if (seven) {
                op_rep(0x20);
                lda16(ea_dp(ACTOR_SPEED_Y));
                if (!IF_N) {
                    cmp16_v(0x0100);
                    op_sep(0x20);
                    if (IF_C)
                        stz8(ea_dp(ACTOR_ACCEL_Y));
                }
                op_sep(0x20);
            } else {
                jsl_frame(0x83EE63u); abi_ActorOutOfWallProbe4(); cpu.s = (uint16_t)(cpu.s + 3);
                if (IF_C) {
                    lda8(ea_dp(ACTOR_FACING));
                    eor8_v(0x01);
                    sta8(ea_dp(ACTOR_FACING));
                    sta8(ea_dp(ACTOR_FLIP));
                }
            }
            jsl_frame(seven ? 0x83EF76u : 0x83EE71u); abi_ActorOutOfGround(); cpu.s = (uint16_t)(cpu.s + 3);
        } while (IF_Z);
        lda8_v(0x02);
        jsl_frame(seven ? 0x83EF7Eu : 0x83EE79u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
        lda8_v(0x08);
        actor_wait(seven ? 0x83EF84u : 0x83EE7Fu);
        lda8_v(0x01);
        jsl_frame(seven ? 0x83EF88u : 0x83EE83u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
        lda8_v(0x08);
        actor_wait(seven ? 0x83EF8Eu : 0x83EE89u);
        ldy8_v(preset);
        jsl_frame(seven ? 0x83EF55u : 0x83EE53u); abi_ActorMotionPreset(); cpu.s = (uint16_t)(cpu.s + 3);
    }
}

/* ways 4 ($83:EE8D) and 6: a homing flight that turns toward the player,
   then up for good */
static void k6d_homing(void)
{
    lda8_v(0x08);
    jsl_frame(0x83EE8Fu); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
    lda8_v(0x14);
    sta8(ea_dp(K6D_TURNS));
    lda8(ea_dp(ACTOR_FACING));
    ACC8(asl);
    ACC8(asl);
    ACC8(asl);
    sta8(ea_dp(ACTOR_FACING));
    for (;;) {
        lda8_v(0x0C);
        sta8(ea_dp(K6D_FRAMES));
        do {
            actor_yield(0x83EEA2u);
            ldx8_v(0x18);
            jsl_frame(0x83EEA6u); abi_ActorMoveByVector(); cpu.s = (uint16_t)(cpu.s + 3);
            MEM8(dec, ea_dp(K6D_FRAMES));
        } while (!IF_Z);
        MEM8(dec, ea_dp(K6D_TURNS));
        if (IF_Z)
            break;
        jsl_frame(0x83EEB2u); abi_ActorDirectionToPlayer(); cpu.s = (uint16_t)(cpu.s + 3);
        sec();
        sbc8_v(rd8(ea_dp(ACTOR_FACING)));
        and8_v(0x0F);
        if (IF_Z)
            continue;
        cmp8_v(0x08);
        ldx8(ea_dp(ACTOR_FACING));
        inx8();
        if (IF_C) {
            dex8();
            dex8();
        }
        txa8();
        and8_v(0x0F);
        sta8(ea_dp(ACTOR_FACING));
        clc();
        adc8_v(0x04);
        and8_v(0x0F);
        ACC8(lsr);
        ACC8(lsr);
        ACC8(lsr);
        sta8(ea_dp(ACTOR_FLIP));
    }
    lda8_v(0x0C);
    sta8(ea_dp(K6D_FRAMES));
    lda8(ea_dp(ACTOR_FLIP));
    if (IF_Z)
        lda8_v(0xFF);
    sta8(ea_dp(K6D_TURNS));
    for (;;) {
        lda8(ea_dp(ACTOR_FACING));
        and8_v(0x0F);
        cmp8_v(0x0C);
        if (!IF_Z) {
            MEM8(dec, ea_dp(K6D_FRAMES));
            if (IF_Z) {
                clc();
                lda8(ea_dp(ACTOR_FACING));
                adc8_v(rd8(ea_dp(K6D_TURNS)));
                and8_v(0x0F);
                sta8(ea_dp(ACTOR_FACING));
                lda8_v(0x0C);
                sta8(ea_dp(K6D_FRAMES));
            }
        }
        actor_yield(0x83EEFCu);
        ldx8_v(0x18);
        jsl_frame(0x83EF00u); abi_ActorMoveByVector(); cpu.s = (uint16_t)(cpu.s + 3);
    }
}

/* way 5 ($83:EF06): a spawner of its own kind */
static void k6d_spawner(void)
{
    lda8(ea_dp(ACTOR_07));
    sta8(ea_dp(K6D_STEP));
    for (;;) {
        lda8_v(0xE0);
        sta8(ea_dp(K6D_FRAMES));
        clc();
        lda8(ea_dp(ACTOR_07));
        adc8_v(0x04);
        sta8(ea_dp(ACTOR_07));
        stz8(ea_dp(K6D_SPAWN_SLOT));
        lda8(ea_dp(ACTOR_06));
        jsl_frame(0x83EF19u); abi_ActorSpawnSameLook(); cpu.s = (uint16_t)(cpu.s + 3);
        if (!IF_N) {
            lda8(ea_dp(K6D_STEP));
            sta8(ea_dp(ACTOR_07));
            do {
                actor_yield(0x83EF23u);
                MEM8(dec, ea_dp(K6D_FRAMES));
            } while (!IF_Z);
        }
        do {
            actor_yield(0x83EF29u);
            op_rep(0x20);
            sec();
            lda16(ea_abs(K6D_RANGE_X));
            sbc16_v(rd16(ea_dp(ACTOR_X + 1)));
            if (IF_N) {
                eor16_v(0xFFFF);
                ACC16(inc);
            }
            cmp16_v(0x0090);
            op_sep(0x20);
        } while (!IF_C);
    }
}

/*
 * @routine 83:EE3B Kind6DAway
 * @actor
 * WHAT IT DOES: Kind $6D's run (way 2) once it has stopped rising: each
 * frame it moves on across, for good. No input reaches it: the run's
 * rise before it loops while its vertical speed's top byte is not
 * negative, right after ActorDecelerateY, which never leaves it negative
 * (a negative result is cleared to 0); nothing else names $83:EE3B.
 * cov_kind6d_away.box runs it from a kind $6D record whose resume address
 * is set to it. The ways after it in the ROM ($EE43-EFEF) are
 * Kind6DStart's code, filed under this tag only because they follow it.
 * HOW: Each frame (BRK first) ActorMoveX; again.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's RTI at a resume address set to it. Exit: never returns.
 */
void abi_Kind6DAway(void)
{
    for (;;) {
        actor_yield(0x83EE3Bu);
        jsl_frame(0x83EE3Du); abi_ActorMoveX(); cpu.s = (uint16_t)(cpu.s + 3);
    }
}

/*
 * @routine 83:ECD4 Kind6DStart
 * @actor
 * WHAT IT DOES: Actor kind $6D's start: set up (a copier's cartridge RAM
 * makes it wipe its own record), facing by $07 bits 0-1 (or toward the
 * player), then one of eight ways by $07 bits 2-4: 0 back and forth
 * across, 1 up and down, 2 a run that rises away, 3 and 7 falls and hops
 * (presets $55 and $7F), 4 and 6 a homing flight that turns toward the
 * player, 5 a spawner of its own kind.
 * HOW: The byte at K6D_CHECK + X up and compared: equal, $01-$40 of its
 * record filled with $AA (D-relative) and its $06 0. ACTOR_08 2, ACTOR_09
 * $90, its probes K6D_PROBES, ActorStopX, ActorStopY. $07 bits 0-1 nonzero:
 * facing and flip that shifted right once; else ActorSideOfPlayer's. $48
 * 0. JMP through the table at $83:ED23 by $07 bits 2-4: 0 animation 8,
 * ActorMotionReset 0; each leg: acceleration 4, $60 frames (BRK) of
 * ActorMoveX and ActorAccelerateX (acceleration 0 at speed $C0), flip
 * turned; frames of ActorMoveX and ActorDecelerateX until stopped; facing
 * turned, COP $18. 1: animation 8, facing and $49 0; each leg
 * ActorMotionReset 3, $60 frames of ActorStepYBack or ActorStepY (by the
 * facing) and ActorAccelerateY (gravity 0 at speed $C0), gravity 3,
 * frames of it with ActorDecelerateY until it turns; facing turned. 2:
 * animations 2 then 4, acceleration 2, ActorSpeedXSet $5A, $40 frames of
 * ActorMoveX and ActorAccelerateX; animation 8, acceleration 0, gravity
 * $30, $10 frames of ActorStepYBack, ActorMoveX, ActorAccelerateY; then
 * with ActorDecelerateY until it falls; then ActorMoveX each frame for
 * good. 3: ActorMotionPreset $55, vertical speed 0;
 * animation 8; each frame ActorFly, ActorOutOfWallProbe4 (a wall: facing
 * and flip turned) until ActorOutOfGround; animation 2, COP 8, animation
 * 1, COP 8, preset again; 7 the same with preset $7F, no wall test, and
 * its gravity cut while it falls at $0100 or more. 4 (and 6): animation
 * 8, K6D_TURNS $14, facing
 * times 8 (a direction); each 12 frames of ActorMoveByVector $18, a
 * turn toward ActorDirectionToPlayer; after $14, turning on one way each
 * 12 frames until it points up ($0C), ActorMoveByVector each frame for
 * good. 5: a child of its kind ($07 + 4, slot 0, ActorSpawnSameLook; $07
 * back); made, $E0 frames (BRK); then BRK while within $90 of K6D_RANGE_X,
 * and again once farther.
 * Entry: M 8-bit, X 8-bit (twice its kind); D = the actor's record; DB =
 * $80; reached by the actor loop's jump for kind $6D in state $0C (the
 * table at $83:8CAB). Exit: never returns.
 */
void abi_Kind6DStart(void)
{
    lda8(ea_longx(K6D_CHECK));
    ACC8(inc);
    sta8(ea_longx(K6D_CHECK));
    cmp8_v(rd8(ea_longx(K6D_CHECK)));
    if (IF_Z) {
        ldx8_v(0x40);
        lda8_v(0xAA);
        do {
            sta8(ea_dpx(0x00));
            dex8();
        } while (!IF_Z);
        stz8(ea_dp(ACTOR_06));
    }
    lda8_v(0x02);
    sta8(ea_dp(ACTOR_08));
    lda8_v(0x90);
    sta8(ea_dp(ACTOR_09));
    op_rep(0x20);
    lda16_v(K6D_PROBES);
    sta16(ea_dp(0x16));
    op_sep(0x20);
    jsl_frame(0x83ECFFu);
    abi_ActorStopX();
    cpu.s = (uint16_t)(cpu.s + 3);
    jsl_frame(0x83ED03u);
    abi_ActorStopY();
    cpu.s = (uint16_t)(cpu.s + 3);
    lda8(ea_dp(ACTOR_07));
    and8_v(0x03);
    if (!IF_Z)
        ACC8(lsr);
    else
        { jsl_frame(0x83ED10u); abi_ActorSideOfPlayer(); cpu.s = (uint16_t)(cpu.s + 3); }
    sta8(ea_dp(ACTOR_FACING));
    sta8(ea_dp(ACTOR_FLIP));
    stz8(ea_dp(ACTOR_48));
    lda8(ea_dp(ACTOR_07));
    and8_v(0x1C);
    ACC8(lsr);
    tax8();
    switch (cpu.x) {
    case 0: k6d_across(); return;
    case 2: k6d_updown(); return;
    case 4: k6d_run(); return;
    case 6: k6d_hop(0x55, 0); return;
    case 8: k6d_homing(); return;
    case 10: k6d_spawner(); return;
    case 12: k6d_homing(); return;
    default: k6d_hop(0x7F, 1); return;
    }
}

/*
 * @routine 83:EFF0 Kind6DSecond
 * @actor
 * WHAT IT DOES: Actor kind $6D downed (its state with bit 7 set): knocked
 * away in pose 7, with sound $0F unless a shot downed it while it faced
 * right; it turns to face right, holds 10 frames, then flies each frame
 * until it is out of range.
 * HOW: ActorMotionPreset $1F, AnimStart 7, K6D_FRAMES $0A. K6D_DOWNED zero
 * or its facing with bit 7 set: SoundQueueAdd $0F. Facing bit 7 cleared.
 * Each frame (BRK) AnimStep, K6D_FRAMES times; then each frame (BRK)
 * AnimStep, ActorFly and ActorEndOutOfRange with X = 0.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $6D in a state with bit 7 set (the table
 * at $83:8DFD). Exit: never returns; ends when out of range.
 */
void abi_Kind6DSecond(void)
{
    uint8_t facing;

    y8_load(K6D_KNOCKED);
    jsl_frame(0x83EFF2u);
    abi_ActorMotionPreset();
    cpu.s = (uint16_t)(cpu.s + 3);
    a8_load(K6D_POSE);
    jsl_frame(0x83EFF8u);
    abi_AnimStart();
    cpu.s = (uint16_t)(cpu.s + 3);
    a8_load(0x0A);
    wr8(dpa(K6D_FRAMES), 0x0A);
    a8_load(rd8(dpa(K6D_DOWNED)));
    if ((uint8_t)cpu.a) {
        facing = rd8(dpa(ACTOR_FACING));
        a8_load(facing);
        if (!(facing & 0x80))
            goto face;
    }
    a8_load(K6D_SOUND);
    jsl_frame(0x83F00Au);
    abi_SoundQueueAdd();
    cpu.s = (uint16_t)(cpu.s + 3);
face:
    facing = rd8(dpa(ACTOR_FACING));
    a8_load(facing);
    if (facing & 0x80) {
        a8_load((uint8_t)(facing & 0x7F));
        wr8(dpa(ACTOR_FACING), (uint8_t)cpu.a);
    }
    do {
        actor_yield(0x83F016u);
        jsl_frame(0x83F018u); abi_AnimStep(); cpu.s = (uint16_t)(cpu.s + 3);
    } while (dec8_at(dpa(K6D_FRAMES)));
    for (;;) {
        actor_yield(0x83F020u);
        jsl_frame(0x83F022u); abi_AnimStep(); cpu.s = (uint16_t)(cpu.s + 3);
        jsl_frame(0x83F026u);
        abi_ActorFly();
        cpu.s = (uint16_t)(cpu.s + 3);
        x8_load(0x00);
        /* the JSR's return address on the stack, which ActorEndOutOfRange
           pulls when it ends the actor */
        call_frame(0xF02E, abi_ActorEndOutOfRange);
    }
}
