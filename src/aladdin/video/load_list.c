/*
 * load_list.c: the list a screen is loaded from: a pointer stepped through
 * its bytes (graphics, VRAM copies, palettes). Drafted by cdraft.py; named
 * and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"

#define LOAD_LIST_PTR 0x4Cu        /* in the direct page: the list's next byte (word) */

/*
 * @routine 81:F303 LoadListNext
 * WHAT IT DOES: Moves the load list's pointer on to its next byte.
 * HOW: The word at D + $4C stepped up with 16-bit M.
 * Entry: M 8-bit; called by JSR from the list's reader (81:F1A4), which
 * reads each byte through the pointer. Exit: M 8-bit; A unchanged; N, Z
 * from the new pointer.
 */
void abi_LoadListNext(void)
{
    uint16_t next = (uint16_t)(dp16(LOAD_LIST_PTR) + 1);

    dp16_set(LOAD_LIST_PTR, next);
    nz16_of(next);
    op_sep(P_M);
}

