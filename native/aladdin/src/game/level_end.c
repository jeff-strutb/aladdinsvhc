/*
 * level_end.c: what the game does at a level's end, between the level and
 * what follows it.
 *
 * The original shows a screen here (81:F61D LevelEndTask: "STAGE n CLEAR",
 * the password as four pictures grown as a walker crosses, then a wait for a
 * button). Jeff removed it ("Get rid of the between level password screen.
 * No longer necessary with level select / saves", 2026-11-23;
 * DEPARTURES.md section 8): the screen's code, its tables and its task state
 * are gone, and the game goes straight on to what followed it. What the
 * screen did to the game's state that later screens read is kept below.
 */
#include "game/game.h"
#include "system/state.h"
#include "system/system.h"
#include "system/task.h"

#define LAST_PART 7                 /* the part after which play goes straight on */
#define SHOW_LAYERS 0x17            /* the layers the screen showed, on the main and the sub screen */

/*
 * @routine 81:F61D LevelEndTask
 * WHAT IT DOES: The game's step at a level's end (the player's exits start
 * it, once the level's fade-out is done): the part reached kept in the third
 * kept option and sealed; the gem sets found added to the total and cleared
 * for the next level; the layers shown set as the screen left them; then on to the story card (or, after part 7, the next
 * part), in the same update.
 * HOW: The original's screen between these steps is not made (see above);
 * the steps are its first and last ones, in its order: the part kept and the
 * sum sealed, the sets added to the total, and at its end the set count and
 * the found bits cleared. Nothing between them reads or writes these. The
 * layers are the screen's ($17 on both screens): the story card sets the main
 * screen's itself, but not the sub screen's, which no routine between the
 * level and the card resets, so without this the card would show whatever
 * the level left there (found by comparing the traces, CHECK.md).
 */
TaskRun LevelEndTask(Task *t)
{
    uint8_t part = (uint8_t)g.progress_sequence;

    g.kept_options[2] = part;
    KeptOptionsSeal();
    g.gem_sets_total = (uint8_t)(g.gem_sets_total + g.gem_sets);
    g.gem_sets = 0;
    g.gem_sets_found = 0;
    g.main_layers = SHOW_LAYERS;
    g.sub_layers = SHOW_LAYERS;
    return task_exec(t, part == LAST_PART ? TASK_ENTRY_NEXT_PART : TASK_ENTRY_STORY_CARD);
}
