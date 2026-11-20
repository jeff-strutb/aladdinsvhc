/*
 * pad_buttons.c: the buttons that move a screen on.
 */
#include "input/input.h"
#include "system/state.h"
#include "system/system.h"

/*
 * @routine 80:A04F PadConfirmPressed
 * WHAT IT DOES: Tells whether pad 1 pressed A, X, B, Y or START this
 * frame, the buttons that move a screen on: nonzero when it did.
 * HOW: A and X first; B, Y and START only when neither was pressed (the
 * answer is the same either way: the callers test only whether it is 0).
 */
int PadConfirmPressed(void)
{
    uint16_t pressed = g.pad_pressed[0];

    if (pressed & (PAD_A | PAD_X))
        return 1;
    return (pressed & (PAD_B | PAD_Y | PAD_START)) != 0;
}
