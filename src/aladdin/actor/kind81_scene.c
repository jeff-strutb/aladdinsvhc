/*
 * kind81_scene.c: actor kind $81, which runs its stage's closing scene
 * once Aladdin stands on firm ground at x $1E0 or more.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "wram.h"
#include "wram_video.h"
#include "actor.h"

void abi_PlayerOnFirmGround(void);
void abi_SoundQueueF6With1(void);
void abi_ChainBuildNine(void);
void abi_TaskCreate(void);
void abi_SoundQueueAdd(void);
void abi_SongPlay(void);
void abi_ActorEndPlaced(void);

#define PLAYER_X_AT (PLAYER + ACTOR_X + 1)  /* the player's x, whole pixels (word) */
#define PLAYER_CODE_AT (PLAYER + ACTOR_03)  /* where the player's code goes on (word) */
#define SCENE_ON 0x03FAu        /* counted up while the scene runs; 0 again once the chain is built */
#define V_1A10 0x1A10u          /* counted up (word) as the scene starts */
#define V_1A05 0x1A05u          /* counted up (word) as the scene starts */
#define VIEW_X 0x1773u          /* the view's left edge (pixels, word) */
#define V_1787 0x1787u          /* cleared once the view is in place */
#define V_18DD 0x18DDu          /* cleared once the view is in place */
#define CUE_1B1F 0x1B1Fu        /* the scene goes on once this is nonzero */
#define V_03A0 0x03A0u          /* $41 as the chain is built */
#define V_0399 0x0399u          /* $15 after the chain, and again after the flash */
#define V_0087 0x0087u          /* the scene waits for this to be 0 */
#define V_083F 0x083Fu          /* $12 as the flash begins */
#define V_03A1 0x03A1u          /* $49 at the flash's height */
#define V_7FF047 0x7FF047u      /* $0C as the scene ends */
#define SCENE_DONE 0x1B18u      /* counted up as the scene ends */
#define PLAYER_SCENE_CODE 0xE126u  /* the player's code for the scene */

/* TaskCreate at `site`, its five argument bytes after the JSL: its frame
   as the JSL pushes it; TaskCreate returns past them by its own RTL and
   the C goes on. */
static void task_create(uint32_t site)
{
    jsl_frame(site);
    abi_TaskCreate();
    cpu.unwound = 0;
}

/* The fixed colour, all three parts, one step up or down. */
static uint8_t colour_step(int up)
{
    uint8_t r;

    if (up) {
        (void)inc8_at(dba((uint16_t)COLDATA_RED));
        (void)inc8_at(dba((uint16_t)COLDATA_GREEN));
        (void)inc8_at(dba((uint16_t)COLDATA_BLUE));
    } else {
        (void)dec8_at(dba((uint16_t)COLDATA_RED));
        (void)dec8_at(dba((uint16_t)COLDATA_GREEN));
        (void)dec8_at(dba((uint16_t)COLDATA_BLUE));
    }
    r = rd8(dba((uint16_t)COLDATA_RED));
    a8_load(r);
    return r;
}

/*
 * @routine 84:9391 Kind81Scene
 * @actor
 * WHAT IT DOES: Actor kind $81: waits until Aladdin stands on firm ground
 * at x $1E0 or more, then takes over: his code goes to the scene's, the
 * view scrolls right to $200, and once CUE_1B1F comes the chain of nine
 * is built, the screen shakes, song $22 plays, the screen flashes white
 * and fades back, and the actor ends.
 * HOW: Each frame (BRK first) until PlayerOnFirmGround (C set) with his x
 * $1E0 or more. SCENE_ON up, SoundQueueF6With1, his code $E126, V_1A10 and
 * V_1A05 up. Each frame VIEW_X up by 3 until past $200, then $200;
 * V_1787 and V_18DD cleared. Each frame until CUE_1B1F: SCENE_ON 0,
 * ChainBuildNine, V_03A0 = $41, a yield, V_0399 = $15, COP $3F, TaskCreate
 * (slot 1, ShakeTask), sound $4A; each frame until V_0087 is 0; COP $3F;
 * SongPlay $22; V_083F = $12; the fixed colour black, colour math off,
 * WOBJSEL $80, CGADSUB 2; each frame the colour up by one to $1F; sound
 * $46, V_03A1 = $49, VRAM upload $0A, a yield, upload $0B; COP 8; HDMAEN
 * $0E, V_0399 = $15; each frame the colour down by one to 0; CGWSEL $30,
 * V_7FF047 = $0C, SCENE_DONE up; ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $81
 * in state $0C (the table at $83:8DAB, through the stub at $83:92C2).
 * Exit: never returns; ends in ActorEndPlaced.
 */
void abi_Kind81Scene(void)
{
    uint16_t x, view;
    uint8_t v;

    for (;;) {
        actor_yield(0x849391u);
        jsl_frame(0x849393u);
        abi_PlayerOnFirmGround();
        cpu.s = (uint16_t)(cpu.s + 3);
        if (!(cpu.p & P_C))
            continue;
        op_rep(0x20);
        x = rd16(dba((uint16_t)PLAYER_X_AT));
        a16_load(x);
        cmp16_of(x, 0x01E0);
        op_sep(0x20);
        if (x >= 0x01E0)
            break;
    }
    (void)inc8_at(dba(SCENE_ON));
    jsl_frame(0x8493A8u);
    abi_SoundQueueF6With1();
    cpu.s = (uint16_t)(cpu.s + 3);
    op_rep(0x20);
    a16_load(PLAYER_SCENE_CODE);
    wr16(dba((uint16_t)PLAYER_CODE_AT), PLAYER_SCENE_CODE);
    (void)inc16_at(dba(V_1A10));
    (void)inc16_at(dba(V_1A05));
    /* the view scrolls right to $200 */
    do {
        actor_yield(0x8493BAu);
        flag_set(P_C, 0);
        view = op_adc16(rd16(dba(VIEW_X)), 0x0003, &cpu.p);
        wr16(dba(VIEW_X), view);
        a16_load(0x0200);
        cmp16_of(0x0200, view);
    } while (0x0200 >= view);
    wr16(dba(VIEW_X), 0x0200);
    op_sep(0x20);
    wr8(dba(V_1787), 0);
    wr8(dba(V_18DD), 0);
    do {
        actor_yield(0x8493D9u);
        v = rd8(dba(CUE_1B1F));
        a8_load(v);
    } while (!v);
    wr8(dba(SCENE_ON), 0);
    call_frame(0x93E5, abi_ChainBuildNine);
    a8_load(0x41);
    wr8(dba(V_03A0), 0x41);
    actor_yield(0x8493EBu);
    a8_load(0x15);
    wr8(dba(V_0399), 0x15);
    a8_load(0x3F);
    actor_wait(0x8493F4u);
    task_create(0x8493F6u);                      /* ShakeTask, slot 1 */
    a8_load(0x4A);
    abi_SoundQueueAdd();
    do {
        actor_yield(0x849405u);
        v = rd8(dba(V_0087));
        a8_load(v);
    } while (v);
    a8_load(0x3F);
    actor_wait(0x84940Eu);
    a8_load(0x22);
    jsl_frame(0x849412u);
    abi_SongPlay();
    cpu.s = (uint16_t)(cpu.s + 3);
    a8_load(0x12);
    wr8(dba(V_083F), 0x12);
    /* the flash: the fixed colour added, up from black to white */
    wr8(dba((uint16_t)COLDATA_RED), 0);
    wr8(dba((uint16_t)COLDATA_GREEN), 0);
    wr8(dba((uint16_t)COLDATA_BLUE), 0);
    wr8(dba((uint16_t)CGWSEL_SHADOW), 0);
    a8_load(0x80);
    wr8(dba((uint16_t)WOBJSEL_SHADOW), 0x80);
    a8_load(0x02);
    wr8(dba((uint16_t)CGADSUB_SHADOW), 0x02);
    do {
        a8_load(0x01);
        actor_wait(0x849433u);
        v = colour_step(1);
        cmp8_of(v, 0x1F);
    } while (v != 0x1F);
    a8_load(0x46);
    abi_SoundQueueAdd();
    a8_load(0x49);
    wr8(dba(V_03A1), 0x49);
    a8_load(0x0A);
    wr8(dba((uint16_t)VRAM_UPLOAD_NUMBER), 0x0A);
    actor_yield(0x849455u);
    a8_load(0x0B);
    wr8(dba((uint16_t)VRAM_UPLOAD_NUMBER), 0x0B);
    a8_load(0x08);
    actor_wait(0x84945Eu);
    a8_load(0x0E);
    wr8(dba((uint16_t)HDMAEN_SHADOW), 0x0E);
    a8_load(0x15);
    wr8(dba(V_0399), 0x15);
    /* and back down to black */
    do {
        a8_load(0x01);
        actor_wait(0x84946Cu);
        v = colour_step(0);
    } while (v);
    a8_load(0x30);
    wr8(dba((uint16_t)CGWSEL_SHADOW), 0x30);
    a8_load(0x0C);
    wr8(V_7FF047, 0x0C);
    (void)inc8_at(dba(SCENE_DONE));
    abi_ActorEndPlaced();
}
