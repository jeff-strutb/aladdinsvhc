/*
 * display.c: switching the vertical-blank interrupt and the screen.
 *
 * The game keeps shadows of NMITIMEN and INIDISP in work RAM; the NMI
 * writes INIDISP's shadow to the register every frame.
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

static void set_a8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/*
 * @routine 81:969F NmiEnable
 * WHAT IT DOES: Turns the vertical-blank interrupt on, with the rest of
 * NMITIMEN as its shadow says.
 * HOW: Writes the shadow with bit 7 set to NMITIMEN.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM and whose $4200
 * is NMITIMEN. Exit: A low byte = the value written; N set, Z clear; X, Y,
 * C, V unchanged.
 */
void abi_NmiEnable(void)
{
    set_a8((uint8_t)(rd8(abs_db(NMITIMEN_SHADOW)) | 0x80));
    wr8(abs_db(NMITIMEN & 0xFFFF), (uint8_t)cpu.a);
}

/*
 * @routine 81:96A8 NmiDisable
 * WHAT IT DOES: Turns the vertical-blank interrupt off, keeping the
 * automatic joypad read as its shadow says.
 * HOW: Writes the shadow's bit 0 alone to NMITIMEN.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM and whose $4200
 * is NMITIMEN. Exit: A low byte = the value written; N clear, Z from it;
 * X, Y, C, V unchanged.
 */
void abi_NmiDisable(void)
{
    set_a8((uint8_t)(rd8(abs_db(NMITIMEN_SHADOW)) & 0x01));
    wr8(abs_db(NMITIMEN & 0xFFFF), (uint8_t)cpu.a);
}

/*
 * @routine 81:96C5 ScreenFullBright
 * WHAT IT DOES: Shows the screen at full brightness from the next vertical
 * blank.
 * HOW: INIDISP's shadow = $0F (no forced blank, brightness 15).
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: A low byte = $0F; N, Z clear; X, Y, C, V unchanged.
 */
void abi_ScreenFullBright(void)
{
    set_a8(0x0F);
    wr8(abs_db(INIDISP_SHADOW), 0x0F);
}
