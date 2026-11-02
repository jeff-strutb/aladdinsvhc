/*
 * backdrop_actor.c: pieces of the actor of kind $6E (its start at
 * $83:F09C, its wait routine at $83:F1D5), drawn as the picture of the
 * backdrop layer, not as sprites: it moves right 3 pixels a frame, its
 * picture's frames are uploads cycled from a table, and the backdrop is
 * shown, scrolled to put the picture where the actor is, only while it is
 * near. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram_video.h"

/* The actor's own fields of its record */
#define PICTURE_AT 0x41u           /* the offset of its picture's frame in PICTURE_FRAMES */
#define PICTURE_TIME 0x42u         /* frames left before the next */

/* In the bank DB names: 2 bytes a frame: the upload's number, then its
   frames (bit 7 set on the last of the cycle) */
#define PICTURE_FRAMES 0xD67Bu

#define RANGE_X 0x001A44u          /* the point actors are kept in range of (words) */
#define RANGE_Y 0x001A46u
#define PLAY_X (LAYER0 + LAYER_SCROLL_X)
#define PLAY_Y (LAYER0 + LAYER_SCROLL_Y)
#define BACK_X (LAYER1 + LAYER_SCROLL_X)
#define BACK_Y (LAYER1 + LAYER_SCROLL_Y)
#define TM_SHADOW 0x000399u        /* TM's shadow: the main screen's layers */
#define LAYERS_WITH_BACKDROP 0x17  /* BG1, BG2, BG3 and sprites */
#define LAYERS_NO_BACKDROP 0x15    /* BG1, BG3 and sprites */

/*
 * @routine 83:F211 ActorStepRight3
 * WHAT IT DOES: Moves the actor 3 pixels to the right.
 * HOW: A 24-bit addition of $000300 to its x, a byte at a time.
 * Entry: M 8-bit; D = the actor's record. Exit: A low = its x's top byte;
 * N, V, Z and C from its addition. M 8-bit; A high, X and Y unchanged.
 */
void abi_ActorStepRight3(void)
{
    uint8_t top;

    flag_set(P_C, 0);
    wr8(dpa(ACTOR_X), op_adc8(rd8(dpa(ACTOR_X)), 0x00, &cpu.p));
    wr8(dpa(ACTOR_X + 1), op_adc8(rd8(dpa(ACTOR_X + 1)), 0x03, &cpu.p));
    top = op_adc8(rd8(dpa(ACTOR_X + 2)), 0x00, &cpu.p);
    wr8(dpa(ACTOR_X + 2), top);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | top);
}

/*
 * @routine 83:F225 BackdropPictureStep
 * WHAT IT DOES: Plays the actor's picture on by a frame of time: when the
 * picture's frame has run its time, the next one (after the last, the
 * first) is asked for as a fixed VRAM upload.
 * HOW: The time steps down; below 0, the offset steps on by 2 (back to 0
 * when the frame left was the last) and the new frame's time (low 7 bits)
 * and upload number are taken.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB the table's bank,
 * whose $0000-$1FFF is work RAM. Exit: time left: N clear, Z from it; A,
 * X, Y and the other flags unchanged. A new frame: A low = its upload
 * number; N and Z from it; Y = its offset. M 8-bit, X 8-bit; A high, X,
 * V and C unchanged.
 */
void abi_BackdropPictureStep(void)
{
    uint8_t at, v;

    if (!(dec8_at(dpa(PICTURE_TIME)) & 0x80))
        return;
    /* the next frame (back to the first after a negative entry): its
       time and its upload */
    at = (uint8_t)(rd8(dpa(PICTURE_AT)) + 2);
    cpu.y = at;
    v = rd8(dba(PICTURE_FRAMES - 1) + at);
    a8_load(v);
    if (v & 0x80) {
        y8_load(0x00);
        at = 0;
    }
    a8_load(rd8(dba(PICTURE_FRAMES + 1) + at) & 0x7F);
    wr8(dpa(PICTURE_TIME), (uint8_t)cpu.a);
    a8_load(rd8(dba(PICTURE_FRAMES) + at));
    wr8(dba(VRAM_UPLOAD_NUMBER), (uint8_t)cpu.a);
    wr8(dpa(PICTURE_AT), at);
}

/* Its distance from the point `from` in one axis, plus 256: below 512
   when within 256 either side. */
static int near_point(uint16_t mine, uint16_t from)
{
    uint16_t d;

    flag_set(P_C, 1);
    d = op_sbc16(mine, rd16(dba(from)), &cpu.p);
    flag_set(P_C, 0);
    d = op_adc16(d, 0x0100, &cpu.p);
    cpu.a = d;
    cmp16_of(d, 0x0200);
    return d < 0x0200;
}

/* The backdrop's scroll in one axis: the actor's offset from the screen's
   middle (128), turned. */
static void backdrop_scroll(uint16_t mine, uint16_t play, uint16_t back)
{
    uint16_t d;

    flag_set(P_C, 1);
    d = op_sbc16(mine, rd16(dba(play)), &cpu.p);
    flag_set(P_C, 1);
    d = op_sbc16(d, 0x0080, &cpu.p);
    a16_load((uint16_t)((d ^ 0xFFFF) + 1));
    wr16(dba(back), cpu.a);
}

/*
 * @routine 83:F244 BackdropActorShow
 * WHAT IT DOES: Shows the backdrop, with the actor's picture on it, while
 * the actor is within 256 pixels of the range point on both axes: the
 * backdrop is scrolled to put its point (128, 128) where the actor is on
 * the screen. Farther away the backdrop is not shown.
 * HOW: With 16-bit A, each distance plus 256 compared unsigned with 512;
 * near: the backdrop's scroll is the play field's plus 128 less the
 * actor's pixels, on each axis, and TM's shadow is $17; else $15.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM. Exit: near: A low = $17; A high = the high
 * byte of the backdrop's new scroll Y; N and Z clear; V and C from the
 * last subtraction. Far: A low = $15; A high = the high byte of the
 * failed distance plus 256; N and Z clear; C set; V from its addition. M
 * 8-bit; X and Y unchanged.
 */
void abi_BackdropActorShow(void)
{
    op_rep(P_M);
    if (near_point(dp16(ACTOR_X + 1), RANGE_X) && near_point(dp16(ACTOR_Y + 1), RANGE_Y)) {
        backdrop_scroll(dp16(ACTOR_X + 1), PLAY_X, BACK_X);
        backdrop_scroll(dp16(ACTOR_Y + 1), PLAY_Y, BACK_Y);
        op_sep(P_M);
        a8_load(LAYERS_WITH_BACKDROP);
        wr8(dba(TM_SHADOW), LAYERS_WITH_BACKDROP);
        return;
    }
    op_sep(P_M);
    a8_load(LAYERS_NO_BACKDROP);
    wr8(dba(TM_SHADOW), LAYERS_NO_BACKDROP);
}
