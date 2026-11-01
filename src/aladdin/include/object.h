/*
 * object.h: an object's record as its code reaches it, through the direct
 * page (D = the record's address). Offsets whose purpose is not yet
 * established keep their offset as their name. The same names as
 * asm/aladdin/include/object.inc, in lower case there.
 */
#ifndef ALADDIN_OBJECT_H
#define ALADDIN_OBJECT_H

#include "machine.h"

#define OBJ_00 0x00      /* the object's state ($8C, $CC ...) */
#define OBJ_07 0x07
#define OBJ_08 0x08      /* low two bits: set from the frame's parity by some objects */
#define OBJ_09 0x09      /* bits 4-5 set by height by some objects */
#define OBJ_0C 0x0C      /* word: X position */
#define OBJ_0F 0x0F      /* word: Y position */
#define OBJ_18 0x18      /* zero: free for the list walk at $84:8A52 */
#define OBJ_19 0x19      /* three bytes, loaded together from a table */
#define OBJ_1D 0x1D      /* three bytes, loaded together from a table (bit 7 of $1F a sign) */
#define OBJ_20 0x20
#define OBJ_37 0x37      /* word: the next record in its list (0 ends it) */
#define OBJ_40 0x40      /* word */
#define OBJ_44 0x44
#define OBJ_45 0x45
#define OBJ_46 0x46
#define OBJ_47 0x47
#define OBJ_48 0x48

/* The bank-0 address of direct-page byte `off`. */
static inline uint32_t dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

#endif
