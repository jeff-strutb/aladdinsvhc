/*
 * line_history.c: a 32-word list at $7E:DFC0 of an actor's past heights,
 * newest first, sent to VRAM as upload 11.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "actor.h"
#include "wram.h"
#include "wram_video.h"

#define LINE_HISTORY 0x7EDFC0u
#define LINE_LAST 0x0018CDu         /* the newest height, less 256 (word) */
#define UPLOAD_LINE_HISTORY 0x0B

/*
 * @routine 84:8EBD ActorYHistoryPush
 * WHAT IT DOES: Pushes the actor's height into the list: the older 31
 * entries move down one, and the new head is its y pixel less 256, in 9
 * bits, with bit 14 set; the value less 256 is kept whole too; the list's
 * upload is asked for.
 * HOW: Word moves from the end down, with DB $7E and 8-bit X; then the new
 * entry with 16-bit A.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: A low = 11; A high = $40 or $41 (bit 8 of the entry);
 * N and Z clear; V and C from the subtraction; X = 0. M 8-bit; Y, DB and
 * the other flags unchanged.
 */
void abi_ActorYHistoryPush(void)
{
    uint8_t x;

    op_rep(P_M);
    for (x = 62; x != 0; x = (uint8_t)(x - 2)) {
        cpu.a = rd16(LINE_HISTORY - 2 + x);
        wr16(LINE_HISTORY + x, cpu.a);
    }
    cpu.x = 0;
    op_sep(P_M);
    op_rep(P_M);
    cpu.a = rd16((uint16_t)(cpu.d + ACTOR_Y + 1));
    op_sub16(0x0100);
    wr16(LINE_LAST, cpu.a);
    op_and16(0x01FF);
    cpu.a |= 0x4000;
    wr16(LINE_HISTORY, cpu.a);
    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | UPLOAD_LINE_HISTORY);
    cpu.p = nz8(cpu.p, UPLOAD_LINE_HISTORY);
    wr8(VRAM_UPLOAD_NUMBER, UPLOAD_LINE_HISTORY);
}
