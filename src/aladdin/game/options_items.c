/*
 * options_items.c: the options screen's sound output item (stereo or
 * monaural), its exit item's empty step, and the cursor's clearing. Left
 * and right on an item call its step through the table at $83:8B59.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "wram_state.h"

void abi_KeptOptionsSeal(void);
void abi_SoundQueueAdd(void);

#define SOUND_OUTPUT (KEPT_OPTIONS + 1) /* the second kept option: bit 7 set, stereo */
#define TEXT_SHOW 0x81972Du        /* shows text string A of the table at $82:B575 */
#define TEXT_STEREO 0x68           /* the string "STEREO" */
#define TEXT_MONAURAL 0x66         /* the string "MONAURAL" */
#define SOUND_CMD_STEREO 0xF7      /* the sound command for stereo; the next, monaural */
/* In the bank DB names: the cursor's place per item (tilemap byte offsets,
   a word per item). */
#define CURSOR_PLACES 0xCE5Du



/*
 * @routine 83:8BCE OptionExitStep
 * WHAT IT DOES: The exit item's step (left or right): nothing.
 * HOW: RTS.
 * Entry: any. Exit: everything unchanged.
 */
void abi_OptionExitStep(void)
{
}

/*
 * @routine 83:8C7B OptionCursorClear
 * WHAT IT DOES: Clears the options screen's cursor beside every item, and
 * asks for the text layer's upload.
 * HOW: For each of the four items, last first, the seven tilemap words
 * OptionCursorDraw writes at its place are zeroed, with 16-bit registers.
 * Entry: M 8-bit, X 8-bit; DB the table's bank, whose $0000-$1FFF is work
 * RAM. Exit: A = 0; X = the first item's place (low byte); Y = $FE; N and
 * Z from the upload flag's new value; M and X 8-bit. V and C unchanged.
 */
void abi_OptionCursorClear(void)
{
    ldy8_v(0x06);
    op_rep(0x30);
    do {
        ldx16(ea_absy(CURSOR_PLACES));
        lda16_v(0x0000);
        sta16(ea_longx(TEXT_TILEMAP - 62));
        sta16(ea_longx(TEXT_TILEMAP));
        sta16(ea_longx(TEXT_TILEMAP + 2));
        sta16(ea_longx(TEXT_TILEMAP + 4));
        sta16(ea_longx(TEXT_TILEMAP + 64));
        sta16(ea_longx(TEXT_TILEMAP + 66));
        sta16(ea_longx(TEXT_TILEMAP + 68));
        dey16();
        dey16();
    } while (!IF_N);
    op_sep(0x30);
    MEM8(inc, ea_abs(TEXT_TILEMAP_UPLOAD));
}






