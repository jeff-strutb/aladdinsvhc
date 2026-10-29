/*
 * sound_command.c: sending queued sound commands to the SPC700.
 *
 * The game queues command bytes in a 32-entry ring. Once a frame one
 * command goes to the driver through APU port 0, but only after the driver
 * has echoed the previous command's sequence number on port 2, so no
 * command is sent before the driver has taken the last one.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "hw.h"
#include "wram.h"

/* Commands $F5 and $F6 take a parameter, the next byte in the ring. */
#define SOUND_CMD_PARAM_A 0xF5
#define SOUND_CMD_PARAM_B 0xF6

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
 * Exit (by the way out taken), as the last instructions leave them:
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
