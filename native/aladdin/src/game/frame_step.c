/*
 * frame_step.c: one frame of play: the actors' turns (the actor loop),
 * then the scroll, the map's tile changes, the play time, the palette and
 * sequence steps, and the sprites, with the frame's queues marked ready for
 * the vertical blank.
 */
#include "actor/actor.h"
#include "game/game.h"
#include "level/level.h"
#include "map/map.h"
#include "system/state.h"
#include "system/task.h"
#include "system/unconverted.h"
#include "video/video.h"
#include "video/view.h"

#define VIEW_HALF 0x0080        /* the range point: the view's corner + this + the view's dx, dy */

/*
 * @routine 81:8311 FrameStep
 * @routine 81:830D FrameStepLong
 * WHAT IT DOES: One frame of play: the player's turn and every other
 * actor's (the actor loop), the scroll and the map, the play time, the
 * sequence and palette cycle steps (not while paused or with pause_block
 * set), and the sprites; the frame's sprites, tile block and VRAM queues
 * are then marked ready.
 * HOW: The player's place kept as the frame began; the tile block queue
 * emptied and the frame not ready; PlayerLoop, OamBuildBegin, ActorLoop,
 * ScrollUpdate, TileChangesApply, LayersScroll, PlacementsScroll,
 * PlayTimeTick; the range point = the view's corner + $80 + the view's dx, dy;
 * SequencesStep, PaletteCyclesStep; ActorsInView, OamDrawActors; the three
 * marks counted up; BgFollowLayers. An actor's TaskExec (system/task.h)
 * ends the frame where it happens: true then, and nothing after it runs.
 */
bool FrameStep(void)
{
    g.frame_start_x = g.actors[0].x;
    g.frame_start_y = g.actors[0].y;
    g.tile_blocks_count = 0;
    g.frame_ready = 0;
    PlayerLoop();
    if (task_exec_pending())
        return true;
    OamBuildBegin();
    ActorLoop();
    if (task_exec_pending())
        return true;
    ScrollUpdate();
    TileChangesApply();
    LayersScroll();
    PlacementsScroll();
    PlayTimeTick();
    g.range_x = (uint16_t)((g.layers[0].scroll_x >> 8) + VIEW_HALF + view.dx);
    g.range_y = (uint16_t)((g.layers[0].scroll_y >> 8) + VIEW_HALF + view.dy);
    if (!(g.paused | g.pause_block)) {
        SequencesStep();
        PaletteCyclesStep();
    }
    ActorsInView();
    OamDrawActors();
    g.frame_ready++;
    g.tile_blocks_pending++;
    g.vram_queue_pending++;
    BgFollowLayers();
    return false;
}
