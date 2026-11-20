/*
 * unconverted.c: placeholders for the routines not converted yet that
 * system/ and the boot screens call (unconverted.h, and the owners' headers
 * that declare them: video/video.h, actor/actor.h). Each is a weak
 * definition: the owner's own definition replaces it at link time, with no
 * edit here. Each prints its name once to stderr (the report counts these
 * lines) and does nothing; one that answers gives "none" (no actor, no
 * button); a task entry ends its task at once, so a screen that waits for it
 * goes on; a routine on a task's chain goes on.
 */
#include <stdio.h>
#include <string.h>

#include "system/unconverted.h"

#define PLACEHOLDER __attribute__((weak))

void unconverted(const char *name)
{
    enum { MAX_NAMES = 128 };
    static const char *said[MAX_NAMES];
    static unsigned count;
    unsigned i;

    for (i = 0; i < count; i++)
        if (!strcmp(said[i], name))
            return;
    if (count < MAX_NAMES)
        said[count++] = name;
    fprintf(stderr, "unconverted: %s\n", name);
}

PLACEHOLDER void PaletteSave(uint8_t first, uint8_t count)
{
    unconverted("PaletteSave");
    (void)first;
    (void)count;
}

PLACEHOLDER void PaletteFadeStep(int8_t step, uint8_t count)
{
    unconverted("PaletteFadeStep");
    (void)step;
    (void)count;
}

PLACEHOLDER void PaletteCycleStart(uint8_t slot, uint8_t cycle)
{
    unconverted("PaletteCycleStart");
    (void)slot;
    (void)cycle;
}

PLACEHOLDER void LayersFill(void)
{
    unconverted("LayersFill");
}

PLACEHOLDER TaskRun ColourFadeInTask(Task *t)
{
    unconverted("ColourFadeInTask");
    return task_exit(t);
}

PLACEHOLDER TaskRun ColourFadeOutTask(Task *t)
{
    unconverted("ColourFadeOutTask");
    return task_exit(t);
}

PLACEHOLDER TaskRun PaletteSequenceTask(Task *t)
{
    unconverted("PaletteSequenceTask");
    return task_exit(t);
}

PLACEHOLDER TaskRun BrightnessUpTask(Task *t)
{
    unconverted("BrightnessUpTask");
    return task_exit(t);
}

PLACEHOLDER TaskRun BrightnessDownTask(Task *t)
{
    unconverted("BrightnessDownTask");
    return task_exit(t);
}

PLACEHOLDER TaskRun Mode7ZoomInTask(Task *t)
{
    unconverted("Mode7ZoomInTask");
    return task_exit(t);
}

PLACEHOLDER TaskRun WindowEffectTask(Task *t)
{
    unconverted("WindowEffectTask");
    return task_exit(t);
}

PLACEHOLDER void SoundSetsLoad(void)
{
    unconverted("SoundSetsLoad");
}

PLACEHOLDER void SongPlay(uint8_t song)
{
    unconverted("SongPlay");
    (void)song;
}

PLACEHOLDER void SoundQueueAdd(uint8_t command)
{
    unconverted("SoundQueueAdd");
    (void)command;
}

PLACEHOLDER void SoundQueueF6(uint8_t param)
{
    unconverted("SoundQueueF6");
    (void)param;
}

PLACEHOLDER void SoundQueueF6With1(void)
{
    unconverted("SoundQueueF6With1");
}

PLACEHOLDER void SoundCommandSend(void)
{
    unconverted("SoundCommandSend");
}

PLACEHOLDER void FrameStep(void)
{
    unconverted("FrameStep");
}

PLACEHOLDER void LevelRamSetup(void)
{
    unconverted("LevelRamSetup");
}

PLACEHOLDER TaskRun TextCardScreen(Task *t)
{
    unconverted("TextCardScreen");
    (void)t;
    return TASK_GOES_ON;
}

PLACEHOLDER void RegionCheck(void)
{
    unconverted("RegionCheck");
}

PLACEHOLDER void LevelMapBuild(void)
{
    unconverted("LevelMapBuild");
}

PLACEHOLDER TaskRun StoryTextTask(Task *t)
{
    unconverted("StoryTextTask");
    return task_exit(t);
}

PLACEHOLDER void TitleMenuCursor(void)
{
    unconverted("TitleMenuCursor");
}

PLACEHOLDER void MenuMarkMove(void)
{
    unconverted("MenuMarkMove");
}

PLACEHOLDER int PadConfirmPressed(void)
{
    unconverted("PadConfirmPressed");
    return 0;
}

PLACEHOLDER void LevelNumberText(void)
{
    unconverted("LevelNumberText");
}

PLACEHOLDER int LevelSelect(void)
{
    unconverted("LevelSelect");
    return 0;
}

PLACEHOLDER TaskRun OptionsTask(Task *t)
{
    unconverted("OptionsTask");
    return task_exit(t);
}

PLACEHOLDER TaskRun PasswordScreen(Task *t)
{
    unconverted("PasswordScreen");
    return task_exit(t);
}

PLACEHOLDER TaskRun GameStartTask(Task *t)
{
    unconverted("GameStartTask");
    return task_exit(t);
}

PLACEHOLDER TaskRun DemoTask(Task *t)
{
    unconverted("DemoTask");
    return task_exit(t);
}

PLACEHOLDER void video_init(void)
{
    unconverted("video_init");
}

PLACEHOLDER void video_vblank(void)
{
    unconverted("video_vblank");
}

PLACEHOLDER void video_draw(void)
{
    unconverted("video_draw");
}

PLACEHOLDER void vram_write(uint16_t address, const void *data, uint32_t count, VramWrite how)
{
    unconverted("vram_write");
    (void)address;
    (void)data;
    (void)count;
    (void)how;
}

PLACEHOLDER void vram_fill(uint16_t address, uint16_t word, uint32_t count)
{
    unconverted("vram_fill");
    (void)address;
    (void)word;
    (void)count;
}

PLACEHOLDER void ScrollStateClear(void)
{
    unconverted("ScrollStateClear");
}

PLACEHOLDER void DisplayDefaults(void)
{
    unconverted("DisplayDefaults");
}

PLACEHOLDER void VramCopy(int entry)
{
    unconverted("VramCopy");
    (void)entry;
}

PLACEHOLDER void VramCopy2(int entry)
{
    unconverted("VramCopy2");
    (void)entry;
}

PLACEHOLDER void UnpackSet(int set)
{
    unconverted("UnpackSet");
    (void)set;
}

PLACEHOLDER void LevelGraphicsLoad(void)
{
    unconverted("LevelGraphicsLoad");
}

PLACEHOLDER void Mode7ScreenSetup(uint8_t row_flag)
{
    unconverted("Mode7ScreenSetup");
    (void)row_flag;
}

PLACEHOLDER void PaletteLoad(int set)
{
    unconverted("PaletteLoad");
    (void)set;
}

PLACEHOLDER void OamBufferClear(void)
{
    unconverted("OamBufferClear");
}

PLACEHOLDER void OamBuildBegin(void)
{
    unconverted("OamBuildBegin");
}

PLACEHOLDER void OamDrawActors(void)
{
    unconverted("OamDrawActors");
}

PLACEHOLDER void TextLayerClear(void)
{
    unconverted("TextLayerClear");
}

PLACEHOLDER void TextLayoutShow(int layout)
{
    unconverted("TextLayoutShow");
    (void)layout;
}

PLACEHOLDER void ActorsFreeAll(void)
{
    unconverted("ActorsFreeAll");
}

PLACEHOLDER Actor *ActorCreate(uint8_t kind)
{
    unconverted("ActorCreate");
    (void)kind;
    return NULL;
}

PLACEHOLDER Actor *ActorSpawnAt(uint8_t kind, uint8_t byte07, uint16_t x, uint16_t y)
{
    unconverted("ActorSpawnAt");
    (void)kind;
    (void)byte07;
    (void)x;
    (void)y;
    return NULL;
}
