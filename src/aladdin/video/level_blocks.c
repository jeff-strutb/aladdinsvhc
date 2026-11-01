/*
 * level_blocks.c: the level's blocks as it loads.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram_video.h"

/*
 * @routine 81:C025 BlockSourceAdd
 * WHAT IT DOES: While a level loads, lists one more block's source.
 * HOW: Stores A at the list's end and steps the end on by a word.
 * Entry: M 16-bit, X 16-bit; A = the block's source; DB a bank whose
 * $0000-$1FFF is work RAM. Exit: X = the entry's offset; N and Z from the
 * new end; A, Y and the other flags unchanged.
 */
void abi_BlockSourceAdd(void)
{
    uint16_t end;

    cpu.x = rd16(BLOCK_SOURCE_END);
    cpu.p = nz16(cpu.p, cpu.x);
    wr16(BLOCK_SOURCES + cpu.x, cpu.a);
    end = (uint16_t)(rd16_rmw(BLOCK_SOURCE_END) + 1);
    wr16_rmw(BLOCK_SOURCE_END, end);
    end = (uint16_t)(rd16_rmw(BLOCK_SOURCE_END) + 1);
    wr16_rmw(BLOCK_SOURCE_END, end);
    cpu.p = nz16(cpu.p, end);
}
