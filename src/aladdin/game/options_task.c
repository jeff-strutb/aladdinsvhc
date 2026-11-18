/*
 * options_task.c: the options screen's task: the screen set up, then each
 * frame the cheat code, the cursor's moves and the items' steps.
 * Asm: asm/aladdin/game/options_task.asm.
 */
#include "machine.h"
#include "native.h"
#include "ops65.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"
#include "actor.h"

#define OPTION_ITEM 0x1B14u        /* the cursor's item, times 2 (0 setting, 2 sound test, 4 output, 6 exit) */
#define SOUND_TEST 0x1B15u         /* the sound test's number, less one (0-34) */
#define OPTION_SETTING 0x1B17u     /* the setting the options screen shows (0-3) */
#define CODE_WAIT 0x1B19u          /* frames since the cheat code's last press */
#define CODE_AT 0x1B1Au            /* presses of the cheat code made (0: none yet) */
#define PRESSED_HIGH 0x037Cu       /* this frame's presses, high byte: B Y Select Start Up Down Left Right */
#define CODE_PRESSES_NOW 0x037Du   /* the presses the cheat code compares (word) */
#define V_0399 0x0399u             /* set to $16 here; purpose not yet established */
#define CODE_PRESSES 0xCE63u       /* in the bank DB names: the cheat code's presses (a word each, from the second) */

void abi_ActorCreate(void);
void abi_DisplayDefaults(void);
void abi_FrameStepLong(void);
void abi_LevelRamSetupLong(void);
void abi_NmiDisable(void);
void abi_NmiEnable(void);
void abi_OamBufferClear(void);
void abi_OptionCursorClear(void);
void abi_OptionCursorDraw(void);
void abi_OptionSettingShow(void);
void abi_OptionStereoShow(void);
void abi_OptionsLeave(void);
void abi_PaletteLoad(void);
void abi_ScreenBlank(void);
void abi_ScrollStateClear(void);
void abi_SongPlay(void);
void abi_SoundQueueAdd(void);
void abi_SoundQueueF2(void);
void abi_SoundTestShow(void);
void abi_TaskCreate(void);
void abi_TaskWait(void);
void abi_TextLayerClear(void);
void abi_TextLayoutShow(void);
void abi_UnpackSet(void);
void abi_VramCopy(void);

/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:8979 OptionsTask
 * WHAT IT DOES: The options screen's task. Sets the screen up: the
 * screen's graphics and palettes, its figure (an actor of kind $82), the
 * task that animates it, its text, the colour math, the cursor on the
 * first item and each item's setting shown. Then every frame: the cheat
 * code (Select, then the presses at CODE_PRESSES one after another, each
 * within 32 frames, sets CHEAT_0009 and plays sound $28); A, B, X or Y on
 * the sound test plays its song, on the exit item leaves; Start leaves;
 * up and down move the cursor (wrapping), with sounds $F2 and $0C; left
 * and right run the item's step through OptionSteps (C clear left, set
 * right), with sound $0C.
 * HOW: NmiDisable, ScreenBlank, ScrollStateClear, OamBufferClear,
 * DisplayDefaults, TextLayerClear; the text's upload ($5C00, $0800
 * bytes); LEVEL = $2B, LevelRamSetupLong; V_1A08 = $FF; unpacked sets $01,
 * $0D, $02, $22 to VRAM at $00, $70, $77, $FC; NmiEnable; V_0399 = $16;
 * palettes $08, $0B, $57. ActorCreate kind $82: state $0C, look $0900, x
 * and y $0080. TaskCreate (slot $18, $04, the task at $81:9E8E); text
 * layout $2E; CGWSEL clear, CGADSUB $90, COLDATA red 2, green 1, blue 6.
 * The item 0, the sound test 0, the setting from KEPT_OPTIONS; the cursor
 * cleared and drawn; the setting, sound test and output shown; the code
 * cleared. Each frame (TaskWait 1, FrameStepLong): the code unless
 * CHEAT_0009 is set; the presses as above.
 * Entry: a task (TaskExec from the title task); M 8-bit, X 8-bit; DB a
 * bank whose $0000-$1FFF is work RAM and that holds CODE_PRESSES and
 * OptionSteps ($83). Exit: by OptionsLeave.
 */
void abi_OptionsTask(void)
{
    jsl_call(0x838979u, abi_NmiDisable);
    jsl_call(0x83897Du, abi_ScreenBlank);
    jsl_call(0x838981u, abi_ScrollStateClear);
    jsl_call(0x838985u, abi_OamBufferClear);
    jsl_call(0x838989u, abi_DisplayDefaults);
    jsl_call(0x83898Du, abi_TextLayerClear);
    op_rep(0x20);
    lda16_v(0x5C00);
    sta16(ea_abs(TEXT_UPLOAD_VRAM));
    lda16_v(0x0800);
    sta16(ea_abs(TEXT_UPLOAD_SIZE));
    op_sep(0x20);
    lda8_v(0x2B);
    sta8(ea_abs(LEVEL));
    jsl_call(0x8389A6u, abi_LevelRamSetupLong);
    lda8_v(0xFF);
    sta8(ea_abs(V_1A08));
    lda8_v(0x01);
    jsl_call(0x8389B1u, abi_UnpackSet);
    ldx8_v(0x00);
    jsl_call(0x8389B7u, abi_VramCopy);
    lda8_v(0x0D);
    jsl_call(0x8389BDu, abi_UnpackSet);
    ldx8_v(0x70);
    jsl_call(0x8389C3u, abi_VramCopy);
    lda8_v(0x02);
    jsl_call(0x8389C9u, abi_UnpackSet);
    ldx8_v(0x77);
    jsl_call(0x8389CFu, abi_VramCopy);
    lda8_v(0x22);
    jsl_call(0x8389D5u, abi_UnpackSet);
    ldx8_v(0xFC);
    jsl_call(0x8389DBu, abi_VramCopy);
    jsl_call(0x8389DFu, abi_NmiEnable);
    lda8_v(0x16);
    sta8(ea_abs(V_0399));
    lda8_v(0x08);
    jsl_call(0x8389EAu, abi_PaletteLoad);
    lda8_v(0x0B);
    jsl_call(0x8389F0u, abi_PaletteLoad);
    lda8_v(0x57);
    jsl_call(0x8389F6u, abi_PaletteLoad);
    lda8_v(0x82);
    jsl_call(0x8389FCu, abi_ActorCreate);
    if (!IF_C) {
        /* the screen's figure */
        lda8_v(0x0C);
        sta8(ea_absx(ACTOR_STATE));
        op_rep(0x20);
        lda16_v(0x0900);
        sta16(ea_absx(ACTOR_LOOK));
        lda16_v(0x0080);
        sta16(ea_absx(ACTOR_X + 1));
        lda16_v(0x0080);
        sta16(ea_absx(ACTOR_Y + 1));
    }
    op_sep(0x20);
    jsl_frame(0x838A1Du); abi_TaskCreate(); cpu.unwound = 0;
    lda8_v(0x2E);
    jsl_call(0x838A28u, abi_TextLayoutShow);
    stz8(ea_abs(CGWSEL_SHADOW));
    lda8_v(0x90);
    sta8(ea_abs(CGADSUB_SHADOW));
    lda8_v(0x02);
    sta8(ea_abs(COLDATA_RED));
    lda8_v(0x01);
    sta8(ea_abs(COLDATA_GREEN));
    lda8_v(0x06);
    sta8(ea_abs(COLDATA_BLUE));
    stz8(ea_abs(OPTION_ITEM));
    stz8(ea_abs(SOUND_TEST));
    lda8(ea_abs(KEPT_OPTIONS));
    sta8(ea_abs(OPTION_SETTING));
    call_frame(0x8A51, abi_OptionCursorClear);
    ldy8(ea_abs(OPTION_ITEM));
    call_frame(0x8A57, abi_OptionCursorDraw);
    lda8(ea_abs(OPTION_SETTING));
    call_frame(0x8A5D, abi_OptionSettingShow);
    call_frame(0x8A60, abi_SoundTestShow);
    call_frame(0x8A63, abi_OptionStereoShow);
    stz8(ea_abs(CODE_WAIT));
    stz8(ea_abs(CODE_AT));
    for (;;) {                                   /* each frame */
        lda8_v(0x01);
        jsl_frame(0x838A6Cu); abi_TaskWait();
        jsl_call(0x838A70u, abi_FrameStepLong);
        lda8(ea_abs(CHEAT_0009));
        if (IF_Z) {
            /* the cheat code: Select first, then each press in turn */
            lda8(ea_abs(CODE_AT));
            if (IF_Z) {
                lda8(ea_abs(CODE_PRESSES_NOW));
                and8_v(0x20);
                if (!IF_Z) {
                    MEM8(inc, ea_abs(CODE_AT));
                    stz8(ea_abs(CODE_WAIT));
                }
            } else {
                MEM8(inc, ea_abs(CODE_WAIT));
                lda8(ea_abs(CODE_WAIT));
                cmp8_v(0x20);
                if (IF_C) {
                    stz8(ea_abs(CODE_WAIT));     /* too slow: start again */
                    stz8(ea_abs(CODE_AT));
                } else {
                    lda8(ea_abs(CODE_AT));
                    ACC8(asl);
                    tax8();
                    op_rep(0x20);
                    lda16(ea_abs(CODE_PRESSES_NOW));
                    if (!IF_Z) {
                        cmp16_v(rd16(ea_absx(CODE_PRESSES)));
                        op_sep(0x20);
                        if (!IF_Z) {
                            stz8(ea_abs(CODE_WAIT));   /* a wrong press: start again */
                            stz8(ea_abs(CODE_AT));
                        } else {
                            stz8(ea_abs(CODE_WAIT));
                            MEM8(inc, ea_abs(CODE_AT));
                            lda8(ea_abs(CODE_AT));
                            cmp8_v(0x08);
                            if (IF_Z) {
                                lda8_v(0x01);
                                sta8(ea_abs(CHEAT_0009));
                                lda8_v(0x28);
                                jsl_call(0x838AC6u, abi_SoundQueueAdd);
                                op_sep(0x20);
                            }
                        }
                    } else {
                        op_sep(0x20);
                    }
                }
            }
        }
        lda8(ea_abs(PRESSED_HIGH));
        ora8_v(rd8(ea_abs(JOY_PRESSED)));
        bit8_i(0xC0);
        if (!IF_Z) {
            /* A, B, X or Y: the sound test plays, the exit item leaves */
            lda8(ea_abs(OPTION_ITEM));
            cmp8_v(0x02);
            if (IF_Z) {
                lda8(ea_abs(SOUND_TEST));
                clc();
                adc8_v(0x10);
                jsl_call(0x838AE3u, abi_SongPlay);
                continue;
            }
            cmp8_v(0x06);
            if (!IF_Z)
                continue;
            abi_OptionsLeave();
            return;
        }
        lda8(ea_abs(PRESSED_HIGH));
        bit8_i(0x10);
        if (!IF_Z) {                             /* Start */
            abi_OptionsLeave();
            return;
        }
        bit8_i(0x08);
        if (!IF_Z) {                             /* up */
            MEM8(dec, ea_abs(OPTION_ITEM));
            MEM8(dec, ea_abs(OPTION_ITEM));
            if (IF_N) {
                lda8_v(0x06);
                sta8(ea_abs(OPTION_ITEM));
            }
        } else {
            bit8_i(0x04);
            if (IF_Z) {
                /* left (C clear) or right (C set): the item's step */
                bit8_i(0x02);
                if (!IF_Z) {
                    clc();
                } else {
                    bit8_i(0x01);
                    if (IF_Z)
                        continue;
                    sec();
                }
                ldx8(ea_abs(OPTION_ITEM));
                /* JSR (OptionSteps,X) at $83:8B4D: its return address on
                   the stack while the step runs */
                op_push8(0x8B);
                op_push8(0x4F);
                call_routine(0x830000u | rd16(0x830000u | (uint16_t)(0x8B59 + cpu.x)), 0);
                cpu.s = (uint16_t)(cpu.s + 2);
                lda8_v(0x0C);
                jsl_call(0x838B52u, abi_SoundQueueAdd);
                continue;
            }
            /* down */
            MEM8(inc, ea_abs(OPTION_ITEM));
            MEM8(inc, ea_abs(OPTION_ITEM));
            lda8(ea_abs(OPTION_ITEM));
            cmp8_v(0x08);
            if (IF_C) {
                lda8_v(0x00);
                sta8(ea_abs(OPTION_ITEM));
            }
        }
        /* the cursor moved */
        jsl_call(0x838B24u, abi_SoundQueueF2);
        lda8_v(0x0C);
        jsl_call(0x838B2Au, abi_SoundQueueAdd);
        call_frame(0x8B30, abi_OptionCursorClear);
        ldy8(ea_abs(OPTION_ITEM));
        call_frame(0x8B36, abi_OptionCursorDraw);
    }
}
