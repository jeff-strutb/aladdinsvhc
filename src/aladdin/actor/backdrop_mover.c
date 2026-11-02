/*
 * backdrop_mover.c: helpers of the actor at $84:9044 that moves the
 * backdrop layer up and down against the play field, and holds its sounds
 * once the camera is past x $08E8.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_MoverEndSound(void);
void abi_MoverSound(void);
void abi_SoundQueueAdd(void);
void abi_BackdropOffsetStep(void);
void abi_CameraPastX08E8(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

#define BACKDROP_STEPS 0xD7CAu  /* by facing, 4 bytes: the backdrop offset's step (in the bank DB names) */
#define MOVER_SOUNDED 0x43u      /* nonzero: its end sound was queued */

/*
 * @routine 84:9083 BackdropOffsetStep
 * WHAT IT DOES: Moves the backdrop layer against the play field by one
 * frame's step for the actor's facing, and counts the actor's frames.
 * HOW: The facing * 4 indexes the steps at $80:D7CA; its 32-bit value is
 * added to backdrop_offset_y ($1A2B); the actor's word at $40 steps down.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR. Exit:
 * M 8-bit; Z set when $40 reached 0; X = the facing * 4.
 */
void abi_BackdropOffsetStep(void)
{
    uint16_t at, n;

    op_rep(P_M);
    at = (uint16_t)(dp16(ACTOR_FACING) << 2);
    cpu.x = (uint8_t)at;
    flag_set(P_C, 0);
    cpu.a = op_adc16(rd16(dba(BACKDROP_OFFSET_Y)), rd16(dba(BACKDROP_STEPS) + cpu.x), &cpu.p);
    wr16(dba(BACKDROP_OFFSET_Y), cpu.a);
    cpu.a = op_adc16(rd16(dba(BACKDROP_OFFSET_Y + 2)), rd16(dba(BACKDROP_STEPS + 2) + cpu.x), &cpu.p);
    wr16(dba(BACKDROP_OFFSET_Y + 2), cpu.a);
    n = (uint16_t)(dp16(ACTOR_40) - 1);
    dp16_set(ACTOR_40, n);
    nz16_of(n);
    op_sep(P_M);
}

/*
 * @routine 84:90B7 CameraPastX08E8
 * WHAT IT DOES: Says whether the play field's scroll x has reached $08E8:
 * C set when it has.
 * HOW: A 16-bit compare of the scroll x with $08E8.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR.
 * Exit: M 8-bit; C, N, Z from the compare; A = the scroll x.
 */
void abi_CameraPastX08E8(void)
{
    uint16_t x = rd16(dba(LAYER0 + LAYER_SCROLL_X));

    cpu.a = x;
    cmp16_of(x, 0x08E8);
    op_sep(P_M);
}

/*
 * @routine 84:90A2 MoverEndSound
 * WHAT IT DOES: Once the camera is past x $08E8, queues sound command $F1
 * one time; before that keeps the actor's flag clear so it can be queued.
 * HOW: CameraPastX08E8; not past: $43 cleared. Past with $43 clear:
 * SoundQueueAdd $F1, $43 up.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSL. Exit: M 8-bit; as SoundQueueAdd when it
 * queued.
 */
void abi_MoverEndSound(void)
{
    uint8_t sounded, n;

    abi_CameraPastX08E8();
    if (!(cpu.p & P_C)) {
        wr8(dpa(MOVER_SOUNDED), 0);
        return;
    }
    sounded = rd8(dpa(MOVER_SOUNDED));
    a8_load(sounded);
    if (sounded)
        return;
    a8_load(0xF1);
    abi_SoundQueueAdd();
    n = (uint8_t)(rd8(dpa(MOVER_SOUNDED)) + 1);
    wr8(dpa(MOVER_SOUNDED), n);
    nz8_of(n);
}

/*
 * @routine 84:90C2 MoverSound
 * WHAT IT DOES: Queues sound command A while the camera is not yet past x
 * $08E8.
 * HOW: A kept in X; CameraPastX08E8; not past: SoundQueueAdd with it.
 * Entry: M 8-bit, X 8-bit; A = the command; DB a bank whose $0000-$1FFF
 * is work RAM; called by JSR. Exit: X = the command; as SoundQueueAdd
 * when it queued.
 */
void abi_MoverSound(void)
{
    x8_load((uint8_t)cpu.a);              /* the command, kept */
    abi_CameraPastX08E8();
    if (cpu.p & P_C)
        return;
    a8_load((uint8_t)cpu.x);
    abi_SoundQueueAdd();
}
