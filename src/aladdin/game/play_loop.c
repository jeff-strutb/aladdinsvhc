/*
 * play_loop.c: the game task's play loop: a frame of play, the frame's
 * checks, the wait for the next frame; a demo ends on Start or its time.
 * Asm: asm/aladdin/game/play_loop.asm.
 */
#include "machine.h"
#include "native.h"
#include "ops65.h"
#include "wram.h"
#include "wram_state.h"
#include "wram_video.h"

#define V_03FA 0x03FAu             /* counted up each time play starts; purpose not yet established */
#define WAIT_FRAMES 0x03FBu        /* nonzero: the frames each turn of the loop waits (else 1) */
#define DEMO_PRESSED_HIGH 0x0382u  /* while a demo plays: the presses it stops on, high byte (bit 4: Start) */
#define DEMO_FRAMES_LEFT 0x0341u   /* frames the demo still plays (word) */

void abi_FrameStep(void);
void abi_HudRefresh(void);
void abi_LevelExitCheck(void);
void abi_LevelLoad(void);
void abi_PauseCheck(void);
void abi_RestartPointCheck(void);
void abi_TaskCreate(void);
void abi_TaskExec(void);
void abi_TaskWait(void);
void abi_PlayLoop(void);

/*
 * @routine 81:8191 PlayLoopStart
 * WHAT IT DOES: The game task's play loop from its start: the level
 * loaded, the fade-in task started, then the loop.
 * HOW: LevelLoad; TaskCreate (slot 1, parameter 4, BrightnessUpTask);
 * V_03FA counted up; on into PlayLoop.
 * Entry: M 8-bit, X 8-bit; the task's start (TaskExec). Exit: never
 * returns.
 */
void abi_PlayLoopStart(void)
{
    call_frame(0x8193, abi_LevelLoad);
    jsl_frame(0x818194u);
    abi_TaskCreate();
    cpu.unwound = 0;
    MEM8(inc, ea_abs(V_03FA));
    abi_PlayLoop();                              /* JMP PlayLoop */
}

/*
 * @routine 81:82CF PlayLoop
 * WHAT IT DOES: The play loop: each turn a frame of play and the frame's
 * checks, then the wait for the next frame (and on while the VRAM queue
 * is still waiting); a demo ends when Start is pressed or its frames run
 * out, and the title screen comes back.
 * HOW: FrameStep, RestartPointCheck, LevelExitCheck, HudRefresh,
 * PauseCheck; TaskWait with WAIT_FRAMES (1 when 0), again while
 * VRAM_QUEUE_PENDING is set. In a demo (PLAY_MODE 2): bit 4 of
 * DEMO_PRESSED_HIGH, or DEMO_FRAMES_LEFT counted down to 0, ends it:
 * TaskExec TitleAfterFadeOut ($80:9C79).
 * Entry: M 8-bit, X 8-bit; reached by JMP from PlayLoopStart (the game
 * task). Exit: never returns.
 */
void abi_PlayLoop(void)
{
    for (;;) {                                   /* each turn */
        call_frame(0x82D1, abi_FrameStep);
        call_frame(0x82D4, abi_RestartPointCheck);
        call_frame(0x82D7, abi_LevelExitCheck);
        call_frame(0x82DA, abi_HudRefresh);
        call_frame(0x82DD, abi_PauseCheck);
        do {
            lda8_v(0x01);
            ldx8(ea_abs(WAIT_FRAMES));
            if (!IF_Z)
                txa8();
            jsl_frame(0x8182E6u);
            abi_TaskWait();
            ldy8(ea_abs(VRAM_QUEUE_PENDING));
        } while (!IF_Z);
        lda8(ea_abs(PLAY_MODE));
        cmp8_v(0x02);
        if (!IF_Z)
            continue;
        lda8(ea_abs(DEMO_PRESSED_HIGH));
        bit8_i(0x10);
        if (IF_Z) {
            op_rep(0x20);
            MEM16(dec, ea_abs(DEMO_FRAMES_LEFT));
            op_sep(0x20);
            if (!IF_Z)
                continue;
        }
        /* the demo ends: the title screen's task after its fade-out */
        jsl_frame(0x818306u);
        abi_TaskExec();
        return;
    }
}
