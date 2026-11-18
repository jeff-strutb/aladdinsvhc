/*
 * continue_screen.c: the screen between a lost life and the next try,
 * where the continues left are shown and one can be spent.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "native.h"
#include "actor.h"
#include "wram.h"
#include "wram_state.h"
#include "wram_video.h"

#define CHOICE_CONTINUE 0x0076u   /* set when Continue is chosen */
#define CHOICE_END 0x0077u        /* set when End is chosen (or no continues are left) */
#define WAIT_LEFT 0x007Eu         /* frames the wait with no continues still runs */
#define SLOT1_STATE (TASK_STATE + 24) /* slot 1's state: the fades run there */
#define PAD1_PRESSED_HIGH (JOY_PRESSED + 1) /* bit 4: Start newly pressed */
#define V_035D 0x035Du            /* cleared with the level on End; purpose not yet established */
#define V_0360 0x0360u            /* cleared here; purpose not yet established */
#define V_0369 0x0369u            /* cleared here; purpose not yet established */
#define V_036F 0x036Fu            /* 1 when play goes on from a continue */
#define V_0373 0x0373u            /* cleared on Continue; purpose not yet established */
#define V_0374 0x0374u
#define V_0375 0x0375u
#define V_03A0 0x03A0u            /* copied to V_03A1 here; purpose not yet established */
#define V_03A1 0x03A1u
#define V_142E 0x142Eu            /* counted up while the screen shows; purpose not yet established */

void abi_ContinuesShow(void);
void abi_FrameEndTimes(void);
void abi_LevelGraphicsLoadAllLong(void);
void abi_LevelRamSetupLong(void);
void abi_NmiDisable(void);
void abi_NmiEnable(void);
void abi_PaletteLoad(void);
void abi_ScreenBlank(void);
void abi_SongPlay(void);
void abi_SoundQueueF6With1(void);
void abi_TaskCreate(void);
void abi_TaskExec(void);
void abi_TextLayoutShow(void);
void abi_UnpackSet(void);
void abi_VramCopy(void);
void abi_VramCopy2(void);
void abi_VramFillMap(void);
void abi_WramC800Clear(void);

/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/* Where the continues digit sits in the text buffer: row 26, column 29. */
#define CONTINUES_DIGIT (TEXT_TILEMAP + 1722)
#define TILE_DIGIT_0 0x30

/*
 * @routine 80:B441 ContinuesShow
 * WHAT IT DOES: Shows the number of continues left on the continue screen.
 * HOW: The digit's tile is $30 plus the count (decimal when D is set, as
 * the CPU adds); it goes into the text buffer and the buffer's upload is
 * asked for.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: A low =
 * the tile; V and C from the addition; N and Z from the pending flag's
 * new value. A high, X, Y and the other flags unchanged.
 */
void abi_ContinuesShow(void)
{
    uint8_t a;

    cpu.p &= (uint8_t)~P_C;
    a = rd8(CONTINUES);
    a = op_adc8(a, TILE_DIGIT_0, &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    wr8(CONTINUES_DIGIT, a);
    a = (uint8_t)(rd8_rmw(TEXT_TILEMAP_UPLOAD) + 1);
    wr8(TEXT_TILEMAP_UPLOAD, a);
    cpu.p = nz8(cpu.p, a);
}

/*
 * @routine 80:B44F WramC800Clear
 * WHAT IT DOES: Clears the 4 KB buffer at $7E:C800 before a level starts.
 * HOW: Zeroes $7E:C800-$7E:D7FF a byte at a time with 16-bit X; P comes
 * back from the stack.
 * Entry: M 8-bit. Exit: A low = 0; X = 0 when it was 8-bit at entry,
 * $1000 when 16-bit; P as at entry. A high and Y unchanged.
 */
void abi_WramC800Clear(void)
{
    uint8_t p = cpu.p;
    uint16_t x;

    op_rep(P_X);
    for (x = 0; x < 0x1000; x++)
        wr8(TILE_BEHAVIOUR + x, 0);
    cpu.x = x;
    cpu.a &= 0xFF00;
    op_plp(p);
}

/*
 * @routine 80:B2E3 ContinueScreenTask
 * WHAT IT DOES: The continue screen, after Aladdin's last life is lost: the
 * screen built and faded in with the continues left shown; with none left,
 * a wait (up to $F0 frames, or until Start); else the choice (Continue or
 * End) by the actor on the screen, a continue spent on Continue. Then the
 * fade-out, and the game goes on (GameStartTask) or the title comes back
 * (TitleStartTask, the level cleared).
 * HOW: Song $2B; NmiDisable, ScreenBlank; level $25 set up; V_0369 and
 * V_0360 cleared; Aladdin's record new (state $0C) with $07 = 6 (continues
 * left) or $0F (none); V_142E counted up; both tilemaps ($4000, $4800)
 * filled; graphics sets 1, 8 (copy $5B, or $77 as a second copy when none
 * are left), $62, $F5, $0E, $31; palettes $60, $4A; V_03A1 = V_03A0;
 * LevelGraphicsLoadAllLong; NmiEnable; WramC800Clear; text layouts $5A and
 * $3E around the fade-in task; ContinuesShow. None left: CHOICE_END = 1,
 * then 64 frames and up to $F0 more (Start ends them). Else: the choice
 * cleared, layout $0A, a frame at a time until CHOICE_CONTINUE or
 * CHOICE_END is set; Continue: CONTINUES down one and shown. Then 40
 * frames; sound $F6 with 1; the fade-out task; frames until slot 1 is
 * free; V_142E cleared. CHOICE_END: LEVEL_035C, V_035D, LEVEL, V_0360 and
 * V_036F cleared, TaskExec TitleStartTask. Else V_036F = 1, V_0373-V_0375
 * cleared, TaskExec GameStartTask.
 * Entry: a task's start (TaskExec); M 8-bit, X 8-bit; DB a bank whose
 * $0000-$1FFF is work RAM. Exit: never returns.
 */
void abi_ContinueScreenTask(void)
{
    lda8_v(0x2B);  /* 80B2E3 lda */
    jsl_call(0x80B2E5u, abi_SongPlay);  /* 80B2E5 jsl */
    jsl_call(0x80B2E9u, abi_NmiDisable);  /* 80B2E9 jsl */
    jsl_call(0x80B2EDu, abi_ScreenBlank);  /* 80B2ED jsl */
    lda8_v(0x25);  /* 80B2F1 lda */
    sta8(ea_abs(LEVEL));  /* 80B2F3 sta */
    jsl_frame(0x80B2F6u); abi_LevelRamSetupLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80B2F6 jsl */
    stz8(ea_abs(V_0369));  /* 80B2FA stz */
    stz8(ea_abs(V_0360));  /* 80B2FD stz */
    lda8_v(0x06);  /* 80B300 lda */
    ldx8(ea_abs(CONTINUES));  /* 80B302 ldx */
    if (!IF_Z) goto L_80B309;  /* 80B305 bne */
    lda8_v(0x0F);  /* 80B307 lda */
L_80B309:
    sta8(ea_abs((PLAYER + ACTOR_07)));  /* 80B309 sta */
    lda8_v(0x0C);  /* 80B30C lda */
    sta8(ea_abs(PLAYER + ACTOR_STATE));  /* 80B30E sta */
    MEM8(inc, ea_abs(V_142E));  /* 80B311 inc */
    op_rep(0x10);  /* 80B314 rep */
    ldx16_v(0x4000);  /* 80B316 ldx */
    jsl_call(0x80B319u, abi_VramFillMap);  /* 80B319 jsl */
    ldx16_v(0x4800);  /* 80B31D ldx */
    jsl_call(0x80B320u, abi_VramFillMap);  /* 80B320 jsl */
    op_sep(0x10);  /* 80B324 sep */
    lda8_v(0x01);  /* 80B326 lda */
    jsl_call(0x80B328u, abi_UnpackSet);  /* 80B328 jsl */
    ldx8_v(0x00);  /* 80B32C ldx */
    jsl_call(0x80B32Eu, abi_VramCopy);  /* 80B32E jsl */
    lda8_v(0x08);  /* 80B332 lda */
    jsl_call(0x80B334u, abi_UnpackSet);  /* 80B334 jsl */
    ldx8(ea_abs(CONTINUES));  /* 80B338 ldx */
    if (!IF_Z) goto L_80B345;  /* 80B33B bne */
    ldx8_v(0x77);  /* 80B33D ldx */
    jsl_call(0x80B33Fu, abi_VramCopy2);  /* 80B33F jsl */
    goto L_80B34B;  /* 80B343 bra */
L_80B345:
    ldx8_v(0x5B);  /* 80B345 ldx */
    jsl_call(0x80B347u, abi_VramCopy);  /* 80B347 jsl */
L_80B34B:
    ldx8_v(0x62);  /* 80B34B ldx */
    jsl_call(0x80B34Du, abi_VramCopy);  /* 80B34D jsl */
    ldx8_v(0xF5);  /* 80B351 ldx */
    jsl_call(0x80B353u, abi_VramCopy);  /* 80B353 jsl */
    lda8_v(0x0E);  /* 80B357 lda */
    jsl_call(0x80B359u, abi_UnpackSet);  /* 80B359 jsl */
    ldx8_v(0x7E);  /* 80B35D ldx */
    jsl_call(0x80B35Fu, abi_VramCopy);  /* 80B35F jsl */
    lda8_v(0x31);  /* 80B363 lda */
    jsl_call(0x80B365u, abi_UnpackSet);  /* 80B365 jsl */
    ldx8_v(0x23);  /* 80B369 ldx */
    jsl_call(0x80B36Bu, abi_VramCopy);  /* 80B36B jsl */
    lda8_v(0x60);  /* 80B36F lda */
    jsl_call(0x80B371u, abi_PaletteLoad);  /* 80B371 jsl */
    lda8_v(0x4A);  /* 80B375 lda */
    jsl_call(0x80B377u, abi_PaletteLoad);  /* 80B377 jsl */
    lda8(ea_abs(V_03A0));  /* 80B37B lda */
    sta8(ea_abs(V_03A1));  /* 80B37E sta */
    jsl_call(0x80B381u, abi_LevelGraphicsLoadAllLong);  /* 80B381 jsl */
    jsl_call(0x80B385u, abi_NmiEnable);  /* 80B385 jsl */
    call_frame(0xB38B, abi_WramC800Clear);  /* 80B389 jsr */
    lda8_v(0x5A);  /* 80B38C lda */
    jsl_call(0x80B38Eu, abi_TextLayoutShow);  /* 80B38E jsl */
    jsl_frame(0x80B392u); abi_TaskCreate(); cpu.unwound = 0;  /* 80B392 jsl */
    lda8_v(0x3E);  /* 80B39B lda */
    jsl_call(0x80B39Du, abi_TextLayoutShow);  /* 80B39D jsl */
    call_frame(0xB3A3, abi_ContinuesShow);  /* 80B3A1 jsr */
    ldx8(ea_abs(CONTINUES));  /* 80B3A4 ldx */
    if (!IF_Z) goto L_80B3CE;  /* 80B3A7 bne */
    lda8_v(0x01);  /* 80B3A9 lda */
    sta8(ea_abs(CHOICE_END));  /* 80B3AB sta */
    stz8(ea_abs(CHOICE_CONTINUE));  /* 80B3AE stz */
    ldx8_v(0x40);  /* 80B3B1 ldx */
    call_frame(0xB3B5, abi_FrameEndTimes);  /* 80B3B3 jsr */
    lda8_v(0xF0);  /* 80B3B6 lda */
    sta8(ea_abs(WAIT_LEFT));  /* 80B3B8 sta */
L_80B3BB:
    ldx8_v(0x01);  /* 80B3BB ldx */
    call_frame(0xB3BF, abi_FrameEndTimes);  /* 80B3BD jsr */
    lda8(ea_abs(PAD1_PRESSED_HIGH));  /* 80B3C0 lda */
    and8_v(0x10);  /* 80B3C3 and */
    if (!IF_Z) goto L_80B3CC;  /* 80B3C5 bne */
    MEM8(dec, ea_abs(WAIT_LEFT));  /* 80B3C7 dec */
    if (!IF_Z) goto L_80B3BB;  /* 80B3CA bne */
L_80B3CC:
    goto L_80B3F2;  /* 80B3CC bra */
L_80B3CE:
    stz8(ea_abs(CHOICE_CONTINUE));  /* 80B3CE stz */
    stz8(ea_abs(CHOICE_END));  /* 80B3D1 stz */
    lda8_v(0x0A);  /* 80B3D4 lda */
    jsl_call(0x80B3D6u, abi_TextLayoutShow);  /* 80B3D6 jsl */
L_80B3DA:
    ldx8_v(0x01);  /* 80B3DA ldx */
    call_frame(0xB3DE, abi_FrameEndTimes);  /* 80B3DC jsr */
    lda8(ea_abs(CHOICE_CONTINUE));  /* 80B3DF lda */
    ora8_v(rd8(ea_abs(CHOICE_END)));  /* 80B3E2 ora */
    if (IF_Z) goto L_80B3DA;  /* 80B3E5 beq */
    lda8(ea_abs(CHOICE_CONTINUE));  /* 80B3E7 lda */
    if (IF_Z) goto L_80B3F2;  /* 80B3EA beq */
    MEM8(dec, ea_abs(CONTINUES));  /* 80B3EC dec */
    call_frame(0xB3F1, abi_ContinuesShow);  /* 80B3EF jsr */
L_80B3F2:
    ldx8_v(0x28);  /* 80B3F2 ldx */
    call_frame(0xB3F6, abi_FrameEndTimes);  /* 80B3F4 jsr */
    jsl_call(0x80B3F7u, abi_SoundQueueF6With1);  /* 80B3F7 jsl */
    jsl_frame(0x80B3FBu); abi_TaskCreate(); cpu.unwound = 0;  /* 80B3FB jsl */
L_80B404:
    ldx8_v(0x01);  /* 80B404 ldx */
    call_frame(0xB408, abi_FrameEndTimes);  /* 80B406 jsr */
    lda8(ea_abs(SLOT1_STATE));  /* 80B409 lda */
    if (!IF_Z) goto L_80B404;  /* 80B40C bne */
    stz8(ea_abs(V_142E));  /* 80B40E stz */
    ldx8(ea_abs(CHOICE_END));  /* 80B411 ldx */
    if (IF_Z) goto L_80B42C;  /* 80B414 beq */
    stz8(ea_abs(LEVEL_035C));  /* 80B416 stz */
    stz8(ea_abs(V_035D));  /* 80B419 stz */
    stz8(ea_abs(LEVEL));  /* 80B41C stz */
    stz8(ea_abs(V_0360));  /* 80B41F stz */
    stz8(ea_abs(V_036F));  /* 80B422 stz */
    jsl_frame(0x80B425u); abi_TaskExec(); return;  /* 80B425 jsl */
L_80B42C:
    lda8_v(0x01);  /* 80B42C lda */
    sta8(ea_abs(V_036F));  /* 80B42E sta */
    stz8(ea_abs(V_0373));  /* 80B431 stz */
    stz8(ea_abs(V_0374));  /* 80B434 stz */
    stz8(ea_abs(V_0375));  /* 80B437 stz */
    jsl_frame(0x80B43Au); abi_TaskExec(); return;  /* 80B43A jsl */
}
