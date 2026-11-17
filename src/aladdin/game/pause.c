/*
 * pause.c: the pause: Start during play freezes every task but the one
 * that runs it, shows PAUSED and pauses the music, until Start again.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "wram_state.h"
#include "wram_video.h"

void abi_SoundQueueAdd(void);
void abi_TaskWait(void);
void abi_TextLayoutShow(void);

#define PAUSE_COOLDOWN 0x000Bu    /* frames before Start can pause again */
#define V_0382 0x0382u            /* a pad byte: bit 4 (Start) pauses and unpauses */
#define PAD_START 0x10
#define PAUSE_BLOCK 0x0404u       /* nonzero: no pause (with PLAY_MODE and V_03FA) */
#define PAUSED 0x0405u            /* counted up while paused */
#define V_03F9 0x03F9u            /* 1 while paused */
#define V_03FA 0x03FAu
#define TEXT_PAUSED 0x16          /* text layouts: PAUSED; with bit 7, blanked */
#define LEVEL_NO_PAUSE_SOUND 0x12 /* the level that pauses without sound $0E */
#define SOUND_PAUSE 0x0E
#define SOUND_MUSIC_PAUSE 0xF3
#define SOUND_MUSIC_GO_ON 0xF4
#define TASK_SLOT 0x18            /* bytes a task slot */

/*
 * @routine 81:8398 PauseCheck
 * WHAT IT DOES: Pauses the game when Start is pressed during play (full
 * brightness, no demo, nothing blocking): every other task frozen, PAUSED
 * shown and the music paused, until Start again; then a short cooldown.
 * HOW: PAUSE_COOLDOWN nonzero, the screen not at full brightness,
 * PAUSE_BLOCK, PLAY_MODE or V_03FA nonzero, or V_0382 bit 4 clear: the
 * cooldown counted down (when nonzero) and back (the code at 81:838E).
 * Else: TEXT_UPLOAD_SIZE $0380, PAUSED up, PAUSED shown, V_03F9 = 1,
 * sound $0E (not on level $12) and $F3; task slots 7 down to 1 have their
 * state pushed and set to 2; each frame (TaskWait 1) until V_0382 bit 4;
 * PAUSED blanked, V_03F9 and PAUSED cleared, cooldown $10, sound $F4; the
 * states pulled back into slots 1 up to 7; one frame more; TEXT_UPLOAD_SIZE
 * $0160.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; called
 * by JSR from the play loop's task (81:82DB), whose stack holds the states
 * while it waits. Exit: M and X 8-bit; not paused: A low = the cooldown, N
 * and Z from it; paused: A = $0160 (M 8-bit), N and Z clear (its load),
 * X = $C0, C as the last TaskWait's resume left it.
 */
void abi_PauseCheck(void)
{
    uint8_t v;

    a8_load(rd8(dba(PAUSE_COOLDOWN)));
    if ((uint8_t)cpu.a)
        goto cooldown;
    v = (uint8_t)(rd8(dba((uint16_t)INIDISP_SHADOW)) & 0x0F);
    a8_load(v);
    cmp8_of(v, 0x0F);
    if (v != 0x0F)
        goto cooldown;
    v = (uint8_t)(rd8(dba(PAUSE_BLOCK)) | rd8(dba((uint16_t)PLAY_MODE)) | rd8(dba(V_03FA)));
    a8_load(v);
    if (v)
        goto cooldown;
    v = rd8(dba(V_0382));
    a8_load(v);
    flag_set(P_Z, !(v & PAD_START));
    if (!(v & PAD_START))
        goto cooldown;

    op_rep(P_M);
    a16_load(0x0380);
    wr16(dba((uint16_t)TEXT_UPLOAD_SIZE), 0x0380);
    op_sep(P_M);
    inc8_at(dba(PAUSED));
    a8_load(TEXT_PAUSED);
    abi_TextLayoutShow();
    a8_load(0x01);
    wr8(dba(V_03F9), 0x01);
    v = rd8(dba((uint16_t)LEVEL));
    a8_load(v);
    cmp8_of(v, LEVEL_NO_PAUSE_SOUND);
    if (v != LEVEL_NO_PAUSE_SOUND) {
        a8_load(SOUND_PAUSE);
        abi_SoundQueueAdd();
    }
    a8_load(SOUND_MUSIC_PAUSE);
    abi_SoundQueueAdd();
    cpu.x = 7 * TASK_SLOT;
    do {                                  /* slots 7 down to 1: kept on the stack, frozen */
        a8_load(rd8(dba((uint16_t)(TASK_STATE + cpu.x))));
        pha8();
        a8_load(0x02);
        wr8(dba((uint16_t)(TASK_STATE + cpu.x)), 0x02);
        flag_set(P_C, 1);
        a8_load((uint8_t)cpu.x);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | op_sbc8((uint8_t)cpu.x, TASK_SLOT, &cpu.p));
        x8_load((uint8_t)cpu.a);
    } while (cpu.x);
    do {
        a8_load(0x01);
        jsl_frame(0x8183F7u);
        abi_TaskWait();
        v = rd8(dba(V_0382));
        a8_load(v);
        flag_set(P_Z, !(v & PAD_START));
    } while (!(v & PAD_START));
    a8_load(TEXT_PAUSED | 0x80);
    abi_TextLayoutShow();
    wr8(dba(V_03F9), 0);
    wr8(dba(PAUSED), 0);
    a8_load(0x10);
    wr8(dba(PAUSE_COOLDOWN), 0x10);
    a8_load(SOUND_MUSIC_GO_ON);
    abi_SoundQueueAdd();
    cpu.x = TASK_SLOT;
    do {                                  /* slots 1 up to 7: their states back */
        pla8();
        wr8(dba((uint16_t)(TASK_STATE + cpu.x)), (uint8_t)cpu.a);
        flag_set(P_C, 0);
        a8_load((uint8_t)cpu.x);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | op_adc8((uint8_t)cpu.x, TASK_SLOT, &cpu.p));
        cpu.x = (uint8_t)cpu.a;
        cmp8_of((uint8_t)cpu.x, 8 * TASK_SLOT);
    } while (cpu.x != 8 * TASK_SLOT);
    a8_load(0x01);
    jsl_frame(0x81842Au);
    abi_TaskWait();
    op_rep(P_M);
    a16_load(0x0160);
    wr16(dba((uint16_t)TEXT_UPLOAD_SIZE), 0x0160);
    op_sep(P_M);
    return;

cooldown:
    v = rd8(dba(PAUSE_COOLDOWN));
    a8_load(v);
    if (v) {
        a8_load((uint8_t)(v - 1));
        wr8(dba(PAUSE_COOLDOWN), (uint8_t)(v - 1));
    }
}
