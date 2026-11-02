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

/* The actor's box (by its $12) round the range point: C clear when the
   point is within it in both x and y. */
static int box_holds_range_point(void)
{
    uint32_t box = dba(VIEW_BOXES) + cpu.x;
    uint16_t d;

    flag_set(P_C, 1);
    d = op_sbc16(rd16(dba(RANGE_X)), dp16(ACTOR_X + 1), &cpu.p);
    flag_set(P_C, 0);
    d = op_adc16(d, rd16(box), &cpu.p);
    cpu.a = d;
    cmp16_of(d, rd16(box + 2));
    if (d >= rd16(box + 2))
        return 0;
    flag_set(P_C, 1);
    d = op_sbc16(rd16(dba(RANGE_Y)), dp16(ACTOR_Y + 1), &cpu.p);
    flag_set(P_C, 0);
    d = op_adc16(d, rd16(box + 4), &cpu.p);
    cpu.a = d;
    cmp16_of(d, rd16(box + 6));
    return d < rd16(box + 6);
}

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
 * Entry: DB = $81; called by JSL. Exit: P and D as at entry; A, X, Y as
 * the walk left them.
 */
void abi_ActorsInView(void)
{
    php();
    phd();
    op_rep(0x30);
    for (int i = 0; i < 4; i++)
        wr16(dba((uint16_t)(VIEW_COUNTS + 2 * i)), 0);
    for (int i = 0; i < 4; i++)
        wr16(dba((uint16_t)(VIEW_LIST_NEXT + 2 * i)), rd16(dba((uint16_t)(VIEW_LIST_STARTS + 2 * i))));
    /* each of the 35 records from the escort's */
    cpu.y = 0x0023;
    cpu.a = cpu.d = ACTOR_ESCORT;
    do {
        uint16_t flags;

        cpu.a = dp16(ACTOR_STATE) & 0x00FF;
        if (cpu.a) {
            cpu.x = dp16(ACTOR_12) & 0x00FF;
            if (box_holds_range_point()) {
                /* in view: bit 14 of $08; listed when bit 3 is set */
                flags = (uint16_t)(dp16(ACTOR_08) | 0x4000);
                dp16_set(ACTOR_08, flags);
                cpu.a = flags;
                nz16_of(flags);
                flag_set(P_Z, !(flags & 0x0008));
                if (flags & 0x0008) {
                    phy16();
                    cpu.x = (uint16_t)((flags & 0x0003) << 1);
                    flag_set(P_C, 0);
                    cpu.y = rd16(dba(VIEW_LIST_NEXT) + cpu.x);
                    cpu.a = cpu.d;
                    wr16(dba(VIEW_LISTS) + cpu.y, cpu.d);
                    inc16_at(dba(VIEW_LIST_NEXT) + cpu.x);
                    inc16_at(dba(VIEW_LIST_NEXT) + cpu.x);
                    cpu.y = cpu.x;
                    cpu.x = rd16(dba(VIEW_GROUP_OF) + cpu.y);
                    inc16_at(dba(VIEW_COUNTS) + cpu.x);
                    ply16();
                }
            } else {
                flags = (uint16_t)(dp16(ACTOR_08) & 0xBFFF);
                dp16_set(ACTOR_08, flags);
                a16_load(flags);
            }
        }
        flag_set(P_C, 0);
        cpu.a = op_adc16(cpu.d, ACTOR_RECORD_SIZE, &cpu.p);
        cpu.d = cpu.a;
        cpu.y = (uint16_t)(cpu.y - 1);
        nz16_of(cpu.y);
    } while (cpu.y);
    pld();
    plp();
}
