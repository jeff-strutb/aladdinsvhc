/*
 * level_flags.c: eight flags at $7E:F260 that a level sets up as it starts, from a mask
 * the level's $035D picks. Drafted by cdraft.py; named and described by
 * hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "wram_video.h"

#define LEVEL_FLAGS 0x7EF260u       /* eight flags (bytes); 1: set up set; bit 7 set by actors later */
#define V_035D 0x00035Du            /* the second byte from the level's entry at $81:B933 */
/* In bank $80: per $035D a mask of the flags to set; and the bit of each
   flag (1, 2, 4 ... $80) */
#define LEVEL_FLAG_MASKS 0xD27Eu
#define FLAG_BITS 0xBCA1u

#define FLAG_SPOTS 0xD24Au  /* by level from $0E: the offset of its spots; there, the count, then 4 bytes a spot: x, y (in the bank DB names) */

/*
 * @routine 83:C2F0 LevelFlagFromMask
 * WHAT IT DOES: Sets flag X of the level's eight to 1 when its bit is set
 * in the mask for the level's $035D, else to 0.
 * HOW: The mask (by $035D) ANDed with the flag's bit; nonzero becomes 1;
 * stored to the flag.
 * Entry: M 8-bit, X 8-bit; X = the flag (0-7); DB = $80; called by JSR (by
 * 83:C2E7 for all eight, from the level's set-up at 81:8BEF, and by
 * actors). Exit: M 8-bit, X 8-bit; A low = the flag set; Y = $035D. No
 * level the game plays has a mask bit set: setting a flag to 1 is reached
 * only by unit_level_flags.box.
 */
void abi_LevelFlagFromMask(void)
{
    uint8_t set;

    y8_load(rd8(dba(V_035D)));
    set = (uint8_t)(rd8(dba(LEVEL_FLAG_MASKS) + cpu.y) & rd8(dba(FLAG_BITS) + cpu.x));
    a8_load(set);
    if (set)
        a8_load(0x01);
    wr8(LEVEL_FLAGS + cpu.x, (uint8_t)cpu.a);
}

/*
 * @routine 83:C2A9 LevelFlagSpotFind
 * WHAT IT DOES: Finds a point ($000C, $000E, each to 8 pixels) in the
 * level's list of flagged spots: C clear and X its flag when found, C set
 * when not.
 * HOW: Both words rounded down to 8; the level less $0E picks the list
 * (from $80:D24A): its count, then 4 bytes a spot (x, y); compared in
 * turn, 16-bit A.
 * Entry: M 8-bit, X 8-bit; $000C, $000E = the point; DB = $80; levels $0E
 * up; called by JSR (83:C181). Exit: M 8-bit; found: C clear, X = the
 * count of spots from it to the list's end less 1; not found: C set
 * (reached only by unit_point_lookup_miss.box).
 */
void abi_LevelFlagSpotFind(void)
{
    uint8_t at;
    uint16_t v;

    /* the point to 8-pixel cells */
    a8_load(0x07);
    trb_at(dba(SCRATCH), 0x07);
    trb_at(dba(SCRATCH + 2), 0x07);
    flag_set(P_C, 1);
    at = op_sbc8(rd8(dba(LEVEL)), 0x0E, &cpu.p);
    cpu.x = (uint8_t)(at << 1);
    op_rep(P_M);
    y8_load(rd8(dba(FLAG_SPOTS) + cpu.x));
    x8_load(rd8(dba(FLAG_SPOTS) + cpu.y));
    do {
        v = rd16(dba(FLAG_SPOTS + 1) + cpu.y) & 0xFFF8;
        cpu.a = v;
        cmp16_of(v, rd16(dba(SCRATCH)));
        if (v == rd16(dba(SCRATCH))) {
            v = rd16(dba(FLAG_SPOTS + 3) + cpu.y) & 0xFFF8;
            cpu.a = v;
            cmp16_of(v, rd16(dba(SCRATCH + 2)));
            if (v == rd16(dba(SCRATCH + 2))) {
                op_sep(P_M);
                x8_load((uint8_t)(cpu.x - 1));
                flag_set(P_C, 0);
                return;
            }
        }
        cpu.y = (uint8_t)(cpu.y + 4);
        x8_load((uint8_t)(cpu.x - 1));
    } while (cpu.x);
    op_sep(P_M);
    flag_set(P_C, 1);
}

