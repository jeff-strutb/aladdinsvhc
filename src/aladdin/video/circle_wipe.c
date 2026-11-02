/*
 * circle_wipe.c: the circle that closes in on a point (the wipe at a level's
 * end), drawn as the window's left and right edge on each scanline.
 */
#include "machine.h"
#include "ops65.h"

/* The circle, drawn by the midpoint method an octant at a time. */
#define CIRCLE_X 0x00002Cu         /* the octant's x, from the radius down (word) */
#define CIRCLE_Y 0x00002Eu         /* the octant's y, from 0 up (word) */
#define CIRCLE_CX 0x000032u        /* the centre's screen x (word) */
#define CIRCLE_CY 0x000034u        /* the centre's scanline (word) */
/* The plot's working bytes. */
#define SPAN_X 0x00000Cu           /* centre x - x, centre x + x (clamped to the screen) */
#define SPAN_Y 0x00000Eu           /* centre x - y, centre x + y */
#define ROW_UP_X 0x000014u         /* scanline centre - x, then its table offset (word) */
#define ROW_DOWN_X 0x000016u       /* scanline centre + x ($FF off screen), then its offset */
#define ROW_UP_Y 0x000018u         /* scanline centre - y, then its offset */
#define ROW_DOWN_Y 0x00001Au       /* scanline centre + y, then its offset */
/* The window's edges per scanline: left, right (a word each). */
#define WINDOW_EDGES 0x7F5000u

/*
 * @routine 81:B2D9 CircleWipePlot
 * WHAT IT DOES: Puts one step of the circle into the window table: its
 * eight symmetric points as the left and right edges of four scanlines.
 * HOW: Clamps centre x -/+ x and -/+ y to 0..$FF, and the scanlines centre
 * -/+ x and -/+ y to $FF when past the screen (scanline $FF is outside the
 * picture); doubles each scanline into a word offset and stores the -/+ y
 * span at scanlines centre -/+ x, the -/+ x span at centre -/+ y.
 * Entry: DB a bank whose $0000-$1FFF is work RAM; called by JSR
 * (CircleWipeStep). Exit: M 16-bit, X 16-bit; A = the -/+ x span (word),
 * X = scanline centre + y's offset; N and Z from the last load; C as the
 * last clamp left it.
 */
void abi_CircleWipePlot(void)
{
    op_sep(0x30);  /* 81B2D9 sep */
    sec();  /* 81B2DB sec */
    lda8(ea_abs(CIRCLE_CX));  /* 81B2DC lda */
    sbc8_v(rd8(ea_abs(CIRCLE_X)));  /* 81B2DF sbc */
    if (IF_C) goto L_81B2E6;  /* 81B2E2 bcs */
    lda8_v(0x00);  /* 81B2E4 lda */
L_81B2E6:
    sta8(ea_abs(SPAN_X));  /* 81B2E6 sta */
    clc();  /* 81B2E9 clc */
    lda8(ea_abs(CIRCLE_CX));  /* 81B2EA lda */
    adc8_v(rd8(ea_abs(CIRCLE_X)));  /* 81B2ED adc */
    if (!IF_C) goto L_81B2F4;  /* 81B2F0 bcc */
    lda8_v(0xFF);  /* 81B2F2 lda */
L_81B2F4:
    sta8(ea_abs((SPAN_X + 1)));  /* 81B2F4 sta */
    sec();  /* 81B2F7 sec */
    lda8(ea_abs(CIRCLE_CX));  /* 81B2F8 lda */
    sbc8_v(rd8(ea_abs(CIRCLE_Y)));  /* 81B2FB sbc */
    if (IF_C) goto L_81B302;  /* 81B2FE bcs */
    lda8_v(0x00);  /* 81B300 lda */
L_81B302:
    sta8(ea_abs(SPAN_Y));  /* 81B302 sta */
    clc();  /* 81B305 clc */
    lda8(ea_abs(CIRCLE_CX));  /* 81B306 lda */
    adc8_v(rd8(ea_abs(CIRCLE_Y)));  /* 81B309 adc */
    if (!IF_C) goto L_81B310;  /* 81B30C bcc */
    lda8_v(0xFF);  /* 81B30E lda */
L_81B310:
    sta8(ea_abs((SPAN_Y + 1)));  /* 81B310 sta */
    sec();  /* 81B313 sec */
    lda8(ea_abs(CIRCLE_CY));  /* 81B314 lda */
    sbc8_v(rd8(ea_abs(CIRCLE_X)));  /* 81B317 sbc */
    if (IF_C) goto L_81B31E;  /* 81B31A bcs */
    lda8_v(0xFF);  /* 81B31C lda */
L_81B31E:
    sta8(ea_abs(ROW_UP_X));  /* 81B31E sta */
    clc();  /* 81B321 clc */
    lda8(ea_abs(CIRCLE_CY));  /* 81B322 lda */
    adc8_v(rd8(ea_abs(CIRCLE_X)));  /* 81B325 adc */
    if (!IF_C) goto L_81B32C;  /* 81B328 bcc */
    lda8_v(0xFF);  /* 81B32A lda */
L_81B32C:
    sta8(ea_abs(ROW_DOWN_X));  /* 81B32C sta */
    sec();  /* 81B32F sec */
    lda8(ea_abs(CIRCLE_CY));  /* 81B330 lda */
    sbc8_v(rd8(ea_abs(CIRCLE_Y)));  /* 81B333 sbc */
    if (IF_C) goto L_81B33A;  /* 81B336 bcs */
    lda8_v(0xFF);  /* 81B338 lda */
L_81B33A:
    sta8(ea_abs(ROW_UP_Y));  /* 81B33A sta */
    clc();  /* 81B33D clc */
    lda8(ea_abs(CIRCLE_CY));  /* 81B33E lda */
    adc8_v(rd8(ea_abs(CIRCLE_Y)));  /* 81B341 adc */
    if (!IF_C) goto L_81B348;  /* 81B344 bcc */
    lda8_v(0xFF);  /* 81B346 lda */
L_81B348:
    sta8(ea_abs(ROW_DOWN_Y));  /* 81B348 sta */
    stz8(ea_abs((ROW_UP_X + 1)));  /* 81B34B stz */
    stz8(ea_abs((ROW_DOWN_X + 1)));  /* 81B34E stz */
    stz8(ea_abs((ROW_UP_Y + 1)));  /* 81B351 stz */
    stz8(ea_abs((ROW_DOWN_Y + 1)));  /* 81B354 stz */
    op_rep(0x30);  /* 81B357 rep */
    MEM16(asl, ea_abs(ROW_UP_X));  /* 81B359 asl */
    MEM16(asl, ea_abs(ROW_DOWN_X));  /* 81B35C asl */
    MEM16(asl, ea_abs(ROW_UP_Y));  /* 81B35F asl */
    MEM16(asl, ea_abs(ROW_DOWN_Y));  /* 81B362 asl */
    ldx16(ea_abs(ROW_UP_X));  /* 81B365 ldx */
    lda16(ea_abs(SPAN_Y));  /* 81B368 lda */
    sta16(ea_longx(WINDOW_EDGES));  /* 81B36B sta */
    ldx16(ea_abs(ROW_DOWN_X));  /* 81B36F ldx */
    lda16(ea_abs(SPAN_Y));  /* 81B372 lda */
    sta16(ea_longx(WINDOW_EDGES));  /* 81B375 sta */
    ldx16(ea_abs(ROW_UP_Y));  /* 81B379 ldx */
    lda16(ea_abs(SPAN_X));  /* 81B37C lda */
    sta16(ea_longx(WINDOW_EDGES));  /* 81B37F sta */
    ldx16(ea_abs(ROW_DOWN_Y));  /* 81B383 ldx */
    lda16(ea_abs(SPAN_X));  /* 81B386 lda */
    sta16(ea_longx(WINDOW_EDGES));  /* 81B389 sta */
    return;  /* 81B38D rts */
}

