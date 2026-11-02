/*
 * sound_command.c: sending queued sound commands to the SPC700.
 *
 * The game queues command bytes in a 32-entry ring. Once a frame one
 * command goes to the driver through APU port 0, but only after the driver
 * has echoed the previous command's sequence number on port 2, so no
 * command is sent before the driver has taken the last one.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "hw.h"
#include "wram.h"
#include "wram_video.h"
#include "ops65.h"

/* Commands $F5 and $F6 take a parameter, the next byte in the ring. */
#define SOUND_CMD_PARAM_A 0xF5
#define SOUND_CMD_PARAM_B 0xF6

/* CPX # (16-bit): N, Z and C from X less `m`. */
static void op_cmp16_x(uint16_t m)
{
    cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(cpu.x - m)) & ~P_C) | (cpu.x >= m ? P_C : 0));
}

/*
 * @routine 80:B463 SoundCommandSend
 * WHAT IT DOES: Sends the next queued sound command to the SPC700, when
 * sending is not held, the queue is not empty and the driver has taken the
 * last command.
 * HOW: The driver has taken the last command when APU port 2 reads back
 * the sequence number. Then the sending flag and the sequence number step
 * up, and the command byte at the read index goes to port 0. Commands $F5
 * and $F6 carry a parameter: the read index steps past the command, and
 * the parameter goes to port 3 (with port 2 cleared) before the command
 * goes to port 0. The read index then steps past the last byte sent,
 * wrapping at 32. The sending flag is cleared on every way out.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
 * whose $2140-$2143 are the APU ports ($00-$3F or $80-$BF); D any.
 * Exit: by the way out taken, as the last instructions leave them:
 *   held: A low = the hold byte; N, Z from it; X, C unchanged.
 *   queue empty: A low = 0 (the hold byte); X = the read index; N, Z, C
 *     from comparing it with the write index.
 *   not taken: A low = the sequence number; X = the read index; N, Z, C
 *     from comparing it with port 2.
 *   sent: A low = the new read index; N, Z from it; C from the last
 *     compare of the command byte ($F5, else $F6); X = the read index the
 *     command was taken at, or the parameter's for $F5 and $F6.
 * A high byte, Y, D, DB and the other flags unchanged.
 */
void abi_SoundCommandSend(void)
{
    uint8_t a, x, cmd, param;

    a = rd8(SOUND_HOLD);
    cpu.p = nz8(cpu.p, a);
    if (a != 0)
        goto done;
    x = rd8(SOUND_QUEUE_READ);
    cpu.x = x;
    cpu.p = cmp8_flags(cpu.p, x, rd8(SOUND_QUEUE_WRITE));
    if (cpu.p & P_Z)
        goto done;
    a = rd8(SOUND_SEQ);
    cpu.p = cmp8_flags(cpu.p, a, rd8(APUIO2));
    if (!(cpu.p & P_Z))
        goto done;
    wr8(SOUND_SENDING, (uint8_t)(rd8_rmw(SOUND_SENDING) + 1));
    wr8(SOUND_SEQ, (uint8_t)(rd8_rmw(SOUND_SEQ) + 1));
    cmd = rd8(SOUND_QUEUE + x);
    a = cmd;
    cpu.p = cmp8_flags(cpu.p, cmd, SOUND_CMD_PARAM_A);
    if (cmd != SOUND_CMD_PARAM_A)
        cpu.p = cmp8_flags(cpu.p, cmd, SOUND_CMD_PARAM_B);
    if (cmd == SOUND_CMD_PARAM_A || cmd == SOUND_CMD_PARAM_B) {
        x = (uint8_t)((x + 1) & 0x1F);
        wr8(SOUND_QUEUE_READ, x);
        cpu.x = x;
        param = rd8(SOUND_QUEUE + x);
        wr8(APUIO2, 0);
        wr8(APUIO3, param);
    }
    wr8(APUIO0, cmd);
    a = (uint8_t)((x + 1) & 0x1F);
    cpu.p = nz8(cpu.p, a);
    wr8(SOUND_QUEUE_READ, a);
done:
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    wr8(SOUND_SENDING, 0);
}

/* Levels $14-$1E queue no sound commands. */
#define LEVEL_QUIET_FIRST 0x0014
#define LEVEL_QUIET_END 0x001F

/*
 * @routine 81:FA00 SoundQueueAdd
 * WHAT IT DOES: Adds sound command A to the queue the NMI sends from,
 * unless it is 0 or the level is one of $14-$1E, which queue none.
 * HOW: The caller's Z says A is 0. With 16-bit X the level word is
 * compared and the write index read as a word; the index then steps,
 * wrapping at 32.
 * Entry: M 8-bit, X 8-bit; A = the command, Z from it; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSL. Exit: queued: A low = the new
 * write index, N and Z from it; X = the old index; C from the level
 * compare. Not queued: A and Z as at entry, or X = the level, N, Z and C
 * from its compare with $1F. X 8-bit; Y and the other flags unchanged.
 */
void abi_SoundQueueAdd(void)
{
    uint16_t x;
    uint8_t a = (uint8_t)cpu.a;

    op_rep(P_X);
    if (!(cpu.p & P_Z)) {
        x = rd16(LEVEL);
        cpu.x = x;
        op_cmp16_x(LEVEL_QUIET_FIRST);
        if (!(cpu.p & P_C) || (op_cmp16_x(LEVEL_QUIET_END), cpu.p & P_C)) {
            x = rd16(SOUND_QUEUE_WRITE);
            cpu.x = x;
            wr8(SOUND_QUEUE + x, a);
            a = (uint8_t)((x + 1) & 0x1F);
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
            cpu.p = nz8(cpu.p, a);
            wr8(SOUND_QUEUE_WRITE, a);
        }
    }
    op_sep(P_X);
}

#define SOUND_SPECIAL 0x03F8       /* the last $F2 or $F6 command queued */
#define SOUND_CMD_F2 0xF2
#define SOUND_CMD_F6 0xF6

/* Special command `cmd` queued and kept as the last one. */
static void queue_special(uint8_t cmd)
{
    a8_load(cmd);
    wr8(dba(SOUND_SPECIAL), cmd);
    abi_SoundQueueAdd();
}

/*
 * @routine 81:F9F7 SoundQueueF2
 * WHAT IT DOES: Queues sound command $F2, keeping it as the last special
 * command.
 * HOW: Then on into SoundQueueAdd.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSL. Exit: as SoundQueueAdd.
 */
void abi_SoundQueueF2(void)
{
    queue_special(SOUND_CMD_F2);
}

/*
 * @routine 81:F9D2 SoundQueueF6With1
 * WHAT IT DOES: Queues sound command $F6 with its parameter 1, keeping
 * $F6 as the last special command.
 * HOW: The parameter waits in A's high byte; SoundQueueAdd for each.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSL. Exit: as SoundQueueAdd for the parameter; A high = $F6.
 */
void abi_SoundQueueF6With1(void)
{
    a8_load(0x01);
    op_xba();                             /* the parameter 1 waits in A's high byte */
    queue_special(SOUND_CMD_F6);
    op_xba();
    abi_SoundQueueAdd();
}
