/*
 * options_items.c: the options screen's sound output item (stereo or
 * monaural), its exit item's empty step, and the cursor's clearing. Left
 * and right on an item call its step through the table at $83:8B59.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "wram_state.h"
void abi_TextLayoutShow(void);

void abi_KeptOptionsSeal(void);
void abi_SoundQueueAdd(void);
void abi_OptionStereoShow(void);
void abi_SoundQueueF2(void);
void abi_TaskExec(void);

#define SOUND_OUTPUT (KEPT_OPTIONS + 1) /* the second kept option: bit 7 set, stereo */
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
 * @routine 83:8BCF OptionsLeave
 * WHAT IT DOES: Leaves the options screen for the title: its sound stopped,
 * then the task starts again at TitleAfterFadeOut.
 * HOW: SoundQueueF2; TaskExec with the entry in the three bytes after its
 * JSL ($80:9C79).
 * Entry: M 8-bit, X 8-bit; reached by JMP from the options task ($83:8979)
 * when its exit item is chosen. Exit: never returns.
 */
void abi_OptionsLeave(void)
{
    abi_SoundQueueF2();
    /* TaskExec at $83:8BD3: its frame, as the JSL pushes it; the three
       bytes after the JSL are the task's new entry. It never returns. */
    jsl_frame(0x838BD3u);
    abi_TaskExec();
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
    static const int16_t cells[7] = { -62, 0, 2, 4, 64, 66, 68 };
    uint8_t up;

    y8_load(0x06);
    op_rep(0x30);
    cpu.a = 0;
    do {                                  /* each of the four items' places */
        cpu.x = rd16(dba(CURSOR_PLACES) + cpu.y);
        for (int i = 0; i < 7; i++)
            wr16((uint32_t)(TEXT_TILEMAP + cells[i] + cpu.x) & 0xFFFFFF, 0x0000);
        cpu.y = (uint16_t)(cpu.y - 2);
        nz16_of(cpu.y);
    } while (!(cpu.y & 0x8000));
    op_sep(0x30);
    up = (uint8_t)(rd8(dba(TEXT_TILEMAP_UPLOAD)) + 1);
    wr8(dba(TEXT_TILEMAP_UPLOAD), up);
    nz8_of(up);
}

/*
 * @routine 83:8BA8 OptionStereoToggle
 * WHAT IT DOES: The sound output item's step (left or right): switches
 * between stereo and monaural, seals the kept options and shows it.
 * HOW: Bit 7 of the second kept option flipped, KeptOptionsSeal, then on
 * into OptionStereoShow.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSR. Exit: as OptionStereoShow.
 */

void abi_OptionStereoToggle(void)
{
    uint8_t out = (uint8_t)(rd8(dba(SOUND_OUTPUT)) ^ 0x80);

    a8_load(out);
    wr8(dba(SOUND_OUTPUT), out);
    abi_KeptOptionsSeal();
    abi_OptionStereoShow();
}

/*
 * @routine 83:8BB4 OptionStereoShow
 * WHAT IT DOES: Shows the sound output on the options screen, STEREO or
 * MONAURAL by bit 7 of the second kept option, and sends the sound driver
 * the matching command.
 * HOW: Text string $68 (STEREO) or $66 (MONAURAL) by TextLayoutShow; then sound command $F7 (stereo) or $F8 (monaural) to
 * SoundQueueAdd.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSR (and on from OptionStereoToggle).
 * Exit: as SoundQueueAdd leaves them (the command is not 0, so Z clear on
 * entry to it); Y = the second kept option; V as TextLayoutShow leaves
 * it.
 */

void abi_OptionStereoShow(void)
{
    uint8_t out = rd8(dba(SOUND_OUTPUT));

    cpu.y = out;
    a8_load((out & 0x80) ? TEXT_STEREO : TEXT_MONAURAL);
    abi_TextLayoutShow();
    out = rd8(dba(SOUND_OUTPUT));
    cpu.y = out;
    a8_load((out & 0x80) ? SOUND_CMD_STEREO : SOUND_CMD_STEREO + 1);
    abi_SoundQueueAdd();
}
