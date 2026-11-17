/*
 * story_card.c: the story card shown between acts: a screen loaded and a
 * text typed out, by a list of commands for each part of the game.
 *
 * A part's list (STORY_LISTS, in bank $80) is a run of commands, each a
 * byte: 0 loads a screen (sets of graphics, VRAM copies and palettes,
 * then its song), 2 types a text (its number follows), 4 ends the card
 * (a fade, then the level starts), 6 fades and goes on with the list, 8
 * goes on to the final act's scene. A text (STORY_TEXTS) is lines, each a
 * tilemap offset (word), an attribute byte, then characters: 0 ends the
 * line, $FF clears the layer and ends it, $FE sets the wait before the
 * next character (word), $FC and $FD end the text; $BA and $B7 are marks
 * put on the row above the next character.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "wram_pad.h"
#include "wram_video.h"

void abi_BrightnessDownAndWait(void);
void abi_BrightnessUpAndWait(void);
void abi_LevelRamSetupLong(void);
void abi_LoadListNext(void);
void abi_NmiDisable(void);
void abi_NmiEnable(void);
void abi_OamBufferClear(void);
void abi_PaletteLoad(void);
void abi_ScreenBlank(void);
void abi_ScrollStateClear(void);
void abi_SongPlay(void);
void abi_SoundQueueF5WithA(void);
void abi_SoundQueueF6With1(void);
void abi_TaskExec(void);
void abi_TaskWait(void);
void abi_TextLayerClear(void);
void abi_TextLayerClearFront(void);
void abi_TextTypeNext(void);
void abi_UnpackSet(void);
void abi_VramCopy(void);
void abi_VramFillMap(void);

#define LOAD_LIST_PTR 0x4Cu         /* direct page: the list's next byte (word, in bank $80) */
#define TEXT_AT 0x4Eu               /* direct page: the next character's offset in TEXT_TILEMAP (word) */
#define TEXT_TYPE_PTR 0x50u         /* direct page: the text's next byte (word, in bank $80) */
#define CARD_COUNT 0x52u            /* direct page: frames left (a byte, or a word while typing) */
#define TEXT_ATTR 0x54u             /* direct page: the line's attribute byte */
#define STORY_LISTS 0xD9F3u         /* bank $80: per part (1-7), its list's offset (word) */
#define STORY_COMMANDS 0xF176u      /* bank $81: per command (byte / 2), where it goes */
#define STORY_TEXTS 0xF3BBu         /* bank $80: per text, its offset (word) */
#define TM_SHADOW 0x0399u           /* TM's shadow: the main screen's layers */
#define TEXT_MARKS (TEXT_TILEMAP - 0x40)   /* a mark's place: the row above the character's */

/*
 * @routine 81:F10D StoryCardTask
 * WHAT IT DOES: The story card between acts: for the part being started
 * (LEVEL_035C; part 0 as part 7) it runs that part's list: a screen
 * loaded and brightened, a text typed a character every 4 frames (any of
 * B, Y, A, X held types at once; Start ends the card), then a fade, and
 * the level starts (or the list goes on, or the final act's scene).
 * HOW: The list's pointer from STORY_LISTS by the part; TextLayerClearFront,
 * a frame's wait; NMI off, screen blanked; the text layer's upload at VRAM
 * $5C00, $800 bytes; LEVEL $30, LevelRamSetupLong, graphics set 1 and
 * VRAM copy 0; NMI on; 48 frames. Then for each command, after CARD_COUNT
 * frames: TM_SHADOW 5 and JMP through STORY_COMMANDS by the list's byte.
 * 0: NMI off, blanked, VRAM maps $4000 and $4800 filled with 0, text layer
 * cleared; then (set $0E, copy $7E) and the list's set, copy and palette,
 * until the next byte is not 0: OamBufferClear, ScrollStateClear, NMI on,
 * that byte the song (SongPlay, SoundQueueF5WithA); BrightnessUpAndWait;
 * 64 frames. 2: the text by the next byte from STORY_TEXTS, line by line:
 * TEXT_AT and TEXT_ATTR from its head; each character after CARD_COUNT
 * frames (4) put in TEXT_TILEMAP at TEXT_AT with the attribute, TEXT_AT on
 * 2, the upload marked; $BA, $B7 put at TEXT_MARKS with no wait or step;
 * the codes above; $FC, $FD: the list's next byte, a frame. 4 (or Start):
 * SoundQueueF6With1 (B = 3), 48 frames, BrightnessDownAndWait, 48 frames,
 * TaskExec to $81:8181. 6: the list's next byte, SoundQueueF6With1 (B =
 * 3), 64 frames, BrightnessDownAndWait, 64 frames. 8: TaskExec to
 * $80:AF42.
 * Entry: M 8-bit, X 8-bit; D = 0; DB = $80; a task's entry (TaskExec from
 * the password screen and other tasks). Exit: never returns.
 */
void abi_StoryCardTask(void)
{
    lda8(ea_abs(LEVEL_035C));  /* 81F10D lda */
    if (IF_Z)  /* 81F110 bne */
        lda8_v(0x07);  /* 81F112 lda */
    ACC8(asl);  /* 81F114 asl */
    tax8();  /* 81F115 tax */
    lda8(ea_absx(STORY_LISTS));  /* 81F116 lda */
    sta8(ea_dp(LOAD_LIST_PTR));  /* 81F119 sta */
    lda8(ea_absx(STORY_LISTS + 1));  /* 81F11B lda */
    sta8(ea_dp(LOAD_LIST_PTR + 1));  /* 81F11E sta */
    abi_TextLayerClearFront();  /* 81F120 jsl */
    lda8_v(0x01);  /* 81F124 lda */
    jsl_frame(0x81F126u); abi_TaskWait();  /* 81F126 jsl */
    abi_NmiDisable();  /* 81F12A jsl */
    abi_ScreenBlank();  /* 81F12E jsl */
    op_rep(0x20);  /* 81F132 rep */
    lda16_v(0x5C00);  /* 81F134 lda */
    sta16(ea_abs(TEXT_UPLOAD_VRAM));  /* 81F137 sta */
    lda16_v(0x0800);  /* 81F13A lda */
    sta16(ea_abs(TEXT_UPLOAD_SIZE));  /* 81F13D sta */
    op_sep(0x20);  /* 81F140 sep */
    lda8_v(0x30);  /* 81F142 lda */
    sta8(ea_abs(LEVEL));  /* 81F144 sta */
    abi_LevelRamSetupLong();  /* 81F147 jsl */
    lda8_v(0x01);  /* 81F14B lda */
    abi_UnpackSet();  /* 81F14D jsl */
    ldx8_v(0x00);  /* 81F151 ldx */
    abi_VramCopy();  /* 81F153 jsl */
    abi_NmiEnable();  /* 81F157 jsl */
    lda8_v(0x30);  /* 81F15B lda */
    sta8(ea_dp(CARD_COUNT));  /* 81F15D sta */
L_81F15F:
    MEM8(dec, ea_dp(CARD_COUNT));  /* 81F15F dec */
    if (!IF_Z) goto L_81F16E;  /* 81F161 bne */
    lda8_v(0x05);  /* 81F163 lda */
    sta8(ea_abs(TM_SHADOW));  /* 81F165 sta */
    lda8(ea_dpi(LOAD_LIST_PTR));  /* 81F168 lda */
    tax8();  /* 81F16A tax */
    switch (cpu.x) {  /* 81F16B jmp (STORY_COMMANDS,x) */
    case 0: goto L_81F180;
    case 2: goto L_81F1F6;
    case 4: goto L_81F2AD;
    case 6: goto L_81F2DA;
    case 8: goto L_81F2FC;
    default:
        jump_routine(0x810000u | rd16(0x810000u | (uint16_t)(STORY_COMMANDS + cpu.x)));
        return;
    }
L_81F16E:
    lda8_v(0x01);  /* 81F16E lda */
    jsl_frame(0x81F170u); abi_TaskWait();  /* 81F170 jsl */
    goto L_81F15F;  /* 81F174 bra */

    /* command 0: a screen loaded */
L_81F180:
    abi_NmiDisable();  /* 81F180 jsl */
    abi_ScreenBlank();  /* 81F184 jsl */
    op_rep(0x10);  /* 81F188 rep */
    ldx16_v(0x4000);  /* 81F18A ldx */
    ldy16_v(0x0000);  /* 81F18D ldy */
    abi_VramFillMap();  /* 81F190 jsl */
    ldx16_v(0x4800);  /* 81F194 ldx */
    ldy16_v(0x0000);  /* 81F197 ldy */
    abi_VramFillMap();  /* 81F19A jsl */
    op_sep(0x10);  /* 81F19E sep */
    abi_TextLayerClear();  /* 81F1A0 jsl */
L_81F1A4:
    lda8_v(0x0E);  /* 81F1A4 lda */
    abi_UnpackSet();  /* 81F1A6 jsl */
    ldx8_v(0x7E);  /* 81F1AA ldx */
    abi_VramCopy();  /* 81F1AC jsl */
    abi_LoadListNext();  /* 81F1B0 jsr */
    lda8(ea_dpi(LOAD_LIST_PTR));  /* 81F1B3 lda */
    abi_UnpackSet();  /* 81F1B5 jsl */
    abi_LoadListNext();  /* 81F1B9 jsr */
    lda8(ea_dpi(LOAD_LIST_PTR));  /* 81F1BC lda */
    tax8();  /* 81F1BE tax */
    abi_VramCopy();  /* 81F1BF jsl */
    abi_LoadListNext();  /* 81F1C3 jsr */
    lda8(ea_dpi(LOAD_LIST_PTR));  /* 81F1C6 lda */
    abi_PaletteLoad();  /* 81F1C8 jsl */
    abi_LoadListNext();  /* 81F1CC jsr */
    lda8(ea_dpi(LOAD_LIST_PTR));  /* 81F1CF lda */
    if (IF_Z) goto L_81F1A4;  /* 81F1D1 beq */
    abi_OamBufferClear();  /* 81F1D3 jsl */
    abi_ScrollStateClear();  /* 81F1D7 jsl */
    abi_NmiEnable();  /* 81F1DB jsl */
    lda8(ea_dpi(LOAD_LIST_PTR));  /* 81F1DF lda */
    abi_SongPlay();  /* 81F1E1 jsl */
    abi_SoundQueueF5WithA();  /* 81F1E5 jsl */
    abi_LoadListNext();  /* 81F1E9 jsr */
    push16(0xF1EE); abi_BrightnessUpAndWait(); cpu.s = (uint16_t)(cpu.s + 2);  /* 81F1EC jsr */
    lda8_v(0x40);  /* 81F1EF lda */
    sta8(ea_dp(CARD_COUNT));  /* 81F1F1 sta */
    goto L_81F15F;  /* 81F1F3 jmp */

    /* command 2: a text typed */
L_81F1F6:
    abi_LoadListNext();  /* 81F1F6 jsr */
    lda8(ea_dpi(LOAD_LIST_PTR));  /* 81F1F9 lda */
    ACC8(asl);  /* 81F1FB asl */
    tax8();  /* 81F1FC tax */
    op_rep(0x20);  /* 81F1FD rep */
    lda16(ea_absx(STORY_TEXTS));  /* 81F1FF lda */
    sta16(ea_dp(TEXT_TYPE_PTR));  /* 81F202 sta */
L_81F204:
    lda16(ea_dpi(TEXT_TYPE_PTR));  /* 81F204 lda */
    sta16(ea_dp(TEXT_AT));  /* 81F206 sta */
    MEM16(inc, ea_dp(TEXT_TYPE_PTR));  /* 81F208 inc */
    MEM16(inc, ea_dp(TEXT_TYPE_PTR));  /* 81F20A inc */
    op_sep(0x20);  /* 81F20C sep */
    lda8(ea_dpi(TEXT_TYPE_PTR));  /* 81F20E lda */
    sta8(ea_dp(TEXT_ATTR));  /* 81F210 sta */
    op_rep(0x20);  /* 81F212 rep */
    lda16_v(0x0004);  /* 81F214 lda */
    sta16(ea_dp(CARD_COUNT));  /* 81F217 sta */
    op_sep(0x20);  /* 81F219 sep */
L_81F21B:
    lda8((uint32_t)PAD1_HELD + 1);  /* 81F21B lda */
    bit8_i(0x10);  /* 81F21F bit */
    if (!IF_Z) goto L_81F2AD;  /* 81F221 beq, 81F223 jmp */
    lda8((uint32_t)PAD1_HELD);  /* 81F226 lda */
    ora8_v(rd8((uint32_t)PAD1_HELD + 1));  /* 81F22A ora */
    bit8_i(0xC0);  /* 81F22E bit */
    if (!IF_Z) goto L_81F23A;  /* 81F230 bne */
    op_rep(0x20);  /* 81F232 rep */
    MEM16(dec, ea_dp(CARD_COUNT));  /* 81F234 dec */
    op_sep(0x20);  /* 81F236 sep */
    if (!IF_Z) goto L_81F276;  /* 81F238 bne */
L_81F23A:
    op_rep(0x20);  /* 81F23A rep */
    lda16_v(0x0004);  /* 81F23C lda */
    sta16(ea_dp(CARD_COUNT));  /* 81F23F sta */
    op_sep(0x20);  /* 81F241 sep */
L_81F243:
    abi_TextTypeNext();  /* 81F243 jsr */
    lda8(ea_dp(TEXT_ATTR));  /* 81F246 lda */
    xba();  /* 81F248 xba */
    lda8(ea_dpi(TEXT_TYPE_PTR));  /* 81F249 lda */
    if (IF_Z) goto L_81F27E;  /* 81F24B beq */
    cmp8_v(0xFC);  /* 81F24D cmp */
    if (IF_C) goto L_81F286;  /* 81F24F bcs */
    cmp8_v(0xBA);  /* 81F251 cmp */
    if (IF_Z) goto L_81F259;  /* 81F253 beq */
    cmp8_v(0xB7);  /* 81F255 cmp */
    if (!IF_Z) goto L_81F265;  /* 81F257 bne */
L_81F259:
    op_rep(0x30);  /* 81F259 rep */
    ldx16(ea_dp(TEXT_AT));  /* 81F25B ldx */
    sta16(ea_longx(TEXT_MARKS));  /* 81F25D sta */
    op_sep(0x30);  /* 81F261 sep */
    goto L_81F243;  /* 81F263 bra */
L_81F265:
    op_rep(0x30);  /* 81F265 rep */
    ldx16(ea_dp(TEXT_AT));  /* 81F267 ldx */
    sta16(ea_longx(TEXT_TILEMAP));  /* 81F269 sta */
    inx16();  /* 81F26D inx */
    inx16();  /* 81F26E inx */
    stx16(ea_dp(TEXT_AT));  /* 81F26F stx */
    op_sep(0x30);  /* 81F271 sep */
    MEM8(inc, ea_abs(TEXT_TILEMAP_UPLOAD));  /* 81F273 inc */
L_81F276:
    lda8_v(0x01);  /* 81F276 lda */
    jsl_frame(0x81F278u); abi_TaskWait();  /* 81F278 jsl */
    goto L_81F21B;  /* 81F27C bra */
L_81F27E:
    abi_TextTypeNext();  /* 81F27E jsr */
    op_rep(0x20);  /* 81F281 rep */
    goto L_81F204;  /* 81F283 jmp */
L_81F286:
    ACC8(inc);  /* 81F286 inc */
    if (IF_N) goto L_81F28F;  /* 81F287 bmi */
    abi_TextLayerClear();  /* 81F289 jsl */
    goto L_81F27E;  /* 81F28D bra */
L_81F28F:
    ACC8(inc);  /* 81F28F inc */
    if (IF_N) goto L_81F2A3;  /* 81F290 bmi */
    abi_TextTypeNext();  /* 81F292 jsr */
    lda8(ea_dpi(TEXT_TYPE_PTR));  /* 81F295 lda */
    sta8(ea_dp(CARD_COUNT));  /* 81F297 sta */
    abi_TextTypeNext();  /* 81F299 jsr */
    lda8(ea_dpi(TEXT_TYPE_PTR));  /* 81F29C lda */
    sta8(ea_dp(CARD_COUNT + 1));  /* 81F29E sta */
    goto L_81F21B;  /* 81F2A0 jmp */
L_81F2A3:
    abi_LoadListNext();  /* 81F2A3 jsr */
    lda8_v(0x01);  /* 81F2A6 lda */
    sta8(ea_dp(CARD_COUNT));  /* 81F2A8 sta */
    goto L_81F15F;  /* 81F2AA jmp */

    /* command 4 (or Start): the card ends, the level starts */
L_81F2AD:
    lda8_v(0x03);  /* 81F2AD lda */
    xba();  /* 81F2AF xba */
    abi_SoundQueueF6With1();  /* 81F2B0 jsl */
    lda8_v(0x30);  /* 81F2B4 lda */
    sta8(ea_dp(CARD_COUNT));  /* 81F2B6 sta */
    do {
        lda8_v(0x01);  /* 81F2B8 lda */
        jsl_frame(0x81F2BAu); abi_TaskWait();  /* 81F2BA jsl */
        MEM8(dec, ea_dp(CARD_COUNT));  /* 81F2BE dec */
    } while (!IF_Z);  /* 81F2C0 bne */
    push16(0xF2C4); abi_BrightnessDownAndWait(); cpu.s = (uint16_t)(cpu.s + 2);  /* 81F2C2 jsr */
    lda8_v(0x30);  /* 81F2C5 lda */
    sta8(ea_dp(CARD_COUNT));  /* 81F2C7 sta */
    do {
        lda8_v(0x01);  /* 81F2C9 lda */
        jsl_frame(0x81F2CBu); abi_TaskWait();  /* 81F2CB jsl */
        MEM8(dec, ea_dp(CARD_COUNT));  /* 81F2CF dec */
    } while (!IF_Z);  /* 81F2D1 bne */
    jsl_frame(0x81F2D3u); abi_TaskExec(); return;  /* 81F2D3 jsl: $81:8181 */

    /* command 6: a fade, and the list goes on */
L_81F2DA:
    abi_LoadListNext();  /* 81F2DA jsr */
    lda8_v(0x03);  /* 81F2DD lda */
    xba();  /* 81F2DF xba */
    abi_SoundQueueF6With1();  /* 81F2E0 jsl */
    lda8_v(0x40);  /* 81F2E4 lda */
    sta8(ea_dp(CARD_COUNT));  /* 81F2E6 sta */
    do {
        lda8_v(0x01);  /* 81F2E8 lda */
        jsl_frame(0x81F2EAu); abi_TaskWait();  /* 81F2EA jsl */
        MEM8(dec, ea_dp(CARD_COUNT));  /* 81F2EE dec */
    } while (!IF_Z);  /* 81F2F0 bne */
    push16(0xF2F4); abi_BrightnessDownAndWait(); cpu.s = (uint16_t)(cpu.s + 2);  /* 81F2F2 jsr */
    lda8_v(0x40);  /* 81F2F5 lda */
    sta8(ea_dp(CARD_COUNT));  /* 81F2F7 sta */
    goto L_81F15F;  /* 81F2F9 jmp */

    /* command 8: on to the final act's scene */
L_81F2FC:
    jsl_frame(0x81F2FCu); abi_TaskExec(); return;  /* 81F2FC jsl: $80:AF42 */
}
