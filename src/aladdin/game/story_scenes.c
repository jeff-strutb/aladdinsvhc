/*
 * story_scenes.c: steps of the story that runs at $80:AF89, six scenes by
 * slot 0's parameter (each set up by an entry of the table at $80:B092),
 * faded in and out, with 64 frames between scenes. A scene may leave a
 * sound command for its end.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"
void abi_ActorSpawnAtLong(void);
void abi_StoryActorPlace(void);

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

/*
 * @routine 80:B196 StoryActorPlace
 * WHAT IT DOES: Places entry X of the story scenes' actors ($80:DB5D): a
 * new actor of its kind, or, for kind 0, the player.
 * HOW: The entry's x and y into the spawn words. A kind: in A's high
 * byte, the $07 byte in its low, ActorSpawnAtLong. Kind 0: the player's
 * $06 cleared, its $07 the byte, the low bytes of x and y its whole
 * pixels' low bytes with their fractions cleared, and its state $0C (new).
 * Entry: M 8-bit, X 8-bit; X = the entry's offset; DB = $80; called by
 * JSR. Exit: M 8-bit, X 8-bit.
 */
void abi_StoryActorPlace(void)
{
    op_rep(0x20);  /* 80B196 rep */
    lda16(ea_absx(0xDB5F));  /* 80B198 lda */
    sta16(ea_abs(0x002C));  /* 80B19B sta */
    lda16(ea_absx(0xDB61));  /* 80B19E lda */
    sta16(ea_abs(0x002E));  /* 80B1A1 sta */
    op_sep(0x20);  /* 80B1A4 sep */
    lda8(ea_absx(0xDB5D));  /* 80B1A6 lda */
    if (IF_Z) goto L_80B1B6;  /* 80B1A9 beq */
    xba();  /* 80B1AB xba */
    lda8(ea_absx(0xDB5E));  /* 80B1AC lda */
    abi_ActorSpawnAtLong();  /* 80B1AF jsl */
L_80B1B3:
    op_sep(0x30);  /* 80B1B3 sep */
    return;  /* 80B1B5 rts */
L_80B1B6:
    stz8(ea_abs(0x08E1));  /* 80B1B6 stz */
    lda8(ea_absx(0xDB5E));  /* 80B1B9 lda */
    sta8(ea_abs(0x08E2));  /* 80B1BC sta */
    ldx8(ea_abs(0x002C));  /* 80B1BF ldx */
    stx8(ea_abs(0x08E7));  /* 80B1C2 stx */
    stz8(ea_abs(0x08E6));  /* 80B1C5 stz */
    ldx8(ea_abs(0x002E));  /* 80B1C8 ldx */
    stx8(ea_abs(0x08EA));  /* 80B1CB stx */
    stz8(ea_abs(0x08E9));  /* 80B1CE stz */
    lda8_v(0x0C);  /* 80B1D1 lda */
    sta8(ea_abs(PLAYER));  /* 80B1D3 sta */
    goto L_80B1B3;  /* 80B1D6 bra */
}
