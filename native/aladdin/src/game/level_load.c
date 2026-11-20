/*
 * level_load.c: loading a level: its graphics, its RAM and VRAM, its
 * screen, then Aladdin; and a card of text shown on its own.
 *
 * Not converted: MirrorCheckTask (81:A37A, game/mirror_check_task.c), the
 * copier check LevelLoad starts in slot 7: on the retail board the ROM
 * reads the same through banks $80 and $00 and there is no cartridge RAM
 * at $70, so it ends at its first turn (TaskExit), changing nothing
 * (plan section 1 item 8: the copier branches are dead).
 */
#include <string.h>

#include "actor/actor.h"
#include "audio/sound.h"
#include "data/level_tables.h"
#include "data/system_tables.h"
#include "game/game.h"
#include "level/level.h"
#include "map/map.h"
#include "system/state.h"
#include "system/system.h"
#include "system/task.h"
#include "system/unconverted.h"
#include "video/video.h"

#define FADE_SLOT 1             /* the colour fades run in slot 1 */
#define TEXT_CARD_LAYOUT 6
#define FIRST_PLAY_LEVEL 0x14   /* levels below this have the short text upload */

/*
 * @routine 81:8668 LevelGraphicsLoadAll
 * WHAT IT DOES: Loads the level's graphics into VRAM: the sets every level
 * uses (1, $0C, $2D), up to two of its own, then its tiles and its actors'.
 * HOW: Set 1 with copy 0, set $0C with copy 4, set $2D with the second
 * table's copy 14; for levels below $1F, its first own set (copy 5) and its
 * second (copy 6), each unless negative (none, and then neither);
 * LevelGraphicsLoad, LevelActorGraphicsLoad.
 */
static void LevelGraphicsLoadAll(void)
{
    UnpackSet(1);
    VramCopy(0);
    UnpackSet(0x0C);
    VramCopy(4);
    UnpackSet(0x2D);
    VramCopy2(14);
    if (g.level < LEVELS_WITH_SETS) {
        int8_t a = level_gfx_set_a((uint8_t)g.level);

        if (a >= 0) {
            int8_t b;

            UnpackSet(a);
            VramCopy(5);
            b = level_gfx_set_b((uint8_t)g.level);
            if (b >= 0) {
                UnpackSet(b);
                VramCopy(6);
            }
        }
    }
    LevelGraphicsLoad();
    LevelActorGraphicsLoad();
}

/*
 * @routine 81:825F LevelRamSetup
 * @routine 81:825B LevelRamSetupLong
 * WHAT IT DOES: Sets up everything in RAM and VRAM the level g.level starts
 * from: game RAM cleared, the tasks but the game's freed, the text upload
 * size, the play starts counted, no actor of any kind live, the display's
 * defaults, the animation sets, the graphics, the actor records, palettes
 * 2, 0, $0C and 9, the text layer, the scroll limits, the start place, the
 * placements and the level's palettes.
 */
void LevelRamSetup(void)
{
    GameRamClear();
    TaskSlotsFree1To6();
    g.text_upload_bytes = g.level < FIRST_PLAY_LEVEL ? 0x0160 : 0x0800;
    g.play_starts++;
    memset(g.kind_count, 0, sizeof g.kind_count);
    DisplayDefaults();
    AnimSetsLoad();
    LevelGraphicsLoadAll();
    ActorRecordsInit();
    PaletteLoad(2);
    PaletteLoad(0);
    PaletteLoad(0x0C);
    PaletteLoad(9);
    TextLayerClear();
    LevelScrollLimits();
    g.layer_setup_mode = 0;
    LevelStartPlace();
    PlacementsStart();
    LevelPalettesLoad();
}

#define PLAY_DEMO_MASKS { 0x8000, 0x4000, 0x0080, 0x0010 }    /* a demo's buttons by action: B, Y, A, R */
#define HUD_LEVELS 0x14         /* levels below this show the status line */
#define PLAYER_NEW 0x0C         /* a record's state: to start */

enum { LOAD_START, LOAD_BUILD, LOAD_SONG };

/*
 * @routine 81:81A7 LevelLoad
 * @routine 81:81A3 LevelLoadLong
 * WHAT IT DOES: Loads and starts the level: the buttons each action takes
 * (fixed ones in a demo, else by the control option), the screen blanked
 * while LevelRamSetup, the status line, the map, the screen set-up, the
 * layers, spots, tile behaviours and backdrop are made, then the screen
 * back on, its song (not in a demo), Aladdin's start, his escort and
 * companion.
 * HOW: TextLayerClearFront, a frame's wait, NmiDisable, ScreenBlank,
 * LevelRamSetup, DisplayDefaults, HudRedrawAll (levels below $14),
 * LevelMapBuild, LevelScreenSetup, LayersFill, LevelSpotsSet,
 * TileBehaviourLoad, LevelBackdropStart, NmiEnable; with no demo, a frame's
 * wait and LevelSongPlay; Aladdin's record to start, his kind and $07 by
 * level; EscortStart, LevelCompanionStart. (The original also clears $03E9,
 * which nothing reads, and starts the copier check: see above.)
 */
TaskRun LevelLoad(Task *t)
{
    static const uint16_t demo_masks[4] = PLAY_DEMO_MASKS;
    uint16_t *at = task_enter(t);
    Actor *player = &g.actors[0];
    unsigned i;

    switch (*at) {
    case LOAD_START:
        for (i = 0; i < 4; i++)
            g.control_masks[i] = g.play_mode ? demo_masks[i] : control_option_mask(g.kept_options[0], i);
        TextLayerClearFront();
        *at = LOAD_BUILD;
        return task_wait(t, 1);
    case LOAD_BUILD:
        NmiDisable();
        ScreenBlank();
        LevelRamSetup();
        DisplayDefaults();
        if (g.level < HUD_LEVELS)
            HudRedrawAll();
        LevelMapBuild();
        LevelScreenSetup();
        LayersFill();
        LevelSpotsSet();
        TileBehaviourLoad();
        LevelBackdropStart();
        NmiEnable();
        if (!g.play_mode) {
            *at = LOAD_SONG;
            return task_wait(t, 1);
        }
        break;
    case LOAD_SONG:
        LevelSongPlay();
        break;
    }
    player->state = PLAYER_NEW;
    player->kind = level_player_kind((uint8_t)g.level);
    player->spawn_byte = level_player_byte((uint8_t)g.level);
    EscortStart();
    LevelCompanionStart();
    return task_return(t);
}

enum { CARD_START, CARD_FADE_IN, CARD_HOLD, CARD_FADE_OUT };

/*
 * @routine 81:ED33 TextCardScreen
 * WHAT IT DOES: Shows a card of text on its own (text layout 6, layer 3
 * alone): the screen made with the NMI off, faded in, held 120 frames,
 * faded out, then the text layer cleared.
 * HOW: Graphics set 1 with copy 0, palette 2; ColourFadeInTask and
 * ColourFadeOutTask in slot 1 (parameter 4), each waited out a frame at a
 * time.
 */
TaskRun TextCardScreen(Task *t)
{
    uint16_t *at = task_enter(t);

    switch (*at) {
    case CARD_START:
        NmiDisable();
        ScreenBlank();
        ScrollStateClear();
        TextLayerClear();
        OamBufferClear();
        DisplayDefaults();
        UnpackSet(1);
        VramCopy(0);
        PaletteLoad(2);
        g.main_layers = 0x04;
        g.sub_layers = 0x04;
        TextLayoutShow(TEXT_CARD_LAYOUT);
        NmiEnable();
        TaskCreate(FADE_SLOT, 4, TASK_ENTRY_COLOUR_FADE_IN);
        *at = CARD_FADE_IN;
        return task_wait(t, 1);
    case CARD_FADE_IN:
        if (task_busy(FADE_SLOT))
            return task_wait(t, 1);
        *at = CARD_HOLD;
        return task_wait(t, 0x78);
    case CARD_HOLD:
        TaskCreate(FADE_SLOT, 4, TASK_ENTRY_COLOUR_FADE_OUT);
        *at = CARD_FADE_OUT;
        return task_wait(t, 1);
    case CARD_FADE_OUT:
        if (task_busy(FADE_SLOT))
            return task_wait(t, 1);
        TextLayerClear();
        break;
    }
    return task_return(t);
}
