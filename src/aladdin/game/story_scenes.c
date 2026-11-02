/*
 * story_scenes.c: steps of the story that runs at $80:AF89, six scenes by
 * slot 0's parameter (each set up by an entry of the table at $80:B092),
 * faded in and out, with 64 frames between scenes. A scene may leave a
 * sound command for its end.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
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

#define STORY_ACTORS 0xDB5Du  /* 6 bytes each: kind (0: the player), $07, x, y (in the bank DB names) */
#define PLACE_X 0x002Cu       /* where ActorSpawnAt places the actor: x, then y (words) */
#define PLACE_Y 0x002Eu

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
    a8_load(0x02);
    wr8(dba(SCENE_END_SOUND), 0x02);
    a8_load(0x40);
    wr8(dba(SCENE_END_WAIT), 0x40);
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
    a8_load(0x40);
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
    uint32_t entry = dba(STORY_ACTORS) + cpu.x;
    uint16_t y = rd16(entry + 4);
    uint8_t kind, b07;

    wr16(dba(PLACE_X), rd16(entry + 2));
    wr16(dba(PLACE_Y), y);
    cpu.a = y;
    kind = rd8(entry);
    a8_load(kind);
    if (kind) {
        op_xba();
        a8_load(rd8(entry + 1));
        abi_ActorSpawnAtLong();
        op_sep(0x30);
        return;
    }
    /* kind 0: the player, set there to start */
    wr8(dba(PLAYER + ACTOR_06), 0);
    b07 = rd8(entry + 1);
    a8_load(b07);
    wr8(dba(PLAYER + ACTOR_07), b07);
    x8_load(rd8(dba(PLACE_X)));
    wr8(dba(PLAYER + ACTOR_X + 1), (uint8_t)cpu.x);
    wr8(dba(PLAYER + ACTOR_X), 0);
    x8_load(rd8(dba(PLACE_Y)));
    wr8(dba(PLAYER + ACTOR_Y + 1), (uint8_t)cpu.x);
    wr8(dba(PLAYER + ACTOR_Y), 0);
    a8_load(0x0C);
    wr8(dba(PLAYER + ACTOR_STATE), 0x0C);
    op_sep(0x30);
}
