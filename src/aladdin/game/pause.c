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

