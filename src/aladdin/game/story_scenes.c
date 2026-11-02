/*
 * story_scenes.c: steps of the story that runs at $80:AF89, six scenes by
 * slot 0's parameter (each set up by an entry of the table at $80:B092),
 * faded in and out, with 64 frames between scenes. A scene may leave a
 * sound command for its end.
 */
#include "machine.h"
#include "ops65.h"

void abi_TaskCreate(void);
void abi_TaskWait(void);

/* Slot 0's own data, read as the scene ends ($80:B02F): when nonzero, the
   parameter of sound command $F6 queued then, and the frames waited after
   it. */
#define SCENE_END_SOUND 0x007Cu
#define SCENE_END_WAIT 0x007Du

/* TaskCreate at site: its frame, as the JSL pushes it; the five bytes
   after the JSL are its arguments. TaskCreate returns past them by its
   own RTL; the C goes on. */
static void task_create(uint32_t site)
{
    jsl_frame(site);
    abi_TaskCreate();
    cpu.unwound = 0;
}

/*
 * @routine 80:B0AE StoryScene2Start
 * WHAT IT DOES: Sets up the story's second scene: sound command $F6 with
 * parameter 2 to be queued as it ends, then a wait of 64 frames, and the
 * scene's task at $81:F344 started.
 * HOW: The two slot 0 bytes; the task created in slot 4 with parameter 3.
 * Entry: M 8-bit, X 8-bit; called by JSR (through the table at
 * $80:B092); DB a bank whose $0000-$1FFF is work RAM. Exit: as
 * TaskCreate leaves them.
 */
void abi_StoryScene2Start(void)
{
    lda8_v(0x02);
    sta8(ea_abs(SCENE_END_SOUND));
    lda8_v(0x40);
    sta8(ea_abs(SCENE_END_WAIT));
    task_create(0x80B0B8u);
}

/*
 * @routine 80:B17B StoryFadeInStart
 * WHAT IT DOES: Starts the screen fading in, over the next frames, as a
 * scene of the story begins.
 * HOW: Creates the fade-in task ($81:9E8E) in slot 1, with parameter 4.
 * Entry: any widths; called by JSR from the story's task; DB a bank whose
 * $0000-$1FFF is work RAM. Exit: as TaskCreate leaves them.
 */
void abi_StoryFadeInStart(void)
{
    task_create(0x80B17Bu);
}

/*
 * @routine 80:B185 StoryFadeOutStart
 * WHAT IT DOES: Starts the screen fading out, over the next frames, as a
 * scene of the story ends.
 * HOW: Creates the fade-out task ($81:9EB9) in slot 1, with parameter 4.
 * Entry: any widths; called by JSR from the story's task; DB a bank whose
 * $0000-$1FFF is work RAM. Exit: as TaskCreate leaves them.
 */
void abi_StoryFadeOutStart(void)
{
    task_create(0x80B185u);
}

/*
 * @routine 80:B18F StoryWait64Frames
 * WHAT IT DOES: Makes the story's task wait 64 frames between scenes,
 * letting the other tasks run.
 * HOW: TaskWait with A = 64.
 * Entry: M 8-bit; called by JSR from the story's task. Exit: when the
 * task resumes, the registers as the wait restores them (A low byte =
 * 64).
 */
void abi_StoryWait64Frames(void)
{
    lda8_v(0x40);
    jsl_frame(0x80B191u);
    abi_TaskWait();
}
