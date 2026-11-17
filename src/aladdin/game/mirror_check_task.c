/*
 * mirror_check_task.c: a task that checks the game runs from its own
 * cartridge: the ROM read through bank $80 and through bank $00 must be
 * the same (a LoROM mirror), and the cartridge must have no RAM at $70.
 * A copier fails one or the other: the game then slows to a stop. Drafted
 * by cdraft.py; completed and described by hand.
 */
#include "machine.h"
#include "ops65.h"

void abi_TaskExit(void);
void abi_TaskWait(void);

#define MIRROR_PROBE 0x9000u       /* two ROM bytes read through banks $80 and $00 */
#define CART_RAM_PROBE 0x0080u     /* in bank $70, from X = $80: $0080 and $0100 written and read back */
#define V_03FB 0x03FBu             /* set to $3F, over and over, once caught */

/*
 * @routine 81:A37A MirrorCheckTask
 * WHAT IT DOES: Checks for a copier, then ends: the two bytes at $9000
 * the same through banks $80 and $00, and $70:0080 not keeping a byte
 * written to it. Caught (no script is): it waits 30 times 63 frames, then
 * forever sets V_03FB to $3F every 255 frames.
 * HOW: DB kept; Y and X = $80:9000 and $9001; with DB 0, CPY and CPX the
 * same bytes; DB = $80 - $10 = $70, X = $80; $23 to $0000,X ($70:0080)
 * and compared; equal: $45 to CART_RAM_PROBE,X ($70:0100) and compared.
 * Not equal: DB back, TaskExit. Caught: DB back; TaskWait $3F 30 times;
 * then V_03FB $3F and TaskWait $FF, for ever.
 * Entry: the task's start: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF
 * is work RAM. Exit: none: the task ends, or never.
 */
void abi_MirrorCheckTask(void)
{
    phb();
    lda8_v(0x80);
    pha8();
    plb();
    ldy8(ea_abs(MIRROR_PROBE));
    ldx8(ea_abs(MIRROR_PROBE + 1));
    lda8_v(0x00);
    pha8();
    plb();
    cpy8_v(rd8(ea_abs(MIRROR_PROBE)));
    if (!IF_Z) goto caught;
    cpx8_v(rd8(ea_abs(MIRROR_PROBE + 1)));
    if (!IF_Z) goto caught;
    ldx8_v(0x80);
    sec();
    lda8_v(0x80);
    sbc8_v(0x10);
    pha8();
    plb();
    lda8_v(0x23);
    sta8(ea_absx(0x0000));
    cmp8_v(rd8(ea_absx(0x0000)));
    if (!IF_Z) goto retail;
    lda8_v(0x45);
    sta8(ea_absx(CART_RAM_PROBE));
    cmp8_v(rd8(ea_absx(CART_RAM_PROBE)));
    if (!IF_Z) goto retail;
caught:
    plb();
    ldx8_v(0x1E);
    do {
        lda8_v(0x3F);
        jsl_frame(0x81A3B5u);
        abi_TaskWait();
        dex8();
    } while (!IF_Z);
    for (;;) {
        lda8_v(0x3F);
        sta8(ea_abs(V_03FB));
        lda8_v(0xFF);
        jsl_frame(0x81A3C3u);
        abi_TaskWait();
    }
retail:
    plb();
    abi_TaskExit();
}
