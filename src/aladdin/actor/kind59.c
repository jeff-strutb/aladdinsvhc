/*
 * kind59.c: actor kind $59, the figures of the Genie's screen (level
 * $2F): by its $07 a pose, or a tally that waits for the screen's step
 * ($00B5) and then shows the lives, a row of lamps, the gems or the apples
 * as they change, each change with a spark (kind $A4) and a sound. Its
 * wait is Kind59Wait (kind_waits_rest.c).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_ActorSpawnSameLook(void);
void abi_SoundQueueAdd(void);
void abi_ActorEndPlaced(void);

#define K59_SHOWN 0x40u           /* the gems' or apples' count shown ($FF: none yet) */
#define K59_ROW_SHOWN 0x41u       /* a row lamp's pose shown */
#define K59_ROW 0x43u             /* a row lamp: its number from 1 (its $07's low 4 bits plus 1) */
#define K59_ROW_AT 0x44u          /* a row lamp: 4 times its $07's low 4 bits, into K59_ROW_PLACES */
#define K59_LIVES_SHOWN 0x45u     /* the lives shown ($FF: none yet) */
#define K59_STEP 0x00B5u          /* the screen's step: 0 waits; positive (negative for the apples) goes on */
#define K59_SCENE_Y 0x00B0u       /* the screen's y (word) */
#define K59_APPLES 0x035Bu
#define K59_GEMS 0x0364u
#define K59_ROW_LIT 0x0365u       /* the row's lamps that show */
#define K59_LIVES 0x0367u
#define K59_ROW_PLACES 0xD3D0u    /* (data bank $80) per row lamp, its x and its y below the screen (words) */
#define K59_SPARK 0xA4u           /* the kind of the spark it spawns at a change */
#define K59_SPARK_SOUND 0x28u
#define K59_COUNT_SOUND 0x0Cu
#define K59_TALLY 0x06u           /* the $07 it keeps once it goes on (and the way for $07 of $10 up) */
#define K59_WAYS 0xDD27u          /* the ways' table, after the jump (bank $83) */

/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/* The lives (83:DD72): each frame the lives shown, from 4 up as pose $0F,
   below as lives + $0C with a spark and its sound. */
static void lives(void)
{
    lda8_v(0xFF);
    sta8(ea_dp(K59_LIVES_SHOWN));
    for (;;) {
        actor_yield(0x83DD76u);
        op_rep(0x21);
        lda16_v(0x0044);
        sta16(ea_dp(ACTOR_X + 1));
        lda16(ea_abs(K59_SCENE_Y));
        adc16_v(0x0010);
        sta16(ea_dp(ACTOR_Y + 1));
        op_sep(0x20);
        lda8(ea_abs(K59_LIVES));
        cmp8_v(rd8(ea_dp(K59_LIVES_SHOWN)));
        if (IF_Z)
            continue;
        sta8(ea_dp(K59_LIVES_SHOWN));
        cmp8_v(0x04);
        if (IF_C) {
            lda8_v(0x0F);
            jsl_call(0x83DD98u, abi_AnimStart);
            continue;
        }
        clc();
        adc8_v(0x0C);
        jsl_call(0x83DDA1u, abi_AnimStart);
        lda8(ea_dp(K59_LIVES_SHOWN));
        clc();
        adc8_v(0x43);
        ACC8(asl);
        sta8(ea_dp(ACTOR_SPAWN_AT));
        lda8(ea_dp(ACTOR_Y + 2));
        if (IF_N)
            continue;
        lda8_v(K59_SPARK);
        jsl_call(0x83DDB3u, abi_ActorSpawnSameLook);
        lda8_v(K59_SPARK_SOUND);
        jsl_call(0x83DDB9u, abi_SoundQueueAdd);
    }
}

/* A lamp of a row (83:DDBF): each frame at its place; hidden while the
   row's lamps fall short of it, else lit ($0B) while the lives reach it,
   dark ($0C) while not, a change with a spark and its sound. */
static void lamp(void)
{
    for (;;) {
        actor_yield(0x83DDBFu);
        ldx8(ea_dp(K59_ROW_AT));
        op_rep(0x21);
        lda16(ea_absx(K59_ROW_PLACES));
        sta16(ea_dp(ACTOR_X + 1));
        lda16(ea_abs(K59_SCENE_Y));
        adc16_v(rd16(ea_absx(K59_ROW_PLACES + 2)));
        sta16(ea_dp(ACTOR_Y + 1));
        op_sep(0x20);
        ldx8(ea_abs(K59_ROW_LIT));
        cpx8_v(rd8(ea_dp(K59_ROW)));
        if (!IF_C) {
            lda8_v(0x08);
            trb8(ea_dp(ACTOR_08));
            continue;
        }
        lda8_v(0x0C);
        ldx8(ea_abs(K59_LIVES));
        cpx8_v(rd8(ea_dp(K59_ROW)));
        if (IF_C)
            ACC8(dec);
        cmp8_v(rd8(ea_dp(K59_ROW_SHOWN)));
        if (IF_Z)
            continue;
        sta8(ea_dp(K59_ROW_SHOWN));
        jsl_call(0x83DDEBu, abi_AnimStart);
        lda8(ea_dp(ACTOR_Y + 2));
        if (IF_N)
            continue;
        lda8_v(K59_SPARK);
        jsl_call(0x83DDF5u, abi_ActorSpawnSameLook);
        lda8_v(K59_SPARK_SOUND);
        jsl_call(0x83DDFBu, abi_SoundQueueAdd);
    }
}

/* Ways 6 and $10 up (83:DD49): wait for the step; then the lives or a
   lamp. */
static void tally(void)
{
    do {
        actor_yield(0x83DD49u);
        lda8(ea_abs(K59_STEP));
    } while (IF_Z);
    if (IF_N) {
        abi_ActorEndPlaced();
        return;
    }
    lda8_v(0x01);
    tsb8(ea_dp(ACTOR_08));
    stz8(ea_dp(K59_ROW_SHOWN));
    lda8(ea_dp(ACTOR_07));
    and8_v(0x0F);
    sta8(ea_dp(K59_ROW));
    ACC8(asl);
    ACC8(asl);
    sta8(ea_dp(K59_ROW_AT));
    MEM8(inc, ea_dp(K59_ROW));
    lda8_v(K59_TALLY);
    sta8(ea_dp(ACTOR_07));
    stz8(ea_dp(ACTOR_SPAWN_AT));
    lda8(ea_dp(K59_ROW));
    ACC8(dec);
    if (!IF_Z)
        lamp();
    else
        lives();
}

/* Way 2 (83:DE07): wait for the step; then the gems. */
static void gems(void)
{
    do {
        actor_yield(0x83DE07u);
        lda8(ea_abs(K59_STEP));
    } while (IF_Z);
    if (IF_N) {
        abi_ActorEndPlaced();
        return;
    }
    lda8_v(0x01);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(K59_TALLY);
    sta8(ea_dp(ACTOR_07));
    stz8(ea_dp(ACTOR_SPAWN_AT));
    for (;;) {
        actor_yield(0x83DE1Du);
        op_rep(0x21);
        lda16(ea_abs(K59_SCENE_Y));
        adc16_v(0x0018);
        sta16(ea_dp(ACTOR_Y + 1));
        lda16_v(0x0024);
        sta16(ea_dp(ACTOR_X + 1));
        op_sep(0x20);
        lda8(ea_abs(K59_GEMS));
        cmp8_v(rd8(ea_dp(K59_SHOWN)));
        if (IF_Z)
            continue;
        sta8(ea_dp(K59_SHOWN));
        cmp8_v(0x0B);
        if (IF_C) {
            lda8_v(0x0A);
            jsl_call(0x83DE3Fu, abi_AnimStart);
        } else {
            jsl_call(0x83DE45u, abi_AnimStart);
        }
        lda8(ea_dp(ACTOR_Y + 2));
        if (IF_N)
            continue;
        lda8_v(K59_COUNT_SOUND);
        jsl_call(0x83DE4Fu, abi_SoundQueueAdd);
        lda8_v(K59_SPARK);
        jsl_call(0x83DE55u, abi_ActorSpawnSameLook);
    }
}

/* Way 8 (83:DE6C): wait for the step; negative: the apples. */
static void apples(void)
{
    do {
        actor_yield(0x83DE6Cu);
        lda8(ea_abs(K59_STEP));
    } while (IF_Z);
    if (!IF_N) {
        abi_ActorEndPlaced();
        return;
    }
    lda8_v(0x01);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(K59_TALLY);
    sta8(ea_dp(ACTOR_07));
    stz8(ea_dp(ACTOR_SPAWN_AT));
    lda8_v(0xFF);
    sta8(ea_dp(K59_SHOWN));
    for (;;) {
        actor_yield(0x83DE86u);
        op_rep(0x21);
        lda16(ea_abs(K59_SCENE_Y));
        adc16_v(0x0014);
        sta16(ea_dp(ACTOR_Y + 1));
        lda16_v(0x004E);
        sta16(ea_dp(ACTOR_X + 1));
        op_sep(0x20);
        lda8(ea_abs(K59_APPLES));
        cmp8_v(rd8(ea_dp(K59_SHOWN)));
        if (IF_Z)
            continue;
        sta8(ea_dp(K59_SHOWN));
        ACC8(inc);
        jsl_call(0x83DEA3u, abi_AnimStart);
        lda8(ea_dp(ACTOR_Y + 2));
        if (IF_N)
            continue;
        lda8_v(K59_COUNT_SOUND);
        jsl_call(0x83DEADu, abi_SoundQueueAdd);
        lda8_v(K59_SPARK);
        jsl_call(0x83DEB3u, abi_ActorSpawnSameLook);
    }
}

/*
 * @routine 83:DD10 Kind59Start
 * @actor
 * WHAT IT DOES: Actor kind $59, a figure of the Genie's screen, by its
 * $07: 0 or 1 poses (facing 0, flip its $07, x pixel $80) for good; 4
 * poses, waits 24 frames and ends; 2, 6 and $10 up wait for the screen's
 * step to be positive (negative: they end), 8 for it to be negative
 * (positive: it ends). Then, its $07 6 from there on, each frame at the
 * screen's y: 2 shows the gems; 8 the apples; $10 the lives; $11-$1F a
 * lamp of a row, its number the $07's low 4 bits plus 1: lit while the
 * lives reach it, dark while not, hidden while the row's lamps
 * ($0365) fall short of it. A change shows (AnimStart) and, with the
 * figure on the screen (its y's top bit clear), spawns a spark (kind $A4)
 * and makes a sound.
 * HOW: $09 bits 7 and 4 set; K59_SHOWN $FF. $07 1: the pose. Else X = the
 * $07 ($06 from $10 up) into the table after the jump. The pose: $08 bit
 * 1 set; facing 0; flip = the $07; x pixels $80 (Y, 8-bit); AnimStart 0;
 * BRK for good. Way 4: $08 bit 0 left as it is (TSB of 0); AnimStart $0F;
 * COP $18; ActorEndPlaced. The waits: BRK until K59_STEP is nonzero; the
 * wrong sign: ActorEndPlaced. Then $08 bit 0 set, $07 6, $21 0 (the
 * spawn point). The lives (way 6, $07's low bits 0; K59_ROW 1, its
 * shown $FF): each frame (BRK first) x $44, y K59_SCENE_Y + $10;
 * K59_LIVES unchanged: again; 4 up: AnimStart $0F; else AnimStart lives +
 * $0C, the spawn point (lives + $43) times 2, and on the screen
 * ActorSpawnSameLook $A4 and SoundQueueAdd $28. A lamp (way 6, low bits
 * nonzero; K59_ROW_SHOWN 0): each frame (BRK first) x and y (plus
 * K59_SCENE_Y) from K59_ROW_PLACES by K59_ROW_AT; K59_ROW_LIT below
 * K59_ROW: $08 bit 3 cleared; else pose $0C, $0B while K59_LIVES reaches
 * K59_ROW; unchanged: again; else AnimStart, and on the screen the spark
 * and sound $28. The gems (way 2): each frame (BRK first) y K59_SCENE_Y
 * + $18, x $24; K59_GEMS unchanged: again; AnimStart it ($0A from $0B
 * up); on the screen sound $0C and the spark. The apples (way 8; shown
 * $FF): each frame (BRK first) y K59_SCENE_Y + $14, x $4E; K59_APPLES
 * unchanged: again; AnimStart it plus 1; on the screen sound $0C and the
 * spark.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $59 in state $0C (the table at $83:8CAB).
 * Exit: never returns; the ways that end leave by ActorEndPlaced.
 */
void abi_Kind59Start(void)
{
    lda8_v(0x90);
    tsb8(ea_dp(ACTOR_09));
    lda8_v(0xFF);
    sta8(ea_dp(K59_SHOWN));
    lda8(ea_dp(ACTOR_07));
    tax8();
    ACC8(dec);
    if (!IF_Z) {
        cpx8_v(0x10);
        if (IF_C)
            ldx8_v(K59_TALLY);
        switch (cpu.x) {
        case 0:
            break;
        case 2:
            gems();
            return;
        case 4:                         /* the brief pose */
            lda8_v(0x00);
            tsb8(ea_dp(ACTOR_08));
            lda8_v(0x0F);
            jsl_call(0x83DE61u, abi_AnimStart);
            lda8_v(0x18);
            actor_wait(0x83DE67u);
            abi_ActorEndPlaced();
            return;
        case 6:
            tally();
            return;
        case 8:
            apples();
            return;
        default:                        /* past the table: its next bytes as words */
            jump_routine(0x830000u | rd16(0x830000u | (uint16_t)(K59_WAYS + cpu.x)));
            return;
        }
    }
    /* the pose */
    lda8_v(0x02);
    tsb8(ea_dp(ACTOR_08));
    stz8(ea_dp(ACTOR_FACING));
    lda8(ea_dp(ACTOR_07));
    sta8(ea_dp(ACTOR_FLIP));
    ldy8_v(0x80);
    sty8(ea_dp(ACTOR_X + 1));
    lda8_v(0x00);
    jsl_call(0x83DD41u, abi_AnimStart);
    for (;;)
        actor_yield(0x83DD45u);
}
