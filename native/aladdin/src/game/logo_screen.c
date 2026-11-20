/*
 * logo_screen.c: the logo screen the game shows at power on, before the
 * title.
 */
#include "game/game.h"
#include "system/state.h"
#include "system/system.h"
#include "system/task.h"
#include "system/unconverted.h"

#define FADE_SLOT 1             /* the fades run in slot 1 */
#define SHINE_SLOT 2            /* the palette sequence in slot 2 */
#define LOGO_SET 3              /* its graphics set and palette */
#define LOGO_SONG 0x10
#define LOGO_SHOW_FRAMES 120
/* pad 1's B, Y, Select, Start, A, X, L and R end the logo early */
#define LOGO_KEYS (PAD_B | PAD_Y | PAD_SELECT | PAD_START | PAD_A | PAD_X | PAD_L | PAD_R)

enum { LOGO_START, LOGO_DARK, LOGO_FADE_IN, LOGO_HOLD, LOGO_SHINE, LOGO_SHOW, LOGO_FADE_OUT };

/*
 * @routine A5:E000 LogoScreen
 * WHAT IT DOES: The logo screen at power on: the logo shown with its tune,
 * faded in, its palettes played through, held for two seconds or until a
 * button, and faded out.
 * HOW: With the NMI off and the screen blanked: scroll and sprites cleared,
 * the display's defaults, graphics set 3 unpacked and VRAM copies 1 and 2,
 * layer 1 alone on the main and sub screens, palette 3; NMI on, song $10.
 * After 60 frames ColourFadeInTask in slot 1 (parameter 2); when it has
 * ended, full brightness and 31 frames; PaletteSequenceTask in slot 2,
 * waited out; then up to 120 frames (counted in the task's parameter, as the
 * original) or until pad 1 holds one of LOGO_KEYS; ColourFadeOutTask in
 * slot 1 (parameter 1), waited out.
 * Runs in slot 0 (BootTask's chain).
 */
TaskRun LogoScreen(Task *t)
{
    uint16_t *at = task_enter(t);

    switch (*at) {
    case LOGO_START:
        NmiDisable();
        ScreenBlank();
        ScrollStateClear();
        OamBufferClear();
        DisplayDefaults();
        UnpackSet(LOGO_SET);
        VramCopy(1);
        VramCopy(2);
        g.main_layers = 0x01;
        g.sub_layers = 0x01;
        PaletteLoad(LOGO_SET);
        NmiEnable();
        SongPlay(LOGO_SONG);
        *at = LOGO_DARK;
        return task_wait(t, 60);
    case LOGO_DARK:
        TaskCreate(FADE_SLOT, 2, TASK_ENTRY_COLOUR_FADE_IN);
        *at = LOGO_FADE_IN;
        return task_wait(t, 1);
    case LOGO_FADE_IN:
        if (task_busy(FADE_SLOT))
            return task_wait(t, 1);
        g.blank = 0;
        g.brightness = 15;
        *at = LOGO_HOLD;
        return task_wait(t, 31);
    case LOGO_HOLD:
        TaskCreate(SHINE_SLOT, 0, TASK_ENTRY_PALETTE_SEQUENCE);
        *at = LOGO_SHINE;
        return task_wait(t, 1);
    case LOGO_SHINE:
        if (task_busy(SHINE_SLOT))
            return task_wait(t, 1);
        t->param = LOGO_SHOW_FRAMES;
        *at = LOGO_SHOW;
        return task_wait(t, 1);
    case LOGO_SHOW:
        if (!(g.pad1_held & LOGO_KEYS) && --t->param != 0)
            return task_wait(t, 1);
        TaskCreate(FADE_SLOT, 1, TASK_ENTRY_COLOUR_FADE_OUT);
        *at = LOGO_FADE_OUT;
        return task_wait(t, 1);
    case LOGO_FADE_OUT:
        if (task_busy(FADE_SLOT))
            return task_wait(t, 1);
        break;
    }
    return task_return(t);
}
