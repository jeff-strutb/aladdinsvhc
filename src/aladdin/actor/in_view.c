/*
 * in_view.c: which actors are near enough the view to be hit, and the
 * lists of them, by their low two bits of $08, that other code walks.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_ActorsInView(void);
#include "actor.h"

/* The actors in view, sorted into four lists by the low two bits of their
   $08 */
#define VIEW_COUNTS 0x1592u       /* the actors in each group (words, by VIEW_GROUP_OF) */
#define VIEW_LIST_NEXT 0x159Au    /* by list: where its next entry goes (words) */
#define VIEW_LISTS 0x1492u        /* the lists: actor records (words) */
#define VIEW_LIST_STARTS 0xBA2Au  /* by list, its first entry's offset (in the bank DB names) */
#define VIEW_BOXES 0xBA02u        /* by hit box: x offset, x span, y offset, y span (words) */
#define VIEW_GROUP_OF 0xBA32u     /* by list, the count it adds to */
#define RANGE_X 0x1A44u           /* the point actors are kept in range of (words) */
#define RANGE_Y 0x1A46u
#define ACTOR_RECORD_SIZE 0x0050u


/*
 * @routine 81:9198 ActorsInView
 * WHAT IT DOES: Marks which actors can be hit this frame (bit 6 of $09):
 * those whose box, picked by their $12, holds the view's point; and lists
 * those with bit 3 of $08 set, by their $08's low two bits.
 * HOW: The counts cleared and the lists' next offsets set from their
 * starts; then for each of the 35 records from actor_escort that is in use:
 * the view point less its x and y, plus the box's offsets, under the box's
 * width and height sets the bit, else clears it; with the bit set and bit
 * 3 of $08 set, the record's address goes on its list and the list's count
 * steps up.
 * Entry: DB = $80; called by JSL. Exit: P and D as at entry; A, X, Y as
 * the walk left them.
 */
void abi_ActorsInView(void)
{
    php();
    phd();
    op_rep(0x30);
    stz16(ea_abs(VIEW_COUNTS));
    stz16(ea_abs(VIEW_COUNTS + 2));
    stz16(ea_abs(VIEW_COUNTS + 4));
    stz16(ea_abs(VIEW_COUNTS + 6));
    for (int i = 0; i < 4; i++) {
        lda16(ea_abs((uint16_t)(VIEW_LIST_STARTS + 2 * i)));
        sta16(ea_abs((uint16_t)(VIEW_LIST_NEXT + 2 * i)));
    }
    /* each of the 35 records from the escort's */
    ldy16_v(0x0023);
    lda16_v(ACTOR_ESCORT);
    tcd();
    do {
        lda16(ea_dp(ACTOR_STATE));
        and16_v(0x00FF);
        if (!IF_Z) {
            lda16(ea_dp(ACTOR_12));
            and16_v(0x00FF);
            tax16();
            /* the range point within the box (by $12): in view */
            sec();
            lda16(ea_abs(RANGE_X));
            sbc16_v(rd16(ea_dp(ACTOR_X + 1)));
            clc();
            adc16_v(rd16(ea_absx(VIEW_BOXES)));
            cmp16_v(rd16(ea_absx(VIEW_BOXES + 2)));
            if (!IF_C) {
                sec();
                lda16(ea_abs(RANGE_Y));
                sbc16_v(rd16(ea_dp(ACTOR_Y + 1)));
                clc();
                adc16_v(rd16(ea_absx(VIEW_BOXES + 4)));
                cmp16_v(rd16(ea_absx(VIEW_BOXES + 6)));
            }
            if (!IF_C) {
                /* in view: bit 14 of $08; listed when bit 3 is set */
                lda16(ea_dp(ACTOR_08));
                ora16_v(0x4000);
                sta16(ea_dp(ACTOR_08));
                lda16(ea_dp(ACTOR_08));
                bit16_i(0x0008);
                if (!IF_Z) {
                    phy16();
                    and16_v(0x0003);
                    cpu.a = asl16_r(cpu.a);
                    tax16();
                    ldy16(ea_absx(VIEW_LIST_NEXT));
                    tdc();
                    sta16(ea_absy(VIEW_LISTS));
                    MEM16(inc, ea_absx(VIEW_LIST_NEXT));
                    MEM16(inc, ea_absx(VIEW_LIST_NEXT));
                    txy16();
                    ldx16(ea_absy(VIEW_GROUP_OF));
                    MEM16(inc, ea_absx(VIEW_COUNTS));
                    ply16();
                }
            } else {
                lda16(ea_dp(ACTOR_08));
                and16_v(0xBFFF);
                sta16(ea_dp(ACTOR_08));
            }
        }
        clc();
        tdc();
        adc16_v(ACTOR_RECORD_SIZE);
        tcd();
        dey16();
    } while (!IF_Z);
    pld();
    plp();
}
