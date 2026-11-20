/*
 * input.c: a frame's buttons: from the keys and controllers (in place of
 * the SNES pads), or in the demo from its script; then the game's four
 * actions through the buttons the options give them.
 *
 * The keys and controller buttons below stand for the pads' buttons until
 * stage 6 gives the game its own controls (plan stage 6).
 */
#include "data/input_tables.h"
#include "system/state.h"
#include "system/system.h"

enum { PLAY_PADS = 0, PLAY_RECORDER = 1, PLAY_DEMO = 2 };

/* keys (nshvc_key: Windows virtual key codes) for pad 1 */
static const struct { nshvc_key key; uint16_t button; } pad_keys[] = {
    { 0x26, PAD_UP }, { 0x28, PAD_DOWN }, { 0x25, PAD_LEFT }, { 0x27, PAD_RIGHT },
    { 'X', PAD_A }, { 'Z', PAD_B }, { 'S', PAD_X }, { 'A', PAD_Y },
    { 'Q', PAD_L }, { 'W', PAD_R }, { 0x0D, PAD_START }, { 0xA1, PAD_SELECT }, { 0x10, PAD_SELECT },
};

/* controller buttons, by their place, for each pad */
static const struct { uint16_t ctl; uint16_t button; } pad_buttons[] = {
    { NSHVC_CTL_UP, PAD_UP }, { NSHVC_CTL_DOWN, PAD_DOWN }, { NSHVC_CTL_LEFT, PAD_LEFT }, { NSHVC_CTL_RIGHT, PAD_RIGHT },
    { NSHVC_CTL_EAST, PAD_A }, { NSHVC_CTL_SOUTH, PAD_B }, { NSHVC_CTL_NORTH, PAD_X }, { NSHVC_CTL_WEST, PAD_Y },
    { NSHVC_CTL_LB, PAD_L }, { NSHVC_CTL_RB, PAD_R }, { NSHVC_CTL_MENU, PAD_START }, { NSHVC_CTL_BACK, PAD_SELECT },
};

static uint16_t pad_now(const nshvc_input *input, unsigned pad)
{
    uint16_t now = 0;
    unsigned i;

    if (!input)
        return 0;
    if (pad == 0)
        for (i = 0; i < sizeof pad_keys / sizeof pad_keys[0]; i++)
            if (NSHVC_KEY_HELD(input, pad_keys[i].key))
                now |= pad_keys[i].button;
    if (input->controller[pad].on)
        for (i = 0; i < sizeof pad_buttons / sizeof pad_buttons[0]; i++)
            if (input->controller[pad].buttons & pad_buttons[i].ctl)
                now |= pad_buttons[i].button;
    return now;
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
