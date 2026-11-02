/*
 * level_events.c: levels' event tasks that wait for the camera or the
 * player to get somewhere, then change the backdrop, the colours or the
 * map.
 */
#include "machine.h"
#include "ops65.h"
void abi_BandsSpeedUpTask(void);
void abi_GreyPastTask(void);
void abi_RedOutTask(void);
void abi_PaletteRedDown(void);
void abi_ClimbTopTask(void);
void abi_TaskWait(void);
void abi_TaskExit(void);
#include "wram.h"
#include "wram_video.h"

/*
 * @routine 81:A0C3 BandsSpeedUpTask
 * WHAT IT DOES: Level $06's task that speeds the backdrop bands up once
 * the camera gets far into the level, and keeps them at it.
 * HOW: The 11 bands' speeds from the table at $80:C5D6; then, each frame,
 * until layer 0's scroll x reaches $0680, nothing; then every frame each
 * speed grows by its step ($80:C5C0) up to its most ($80:C5EC).
 * Entry: the task's start: M 8-bit, X 8-bit; DB = $80. Exit: none: it
 * runs on until the level ends.
 */
void abi_BandsSpeedUpTask(void)
{
    op_rep(0x20);  /* 81A0C3 rep */
    lda16_v(0x000B);  /* 81A0C5 lda */
    sta16(ea_abs(SCRATCH));  /* 81A0C8 sta */
    ldx8_v(0x00);  /* 81A0CB ldx */
    txy8();  /* 81A0CD txy */
L_81A0CE:
    lda16(ea_absy(0xC5D6));  /* 81A0CE lda */
    sta16(ea_longx(BAND_SPEED));  /* 81A0D1 sta */
    iny8();  /* 81A0D5 iny */
    iny8();  /* 81A0D6 iny */
    inx8();  /* 81A0D7 inx */
    inx8();  /* 81A0D8 inx */
    inx8();  /* 81A0D9 inx */
    inx8();  /* 81A0DA inx */
    MEM16(dec, ea_abs(SCRATCH));  /* 81A0DB dec */
    if (!IF_Z) goto L_81A0CE;  /* 81A0DE bne */
    op_sep(0x20);  /* 81A0E0 sep */
    op_rep(0x10);  /* 81A0E2 rep */
L_81A0E4:
    lda8_v(0x01);  /* 81A0E4 lda */
    jsl_frame(0x81A0E6u); abi_TaskWait();  /* 81A0E6 jsl */
    ldx16(ea_abs(0x1773));  /* 81A0EA ldx */
    cpx16_v(0x0680);  /* 81A0ED cpx */
    if (!IF_C) goto L_81A0E4;  /* 81A0F0 bcc */
    op_sep(0x10);  /* 81A0F2 sep */
L_81A0F4:
    op_rep(0x20);  /* 81A0F4 rep */
    lda16_v(0x000B);  /* 81A0F6 lda */
    sta16(ea_abs(SCRATCH));  /* 81A0F9 sta */
    ldx8_v(0x00);  /* 81A0FC ldx */
    txy8();  /* 81A0FE txy */
L_81A0FF:
    clc();  /* 81A0FF clc */
    lda16(ea_longx(BAND_SPEED));  /* 81A100 lda */
    adc16_v(rd16(ea_absy(0xC5C0)));  /* 81A104 adc */
    cmp16_v(rd16(ea_absy(0xC5EC)));  /* 81A107 cmp */
    if (!IF_C) goto L_81A10F;  /* 81A10A bcc */
    lda16(ea_absy(0xC5EC));  /* 81A10C lda */
L_81A10F:
    sta16(ea_longx(BAND_SPEED));  /* 81A10F sta */
    iny8();  /* 81A113 iny */
    iny8();  /* 81A114 iny */
    inx8();  /* 81A115 inx */
    inx8();  /* 81A116 inx */
    inx8();  /* 81A117 inx */
    inx8();  /* 81A118 inx */
    MEM16(dec, ea_abs(SCRATCH));  /* 81A119 dec */
    if (!IF_Z) goto L_81A0FF;  /* 81A11C bne */
    op_sep(0x20);  /* 81A11E sep */
    lda8_v(0x01);  /* 81A120 lda */
    jsl_frame(0x81A122u); abi_TaskWait();  /* 81A122 jsl */
    goto L_81A0F4;  /* 81A126 bra */
}

/*
 * @routine 81:A128 GreyPastTask
 * WHAT IT DOES: A level's task that greys the screen down a little once
 * the player gets past x $0E00, then ends.
 * HOW: Each frame until the player's x reaches $0E00, nothing; then colour
 * math subtracts a grey of 4 (CGADSUB's shadow $82, CGWSEL's 0, WOBJSEL's
 * $80, the fixed colour 4, 4, 4), the palette upload is asked for, and
 * TaskExit.
 * Entry: the task's start: M 8-bit; DB a bank whose $0000-$1FFF is work
 * RAM. Exit: none: the task ends.
 */
void abi_GreyPastTask(void)
{
    op_rep(0x10);  /* 81A128 rep */
L_81A12A:
    lda8_v(0x01);  /* 81A12A lda */
    jsl_frame(0x81A12Cu); abi_TaskWait();  /* 81A12C jsl */
    ldx16(ea_abs(0x08E7));  /* 81A130 ldx */
    cpx16_v(0x0E00);  /* 81A133 cpx */
    if (!IF_C) goto L_81A12A;  /* 81A136 bcc */
    lda8_v(0x82);  /* 81A138 lda */
    sta8(ea_abs(CGADSUB_SHADOW));  /* 81A13A sta */
    stz8(ea_abs(CGWSEL_SHADOW));  /* 81A13D stz */
    lda8_v(0x80);  /* 81A140 lda */
    sta8(ea_abs(WOBJSEL_SHADOW));  /* 81A142 sta */
    lda8_v(0x04);  /* 81A145 lda */
    sta8(ea_abs(COLDATA_RED));  /* 81A147 sta */
    lda8_v(0x04);  /* 81A14A lda */
    sta8(ea_abs(COLDATA_GREEN));  /* 81A14C sta */
    lda8_v(0x04);  /* 81A14F lda */
    sta8(ea_abs(COLDATA_BLUE));  /* 81A151 sta */
    MEM8(inc, ea_abs(PALETTE_UPLOAD));  /* 81A154 inc */
    abi_TaskExit(); return;  /* 81A157 jml */
}

/*
 * @routine 81:A15B RedOutTask
 * WHAT IT DOES: Level $07's task that, once the camera passes x $1A00,
 * takes the red out of palette 2 and adds blue over 8 steps a quarter
 * second apart, then ends.
 * HOW: Each frame until layer 0's scroll x reaches $1A00, nothing; then
 * colour math adds the fixed colour to BG1 (CGADSUB's shadow 1, CGWSEL's
 * 0, WOBJSEL's $80, $0080 cleared); 7 times: 15 frames, PaletteRedDown and
 * the fixed colour's blue up by one; then PaletteRedDown again and
 * TaskExit.
 * Entry: the task's start: M 8-bit; DB a bank whose $0000-$1FFF is work
 * RAM. Exit: none: the task ends.
 */
void abi_RedOutTask(void)
{
    op_rep(0x10);  /* 81A15B rep */
L_81A15D:
    lda8_v(0x01);  /* 81A15D lda */
    jsl_frame(0x81A15Fu); abi_TaskWait();  /* 81A15F jsl */
    ldx16(ea_abs(0x1773));  /* 81A163 ldx */
    cpx16_v(0x1A00);  /* 81A166 cpx */
    if (!IF_C) goto L_81A15D;  /* 81A169 bcc */
    lda8_v(0x01);  /* 81A16B lda */
    sta8(ea_abs(CGADSUB_SHADOW));  /* 81A16D sta */
    stz8(ea_abs(CGWSEL_SHADOW));  /* 81A170 stz */
    stz8(ea_abs(0x0080));  /* 81A173 stz */
    lda8_v(0x80);  /* 81A176 lda */
    sta8(ea_abs(WOBJSEL_SHADOW));  /* 81A178 sta */
    ldy16_v(0x0007);  /* 81A17B ldy */
    ldx16_v(0x0008);  /* 81A17E ldx */
L_81A181:
    lda8_v(0x0F);  /* 81A181 lda */
    jsl_frame(0x81A183u); abi_TaskWait();  /* 81A183 jsl */
    abi_PaletteRedDown();  /* 81A187 jsr */
    MEM8(inc, ea_abs(COLDATA_BLUE));  /* 81A18A inc */
    dey16();  /* 81A18D dey */
    if (!IF_Z) goto L_81A181;  /* 81A18E bne */
    abi_PaletteRedDown();  /* 81A190 jsr */
    abi_TaskExit(); return;  /* 81A193 jml */
}

/*
 * @routine 81:A197 PaletteRedDown
 * WHAT IT DOES: Takes one step of red from each colour of palette 2
 * (floor 0) and asks for the upload, unless X is 0.
 * HOW: With X nonzero (kept on the stack), for the 16 colours from the
 * last: the low byte's red (low 5 bits) down by one, not below 0, its
 * other bits kept (scratch $0C); then the palette upload is asked for.
 * Entry: M 8-bit, X 16-bit; X = nonzero to act; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR. Exit: X as at entry; with it
 * nonzero, A low byte = colour 0's new low byte; scratch $0C its other
 * bits.
 */
void abi_PaletteRedDown(void)
{
    txa8();  /* 81A197 txa */
    if (IF_Z) goto L_81A1BF;  /* 81A198 beq */
    phx16();  /* 81A19A phx */
    ldx16_v(0x001E);  /* 81A19B ldx */
L_81A19E:
    lda8(ea_longx(0x7EFD40u));  /* 81A19E lda */
    pha8();  /* 81A1A2 pha */
    and8_v(0xE0);  /* 81A1A3 and */
    sta8(ea_abs(SCRATCH));  /* 81A1A5 sta */
    pla8();  /* 81A1A8 pla */
    and8_v(0x1F);  /* 81A1A9 and */
    ACC8(dec);  /* 81A1AB dec */
    if (!IF_N) goto L_81A1B0;  /* 81A1AC bpl */
    lda8_v(0x00);  /* 81A1AE lda */
L_81A1B0:
    ora8_v(rd8(ea_abs(SCRATCH)));  /* 81A1B0 ora */
    sta8(ea_longx(0x7EFD40u));  /* 81A1B3 sta */
    dex16();  /* 81A1B7 dex */
    dex16();  /* 81A1B8 dex */
    if (!IF_N) goto L_81A19E;  /* 81A1B9 bpl */
    MEM8(inc, ea_abs(PALETTE_UPLOAD));  /* 81A1BB inc */
    plx16();  /* 81A1BE plx */
L_81A1BF:
    return;  /* 81A1BF rts */
}

/*
 * @routine 81:A30D ClimbTopTask
 * WHAT IT DOES: Level $0A's (and $19's) task for the climb: puts the
 * player's sprites in front while near the top, and once the player
 * stands at the top changes the colour math and opens the way right.
 * HOW: Each frame (until the restart point is set): the player's sprite
 * priority bits ($08E4 bits 4-5) = $10 when its y is under 256, else 0;
 * on when the camera is at the top (scroll y 0), the player's x low byte
 * $80 or more and y low byte under $78. Then the scroll y limit's high
 * byte cleared and, not on level $19, $0399 = $17, $039A = $10, CGWSEL's
 * shadow 3, CGADSUB's $53; each frame until the scroll x's high byte is
 * 1; then the block map's byte at $7F:F043 = $FE and TaskExit.
 * Entry: the task's start: M 8-bit, X 8-bit; DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: the task ends.
 */
void abi_ClimbTopTask(void)
{
L_81A30D:
    lda8_v(0x01);  /* 81A30D lda */
    jsl_frame(0x81A30Fu); abi_TaskWait();  /* 81A30F jsl */
    lda8(ea_abs(0x0360));  /* 81A313 lda */
    if (!IF_Z) goto L_81A345;  /* 81A316 bne */
    lda8(ea_abs(0x08E4));  /* 81A318 lda */
    and8_v(0xCF);  /* 81A31B and */
    sta8(ea_abs(0x08E4));  /* 81A31D sta */
    lda8_v(0x00);  /* 81A320 lda */
    ldx8(ea_abs(0x08EB));  /* 81A322 ldx */
    if (!IF_Z) goto L_81A329;  /* 81A325 bne */
    lda8_v(0x10);  /* 81A327 lda */
L_81A329:
    ora8_v(rd8(ea_abs(0x08E4)));  /* 81A329 ora */
    sta8(ea_abs(0x08E4));  /* 81A32C sta */
    lda8(ea_abs(0x1777));  /* 81A32F lda */
    ora8_v(rd8(ea_abs(0x1778)));  /* 81A332 ora */
    if (!IF_Z) goto L_81A30D;  /* 81A335 bne */
    lda8(ea_abs(0x08E7));  /* 81A337 lda */
    cmp8_v(0x80);  /* 81A33A cmp */
    if (!IF_C) goto L_81A30D;  /* 81A33C bcc */
    lda8(ea_abs(0x08EA));  /* 81A33E lda */
    cmp8_v(0x78);  /* 81A341 cmp */
    if (IF_C) goto L_81A30D;  /* 81A343 bcs */
L_81A345:
    stz8(ea_abs(0x1A07));  /* 81A345 stz */
    lda8(ea_abs(LEVEL));  /* 81A348 lda */
    cmp8_v(0x19);  /* 81A34B cmp */
    if (IF_Z) goto L_81A363;  /* 81A34D beq */
    lda8_v(0x17);  /* 81A34F lda */
    sta8(ea_abs(0x0399));  /* 81A351 sta */
    lda8_v(0x10);  /* 81A354 lda */
    sta8(ea_abs(0x039A));  /* 81A356 sta */
    lda8_v(0x03);  /* 81A359 lda */
    sta8(ea_abs(CGWSEL_SHADOW));  /* 81A35B sta */
    lda8_v(0x53);  /* 81A35E lda */
    sta8(ea_abs(CGADSUB_SHADOW));  /* 81A360 sta */
L_81A363:
    lda8_v(0x01);  /* 81A363 lda */
    jsl_frame(0x81A365u); abi_TaskWait();  /* 81A365 jsl */
    lda8(ea_abs(0x1774));  /* 81A369 lda */
    cmp8_v(0x01);  /* 81A36C cmp */
    if (!IF_Z) goto L_81A363;  /* 81A36E bne */
    lda8_v(0xFE);  /* 81A370 lda */
    sta8(0x7FF043u);  /* 81A372 sta */
    abi_TaskExit(); return;  /* 81A376 jml */
}
