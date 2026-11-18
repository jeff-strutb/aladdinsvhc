/*
 * backdrop_actor.c: the actor of kind $6E (its start, Kind6EStart, its
 * wait routine at $83:F1D5 and its pieces), drawn as the picture of the
 * backdrop layer, not as sprites: it moves right 3 pixels a frame, its
 * picture's frames are uploads cycled from a table, and the backdrop is
 * shown, scrolled to put the picture where the actor is, only while it is
 * near. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram_video.h"

/* The actor's own fields of its record */
#define PICTURE_AT 0x41u           /* the offset of its picture's frame in PICTURE_FRAMES */
#define PICTURE_TIME 0x42u         /* frames left before the next */

/* In the bank DB names: 2 bytes a frame: the upload's number, then its
   frames (bit 7 set on the last of the cycle) */
#define PICTURE_FRAMES 0xD67Bu

#define RANGE_X 0x001A44u          /* the point actors are kept in range of (words) */
#define RANGE_Y 0x001A46u
#define PLAY_X (LAYER0 + LAYER_SCROLL_X)
#define PLAY_Y (LAYER0 + LAYER_SCROLL_Y)
#define BACK_X (LAYER1 + LAYER_SCROLL_X)
#define BACK_Y (LAYER1 + LAYER_SCROLL_Y)
#define TM_SHADOW 0x000399u        /* TM's shadow: the main screen's layers */
#define LAYERS_WITH_BACKDROP 0x17  /* BG1, BG2, BG3 and sprites */
#define LAYERS_NO_BACKDROP 0x15    /* BG1, BG3 and sprites */

#define K6E_STEP 0x40u          /* the spawner's offset in K6E_SPAWNS; else frames left of a stretch */
#define K6E_COPIER 0x4Au        /* set when the copier checks found one */
#define K6E_SOUND_TIME 0x48u    /* frames to Kind6EWait's next sound */
#define K6E_QUIET 0x49u         /* nonzero: Kind6EWait sounds no more */
#define K6E_RAMP 0x43u          /* its gravity ramp (word; the high byte is the gravity) */
#define K6E_PAIRS_AT 0x47u      /* its offset in K6E_PAIRS */
#define K6E_CHILD_SLOT 0x21u    /* the slot byte its children are made with */
#define K6E_SPAWNS 0xD687u      /* (data bank) 4 bytes a child: scroll x (word), $07, slot */
#define K6E_MOTIONS 0xD69Cu     /* (data bank) by $07: its motion preset */
#define K6E_PAIRS 0xD69Fu       /* (data bank) decelerate's step, then frames of speeding up; 0 ends */
#define K6E_18CA 0x18CAu
#define K6E_0834 0x0834u
#define K6E_083D 0x083Du        /* $10 when a copier was found */
#define K6E_SCROLL_X 0x1773u    /* the play field's scroll x (word) */
#define K6E_RANGE_X 0x1A44u
#define K6E_RANGE_Y 0x1A46u
#define K6E_TM_SHADOW 0x0399u

void abi_ActorAccelerateX(void);
void abi_ActorDecelerateX(void);
void abi_ActorEndPlaced(void);
void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_ActorMoveX(void);
void abi_ActorSpawnShared(void);
void abi_ActorSpeedXSet(void);
void abi_ActorStepRight3(void);
void abi_SoundQueueAdd(void);

/* $07 0 ($83:F058): makes a child each time the scroll passes the list's
   next point; never ends. */
static void k6e_spawner(void)
{
    stz8(ea_dp(ACTOR_FLIP));
    stz8(ea_dp(K6E_STEP));
    lda8_v(0x01);
    sta8(ea_abs(K6E_18CA));
    lda8_v(0x01);
    sta8(ea_abs(K6E_0834));
    op_rep(0x20);
    for (;;) {
        actor_yield(0x83F068u);
        ldx8(ea_dp(K6E_STEP));
        lda16(ea_abs(K6E_SCROLL_X));
        cmp16_v(rd16(ea_absx(K6E_SPAWNS)));
        if (!IF_C)
            continue;
        lda16(ea_abs(K6E_RANGE_X));
        sta16(ea_dp(ACTOR_X + 1));
        lda16(ea_abs(K6E_RANGE_Y));
        sta16(ea_dp(ACTOR_Y + 1));
        op_sep(0x20);
        lda8(ea_absx(K6E_SPAWNS + 2));
        sta8(ea_dp(ACTOR_07));
        lda8(ea_absx(K6E_SPAWNS + 3));
        sta8(ea_dp(K6E_CHILD_SLOT));
        phx8();
        lda8_v(0x6E);
        abi_ActorSpawnShared();
        plx8();
        inx8();
        inx8();
        inx8();
        inx8();
        stx8(ea_dp(K6E_STEP));
        op_rep(0x20);
    }
}

/* $07 1 and 2 ($83:F09C): rises, glides right, then speeds off right. */
static void k6e_flyer(void)
{
    stz8(ea_dp(K6E_COPIER));
    phb();
    lda8_v(0x80);
    pha8();
    plb();
    ldy8(ea_abs(0xA000));
    ldx8(ea_abs(0xA001));
    lda8_v(0x00);
    pha8();
    plb();
    cpy8_v(rd8(ea_abs(0xA000)));
    if (!IF_Z)
        goto copier;
    cpx8_v(rd8(ea_abs(0xA001)));
    if (!IF_Z)
        goto copier;
    ldx8_v(0x20);
    sec();
    lda8_v(0x90);
    sbc8_v(0x20);
    pha8();
    plb();
    lda8_v(0x23);
    sta8(ea_absx(0x0000));
    cmp8_v(rd8(ea_absx(0x0000)));
    if (!IF_Z)
        goto cart;
    lda8_v(0x23);
    sta8(ea_absx(0x0020));
    cmp8_v(rd8(ea_absx(0x0020)));
    if (!IF_Z)
        goto cart;
copier:
    plb();
    MEM8(inc, ea_dp(K6E_COPIER));
    goto fly;
cart:
    plb();
fly:
    stz8(ea_dp(PICTURE_AT));
    stz8(ea_dp(PICTURE_TIME));
    lda8_v(0x80);
    sta8(ea_dp(ACTOR_09));
    lda8_v(0x2A);
    abi_SoundQueueAdd();
    lda8_v(0x9D);
    sta8(ea_dp(K6E_SOUND_TIME));
    stz8(ea_dp(K6E_QUIET));
    ldx8(ea_dp(ACTOR_07));
    ldy8(ea_absx(K6E_MOTIONS));
    abi_ActorMotionPreset();
    do {
        actor_yield(0x83F0F7u);
        abi_ActorFly();
        abi_ActorStepRight3();
        lda8(ea_dp(ACTOR_SPEED_Y + 2));
    } while (IF_N);
    lda8_v(0x30);
    sta8(ea_dp(K6E_STEP));
    do {
        actor_yield(0x83F108u);
        abi_ActorStepRight3();
        MEM8(dec, ea_dp(K6E_STEP));
    } while (!IF_Z);
    lda8(ea_dp(K6E_COPIER));
    if (!IF_Z) {
        lda8_v(0x10);
        sta8(ea_abs(K6E_083D));
    }
    MEM8(inc, ea_dp(K6E_QUIET));
    lda8_v(0x2B);
    abi_SoundQueueAdd();
    stz8(ea_dp(ACTOR_SPEED_X));
    stz8(ea_dp(ACTOR_SPEED_X + 1));
    stz8(ea_dp(ACTOR_SPEED_X + 2));
    stz8(ea_dp(K6E_RAMP));
    stz8(ea_dp(K6E_RAMP + 1));
    lda8_v(0x10);
    sta8(ea_dp(ACTOR_ACCEL_X));
    do {
        actor_yield(0x83F130u);
        abi_ActorAccelerateX();
        op_rep(0x20);
        lda16_v(0x0200);
        cmp16_v(rd16(ea_dp(ACTOR_SPEED_X)));
        if (!IF_C)
            sta16(ea_dp(ACTOR_SPEED_X));
        clc();
        lda16(ea_dp(K6E_RAMP));
        adc16_v(0x0080);
        sta16(ea_dp(K6E_RAMP));
        lda16_v(0x3000);
        cmp16_v(rd16(ea_dp(K6E_RAMP)));
        if (!IF_C)
            sta16(ea_dp(K6E_RAMP));
        op_sep(0x20);
        lda8(ea_dp(K6E_RAMP + 1));
        sta8(ea_dp(ACTOR_ACCEL_Y));
        abi_ActorFly();
        abi_ActorStepRight3();
        lda8(ea_abs(K6E_TM_SHADOW));
        cmp8_v(0x15);
    } while (!IF_Z);
    abi_ActorEndPlaced();
}

/* $07 3 ($83:F169): slows and speeds by the pairs of its list, turning
   each time; at the list's 0, a sound and it ends. */
static void k6e_pairs(void)
{
    stz8(ea_dp(PICTURE_AT));
    stz8(ea_dp(PICTURE_TIME));
    stz8(ea_dp(K6E_PAIRS_AT));
    lda8_v(0x2A);
    abi_SoundQueueAdd();
    lda8_v(0x9D);
    sta8(ea_dp(K6E_SOUND_TIME));
    stz8(ea_dp(K6E_QUIET));
    lda8_v(0x80);
    sta8(ea_dp(ACTOR_09));
    stz8(ea_dp(ACTOR_FACING));
    ldy8_v(0x45);
    abi_ActorSpeedXSet();
    do {
        ldx8(ea_dp(K6E_PAIRS_AT));
        lda8(ea_absx(K6E_PAIRS));
        sta8(ea_dp(ACTOR_ACCEL_X));
        MEM8(inc, ea_dp(K6E_PAIRS_AT));
        do {
            actor_yield(0x83F190u);
            abi_ActorDecelerateX();
            abi_ActorMoveX();
            abi_ActorStepRight3();
            lda8(ea_dp(ACTOR_SPEED_X));
            ora8_v(rd8(ea_dp(ACTOR_SPEED_X + 1)));
            ora8_v(rd8(ea_dp(ACTOR_SPEED_X + 2)));
        } while (!IF_Z);
        lda8(ea_dp(ACTOR_FACING));
        eor8_v(0x01);
        sta8(ea_dp(ACTOR_FACING));
        ldx8(ea_dp(K6E_PAIRS_AT));
        lda8(ea_absx(K6E_PAIRS));
        sta8(ea_dp(K6E_STEP));
        MEM8(inc, ea_dp(K6E_PAIRS_AT));
        do {
            actor_yield(0x83F1B4u);
            abi_ActorAccelerateX();
            abi_ActorMoveX();
            abi_ActorStepRight3();
            MEM8(dec, ea_dp(K6E_STEP));
        } while (!IF_Z);
        ldx8(ea_dp(K6E_PAIRS_AT));
        lda8(ea_absx(K6E_PAIRS + 1));
    } while (!IF_Z);
    lda8_v(0x23);
    abi_SoundQueueAdd();
    abi_ActorEndPlaced();
}

/*
 * @routine 83:F049 Kind6EStart
 * @actor
 * WHAT IT DOES: Actor kind $6E, the backdrop picture: by its $07, 0 a
 * spawner that makes a kind $6E child each time the view's scroll passes
 * the next point of its list; 1 and 2 one that rises and glides right,
 * then speeds off right until the main screen's layers are $15 and ends
 * (a copier makes it set $083D); 3 one that slows and speeds by a list of
 * pairs, turning each time, then a sound and ends.
 * HOW: JMP through the table at $83:F050 by $07. 0: facing, K6E_STEP 0,
 * K6E_18CA and K6E_0834 1; each frame (BRK) while the scroll x ($1773) is
 * at or past the list's word: its x, y = RANGE_X, RANGE_Y, $07 and the
 * slot from the list, ActorSpawnShared $6E (X kept), K6E_STEP + 4. 1, 2:
 * K6E_COPIER set when $80:A000-$A001 do not read the same at $00, or $23
 * reads back at $70:0020 and $70:0040; PICTURE_AT, PICTURE_TIME 0,
 * ACTOR_09 $80, SoundQueueAdd $2A, K6E_SOUND_TIME $9D, K6E_QUIET 0,
 * ActorMotionPreset by K6E_MOTIONS; each frame (BRK) ActorFly and
 * ActorStepRight3 while rising; $30 frames of ActorStepRight3; a copier:
 * K6E_083D $10; K6E_QUIET up, SoundQueueAdd $2B, its speed 0, the ramp 0,
 * acceleration $10; each frame (BRK) ActorAccelerateX (speed at most
 * $0200), the ramp + $80 (at most $3000), gravity its high byte,
 * ActorFly, ActorStepRight3, until TM_SHADOW is $15: ActorEndPlaced. 3:
 * PICTURE_AT, PICTURE_TIME, K6E_PAIRS_AT 0, SoundQueueAdd $2A,
 * K6E_SOUND_TIME $9D, K6E_QUIET 0, ACTOR_09 $80, facing 0,
 * ActorSpeedXSet $45; for each pair of K6E_PAIRS: acceleration the first,
 * each frame (BRK) ActorDecelerateX, ActorMoveX, ActorStepRight3 until
 * its speed is 0; facing turned; the second's frames (BRK) of
 * ActorAccelerateX, ActorMoveX, ActorStepRight3; at the list's 0,
 * SoundQueueAdd $23 and ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $6E in state $0C (the table at
 * $83:8CAB). Exit: never returns.
 */
void abi_Kind6EStart(void)
{
    lda8(ea_dp(ACTOR_07));
    ACC8(asl);
    tax8();
    switch (cpu.x) {
    case 0:
        k6e_spawner();
        return;
    case 2:
    case 4:
        k6e_flyer();
        return;
    case 6:
        k6e_pairs();
        return;
    default:                        /* past the table's 4 words: no $07 names one */
        jump_routine(0x830000u | rd16(0x830000u | (uint16_t)(0xF050 + cpu.x)));
        return;
    }
}

/*
 * @routine 83:F211 ActorStepRight3
 * WHAT IT DOES: Moves the actor 3 pixels to the right.
 * HOW: A 24-bit addition of $000300 to its x, a byte at a time.
 * Entry: M 8-bit; D = the actor's record. Exit: A low = its x's top byte;
 * N, V, Z and C from its addition. M 8-bit; A high, X and Y unchanged.
 */
void abi_ActorStepRight3(void)
{
    uint8_t top;

    flag_set(P_C, 0);
    wr8(dpa(ACTOR_X), op_adc8(rd8(dpa(ACTOR_X)), 0x00, &cpu.p));
    wr8(dpa(ACTOR_X + 1), op_adc8(rd8(dpa(ACTOR_X + 1)), 0x03, &cpu.p));
    top = op_adc8(rd8(dpa(ACTOR_X + 2)), 0x00, &cpu.p);
    wr8(dpa(ACTOR_X + 2), top);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | top);
}

/*
 * @routine 83:F225 BackdropPictureStep
 * WHAT IT DOES: Plays the actor's picture on by a frame of time: when the
 * picture's frame has run its time, the next one (after the last, the
 * first) is asked for as a fixed VRAM upload.
 * HOW: The time steps down; below 0, the offset steps on by 2 (back to 0
 * when the frame left was the last) and the new frame's time (low 7 bits)
 * and upload number are taken.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB the table's bank,
 * whose $0000-$1FFF is work RAM. Exit: time left: N clear, Z from it; A,
 * X, Y and the other flags unchanged. A new frame: A low = its upload
 * number; N and Z from it; Y = its offset. M 8-bit, X 8-bit; A high, X,
 * V and C unchanged.
 */
void abi_BackdropPictureStep(void)
{
    uint8_t at, v;

    if (!(dec8_at(dpa(PICTURE_TIME)) & 0x80))
        return;
    /* the next frame (back to the first after a negative entry): its
       time and its upload */
    at = (uint8_t)(rd8(dpa(PICTURE_AT)) + 2);
    cpu.y = at;
    v = rd8(dba(PICTURE_FRAMES - 1) + at);
    a8_load(v);
    if (v & 0x80) {
        y8_load(0x00);
        at = 0;
    }
    a8_load(rd8(dba(PICTURE_FRAMES + 1) + at) & 0x7F);
    wr8(dpa(PICTURE_TIME), (uint8_t)cpu.a);
    a8_load(rd8(dba(PICTURE_FRAMES) + at));
    wr8(dba(VRAM_UPLOAD_NUMBER), (uint8_t)cpu.a);
    wr8(dpa(PICTURE_AT), at);
}

/* Its distance from the point `from` in one axis, plus 256: below 512
   when within 256 either side. */
static int near_point(uint16_t mine, uint16_t from)
{
    uint16_t d;

    flag_set(P_C, 1);
    d = op_sbc16(mine, rd16(dba(from)), &cpu.p);
    flag_set(P_C, 0);
    d = op_adc16(d, 0x0100, &cpu.p);
    cpu.a = d;
    cmp16_of(d, 0x0200);
    return d < 0x0200;
}

/* The backdrop's scroll in one axis: the actor's offset from the screen's
   middle (128), turned. */
static void backdrop_scroll(uint16_t mine, uint16_t play, uint16_t back)
{
    uint16_t d;

    flag_set(P_C, 1);
    d = op_sbc16(mine, rd16(dba(play)), &cpu.p);
    flag_set(P_C, 1);
    d = op_sbc16(d, 0x0080, &cpu.p);
    a16_load((uint16_t)((d ^ 0xFFFF) + 1));
    wr16(dba(back), cpu.a);
}

/*
 * @routine 83:F244 BackdropActorShow
 * WHAT IT DOES: Shows the backdrop, with the actor's picture on it, while
 * the actor is within 256 pixels of the range point on both axes: the
 * backdrop is scrolled to put its point (128, 128) where the actor is on
 * the screen. Farther away the backdrop is not shown.
 * HOW: With 16-bit A, each distance plus 256 compared unsigned with 512;
 * near: the backdrop's scroll is the play field's plus 128 less the
 * actor's pixels, on each axis, and TM's shadow is $17; else $15.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM. Exit: near: A low = $17; A high = the high
 * byte of the backdrop's new scroll Y; N and Z clear; V and C from the
 * last subtraction. Far: A low = $15; A high = the high byte of the
 * failed distance plus 256; N and Z clear; C set; V from its addition. M
 * 8-bit; X and Y unchanged.
 */
void abi_BackdropActorShow(void)
{
    op_rep(P_M);
    if (near_point(dp16(ACTOR_X + 1), RANGE_X) && near_point(dp16(ACTOR_Y + 1), RANGE_Y)) {
        backdrop_scroll(dp16(ACTOR_X + 1), PLAY_X, BACK_X);
        backdrop_scroll(dp16(ACTOR_Y + 1), PLAY_Y, BACK_Y);
        op_sep(P_M);
        a8_load(LAYERS_WITH_BACKDROP);
        wr8(dba(TM_SHADOW), LAYERS_WITH_BACKDROP);
        return;
    }
    op_sep(P_M);
    a8_load(LAYERS_NO_BACKDROP);
    wr8(dba(TM_SHADOW), LAYERS_NO_BACKDROP);
}
