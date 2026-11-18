/*
 * scene_steps.c: three steps the code that places Aladdin in a scene (from
 * $81:E154) calls each frame: his animation started or stepped and his
 * picture sent, and a step across with the animation.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"

void abi_ActorMoveX(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_ActorPictureSend(void);
void abi_PlayerSceneAnimStep(void);

/* A JSL from C at `site`: its return address on the stack while the
   callee runs, as the original's. */
static void jsl_at(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 81:E224 PlayerSceneMoveStep
 * WHAT IT DOES: A frame of Aladdin walking in a scene: a step across, then
 * his animation stepped and his picture sent.
 * HOW: ActorMoveX, then PlayerSceneAnimStep.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR from the scene code. Exit: as
 * PlayerSceneAnimStep.
 */
void abi_PlayerSceneMoveStep(void)
{
    jsl_at(0x81E224u, abi_ActorMoveX);
    call_frame(0xE22Au, abi_PlayerSceneAnimStep);
}

/*
 * @routine 81:E22C PlayerSceneAnimStart
 * WHAT IT DOES: Starts Aladdin's animation A in a scene and sends his
 * picture.
 * HOW: AnimStart, then ActorPictureSend.
 * Entry: M 8-bit, X 8-bit; A = the animation; D = the player's record;
 * DB a bank whose $0000-$1FFF is work RAM; called by JSR from the scene
 * code. Exit: as ActorPictureSend.
 */
void abi_PlayerSceneAnimStart(void)
{
    jsl_at(0x81E22Cu, abi_AnimStart);
    jsl_at(0x81E230u, abi_ActorPictureSend);
}

/*
 * @routine 81:E235 PlayerSceneAnimStep
 * WHAT IT DOES: Steps Aladdin's animation in a scene and sends his picture.
 * HOW: AnimStep, then ActorPictureSend.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR from the scene code and
 * PlayerSceneMoveStep. Exit: as ActorPictureSend.
 */
void abi_PlayerSceneAnimStep(void)
{
    jsl_at(0x81E235u, abi_AnimStep);
    jsl_at(0x81E239u, abi_ActorPictureSend);
}
