/*
 * shake.c: shaking a layer up and down.
 *
 * A shake task runs with its slot as the direct page. Its record holds the
 * layer (its byte offset in the layer offsets), how many sways, the frames
 * each half-sway lasts and the pixels it moves each frame. Each frame of a
 * half-sway moves the layer's vertical offset by the step and waits one
 * frame.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "wram_video.h"

void abi_TaskWait(void);
void abi_TaskExit(void);
void abi_ShakeUp(void);
void abi_ShakeDown(void);

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

static uint32_t dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

/* The shake task's record, from its slot (the direct page). */
#define SHAKE_LAYER 0x75u          /* the layer's byte offset in LAYER_OFFSET_Y */
#define SHAKE_SWAYS 0x76u          /* sways left */
#define SHAKE_TABLE 0xBAA3u        /* per shake, 4 bytes: layer, sways, frames, step */
#define SHAKE_FRAMES 0x77u         /* frames in a half-sway */
#define SHAKE_STEP 0x78u           /* pixels a frame */

/* One half-sway: `down` adds the step to the layer's vertical offset each
   frame, else takes it away; `site` is the JSL to TaskWait. */
static void sway(int down, uint32_t site)
{
    uint8_t a, v;

    cpu.x = rd8(dp(SHAKE_LAYER));
    cpu.y = rd8(dp(SHAKE_FRAMES));
    do {
        uint16_t at = (uint16_t)(LAYER_OFFSET_Y + cpu.x);

        if (down) {
            cpu.p &= (uint8_t)~P_C;
            v = rd8(abs_db(at));
            a = op_adc8(v, rd8(dp(SHAKE_STEP)), &cpu.p);
            wr8(abs_db(at), a);
            a = op_adc8(rd8(abs_db((uint16_t)(at + 1))), 0, &cpu.p);
        } else {
            cpu.p |= P_C;
            v = rd8(abs_db(at));
            a = op_sbc8(v, rd8(dp(SHAKE_STEP)), &cpu.p);
            wr8(abs_db(at), a);
            a = op_sbc8(rd8(abs_db((uint16_t)(at + 1))), 0, &cpu.p);
        }
        wr8(abs_db((uint16_t)(at + 1)), a);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
        cpu.p = nz8(cpu.p, 0x01);
        jsl_frame(site);
        abi_TaskWait();
        cpu.y = (uint8_t)(cpu.y - 1);
        cpu.p = nz8(cpu.p, (uint8_t)cpu.y);
    } while (cpu.y != 0);
}

/*
 * @routine 81:A1FE ShakeUp
 * WHAT IT DOES: The upward half of a sway: the layer moves up by the step
 * each frame for the half-sway's frames.
 * HOW: Each frame takes the step from the layer's 16-bit vertical offset
 * and waits one frame.
 * Entry: M 8-bit, X 8-bit; D = the shake task's slot; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR from the task. Exit: Y = 0; Z set,
 * N clear; X = the layer; A, D, DB and the other flags as the last wait
 * restores them.
 */
void abi_ShakeUp(void)
{
    sway(0, 0x81A215);
}

/*
 * @routine 81:A21D ShakeDown
 * WHAT IT DOES: The downward half of a sway: the layer moves down by the
 * step each frame for the half-sway's frames.
 * HOW: Each frame adds the step to the layer's 16-bit vertical offset and
 * waits one frame.
 * Entry: M 8-bit, X 8-bit; D = the shake task's slot; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR from the task. Exit: Y = 0; Z set,
 * N clear; X = the layer; A, D, DB and the other flags as the last wait
 * restores them.
 */
void abi_ShakeDown(void)
{
    sway(1, 0x81A234);
}


/*
 * @routine 81:A1D5 ShakeTask
 * WHAT IT DOES: A task that shakes a layer up and down: the shake its
 * parameter picks, then it ends.
 * HOW: Its slot becomes the direct page; the parameter (a byte offset in
 * the shake table at $BAA3) is replaced by the shake's four bytes: the
 * layer, the sways, the frames of a half-sway and the step; then
 * ShakeUp and ShakeDown for each sway; then TaskExit.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; DB =
 * $81. Exit: none: the task ends.
 */
void abi_ShakeTask(void)
{
    uint32_t shake;
    uint8_t sways, step;

    /* D = the task's slot: its fields */
    cpu.a = (uint8_t)cpu.y;
    nz8_of((uint8_t)cpu.y);
    cpu.d = cpu.a;
    x8_load(rd8(dpa(SHAKE_LAYER)));
    shake = dba(SHAKE_TABLE) + cpu.x;
    wr8(dpa(SHAKE_LAYER), rd8(shake));
    wr8(dpa(SHAKE_SWAYS), rd8(shake + 1));
    wr8(dpa(SHAKE_FRAMES), rd8(shake + 2));
    step = rd8(shake + 3);
    wr8(dpa(SHAKE_STEP), step);
    a8_load(step);
    do {
        call_frame(0xA1F2, abi_ShakeUp);
        call_frame(0xA1F5, abi_ShakeDown);
        sways = (uint8_t)(rd8(dpa(SHAKE_SWAYS)) - 1);
        wr8(dpa(SHAKE_SWAYS), sways);
        nz8_of(sways);
    } while (sways);
    abi_TaskExit();
}

/* A task no code creates: it runs only where cov_unused_tasks.box creates
   it. */
#define WOBBLES 0xBA76u             /* (bank $80) per wobble, 4 bytes: x step, y step, frames a swing, frames in all */
#define BG1_Y_SHIFT (BG1_X_SHIFT + 4)

/*
 * @routine 81:9F98 Bg1WobbleTask
 * WHAT IT DOES: A task that wobbles layer 1: its x and y shifts move by a
 * step each frame, turning back after each swing, for the frames the
 * wobble its parameter picks lasts; then the shifts are 0 again and it
 * ends. No code creates it.
 * HOW: D = its slot's record ($6F + Y, the old D pushed); from WOBBLES at
 * the parameter: the x step to $07 (word), the y step to $09 (word), the
 * frames a swing to $0B and $0C, the frames in all to $0D. Each frame:
 * TaskWait 1, BG1_X_SHIFT and BG1_Y_SHIFT plus the steps; the frames in
 * all down (at 0 the shifts cleared, D pulled, TaskExit); the swing's
 * frames down, and at 0 the steps negated and the swing begun again.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the wobble's offset in WOBBLES; DB = $80. Exit: none: the
 * task ends.
 */
void abi_Bg1WobbleTask(void)
{
    op_rep(0x20);  /* 819F98 rep */
    phd();  /* 819F9A phd */
    clc();  /* 819F9B clc */
    tya16();  /* 819F9C tya */
    adc16_v(0x006F);  /* 819F9D adc */
    tcd();  /* 819FA0 tcd */
    op_sep(0x20);  /* 819FA1 sep */
    ldx8(ea_dp(0x06));  /* 819FA3 ldx */
    lda8(ea_absx(WOBBLES));  /* 819FA5 lda */
    sta8(ea_dp(0x07));  /* 819FA8 sta */
    stz8(ea_dp(0x08));  /* 819FAA stz */
    lda8(ea_absx(WOBBLES + 1));  /* 819FAC lda */
    sta8(ea_dp(0x09));  /* 819FAF sta */
    stz8(ea_dp(0x0A));  /* 819FB1 stz */
    lda8(ea_absx(WOBBLES + 2));  /* 819FB3 lda */
    sta8(ea_dp(0x0B));  /* 819FB6 sta */
    lda8(ea_absx(WOBBLES + 3));  /* 819FB8 lda */
    sta8(ea_dp(0x0D));  /* 819FBB sta */
    for (;;) {
        lda8(ea_dp(0x0B));  /* 819FBD lda */
        sta8(ea_dp(0x0C));  /* 819FBF sta */
        for (;;) {
            lda8_v(0x01);  /* 819FC1 lda */
            jsl_frame(0x819FC3u); abi_TaskWait();  /* 819FC3 jsl */
            op_rep(0x20);  /* 819FC7 rep */
            clc();  /* 819FC9 clc */
            lda16(ea_dp(0x07));  /* 819FCA lda */
            adc16_v(rd16(ea_abs((uint16_t)BG1_X_SHIFT)));  /* 819FCC adc */
            sta16(ea_abs((uint16_t)BG1_X_SHIFT));  /* 819FCF sta */
            clc();  /* 819FD2 clc */
            lda16(ea_dp(0x09));  /* 819FD3 lda */
            adc16_v(rd16(ea_abs((uint16_t)BG1_Y_SHIFT)));  /* 819FD5 adc */
            sta16(ea_abs((uint16_t)BG1_Y_SHIFT));  /* 819FD8 sta */
            op_sep(0x20);  /* 819FDB sep */
            MEM8(dec, ea_dp(0x0D));  /* 819FDD dec */
            if (IF_Z) {  /* 819FDF beq */
                stz8(ea_abs((uint16_t)BG1_X_SHIFT));  /* 819FFB stz */
                stz8(ea_abs((uint16_t)(BG1_X_SHIFT + 1)));  /* 819FFE stz */
                stz8(ea_abs((uint16_t)BG1_Y_SHIFT));  /* 81A001 stz */
                stz8(ea_abs((uint16_t)(BG1_Y_SHIFT + 1)));  /* 81A004 stz */
                pld();  /* 81A007 pld */
                abi_TaskExit();  /* 81A008 jml */
                return;
            }
            MEM8(dec, ea_dp(0x0C));  /* 819FE1 dec */
            if (IF_Z)  /* 819FE3 bne */
                break;
        }
        op_rep(0x20);  /* 819FE5 rep */
        lda16(ea_dp(0x07));  /* 819FE7 lda */
        eor16_v(0xFFFF);  /* 819FE9 eor */
        ACC16(inc);  /* 819FEC inc */
        sta16(ea_dp(0x07));  /* 819FED sta */
        lda16(ea_dp(0x09));  /* 819FEF lda */
        eor16_v(0xFFFF);  /* 819FF1 eor */
        ACC16(inc);  /* 819FF4 inc */
        sta16(ea_dp(0x09));  /* 819FF5 sta */
        op_sep(0x20);  /* 819FF7 sep */
    }
}
