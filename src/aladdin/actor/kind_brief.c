/*
 * kind_brief.c: actors that only play an animation for a few frames and
 * end.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_ActorEnd(void);
void abi_SoundQueueAdd(void);

static uint16_t dp(uint16_t off)
{
    return (uint16_t)(cpu.d + off);
}

static void set_a8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/*
 * @routine 81:EB90 ActorBriefAnim8
 * @actor
 * WHAT IT DOES: An actor (kind 17) that plays animation 8 for 8 frames,
 * then ends.
 * HOW: Sets bit 4 of its flags at $09 and $2C to 4, starts animation 8
 * (AnimStart), then each frame yields and steps the animation
 * (AnimStep), 8 times; then ActorEnd.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; S the actor loop's;
 * reached by the actor loop's jump. Exit: never returns: yields at each
 * BRK, and ends in ActorEnd.
 */
void abi_ActorBriefAnim8(void)
{
    uint8_t n;

    set_a8(rd8(dp(ACTOR_09)));
    set_a8((uint8_t)cpu.a | 0x10);
    wr8(dp(ACTOR_09), (uint8_t)cpu.a);
    set_a8(0x08);
    abi_AnimStart();
    set_a8(0x04);
    wr8(dp(ACTOR_2C), 0x04);
    set_a8(0x08);
    wr8(dp(ACTOR_40), 0x08);
    do {
        actor_yield(0x81EBA4);
        abi_AnimStep();
        n = (uint8_t)(rd8_rmw(dp(ACTOR_40)) - 1);
        wr8(dp(ACTOR_40), n);
        cpu.p = nz8(cpu.p, n);
    } while (n != 0);
    abi_ActorEnd();
}

/*
 * @routine 81:EBB2 ActorBriefAnimB
 * @actor
 * WHAT IT DOES: Actor kind $13: plays sound 8 and animation $0B for 6
 * frames, then ends.
 * HOW: SoundQueueAdd 8; bit 0 of its $08 set; $09 = $10; AnimStart $0B;
 * $2C = 4; then each frame yields and steps the animation (AnimStep), 6
 * times; then ActorEnd.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; S the actor loop's;
 * reached by the actor loop's jump for kind $13 in state $0C (the table
 * at $83:8CAB, through the stub at $83:9222). Exit: never returns: yields
 * at each BRK, and ends in ActorEnd.
 */
void abi_ActorBriefAnimB(void)
{
    uint8_t frames;

    a8_load(0x08);
    abi_SoundQueueAdd();
    wr8(dpa(ACTOR_08), (uint8_t)(rd8(dpa(ACTOR_08)) | 0x01));
    wr8(dpa(ACTOR_09), 0x10);
    a8_load(0x0B);
    abi_AnimStart();
    wr8(dpa(ACTOR_2C), 0x04);
    a8_load(0x06);
    wr8(dpa(ACTOR_40), 0x06);
    do {
        actor_yield(0x81EBD0);
        abi_AnimStep();
        frames = (uint8_t)(rd8(dpa(ACTOR_40)) - 1);
        wr8(dpa(ACTOR_40), frames);
        nz8_of(frames);
    } while (frames);
    abi_ActorEnd();
}
