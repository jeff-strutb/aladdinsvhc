/*
 * unconverted.h: the routines system/ and the boot screens call that are
 * not converted yet. Those an owner's header already declares (video.h,
 * actor.h) are declared there; the rest are declared here with the
 * parameters the calls give them, as the interface asked of each owner
 * (plan, "Cross-area calls"), until the owner's header has them.
 * unconverted.c holds a placeholder for each that prints its name once to
 * stderr; the owner's definition replaces it at link time. When nothing is
 * left here or there, both files go.
 *
 * Numbers are the record's own: a set, a song, a layout, a table's entry.
 */
#ifndef ALADDIN_UNCONVERTED_H
#define ALADDIN_UNCONVERTED_H

#include <stdint.h>

#include "actor/actor.h"
#include "system/task.h"
#include "video/video.h"

/* ---- video/ (native-video): not in video.h yet ---- */
void PaletteSave(uint8_t first, uint8_t count); /* 81:9DAA: palettes first.. kept to fade from */
void PaletteFadeStep(int8_t step, uint8_t count); /* 81:9DCD */
void PaletteCycleStart(uint8_t slot, uint8_t cycle); /* 81:EC8A PaletteCycleStartLong: the record's X / 14, A */
void LayersFill(void);                      /* 81:C05C LayersFillLong */
TaskRun ColourFadeInTask(Task *t);          /* 81:9F0E */
TaskRun ColourFadeOutTask(Task *t);         /* 81:9F52 */
TaskRun PaletteSequenceTask(Task *t);       /* 81:9E75 */
TaskRun BrightnessUpTask(Task *t);          /* 81:9E8E */
TaskRun BrightnessDownTask(Task *t);        /* 81:9EB9 */
TaskRun Mode7ZoomInTask(Task *t);           /* 81:B157 */
TaskRun WindowEffectTask(Task *t);          /* 82:8000 */

/* ---- audio/ (native-sound) ---- */
void SoundSetsLoad(void);                   /* 80:B4AA */
void SongPlay(uint8_t song);                /* 81:FA23 */
void SoundQueueAdd(uint8_t command);        /* 81:FA00 */
void SoundQueueF6(uint8_t param);           /* 81:F9D4 */
void SoundQueueF6With1(void);               /* 81:F9D2 */
void SoundCommandSend(void);                /* the NMI's sound send (80:8323), once a frame */

/* ---- game/, map/, level/, input/: this session's, not yet converted ---- */
void FrameStep(void);                       /* 81:830D FrameStepLong (game/frame_step.c) */
void LevelRamSetup(void);                   /* 81:825B LevelRamSetupLong, for g.level (game/level_load.c) */
TaskRun TextCardScreen(Task *t);            /* 81:ED33 (game/level_load.c) */
void RegionCheck(void);                     /* 84:AFAE (game/password_screen.c) */
void LevelMapBuild(void);                   /* 81:BEEB LevelMapBuildLong (map/level_map.c) */
TaskRun StoryTextTask(Task *t);             /* 81:F332 (game/story_text.c) */
void TitleMenuCursor(void);                 /* 81:EDA2 TitleMenuCursorLong (game/menu_mark.c) */
void MenuMarkMove(void);                    /* 81:EE04 (game/menu_mark_draw.c) */
int PadConfirmPressed(void);                /* 80:A04F: A, X, B, Y or START newly pressed (input/pad_buttons.c) */

/* ---- past the title (stage 5) ---- */
void LevelNumberText(void);                 /* 80:A0B1 */
int LevelSelect(void);                      /* 80:A05C: 1 when it changed the level (the record's C clear) */
TaskRun OptionsTask(Task *t);               /* 83:8979 */
TaskRun PasswordScreen(Task *t);            /* 84:AD50 */
TaskRun GameStartTask(Task *t);             /* 81:8095 */
TaskRun DemoTask(Task *t);                  /* 81:8031 */

/* A placeholder's report: `name` printed once to stderr. */
void unconverted(const char *name);

#endif
