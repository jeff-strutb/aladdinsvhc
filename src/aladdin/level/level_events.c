/*
 * level_events.c: levels' event tasks that wait for the camera or the
 * player to get somewhere, then change the backdrop, the colours or the
 * map.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "cpu_ops.h"
#include "native.h"
void abi_BandsSpeedUpTask(void);
void abi_GreyPastTask(void);
void abi_RedOutTask(void);
void abi_PaletteRedDown(void);
void abi_ClimbTopTask(void);
void abi_TaskWait(void);
void abi_TaskExit(void);
#include "wram.h"
#include "wram_video.h"

/* The task's next frame (its JSL returning at `site`). */
static void next_frame(uint32_t site, uint8_t frames)
{
    a8_load(frames);
    jsl_frame(site);
    abi_TaskWait();
}

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
    uint8_t band, x, y;
    uint16_t speed, most;

    /* the 11 bands' starting speeds */
    op_rep(P_M);
    wr16(dba(SCRATCH), 0x000B);
    for (band = 0, x = 0, y = 0; band < 11; band++, x += 4, y += 2) {
        cpu.a = rd16(dba((uint16_t)(0xC5D6 + y)));
        wr16(BAND_SPEED + x, cpu.a);
        wr16(dba(SCRATCH), (uint16_t)(10 - band));
    }
    cpu.x = x;
    cpu.y = y;
    op_sep(P_M);
    op_rep(P_X);
    /* until the camera is far enough in */
    do {
        next_frame(0x81A0E6u, 0x01);
        cpu.x = rd16(dba(0x1773));
        cmp16_of(cpu.x, 0x0680);
    } while (cpu.x < 0x0680);
    op_sep(P_X);
    /* then each frame each speed up by its step, to its most */
    for (;;) {
        op_rep(P_M);
        for (band = 0, x = 0, y = 0; band < 11; band++, x += 4, y += 2) {
            flag_set(P_C, 0);
            speed = op_adc16(rd16(BAND_SPEED + x), rd16(dba((uint16_t)(0xC5C0 + y))), &cpu.p);
            most = rd16(dba((uint16_t)(0xC5EC + y)));
            cmp16_of(speed, most);
            if (speed >= most)
                speed = most;
            cpu.a = speed;
            wr16(BAND_SPEED + x, speed);
            wr16(dba(SCRATCH), (uint16_t)(10 - band));
        }
        cpu.x = x;
        cpu.y = y;
        op_sep(P_M);
        next_frame(0x81A122u, 0x01);
    }
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
    op_rep(P_X);
    do {
        next_frame(0x81A12Cu, 0x01);
        cpu.x = rd16(dba(0x08E7));
        cmp16_of(cpu.x, 0x0E00);
    } while (cpu.x < 0x0E00);
    /* colour math takes a grey of 4 off */
    wr8(dba(CGADSUB_SHADOW), 0x82);
    wr8(dba(CGWSEL_SHADOW), 0x00);
    wr8(dba(WOBJSEL_SHADOW), 0x80);
    wr8(dba(COLDATA_RED), 0x04);
    wr8(dba(COLDATA_GREEN), 0x04);
    a8_load(0x04);
    wr8(dba(COLDATA_BLUE), 0x04);
    inc8_at(dba(PALETTE_UPLOAD));
    abi_TaskExit();
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
    op_rep(P_X);
    do {
        next_frame(0x81A15Fu, 0x01);
        cpu.x = rd16(dba(0x1773));
        cmp16_of(cpu.x, 0x1A00);
    } while (cpu.x < 0x1A00);
    /* colour math adds the fixed colour to BG1; 8 steps of red down, the
       first 7 with blue up, 15 frames apart */
    a8_load(0x01);
    wr8(dba(CGADSUB_SHADOW), 0x01);
    wr8(dba(CGWSEL_SHADOW), 0x00);
    wr8(dba(0x0080), 0x00);
    a8_load(0x80);
    wr8(dba(WOBJSEL_SHADOW), 0x80);
    cpu.y = 0x0007;
    cpu.x = 0x0008;
    do {
        next_frame(0x81A183u, 0x0F);
        abi_PaletteRedDown();
        inc8_at(dba(COLDATA_BLUE));
        cpu.y--;
        nz16_of(cpu.y);
    } while (cpu.y);
    abi_PaletteRedDown();
    abi_TaskExit();
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
    int16_t at;
    uint8_t c, red;

    a8_load((uint8_t)cpu.x);
    if (!(uint8_t)cpu.x)
        return;
    /* each colour's red down one, not below 0 */
    for (at = 0x1E; at >= 0; at -= 2) {
        c = rd8(0x7EFD40u + at);
        wr8(dba(SCRATCH), c & 0xE0);
        red = c & 0x1F;
        red = red ? (uint8_t)(red - 1) : 0;
        c = (uint8_t)(red | rd8(dba(SCRATCH)));
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | c);
        wr8(0x7EFD40u + at, c);
    }
    inc8_at(dba(PALETTE_UPLOAD));
    nz16_of(cpu.x);
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
    uint8_t v;

    for (;;) {
        next_frame(0x81A30Fu, 0x01);
        if (rd8(dba(0x0360)))
            break;
        /* the player's sprites in front while its y is under 256 */
        wr8(dba(0x08E4), rd8(dba(0x08E4)) & 0xCF);
        x8_load(rd8(dba(0x08EB)));
        v = (uint8_t)(((uint8_t)cpu.x ? 0x00 : 0x10) | rd8(dba(0x08E4)));
        wr8(dba(0x08E4), v);
        /* at the top: the camera's y 0, the player's x low byte $80 on and
           its y low byte under $78 */
        if (rd8(dba(0x1777)) | rd8(dba(0x1778)))
            continue;
        v = rd8(dba(0x08E7));
        cmp8_of(v, 0x80);
        if (v < 0x80)
            continue;
        v = rd8(dba(0x08EA));
        cmp8_of(v, 0x78);
        if (v < 0x78)
            break;
    }
    wr8(dba(0x1A07), 0);
    v = rd8(dba(LEVEL));
    cmp8_of(v, 0x19);
    if (v != 0x19) {
        wr8(dba(0x0399), 0x17);
        wr8(dba(0x039A), 0x10);
        wr8(dba(CGWSEL_SHADOW), 0x03);
        wr8(dba(CGADSUB_SHADOW), 0x53);
    }
    /* until the scroll x's high byte is 1; then the way right opened */
    do {
        next_frame(0x81A365u, 0x01);
        v = rd8(dba(0x1774));
        cmp8_of(v, 0x01);
    } while (v != 0x01);
    a8_load(0xFE);
    wr8(0x7FF043u, 0xFE);
    abi_TaskExit();
}
