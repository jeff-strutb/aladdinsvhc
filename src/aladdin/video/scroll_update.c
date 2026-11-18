/*
 * scroll_update.c: the play field's scroll each frame.
 *
 * The level picks one of a table of scroll handlers (SCROLL_KIND): the
 * camera that follows the player inside a window, scrolls that run on
 * their own at a set speed, or none. Every handler but the last two ends
 * in the same tail: the scroll kept inside the level, empty blocks
 * stopping it (camera.c), then the backdrop's scroll (parallax.c).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "wram.h"
#include "wram_video.h"
#include "wram_player.h"
#include "actor.h"

#define SCROLL_X (LAYER0 + LAYER_SCROLL_X)
#define SCROLL_Y (LAYER0 + LAYER_SCROLL_Y)
#define SCROLL_KIND 0x00083Du      /* the level's scroll handler, times 2 */
#define V_1A09 0x001A09u           /* added to V_1A08 for the camera's window (word) */
#define CAMERA_AIM_X 0x0003F1u     /* added to the scroll X for the point the camera keeps the player from (word) */
#define CAMERA_PUSH 0x001A0Au      /* how the camera moved across this frame: 0 not, 1 right, 2 left (word) */
#define SCROLL_X_HELD 0x001A10u    /* nonzero: no follow across and no stop at empty blocks across (word) */
#define SCROLL_Y_HELD 0x001A12u    /* the same up and down (word) */
#define SCROLL_X_MIN 0x001A40u     /* the scroll X's least (word) */
#define SCROLL_X_MAX 0x001A42u     /* and its most (word) */
#define V_1427 0x001427u           /* read here only for its low 4 bits (word) */
#define TARGET_X SCRATCH           /* the camera's point: x (word) */
#define TARGET_Y (SCRATCH + 2)     /* and y (word) */
#define PLAYER_X_AT (PLAYER + ACTOR_X + 1) /* the player's x, whole pixels (word) */
#define PLAYER_Y_AT (PLAYER + ACTOR_Y + 1) /* the player's y, whole pixels (word) */

/* In the bank DB names ($80): the handlers' table is in this bank. The
   camera's windows by V_1A08 + V_1A09: x bias, x width, y bias, y height,
   y offset of the camera's point (words); the two set speeds of handlers
   10 and 14 (24-bit, from the fraction byte: 4 bytes each). */
#define SCROLL_HANDLERS 0x81C5AAu
#define CAMERA_WINDOWS 0xC672u
#define WINDOW_X_BIAS 0
#define WINDOW_X_WIDTH 2
#define WINDOW_Y_BIAS 4
#define WINDOW_Y_HEIGHT 6
#define WINDOW_POINT_Y 8
#define SCROLL_SPEEDS 0xC6EAu

void abi_CameraStopAcross(void);
void abi_CameraStopUpDown(void);
void abi_ParallaxUpdate(void);

/* INC of a word, as the CPU makes it: the low byte and the high read,
   then the high byte written first and the low after. */
static void inc16_rmw(uint32_t addr)
{
    uint16_t v = (uint16_t)(rd16_rmw(addr) + 1);

    wr16_rmw(addr, v);
    cpu.p = nz16(cpu.p, v);
}

/* A 24-bit add to the scroll X, from its fraction byte: low word, then the
   high word with the carry. */
static void scroll_x_add(uint16_t low, uint16_t high)
{
    flag_set(P_C, 0);
    cpu.a = op_adc16(rd16(SCROLL_X - 1), low, &cpu.p);
    wr16(SCROLL_X - 1, cpu.a);
    cpu.a = op_adc16(rd16(SCROLL_X + 1), high, &cpu.p);
    wr16(SCROLL_X + 1, cpu.a);
}

/* The common end, from 81:C71E: the scroll kept to 0 and its limits, the
   stops at empty blocks unless held, then the backdrop. */
static void tail(void)
{
    op_lda16(SCROLL_X);
    if (cpu.p & P_N) {
        wr16(SCROLL_X, 0);
    } else {
        op_cmp16(rd16(SCROLL_LIMIT_X));
        if (cpu.p & P_C) {
            op_lda16(SCROLL_LIMIT_X);
            wr16(SCROLL_X, cpu.a);
        } else {
            op_cmp16(rd16(SCROLL_X_MIN));
            if (!(cpu.p & P_C)) {
                op_lda16(SCROLL_X_MIN);
                wr16(SCROLL_X, cpu.a);
            } else {
                op_cmp16(rd16(SCROLL_X_MAX));
                if (cpu.p & P_C) {
                    op_lda16(SCROLL_X_MAX);
                    wr16(SCROLL_X, cpu.a);
                }
            }
        }
    }
    op_lda16(SCROLL_Y);
    if (cpu.p & P_N) {
        wr16(SCROLL_Y, 0);
    } else {
        op_cmp16(rd16(SCROLL_LIMIT_Y));
        if (cpu.p & P_C) {
            op_lda16(SCROLL_LIMIT_Y);
            wr16(SCROLL_Y, cpu.a);
        }
    }
    op_lda16(SCROLL_X_HELD);
    if (cpu.a == 0)
        call_frame(0xC769, abi_CameraStopAcross);      /* 81C767 jsr */
    op_lda16(SCROLL_Y_HELD);
    if (cpu.a == 0)
        call_frame(0xC771, abi_CameraStopUpDown);      /* 81C76F jsr */
    abi_ParallaxUpdate();
}

/* From 81:C6E9: up and down, the player's y against the window: past it,
   the scroll Y moves by how far, at most 6 down and 6 up. */
static void follow_down(uint16_t w)
{
    uint16_t d, py;

    op_lda16(SCROLL_Y_HELD);
    if (cpu.a != 0)
        return;
    flag_set(P_C, 1);
    py = rd16(PLAYER_Y_AT);
    d = op_sbc16(py, rd16(TARGET_Y), &cpu.p);
    flag_set(P_C, 0);
    d = op_adc16(d, rd16(dba((uint16_t)(CAMERA_WINDOWS + WINDOW_Y_BIAS + w))), &cpu.p);
    cpu.a = d;
    op_cmp16(rd16(dba((uint16_t)(CAMERA_WINDOWS + WINDOW_Y_HEIGHT + w))));
    if (!(cpu.p & P_C))
        return;
    cpu.y = d;
    nz16_of(d);
    if (!(d & 0x8000)) {
        flag_set(P_C, 1);
        cpu.a = op_sbc16(d, rd16(dba((uint16_t)(CAMERA_WINDOWS + WINDOW_Y_HEIGHT + w))), &cpu.p);
        op_cmp16(0x0007);
        if (cpu.p & P_C)
            op_lda16_imm(0x0006);
    } else {
        op_cmp16(0xFFFA);
        if (!(cpu.p & P_C))
            op_lda16_imm(0xFFFA);
    }
    flag_set(P_C, 0);
    cpu.a = op_adc16(cpu.a, rd16(SCROLL_Y), &cpu.p);
    wr16(SCROLL_Y, cpu.a);
}

/* From 81:C6AE: across, the same with the player's x: at most 3 a frame
   right, 3 left; CAMERA_PUSH 1 for a move right, 2 for one left. */
static void follow_across(uint16_t w)
{
    uint16_t d, px;

    op_lda16(SCROLL_X_HELD);
    if (cpu.a != 0)
        return;
    flag_set(P_C, 1);
    px = rd16(PLAYER_X_AT);
    d = op_sbc16(px, rd16(TARGET_X), &cpu.p);
    flag_set(P_C, 0);
    d = op_adc16(d, rd16(dba((uint16_t)(CAMERA_WINDOWS + WINDOW_X_BIAS + w))), &cpu.p);
    cpu.a = d;
    op_cmp16(rd16(dba((uint16_t)(CAMERA_WINDOWS + WINDOW_X_WIDTH + w))));
    if (!(cpu.p & P_C))
        return;
    inc16_rmw(CAMERA_PUSH);
    cpu.y = d;
    nz16_of(d);
    if (!(d & 0x8000)) {
        flag_set(P_C, 1);
        cpu.a = op_sbc16(d, rd16(dba((uint16_t)(CAMERA_WINDOWS + WINDOW_X_WIDTH + w))), &cpu.p);
        op_cmp16(0x0004);
        if (cpu.p & P_C)
            op_lda16_imm(0x0003);
    } else {
        inc16_rmw(CAMERA_PUSH);
        cpu.a = d;
        op_cmp16(0xFFFD);
        if (!(cpu.p & P_C))
            op_lda16_imm(0xFFFD);
    }
    flag_set(P_C, 0);
    cpu.a = op_adc16(cpu.a, rd16(SCROLL_X), &cpu.p);
    wr16(SCROLL_X, cpu.a);
}

/* Handler 0, from 81:C67C: the camera that follows the player. V_1A08
   negative: it stays where it is. */
static void follow(void)
{
    uint16_t w, sx, sy;

    op_rep(0x20);
    op_lda16(SCROLL_X);
    wr16(SCROLL_X_BEFORE, cpu.a);
    op_lda16(SCROLL_Y);
    wr16(SCROLL_Y_BEFORE, cpu.a);
    x8_load(rd8(V_1A08));
    if (cpu.x & 0x80) {
        op_sep(0x20);
        return;
    }
    op_txa16();
    flag_set(P_C, 0);
    cpu.a = op_adc16(cpu.a, rd16(V_1A09), &cpu.p);
    x8_load((uint8_t)cpu.a);
    wr16(CAMERA_PUSH, 0);
    op_rep(0x30);
    w = cpu.x;
    flag_set(P_C, 0);
    sx = rd16(SCROLL_X);
    cpu.a = op_adc16(sx, rd16(CAMERA_AIM_X), &cpu.p);
    wr16(TARGET_X, cpu.a);
    flag_set(P_C, 0);
    sy = rd16(SCROLL_Y);
    cpu.a = op_adc16(sy, rd16(dba((uint16_t)(CAMERA_WINDOWS + WINDOW_POINT_Y + w))), &cpu.p);
    wr16(TARGET_Y, cpu.a);
    follow_across(w);
    follow_down(w);
    tail();
}

/* Handlers 10 and 14, from 81:C60F: the scroll X runs on at set speed s
   (0 or 4); up and down, the camera follows as handler 0's with window 0. */
static void run_at(uint8_t s)
{
    uint16_t v;

    x8_load(s);
    op_rep(0x30);
    flag_set(P_C, 0);
    v = rd16(SCROLL_X - 1);
    cpu.a = op_adc16(v, rd16(dba((uint16_t)(SCROLL_SPEEDS + s))), &cpu.p);
    wr16(SCROLL_X - 1, cpu.a);
    v = rd16(SCROLL_X + 1);
    cpu.a = op_adc16(v, rd16(dba((uint16_t)(SCROLL_SPEEDS + 2 + s))), &cpu.p);
    wr16(SCROLL_X + 1, cpu.a);
    wr16(CAMERA_PUSH, 0);
    cpu.x = 0;
    nz16_of(0);
    flag_set(P_C, 0);
    v = rd16(SCROLL_Y);
    cpu.a = op_adc16(v, rd16(dba((uint16_t)(CAMERA_WINDOWS + WINDOW_POINT_Y))), &cpu.p);
    wr16(TARGET_Y, cpu.a);
    follow_down(0);
    tail();
}

/*
 * @routine 81:C5A4 ScrollUpdate
 * WHAT IT DOES: Moves the play field's scroll for this frame the way the
 * level asks: the camera that follows the player (0), a scroll that runs
 * on right, slowly, keeping the player on screen (2), at 3 a frame (4),
 * fast (6), or down and left (8), at a set speed with the camera
 * following up and down (10, 14), or none (12); then keeps it in the
 * level and sets the backdrop's.
 * HOW: JMP through its table by SCROLL_KIND. 0: the scroll before the
 * move kept; with V_1A08 not negative, the window V_1A08 + V_1A09 of
 * CAMERA_WINDOWS: the player's offset from the camera's point (the
 * scroll plus CAMERA_AIM_X, and the window's y offset), plus the
 * window's bias, past its width or height moves the scroll by the
 * excess, at most 3 across and 6 up or down, unless held. 2: the scroll
 * X plus $A0 (24 bits); with V_1427's low 4 bits 0, the player's x at
 * most the scroll X + $F0. 4: plus 3 a frame. 6: plus $300. 8: less 8 across,
 * plus 8 down. 10 and 14: plus SCROLL_SPEEDS 0 or 1; up and down as 0.
 * Then (not 12, nor 0 with V_1A08 negative): the scroll X not below 0,
 * at most SCROLL_LIMIT_X, else within SCROLL_X_MIN and SCROLL_X_MAX; the
 * scroll Y not below 0 nor past SCROLL_LIMIT_Y; CameraStopAcross and
 * CameraStopUpDown unless held; then ParallaxUpdate. Kind 16's entry is a
 * bare RTS no script runs.
 * Entry: M 8-bit, X 8-bit; DB = $80. Exit: as ParallaxUpdate leaves the
 * registers; for kind 0 with V_1A08 negative, M 8-bit, X 8-bit, A = the
 * scroll Y, X = V_1A08, N set.
 */
void abi_ScrollUpdate(void)
{
    x8_load(rd8(dba((uint16_t)SCROLL_KIND)));
    switch (cpu.x) {
    case 0:
        follow();
        return;
    case 2:
        op_rep(0x30);
        scroll_x_add(0x00A0, 0);
        op_lda16(V_1427);
        op_and16(0x000F);
        if (cpu.a == 0) {
            op_lda16(SCROLL_X);
            flag_set(P_C, 0);
            cpu.a = op_adc16(cpu.a, 0x00F0, &cpu.p);
            op_cmp16(rd16(PLAYER_X_AT));
            if (!(cpu.p & P_C))
                wr16(PLAYER_X_AT, cpu.a);
        }
        tail();
        return;
    case 4:
        /* an add of 0 to the fraction byte with 8-bit A: only the flags */
        flag_set(P_C, 0);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | op_adc8(rd8(SCROLL_X - 1), 0, &cpu.p));
        wr8(SCROLL_X - 1, (uint8_t)cpu.a);
        op_rep(0x30);
        op_lda16(SCROLL_X);
        wr16(SCROLL_X_BEFORE, cpu.a);
        op_lda16(SCROLL_X);
        cpu.a = op_adc16(cpu.a, 0x0003, &cpu.p);
        wr16(SCROLL_X, cpu.a);
        tail();
        return;
    case 6:
        op_rep(0x30);
        scroll_x_add(0x0300, 0);
        tail();
        return;
    case 8:
        op_rep(0x30);
        op_lda16(SCROLL_X);
        wr16(SCROLL_X_BEFORE, cpu.a);
        op_lda16(SCROLL_Y);
        wr16(SCROLL_Y_BEFORE, cpu.a);
        flag_set(P_C, 1);
        cpu.a = op_sbc16(rd16(SCROLL_X), 0x0008, &cpu.p);
        wr16(SCROLL_X, cpu.a);
        flag_set(P_C, 0);
        cpu.a = op_adc16(rd16(SCROLL_Y), 0x0008, &cpu.p);
        wr16(SCROLL_Y, cpu.a);
        tail();
        return;
    case 10:
        run_at(0);
        return;
    case 12:
        op_rep(0x30);
        abi_ParallaxUpdate();
        return;
    case 14:
        run_at(4);
        return;
    case 16:
        return;
    default:
        jump_routine(0x810000u | rd16(0x810000u | (uint16_t)(SCROLL_HANDLERS + cpu.x)));
    }
}
