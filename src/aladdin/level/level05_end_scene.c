/*
 * level05_end_scene.c: level 5's closing scene, a task its screen set-up
 * starts ($81:8954, through the table by level at $81:8802): once Aladdin
 * gets far enough, the camera is taken over, actors play the scene out on
 * cues, part of the map is rebuilt, the screen reddens and the level ends.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

void abi_ActorSpawnAt(void);
void abi_ActorsFreeAll(void);
void abi_MapTileAt(void);
void abi_MapTileSet(void);
void abi_SongPlay(void);
void abi_SoundQueueF6(void);
void abi_TaskCreate(void);
void abi_TaskExit(void);
void abi_TaskWait(void);

#define CUE_BD 0x00BDu          /* the scene's cues (eight bytes, $BD-$C4), cleared as it starts */
#define CUE_BF 0x00BFu
#define CUE_C0 0x00C0u          /* set by the task as the map is rebuilt */
#define CUE_C1 0x00C1u          /* set by the task at the second song */
#define CUE_C3 0x00C3u
#define CUE_C4 0x00C4u
#define SCROLL_X (LAYER0 + LAYER_SCROLL_X)        /* the play field's scroll x (word) */
#define SCROLL_Y (LAYER0 + LAYER_SCROLL_Y)        /* and y (word) */
#define PLAYER_X (PLAYER + ACTOR_X + 1)           /* Aladdin's x, whole pixels (word) */
#define V_03FA 0x03FAu
#define V_1A10 0x1A10u
#define V_1A12 0x1A12u
#define V_083D 0x083Du
#define V_0087 0x0087u
#define V_0347 0x0347u
#define V_035D 0x035Du
#define V_0360 0x0360u
#define V_1423 0x1423u
#define V_1454 0x1454u
#define WINDOW_03A6 0x03A6u     /* the windows' settings, $03A6-$03AE */
#define SUB_LAYERS_0399 0x0399u /* layers the ending copies to the main and sub window masks */
#define SUB_LAYERS_039A 0x039Au
#define PLACE_X 0x002Cu         /* where ActorSpawnAt places the actor: x, then y (words) */
#define PLACE_Y 0x002Eu
#define FROM_X 0x2Cu            /* the map copy's (direct page, words): source x, y */
#define FROM_Y 0x2Eu
#define TO_X 0x30u              /* destination x, y */
#define TO_Y 0x32u
#define COLUMNS 0x34u           /* tiles left in the row */
#define ROWS 0x36u              /* rows left */

/* The task waits `frames` frames, its JSL returning at `site`. */
static void wait_frames(uint32_t site, uint8_t frames)
{
    a8_load(frames);
    jsl_frame(site);
    abi_TaskWait();
}

/* ActorSpawnAt of kind `kind` with its $07 `variant` at (x, y); A is 16-bit
   with x in it, and `narrow` is the SEP between the place and the kind. */
static void spawn(uint16_t x, uint16_t y, uint8_t narrow, uint8_t kind, uint8_t variant)
{
    wr16(dba(PLACE_X), x);
    a16_load(y);
    wr16(dba(PLACE_Y), y);
    op_sep(narrow);
    a8_load(kind);
    op_xba();
    a8_load(variant);
    abi_ActorSpawnAt();
}

/* Each frame until the cue at `cue` is set. */
static void wait_cue(uint32_t site, uint16_t cue)
{
    uint8_t v;

    do {
        wait_frames(site, 0x01);
        v = rd8(dba(cue));
        a8_load(v);
    } while (!v);
}

/*
 * @routine 80:A2A8 Level05EndScene
 * WHAT IT DOES: Level 5's closing scene. Once Aladdin, alive, is at x
 * $0E20 or more with the play field's scroll y under 256, he is given
 * scripted code, and on cue $C3 every actor goes and an actor of kind $2E
 * comes; the camera is brought down to y $B8 (or cue $C4) and on to x
 * $0E00, song $30; on cue $BF song $2F and the windows and colour maths
 * set, two tasks started; on cue $BD three actors placed (kinds $2E, $70,
 * $93) and an 8 by 7 block of the map's 16-pixel tiles copied from ($0A10,
 * $0240) to ($0D60, $01E0); then the screen goes red over 29 steps and
 * the level ends.
 * HOW: CUE_BD-$C4 cleared; X 16-bit. Each frame (TaskWait 1) until
 * SCROLL_Y's high byte is 0, PLAYER_X >= $0E20 and HEALTH nonzero: $03FA
 * up, the player's $03 word $CB3B and $07 $0A. Each frame until CUE_C3:
 * V_1A08 0, SoundQueueF6 2, ActorsFreeAll, ActorSpawnAt kind $2E ($07 2)
 * at ($0E80, $0148). Each frame until SCROLL_Y = $B8 or CUE_C4: SCROLL_Y
 * $B8, $083D 4, $1A10 and $1A12 up. Each frame until SCROLL_X >= $0E00:
 * SongPlay $30. Each frame until CUE_BF: CUE_C1 up, SongPlay $2F, 16
 * frames; TMW, TSW and CGADSUB $17, CGWSEL $20, $03AA $FF, $03AB and
 * WOBJSEL $0F, $03AD $15, $03AE 1, $03A6 and $03A8 0, $03A7 and $03A9
 * $FF; TaskCreate of WindowEffectTask twice: slot 1 ($18) with
 * parameter $10, slot 6 ($90) with parameter $16. Each frame until CUE_BD: ActorSpawnAt
 * kinds $2E ($07 3) at ($0E80, $0118), $70 ($07 1) at ($0DD0, $0228), $93
 * ($07 0) at ($0DB0, $0220); the map copy by MapTileAt and MapTileSet,
 * rows of 8 by 16 pixels, 7 rows. 128 frames; $0399 and $039A $13;
 * $083D 8, 28 frames, $083D 0; 180 frames; CUE_C0 up; $0087 0; 100
 * frames. CGADSUB $23, CGWSEL and the fixed colour's green and blue 0;
 * its red from 3 up by one every 3 frames until it wraps to 0; $0360 and
 * $035D 0, LEVEL_035C up, the player's facing and flip 0, $03 word
 * $E7B1; $0347 bit 7 set; $1423 and $1454 up; TaskExit.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; D
 * the task's direct page; a task's entry (TaskCreate slot $48 at
 * $81:895F). Exit: never returns (TaskExit).
 */
void abi_Level05EndScene(void)
{
    uint16_t at, x;
    uint8_t v;

    for (at = CUE_BD; at <= CUE_C4; at++)
        wr8(dba(at), 0);
    op_rep(0x10);

    /* until Aladdin, alive, is far enough in at the top of the play field */
    for (;;) {
        wait_frames(0x80A2C4u, 0x01);
        v = rd8(dba((uint16_t)(SCROLL_Y + 1)));
        a8_load(v);
        if (v)
            continue;
        x = rd16(dba((uint16_t)PLAYER_X));
        cpu.x = x;
        cmp16_of(x, 0x0E20);
        if (x < 0x0E20)
            continue;
        v = rd8(dba((uint16_t)HEALTH));
        a8_load(v);
        if (v)
            break;
    }
    (void)inc8_at(dba(V_03FA));
    op_rep(0x20);
    a16_load(0xCB3B);
    wr16(dba((uint16_t)(PLAYER + ACTOR_03)), 0xCB3B);
    op_sep(0x30);
    a8_load(0x0A);
    wr8(dba((uint16_t)(PLAYER + ACTOR_07)), 0x0A);

    /* on cue $C3: the actors go and one of kind $2E comes */
    wait_cue(0x80A2EEu, CUE_C3);
    wr8(dba((uint16_t)V_1A08), 0);
    a8_load(0x02);
    abi_SoundQueueF6();
    abi_ActorsFreeAll();
    op_rep(0x20);
    a16_load(0x0E80);
    spawn(0x0E80, 0x0148, 0x20, 0x2E, 0x02);
    op_sep(0x20);

    /* the camera down to y $B8 (or cue $C4) */
    for (;;) {
        wait_frames(0x80A320u, 0x01);
        cpu.x = 0x00B8;
        cmp16_of(0x00B8, rd16(dba((uint16_t)SCROLL_Y)));
        if (rd16(dba((uint16_t)SCROLL_Y)) == 0x00B8)
            break;
        v = rd8(dba(CUE_C4));
        a8_load(v);
        if (v)
            break;
    }
    cpu.x = 0x00B8;
    nz16_of(0x00B8);
    wr16(dba((uint16_t)SCROLL_Y), 0x00B8);
    a8_load(0x04);
    wr8(dba(V_083D), 0x04);
    (void)inc8_at(dba(V_1A10));
    (void)inc8_at(dba(V_1A12));

    /* and across to x $0E00 */
    do {
        wait_frames(0x80A344u, 0x01);
        x = rd16(dba((uint16_t)SCROLL_X));
        cpu.x = x;
        cmp16_of(x, 0x0E00);
    } while (x < 0x0E00);
    op_sep(0x10);
    a8_load(0x30);
    abi_SongPlay();

    /* on cue $BF: the second song, the windows and colour maths, two tasks */
    wait_cue(0x80A35Au, CUE_BF);
    (void)inc8_at(dba(CUE_C1));
    a8_load(0x2F);
    abi_SongPlay();
    wait_frames(0x80A36Eu, 0x10);
    a8_load(0x17);
    wr8(dba(TMW_SHADOW), 0x17);
    wr8(dba(TSW_SHADOW), 0x17);
    wr8(dba(CGADSUB_SHADOW), 0x17);
    a8_load(0x20);
    wr8(dba(CGWSEL_SHADOW), 0x20);
    a8_load(0xFF);
    wr8(dba(WINDOW_03A6 + 4), 0xFF);
    a8_load(0x0F);
    wr8(dba(WINDOW_03A6 + 5), 0x0F);
    wr8(dba(WOBJSEL_SHADOW), 0x0F);
    a8_load(0x15);
    wr8(dba(WINDOW_03A6 + 7), 0x15);
    a8_load(0x01);
    wr8(dba(WINDOW_03A6 + 8), 0x01);
    a8_load(0x00);
    wr8(dba(WINDOW_03A6), 0x00);
    wr8(dba(WINDOW_03A6 + 2), 0x00);
    a8_load(0xFF);
    wr8(dba(WINDOW_03A6 + 1), 0xFF);
    wr8(dba(WINDOW_03A6 + 3), 0xFF);
    jsl_frame(0x80A3A8u); abi_TaskCreate(); cpu.unwound = 0;
    jsl_frame(0x80A3B1u); abi_TaskCreate(); cpu.unwound = 0;

    /* on cue $BD: three actors placed, and the map rebuilt */
    wait_cue(0x80A3BCu, CUE_BD);
    op_rep(0x20);
    a16_load(0x0E80);
    spawn(0x0E80, 0x0118, 0x20, 0x2E, 0x03);
    a16_load(0x0DD0);
    spawn(0x0DD0, 0x0228, 0x30, 0x70, 0x01);
    a16_load(0x0DB0);
    spawn(0x0DB0, 0x0220, 0x30, 0x93, 0x00);
    a16_load(0x0240);
    dp16_set(FROM_Y, 0x0240);
    a16_load(0x01E0);
    dp16_set(TO_Y, 0x01E0);
    a16_load(0x0007);
    dp16_set(ROWS, 0x0007);
    do {
        a16_load(0x0A10);
        dp16_set(FROM_X, 0x0A10);
        a16_load(0x0D60);
        dp16_set(TO_X, 0x0D60);
        a16_load(0x0008);
        dp16_set(COLUMNS, 0x0008);
        do {
            cpu.x = dp16(FROM_X);
            cpu.y = dp16(FROM_Y);
            nz16_of(cpu.y);
            abi_MapTileAt();
            cpu.x = dp16(TO_X);
            cpu.y = dp16(TO_Y);
            nz16_of(cpu.y);
            abi_MapTileSet();
            flag_set(P_C, 0);
            cpu.a = op_adc16(dp16(FROM_X), 0x0010, &cpu.p);
            dp16_set(FROM_X, cpu.a);
            flag_set(P_C, 0);
            cpu.a = op_adc16(dp16(TO_X), 0x0010, &cpu.p);
            dp16_set(TO_X, cpu.a);
            dp16_set(COLUMNS, (uint16_t)(dp16(COLUMNS) - 1));
            nz16_of(dp16(COLUMNS));
        } while (dp16(COLUMNS));
        flag_set(P_C, 0);
        cpu.a = op_adc16(dp16(FROM_Y), 0x0010, &cpu.p);
        dp16_set(FROM_Y, cpu.a);
        flag_set(P_C, 0);
        cpu.a = op_adc16(dp16(TO_Y), 0x0010, &cpu.p);
        dp16_set(TO_Y, cpu.a);
        dp16_set(ROWS, (uint16_t)(dp16(ROWS) - 1));
        nz16_of(dp16(ROWS));
    } while (dp16(ROWS));
    op_sep(0x30);

    wait_frames(0x80A463u, 0x80);
    a8_load(0x13);
    wr8(dba(SUB_LAYERS_0399), 0x13);
    wr8(dba(SUB_LAYERS_039A), 0x13);
    a8_load(0x08);
    wr8(dba(V_083D), 0x08);
    wait_frames(0x80A476u, 0x1C);
    wr8(dba(V_083D), 0);
    wait_frames(0x80A47Fu, 0xB4);
    (void)inc8_at(dba(CUE_C0));
    wr8(dba(V_0087), 0);
    wait_frames(0x80A48Bu, 0x64);

    /* the screen reddens: the fixed colour's red from 3 up, every 3 frames */
    a8_load(0x23);
    wr8(dba(CGADSUB_SHADOW), 0x23);
    wr8(dba(CGWSEL_SHADOW), 0);
    wr8(dba(COLDATA_GREEN), 0);
    wr8(dba(COLDATA_BLUE), 0);
    v = 0x03;
    a8_load(v);
    do {
        wr8(dba(COLDATA_RED), v);
        wait_frames(0x80A4A4u, 0x03);
        v = (uint8_t)((rd8(dba(COLDATA_RED)) + 1) & 0x1F);
        a8_load(v);
    } while (v);

    /* the level ends */
    wr8(dba(V_0360), 0);
    wr8(dba(V_035D), 0);
    (void)inc8_at(dba((uint16_t)LEVEL_035C));
    wr8(dba((uint16_t)(PLAYER + ACTOR_FACING)), 0);
    wr8(dba((uint16_t)(PLAYER + ACTOR_FLIP)), 0);
    op_rep(0x20);
    a16_load(0xE7B1);
    wr16(dba((uint16_t)(PLAYER + ACTOR_03)), 0xE7B1);
    op_sep(0x20);
    a8_load(0x80);
    tsb_at(dba(V_0347), 0x80);
    (void)inc8_at(dba(V_1423));
    (void)inc8_at(dba(V_1454));
    abi_TaskExit();
}
