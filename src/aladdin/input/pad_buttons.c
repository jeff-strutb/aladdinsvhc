/*
 * pad_buttons.c: questions about the buttons pressed this frame.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"
#include "wram_state.h"

/*
 * @routine 80:A04F PadConfirmPressed
 * WHAT IT DOES: Tells whether pad 1 pressed A, X, B, Y or START this
 * frame, the buttons that move a screen on.
 * HOW: A and X are bits 7 and 6 of the pressed word's low byte; B, Y and
 * START bits 7, 6 and 4 of its high byte, tested only when neither A nor
 * X was pressed.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: A low =
 * the pressed bits found (A or X, else B, Y or START), Z set when none, N
 * from bit 7. A high, X, Y and the other flags unchanged.
 */
void abi_PadConfirmPressed(void)
{
    uint8_t a = (uint8_t)(rd8(JOY_PRESSED) & 0xC0);

    if (a == 0)
        a = (uint8_t)(rd8(JOY_PRESSED + 1) & 0xD0);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = nz8(cpu.p, a);
}
