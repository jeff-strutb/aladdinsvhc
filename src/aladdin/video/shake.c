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
    uint8_t a;

    cpu.x = rd8(dp(SHAKE_LAYER));
    cpu.y = rd8(dp(SHAKE_FRAMES));
    do {
        uint16_t at = (uint16_t)(LAYER_OFFSET_Y + cpu.x);

        if (down) {
            cpu.p &= (uint8_t)~P_C;
            a = op_adc8(rd8(abs_db(at)), rd8(dp(SHAKE_STEP)), &cpu.p);
            wr8(abs_db(at), a);
            a = op_adc8(rd8(abs_db((uint16_t)(at + 1))), 0, &cpu.p);
        } else {
            cpu.p |= P_C;
            a = op_sbc8(rd8(abs_db(at)), rd8(dp(SHAKE_STEP)), &cpu.p);
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
