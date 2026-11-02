/*
 * sound_queue_f6.c: sound command $F6 queued with the parameter the caller
 * gives (the entry inside SoundQueueF6With1, past its LDA #1).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"

void abi_SoundQueueAdd(void);

#define SOUND_SPECIAL 0x03F8u      /* the last $F2 or $F6 command queued */
#define SOUND_CMD_F6 0xF6          /* takes a parameter, the next byte queued */

/*
 * @routine 81:F9D4 SoundQueueF6
 * WHAT IT DOES: Queues sound command $F6 with parameter A, keeping $F6 as
 * the last special command.
 * HOW: The parameter waits in A's high byte while $F6 is queued, then
 * SoundQueueAdd for it (not queued when it is 0).
 * Entry: M 8-bit, X 8-bit; A = the parameter; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSL. Exit: as SoundQueueAdd for the
 * parameter; A high = A low as SoundQueueAdd left it after the $F6 (the
 * new write index, or $F6 when it queued none).
 */
void abi_SoundQueueF6(void)
{
    op_xba();                             /* the parameter waits in A's high byte */
    a8_load(SOUND_CMD_F6);
    wr8(dba(SOUND_SPECIAL), SOUND_CMD_F6);
    abi_SoundQueueAdd();
    op_xba();
    abi_SoundQueueAdd();
}
