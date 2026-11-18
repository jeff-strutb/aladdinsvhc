/*
 * anim_sets.c: which animation sets a level's actors need.
 *
 * Each actor type uses one animation set. Loading a level marks the sets
 * of the types it has (the always-present types and the level's own) and
 * lists each set once, so each is loaded once.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "cpu_ops.h"
#include "native.h"
void abi_AnimSetsLoad(void);
#include "wram.h"
#include "wram_video.h"
#include "actor_anim.h"

void abi_AnimSetsListEnd(void);
void abi_AnimSetsPlaces(void);

#define NO_SET 0xFF
/* Each type's animation set, 256 bytes in bank $81; the first 32 types
   are present in every level. */
#define TYPE_ANIM_SETS 0x81EF6Fu
#define ALWAYS_TYPES 32
/* Per level, the offset (from LEVEL_ACTORS) of its actor list in bank
   $82: a count, then 6-byte entries whose first byte is the type. */
#define LEVEL_ACTORS 0x8A7Au
#define LEVEL_ACTOR_SIZE 6

/* AnimSetsLoad's banks and sources, and its direct page (D = the
   caller's: 0); the lists as the data bank $7F sees them */
#define SET_PICTURES 0x8000u        /* per set: its picture list's offset (word), then its bank less $A6 */
#define SET_PICTURES_BANK 0xA6
#define SET_TABLES 0xC000u          /* per set: its animation table's offset (word) */
#define SET_TABLES_BANK 0xA5
#define LEVEL_DATA_BANK 0x82
#define WORK_BANK_HIGH 0x7F
#define LOAD_SETS_LEFT 0x0C
#define LOAD_LIST_AT 0x10
#define LOAD_LEFT 0x16
#define LOAD_SOURCE 0x18            /* long pointer (bank at $1A) */
#define SET_OF_TYPE 0xF000u         /* ANIM_SET_OF_TYPE in bank $7F */
#define SETS_USED 0xF100u           /* ANIM_SETS_USED in bank $7F */
#define SET_PLACE_OF_TYPE 0xE000u   /* ANIM_SET_PLACE_OF_TYPE in bank $7F */

/*
 * @routine 81:9D4C AnimSetsFill
 * WHAT IT DOES: Sets every actor type's animation set to A.
 * HOW: Stores A in all 256 bytes, the first, then the last down.
 * Entry: M 8-bit, X 8-bit; A low byte = the value. Exit: X = 0; Z set,
 * N clear; A, Y, C, V unchanged.
 */
void abi_AnimSetsFill(void)
{
    ldx8_v(0x00);
    do {                                  /* the first, then the last down */
        sta8(ea_longx(ANIM_SET_OF_TYPE));
        dex8();
    } while (!IF_Z);
}

/*
 * @routine 81:9D4A AnimSetsClear
 * WHAT IT DOES: Marks every actor type as needing no animation set.
 * HOW: AnimSetsFill with $FF.
 * Entry: M 8-bit, X 8-bit. Exit: A low byte = $FF; X = 0; Z set, N clear;
 * Y, C, V unchanged.
 */
void abi_AnimSetsClear(void)
{
    lda8_v(NO_SET);
    abi_AnimSetsFill();                   /* on into it */
}

/*
 * @routine 81:906B AnimSetsAlways
 * WHAT IT DOES: Marks the animation sets of the 32 types every level has.
 * HOW: Copies the first 32 bytes of the type table, last first.
 * Entry: M 8-bit, X 8-bit; called by JSR. Exit: A low byte = type 0's set;
 * X = $FF; N set, Z clear; Y, C, V unchanged.
 */
void abi_AnimSetsAlways(void)
{
    ldx8_v(ALWAYS_TYPES - 1);
    do {
        lda8(ea_longx(TYPE_ANIM_SETS));
        sta8(ea_longx(ANIM_SET_OF_TYPE));
        dex8();
    } while (!IF_N);
}

/*
 * @routine 81:9D0C AnimSetsOfLevel
 * WHAT IT DOES: Marks the animation sets of the actor types the level has.
 * HOW: With the data bank at $82: the level's actor list; for each of its
 * entries, the type's set is copied from the type table, counting in
 * scratch $0C.
 * Entry: M 8-bit, X 8-bit. Exit: M and X 8-bit; DB as at entry, N and Z
 * from it (PLB); with entries, A = the entry offset past the last (low
 * byte; C, V from its ADC), X = the last type, Y = the low byte of that
 * offset; with none, A = 0, Y = the list's offset, X, C, V unchanged.
 */
void abi_AnimSetsOfLevel(void)
{
    phb();
    lda8_v(LEVEL_DATA_BANK);
    pha8();
    plb();
    op_rep(0x30);
    lda16(ea_abs((uint16_t)LEVEL));
    cpu.a = asl16_r(cpu.a);
    tay16();
    lda16(ea_absy(LEVEL_ACTORS));
    tay16();
    lda16(ea_absy(LEVEL_ACTORS));
    and16_v(0x00FF);
    if (!IF_Z) {
        sta16(ea_abs(SCRATCH));
        iny16();
        do {                              /* each of the level's actors */
            lda16(ea_absy(LEVEL_ACTORS));
            and16_v(0x00FF);
            tax16();
            op_sep(0x20);
            lda8(ea_longx(TYPE_ANIM_SETS));
            sta8(ea_longx(ANIM_SET_OF_TYPE));
            op_rep(0x20);
            clc();
            tya16();
            adc16_v(LEVEL_ACTOR_SIZE);
            tay16();
            MEM16(dec, ea_abs(SCRATCH));
        } while (!IF_Z);
    }
    op_sep(0x30);
    plb();
}

/*
 * @routine 81:902C AnimSetsList
 * WHAT IT DOES: Lists each animation set the level's types use, once.
 * HOW: With the data bank at $7F: for each type with a set, looks for the
 * set in the list; if it is not there, it takes the end's place, with a 0
 * after it and a new end mark.
 * Entry: M 8-bit, X 8-bit; called by JSR. Exit: X = 0; A low byte and Y
 * as the last type's search leaves them; C from its last compare; DB as at
 * entry, N and Z from it (PLB); V unchanged.
 */
void abi_AnimSetsList(void)
{
    phb();
    lda8_v(WORK_BANK_HIGH);
    pha8();
    plb();
    lda8_v(NO_SET);
    sta8(ea_abs(SETS_USED));
    ldx8_v(0x00);
    do {                                  /* each actor type */
        lda8(ea_absx(SET_OF_TYPE));
        cmp8_v(NO_SET);
        if (!IF_Z) {
            ldy8_v(0x00);
            for (;;) {                    /* already listed? */
                lda8(ea_absx(SET_OF_TYPE));
                cmp8_v(rd8(ea_absy(SETS_USED)));
                if (IF_Z)
                    break;
                lda8(ea_absy(SETS_USED));
                iny8();
                iny8();
                cmp8_v(NO_SET);
                if (IF_Z) {               /* the end: listed there */
                    lda8(ea_absx(SET_OF_TYPE));
                    dey8();
                    dey8();
                    sta8(ea_absy(SETS_USED));
                    iny8();
                    lda8_v(0x00);
                    sta8(ea_absy(SETS_USED));
                    iny8();
                    lda8_v(NO_SET);
                    sta8(ea_absy(SETS_USED));
                    break;
                }
            }
        }
        inx8();
    } while (!IF_Z);
    plb();
}

/*
 * @routine 81:9079 AnimSetsListEnd
 * WHAT IT DOES: Finds the end of the level's list of animation sets.
 * HOW: Steps through the list a pair at a time until the $FF end mark.
 * Entry: M 8-bit, X 8-bit; called by JSR. Exit: X = the end mark's offset
 * + 2 (twice the sets listed, + 2); A low byte = $FF; Z set, C set, N
 * clear; Y, V unchanged.
 */
void abi_AnimSetsListEnd(void)
{
    ldx8_v(0x00);
    do {
        lda8(ea_longx(ANIM_SETS_USED));
        inx8();
        inx8();
        cmp8_v(NO_SET);
    } while (!IF_Z);
}

/*
 * @routine 81:9086 AnimSetsPlaces
 * WHAT IT DOES: For each actor type, finds where its animation set is in
 * the level's list of sets.
 * HOW: With the data bank at $7F: AnimSetsListEnd gives the sets listed;
 * for each of the 256 types the place is 4 (not listed) unless its set is
 * found in the list, when it is the set's place (offset / 2); the count
 * of sets left to look at is kept in scratch $0E.
 * Entry: M 8-bit, X 8-bit; called by JSR. Exit: Y = 0; X as the last
 * type's search left it; $0C = the sets listed; DB as at entry, N and Z
 * from it (PLB).
 */
void abi_AnimSetsPlaces(void)
{
    phb();
    lda8_v(WORK_BANK_HIGH);
    pha8();
    plb();
    call_frame(0x908D, abi_AnimSetsListEnd);
    txa8();
    set_al(lsr8_r(al()));
    set_al(dec8_r(al()));
    sta8(ea_abs(SCRATCH));                /* the sets listed ($7F:000C) */
    ldy8_v(0x00);
    do {                                  /* each actor type */
        lda8(ea_abs(SCRATCH));
        sta8(ea_abs(SCRATCH + 2));
        lda8_v(0x04);
        sta8(ea_absy(SET_PLACE_OF_TYPE));
        ldx8_v(0x00);
        for (;;) {
            lda8(ea_absy(SET_OF_TYPE));
            cmp8_v(rd8(ea_absx(SETS_USED)));
            if (IF_Z) {
                txa8();
                set_al(lsr8_r(al()));
                sta8(ea_absy(SET_PLACE_OF_TYPE));
                break;
            }
            inx8();
            inx8();
            MEM8(dec, ea_abs(SCRATCH + 2));
            if (IF_Z)
                break;
        }
        iny8();
    } while (!IF_Z);
    plb();
}

/*
 * @routine 81:8F47 AnimSetsLoad
 * WHAT IT DOES: Loads the level's animation sets: marks and lists the sets
 * its actor types use, and copies each set's picture list and animation
 * tables into work RAM.
 * HOW: With DB = $A6: AnimSetsClear, AnimSetsOfLevel, AnimSetsAlways,
 * AnimSetsList, AnimSetsListEnd. For each set listed: its picture list's
 * place ($7E:2000 + X) goes to anim_set_gfx; its entry in the table at
 * $A6:8000 (set * 3: offset from $8000, bank from $A6) gives a byte count
 * and the bytes, copied through the long pointer at $18 (each step past
 * $FFFF going on at $8000 of the next bank). Then with DB = $A5: for each
 * set, its table's place ($7E:B000 + X) goes to anim_set_table, and the
 * words of its table (offset at $A5:C000 + set * 2; a count / 2 + 1) are
 * copied. Last, AnimSetsPlaces. Scratch $0C counts the sets, $10 indexes
 * the lists, $16 counts bytes or words. The step at $81:8FAD (a count at
 * a bank's last two bytes) is in no set and no script runs it.
 * Entry: DB, P kept on the stack; D = 0; called by JSL. Exit: P and DB as
 * at entry; A, X, Y as AnimSetsPlaces leaves them.
 */
void abi_AnimSetsLoad(void)
{
    op_sep(0x30);
    phb();
    php();
    lda8_v(SET_PICTURES_BANK);
    pha8();
    plb();
    jsl_frame(0x818F4Fu); abi_AnimSetsClear(); cpu.s = (uint16_t)(cpu.s + 3);
    jsl_frame(0x818F53u); abi_AnimSetsOfLevel(); cpu.s = (uint16_t)(cpu.s + 3);
    call_frame(0x8F59, abi_AnimSetsAlways);
    call_frame(0x8F5C, abi_AnimSetsList);
    call_frame(0x8F5F, abi_AnimSetsListEnd);
    txa8();
    op_rep(0x30);
    and16_v(0x00FF);
    cpu.a = lsr16_r(cpu.a);
    cpu.a = dec16_r(cpu.a);
    sta16(ea_dp(LOAD_SETS_LEFT));
    stz16(ea_dp(LOAD_LIST_AT));
    ldx16_v(0x0000);
    do {                                  /* each set's picture list */
        phx16();
        txa16();
        ldx16(ea_dp(LOAD_LIST_AT));
        MEM16(inc, ea_dp(LOAD_LIST_AT));
        MEM16(inc, ea_dp(LOAD_LIST_AT));
        sta16(ea_longx(ANIM_SET_GFX));
        lda16(ea_longx(ANIM_SETS_USED));
        cpu.a = asl16_r(cpu.a);
        clc();
        adc16_v(rd16(ea_longx(ANIM_SETS_USED)));
        tay16();
        plx16();
        lda16(ea_absy(SET_PICTURES));
        pha16();
        iny16();
        iny16();
        stz16(ea_dp(LOAD_SOURCE));
        lda16(ea_absy(SET_PICTURES));
        and16_v(0x00FF);
        clc();
        adc16_v(SET_PICTURES_BANK);
        sta16(ea_dp(LOAD_SOURCE + 2));
        pla16();
        clc();
        adc16_v(0x8000);
        tay16();
        lda16(ea_dpily(LOAD_SOURCE));
        sta16(ea_dp(LOAD_LEFT));
        iny16();
        iny16();
        clc();
        cpy16_v(0x8000);
        if (!IF_C) {                      /* past $FFFF: on at $8000 of the next bank */
            tya16();
            clc();
            adc16_v(0x8000);
            tay16();
            MEM16(inc, ea_dp(LOAD_SOURCE + 2));
        }
        do {                              /* its bytes */
            op_sep(0x20);
            lda8(ea_dpily(LOAD_SOURCE));
            sta8(ea_longx(PICTURE_LIST));
            inx16();
            iny16();
            clc();
            cpy16_v(0x8000);
            if (!IF_C) {
                op_rep(0x20);
                tya16();
                clc();
                adc16_v(0x8000);
                tay16();
                MEM16(inc, ea_dp(LOAD_SOURCE + 2));
            }
            op_rep(0x20);
            MEM16(dec, ea_dp(LOAD_LEFT));
        } while (!IF_Z);
        MEM16(dec, ea_dp(LOAD_SETS_LEFT));
    } while (!IF_Z);
    op_sep(0x30);
    lda8_v(SET_TABLES_BANK);
    pha8();
    plb();
    call_frame(0x8FE1, abi_AnimSetsListEnd);
    op_rep(0x30);
    txa16();
    and16_v(0x00FF);
    cpu.a = lsr16_r(cpu.a);
    cpu.a = dec16_r(cpu.a);
    sta16(ea_dp(LOAD_SETS_LEFT));
    stz16(ea_dp(LOAD_LIST_AT));
    ldx16_v(0x0000);
    do {                                  /* each set's animation table */
        phx16();
        txa16();
        ldx16(ea_dp(LOAD_LIST_AT));
        MEM16(inc, ea_dp(LOAD_LIST_AT));
        MEM16(inc, ea_dp(LOAD_LIST_AT));
        sta16(ea_longx(ANIM_SET_TABLE));
        lda16(ea_longx(ANIM_SETS_USED));
        cpu.a = asl16_r(cpu.a);
        tay16();
        plx16();
        lda16(ea_absy(SET_TABLES));
        tay16();
        lda16(ea_absy(SET_TABLES));
        cpu.a = lsr16_r(cpu.a);
        cpu.a = inc16_r(cpu.a);
        sta16(ea_dp(LOAD_LEFT));
        iny16();
        iny16();
        do {                              /* its words */
            lda16(ea_absy(SET_TABLES));
            sta16(ea_longx(ANIM_DATA));
            iny16();
            iny16();
            inx16();
            inx16();
            MEM16(dec, ea_dp(LOAD_LEFT));
        } while (!IF_Z);
        MEM16(dec, ea_dp(LOAD_SETS_LEFT));
    } while (!IF_Z);
    op_sep(0x30);
    call_frame(0x9028, abi_AnimSetsPlaces);
    plp();
    plb();
}
