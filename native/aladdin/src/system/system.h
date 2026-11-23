/*
 * system.h: power-on, the frame's NMI work, the pads, the display switches,
 * random numbers and the clears other areas call (the record's system/).
 */
#ifndef ALADDIN_SYSTEM_H
#define ALADDIN_SYSTEM_H

#include <stdint.h>

#include "nshvc/nshvc.h"
#include "system/task.h"

/* SNES pad bits, as the game keeps a pad's buttons (g.pad_held) */
enum {
    PAD_R = 1u << 4, PAD_L = 1u << 5, PAD_X = 1u << 6, PAD_A = 1u << 7,
    PAD_RIGHT = 1u << 8, PAD_LEFT = 1u << 9, PAD_DOWN = 1u << 10, PAD_UP = 1u << 11,
    PAD_START = 1u << 12, PAD_SELECT = 1u << 13, PAD_Y = 1u << 14, PAD_B = 1u << 15
};

/* power_on.c */
void PowerOn(void);                     /* 80:8000: at init */

/* nmi.c */
void NmiHandler(void);                  /* 80:8111: the NMI's game work, before the tasks' turns */
void NmiPictureWork(void);              /* its picture work (video_vblank), after them */

/* display.c */
void NmiEnable(void);                   /* 81:969F */
void NmiDisable(void);                  /* 81:96A8 */
void ScreenBlank(void);                 /* 81:96B1 */
void ScreenFullBright(void);            /* 81:96C5 */

/* input.c */
void InputRead(const nshvc_input *input);   /* 81:BDB1 */
void JoypadRead(const nshvc_input *input);  /* 81:BDCC */
void ControlsRead(void);                /* 81:BDE9 */

/* frame.c: a task's frame ends; each waits */
TaskRun Wait64Frames(Task *t);          /* 80:AEB9 */
TaskRun FrameEnd(Task *t);              /* 80:AE9A */
TaskRun FrameEndTimes(Task *t, unsigned frames);    /* 80:B2D3: frames 1-256 (the record's X, 0 for 256) */

/* random.c */
uint8_t RandomNext(void);               /* 81:981B: the new high byte */

/* options.c */
uint8_t KeptOptionsSum(void);           /* 80:80FE */
void KeptOptionsSeal(void);             /* 80:810A */

/* reset.c */
void LevelCountersClear(void);          /* 81:9647 */
void GameRamClear(void);                /* 81:9655 */
void Table15F1Clear(void);              /* 81:968F */
void LevelTablesClear(void);            /* 81:967B */

/* dev_start.c: a developer's start at a screen, by the environment (ALADDIN_DEV_*; docs/aladdin/native/CHECK.md) */
void DevStartApply(void);               /* read the environment once: the cheat, the chosen screen's state */
bool DevBotInput(uint64_t u, nshvc_input *in);   /* a developer's player: the input for this update, false the engine's own (dev_bot.c) */
bool DevLevelInput(uint64_t u, nshvc_input *in); /* ALADDIN_DEV_LEVEL=PART-STAGE: the level select's keys until update 3903, false the engine's own (dev_level.c) */
void DevMapDumpLevel(uint64_t u);       /* ALADDIN_DEV_MAPDUMP=DIR: the level's whole map as pictures, at update 1, then the run ends (dev_mapdump.c) */
int DevMapDumpHeld(void);               /* the dump holds the loaded level on the screen (ALADDIN_DEV_MAPDUMP_HOLD) */
extern uint16_t *DevVramShadow;         /* non-null while the dump is on: video.c copies each video memory write here */
void DevMapDump(uint64_t u);           /* the level's map as text by the environment (dev_bot.c) */
void DevDump(uint64_t u);              /* the whole state as bytes at an update by the environment (dev_bot.c) */
void DevSnapshotSave(uint64_t u);       /* ALADDIN_DEV_SNAP_SAVE=FILE@U (dev_snapshot.c) */
int DevSnapshotLoad(uint64_t u);        /* ALADDIN_DEV_SNAP_LOAD=FILE@L: 1 when this update loaded it and runs no step */
void DevShot(uint64_t u);              /* a picture every N updates by the environment (dev_bot.c) */
void DevBotStep(void (*step)(const nshvc_input *));
struct LevelStart;
void DevStartAt(struct LevelStart *s);   /* ALADDIN_DEV_AT=x,y: where a level's play starts (dev_start.c) */
TaskEntry DevStartEntry(void);          /* the chosen screen's task entry, or TASK_ENTRY_NONE */

#endif
