/*
 * input.c: a frame's buttons: from the keys and controllers (in place of
 * the SNES pads), or in the demo from its script; then the game's four
 * actions through the buttons the options give them.
 *
 * The keys and controller buttons that stand for the pads' buttons are
 * input/controls.c's, which the menu lets the player change.
 */
#include "data/input_tables.h"
#include "input/controls.h"
#include "system/state.h"
#include "system/system.h"

enum { PLAY_PADS = 0, PLAY_RECORDER = 1, PLAY_DEMO = 2 };

static uint16_t pad_now(const nshvc_input *input, unsigned pad)
{
    return controls_pad(input, pad);
}

/*
 * @routine 81:BDCC JoypadRead
 * WHAT IT DOES: Takes this frame's buttons for pads 1 and 2: the ones held,
 * and the ones pressed since the last read.
 */
void JoypadRead(const nshvc_input *input)
{
    unsigned pad;

    for (pad = 0; pad < 2; pad++) {
        uint16_t now = pad_now(input, pad);

        g.pad_pressed[pad] = (uint16_t)(now & ~g.pad_held[pad]);
        g.pad_held[pad] = now;
    }
}

/* The demo's input: the real pad 1's START kept (so a player can end the
   demo), the rest from the demo's script, a step of it each time its frames
   run out. */
static void demo_input(const nshvc_input *input)
{
    uint8_t before = (uint8_t)(g.pad_held[0] >> 8), held;

    JoypadRead(input);
    held = (uint8_t)(((g.pad_held[0] >> 8) & (PAD_START >> 8)) | g.demo_buttons);
    g.pad_held[0] = (uint16_t)(held << 8);
    g.pad_pressed[0] = (uint16_t)((held & (uint8_t)~before) << 8);
    g.pad_held[1] = 0;
    g.pad_pressed[1] = 0;
    if (--g.demo_input_frames == 0) {
        DemoStep step = demo_step_at(g.demo_input_at);

        /* the original stepped the offset's low byte alone */
        g.demo_input_at = (uint16_t)((g.demo_input_at & 0xFF00u) | ((g.demo_input_at + 2u) & 0xFFu));
        g.demo_buttons = (uint8_t)(step.buttons & ~(PAD_START >> 8));
        g.demo_input_frames = step.frames;
    }
}

/*
 * @routine 81:BDB1 InputRead
 * WHAT IT DOES: Takes this frame's buttons and turns them into the game's
 * actions: from the pads, or in the demo from its script.
 * HOW: Play mode 1 (the development recorder, 81:BE5A InputRecord) is never
 * set on retail (wram_state.h; its buffer is cartridge RAM this board does
 * not have): not converted, dead.
 */
void InputRead(const nshvc_input *input)
{
    if (g.play_mode == PLAY_DEMO)
        demo_input(input);
    else
        JoypadRead(input);
    ControlsRead();
}

/*
 * @routine 81:BDE9 ControlsRead
 * WHAT IT DOES: Turns pad 1's buttons into the game's four actions, held
 * and newly pressed, through the button the options give each.
 * HOW: The last two actions' presses are tested against the low byte of
 * the buttons alone (A, X, L, R), as the original's 8-bit tests.
 */
void ControlsRead(void)
{
    unsigned k;

    g.pad1_held = g.pad_held[0];
    g.pad1_pressed = g.pad_pressed[0];
    for (k = 0; k < 4; k++) {
        uint16_t pressed = k < 2 ? g.pad1_pressed : (uint16_t)(g.pad1_pressed & 0xFFu);

        g.action_held[k] = (g.pad1_held & g.control_masks[k]) != 0;
        g.action_pressed[k] = (pressed & g.control_masks[k]) != 0;
    }
}
