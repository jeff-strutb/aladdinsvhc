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

#include "game/game.h"
#include "level/level.h"
#include "map/map.h"
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

PLACEHOLDER TaskRun WindowEffectTask(Task *t)
{
    unconverted("WindowEffectTask");
    return task_exit(t);
}

PLACEHOLDER uint16_t TilemapWord(uint16_t x, uint16_t y)
{
    unconverted("TilemapWord");
    (void)x;
    (void)y;
    return 0;
}

PLACEHOLDER void BlockSourceAdd(uint16_t source)
{
    unconverted("BlockSourceAdd");
    (void)source;
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


PLACEHOLDER void ScrollUpdate(void)
{
    unconverted("ScrollUpdate");
}

PLACEHOLDER void LayersScroll(void)
{
    unconverted("LayersScroll");
}

PLACEHOLDER void BgFollowLayers(void)
{
    unconverted("BgFollowLayers");
}

PLACEHOLDER void PaletteCyclesStep(void)
{
    unconverted("PaletteCyclesStep");
}

PLACEHOLDER void LevelScrollLimits(void)
{
    unconverted("LevelScrollLimits");
}

PLACEHOLDER void LevelActorGraphicsLoad(void)
{
    unconverted("LevelActorGraphicsLoad");
}

/* ---- game/, level/, map/ (stage 5's levels and screens): declared in their headers ---- */

PLACEHOLDER TaskRun DemoTask(Task *t)
{
    unconverted("DemoTask");
    return task_exit(t);
}

PLACEHOLDER TaskRun GameStartTask(Task *t)
{
    unconverted("GameStartTask");
    return task_exit(t);
}

PLACEHOLDER TaskRun NextPartTask(Task *t)
{
    unconverted("NextPartTask");
    return task_exit(t);
}

PLACEHOLDER int PlayerSceneCode(Actor *a)
{
    unconverted("PlayerSceneCode");
    (void)a;
    return 1;
}

PLACEHOLDER void TileBehaviourLoad(void)
{
    unconverted("TileBehaviourLoad");
}

PLACEHOLDER void TileChangeAdd(uint16_t tile, uint16_t x, uint16_t y)
{
    unconverted("TileChangeAdd");
    (void)tile;
    (void)x;
    (void)y;
}

PLACEHOLDER uint16_t MapQuarterAt(uint16_t x, uint16_t y)
{
    unconverted("MapQuarterAt");
    (void)x;
    (void)y;
    return 0;
}

PLACEHOLDER void ProbeAtActor(const Actor *a, MapPoint *p)
{
    unconverted("ProbeAtActor");
    (void)a;
    (void)p;
}

PLACEHOLDER void ProbeLeft(const Actor *a, MapPoint *p)
{
    unconverted("ProbeLeft");
    (void)a;
    (void)p;
}

PLACEHOLDER void ProbeRight(const Actor *a, MapPoint *p)
{
    unconverted("ProbeRight");
    (void)a;
    (void)p;
}

PLACEHOLDER uint8_t TileAt(const Actor *a, MapPoint *p)
{
    unconverted("TileAt");
    (void)a;
    (void)p;
    return TILE_KIND_EMPTY;
}

PLACEHOLDER void SurfaceHeight(MapPoint *p, uint8_t kind)
{
    unconverted("SurfaceHeight");
    (void)p;
    (void)kind;
}

PLACEHOLDER int PointInGround(const Actor *a, MapPoint *p)
{
    unconverted("PointInGround");
    (void)a;
    (void)p;
    return 0;
}

PLACEHOLDER int ActorInGround(const Actor *a, MapPoint *p)
{
    unconverted("ActorInGround");
    (void)a;
    (void)p;
    return 0;
}

PLACEHOLDER int GroundRight(const Actor *a, MapPoint *p)
{
    unconverted("GroundRight");
    (void)a;
    (void)p;
    return 0;
}

PLACEHOLDER int GroundLeft(const Actor *a, MapPoint *p)
{
    unconverted("GroundLeft");
    (void)a;
    (void)p;
    return 0;
}

PLACEHOLDER int GroundInFront(const Actor *a, MapPoint *p)
{
    unconverted("GroundInFront");
    (void)a;
    (void)p;
    return 0;
}

PLACEHOLDER int GroundBehind(const Actor *a, MapPoint *p)
{
    unconverted("GroundBehind");
    (void)a;
    (void)p;
    return 0;
}

PLACEHOLDER int GroundBehindBit4(Actor *a, MapPoint *p)
{
    unconverted("GroundBehindBit4");
    (void)a;
    (void)p;
    return 0;
}

PLACEHOLDER uint8_t GroundBehindBit3(Actor *a, MapPoint *p)
{
    unconverted("GroundBehindBit3");
    (void)a;
    (void)p;
    return 0;
}

PLACEHOLDER int ActorOutOfWall(Actor *a, uint8_t probe)
{
    unconverted("ActorOutOfWall");
    (void)a;
    (void)probe;
    return 0;
}

PLACEHOLDER int ActorOutOfWallProbe4(Actor *a)
{
    unconverted("ActorOutOfWallProbe4");
    (void)a;
    return 0;
}

PLACEHOLDER int ActorOutOfWalls(Actor *a)
{
    unconverted("ActorOutOfWalls");
    (void)a;
    return 0;
}

PLACEHOLDER int ActorSnapToGround(Actor *a)
{
    unconverted("ActorSnapToGround");
    (void)a;
    return 0;
}

PLACEHOLDER int ActorOutOfGround(Actor *a)
{
    unconverted("ActorOutOfGround");
    (void)a;
    return 0;
}

PLACEHOLDER int ActorSettleOnFloor(Actor *a)
{
    unconverted("ActorSettleOnFloor");
    (void)a;
    return 0;
}

PLACEHOLDER int PlayerOnFirmGround(void)
{
    unconverted("PlayerOnFirmGround");
    return 0;
}

PLACEHOLDER int PlayerOnFirmGroundProbe8(void)
{
    unconverted("PlayerOnFirmGroundProbe8");
    return 0;
}

PLACEHOLDER int PlayerOnFirmGroundProbe4(void)
{
    unconverted("PlayerOnFirmGroundProbe4");
    return 0;
}

PLACEHOLDER void LevelCompanionStart(void)
{
    unconverted("LevelCompanionStart");
}

PLACEHOLDER void LevelSongPlay(void)
{
    unconverted("LevelSongPlay");
}

PLACEHOLDER void RestartPointCheck(void)
{
    unconverted("RestartPointCheck");
}

PLACEHOLDER void LevelSpotsSet(void)
{
    unconverted("LevelSpotsSet");
}

PLACEHOLDER void LevelExitCheck(void)
{
    unconverted("LevelExitCheck");
}

PLACEHOLDER void LevelScreenSetup(void)
{
    unconverted("LevelScreenSetup");
}

PLACEHOLDER void LevelBackdropStart(void)
{
    unconverted("LevelBackdropStart");
}

PLACEHOLDER void ViewEdgeRuleRun(Actor *player)
{
    unconverted("ViewEdgeRuleRun");
    (void)player;
}
