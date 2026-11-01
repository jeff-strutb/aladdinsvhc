/*
 * joypad.c: reading the joypads once a frame.
 *
 * The hardware reads the pads by itself after each vertical blank (auto
 * joypad read). The game keeps the buttons held ($0377) and the buttons
 * newly pressed this frame ($037B), a word each for pads 1 and 2.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "hw.h"
#include "wram.h"

/* The data bank's view of `addr` (absolute addressing). */
static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

/*
 * @routine 81:BDCC JoypadRead
 * WHAT IT DOES: Takes this frame's buttons from pads 1 and 2: the ones
 * held, and the ones pressed since the last read.
 * HOW: Waits for the hardware's automatic read to finish, then for each of
 * the four bytes (pad 2 high to pad 1 low): newly pressed = pressed now
 * and not held before; held = pressed now.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
 * whose $4212-$421B are the CPU's registers; called by JSR.
 * Exit: X = $FF; A low byte = pad 1's low byte; N set, Z clear (DEX);
 * C clear (the wait's LSR); Y, V unchanged.
 */
void abi_JoypadRead(void)
{
    uint8_t a;
    int x;

    do {
        a = rd8(abs_db(HVBJOY & 0xFFFF));
        cpu.p = (uint8_t)((cpu.p & ~P_C) | (a & 1));
        a >>= 1;
    } while (cpu.p & P_C);
    for (x = 3; x >= 0; x--) {
        a = (uint8_t)(rd8(abs_db(JOY_HELD + x)) ^ 0xFF);
        a &= rd8(abs_db((JOY1L & 0xFFFF) + x));
        wr8(abs_db(JOY_PRESSED + x), a);
        a = rd8(abs_db((JOY1L & 0xFFFF) + x));
        wr8(abs_db(JOY_HELD + x), a);
    }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.x = 0xFF;
    cpu.p = nz8(cpu.p, 0xFF);
}
