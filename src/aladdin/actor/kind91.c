/*
 * kind91.c: actor kind $91, a marker that steers the level's escort actor:
 * by its $07 it sets the escort's mode once one has passed the other, or
 * at once, or sets or clears one of the escort's flags, and ends.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorEndPlaced(void);

#define ESCORT_X_PIXELS (ACTOR_ESCORT + ACTOR_X + 1)   /* the escort's x (pixels, word) */
#define ESCORT_Y_PIXELS (ACTOR_ESCORT + ACTOR_Y + 1)   /* the escort's y (pixels, word) */
#define ESCORT_45 (ACTOR_ESCORT + 0x45)                /* nonzero: the escort is busy; a marker waits */
#define ESCORT_46 (ACTOR_ESCORT + 0x46)                /* an escort flag the markers set (variant 4) and clear (5) */
#define ESCORT_MODE (ACTOR_ESCORT + 0x4D)              /* the escort's mode, which the markers set */
#define KIND91_VARIANTS 0xF53Fu                        /* (bank $A5) by $07: where each variant goes */

/*
 * @routine A5:F534 Kind91Start
 * @actor
 * WHAT IT DOES: Actor kind $91, a marker for the level's escort actor.
 * By $07: 0, once the escort is idle and this marker is at or past the
 * escort's x, the escort's mode 4 (and so each frame on); 1, once the
 * escort is idle and at or past the marker's x, mode 5 (each frame on);
 * 2, mode 6 if the escort is idle; 3, if the escort is busy: idle again,
 * mode 1 and the escort moved to the marker's x and y; 4, ESCORT_46 set;
 * 5, ESCORT_46 cleared. Variants 2 to 5 end at once.
 * HOW: ACTOR_09 bit 7 set (its kind's wait routine runs, Kind91FreeOff);
 * JMP through KIND91_VARIANTS by $07 * 2. Variants 0 and 1: each frame
 * (BRK) ESCORT_45 tested; when 0, X 16-bit, the marker's x and the
 * escort's compared (0: marker's - escort's, 1: escort's - marker's) and
 * with no borrow ESCORT_MODE 4 or 5; M back to 8-bit (X stays 16-bit).
 * Variants 2 to 5 end in ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the JML of kind $91's stub
 * ($83:936F) from the actor loop in the start state. Exit: never returns:
 * 0 and 1 run on, 2 to 5 end in ActorEndPlaced.
 */
void abi_Kind91Start(void)
{
    lda8_v(0x80);  /* A5F534 lda */
    tsb8(ea_dp(ACTOR_09));  /* A5F536 tsb */
    lda8(ea_dp(ACTOR_07));  /* A5F538 lda */
    ACC8(asl);  /* A5F53A asl */
    tax8();  /* A5F53B tax */
    switch (cpu.x) {  /* A5F53C jmp (KIND91_VARIANTS,x) */
    case 0:
        for (;;) {
            actor_yield(0xA5F54Bu);  /* A5F54B brk */
            lda8(ea_abs((uint16_t)ESCORT_45));  /* A5F54D lda */
            if (!IF_Z)  /* A5F550 bne */
                continue;
            op_rep(0x10);  /* A5F552 rep */
            ldx16(ea_dp(ACTOR_X + 1));  /* A5F554 ldx */
            cpx16_v(rd16(ea_abs((uint16_t)ESCORT_X_PIXELS)));  /* A5F556 cpx */
            if (IF_C) {  /* A5F559 bcc */
                lda8_v(0x04);  /* A5F55B lda */
                sta8(ea_abs((uint16_t)ESCORT_MODE));  /* A5F55D sta */
            }
            op_sep(0x20);  /* A5F560 sep */
        }
    case 2:
        for (;;) {
            actor_yield(0xA5F564u);  /* A5F564 brk */
            lda8(ea_abs((uint16_t)ESCORT_45));  /* A5F566 lda */
            if (!IF_Z)  /* A5F569 bne */
                continue;
            op_rep(0x10);  /* A5F56B rep */
            ldx16(ea_abs((uint16_t)ESCORT_X_PIXELS));  /* A5F56D ldx */
            cpx16_v(rd16(ea_dp(ACTOR_X + 1)));  /* A5F570 cpx */
            if (IF_C) {  /* A5F572 bcc */
                lda8_v(0x05);  /* A5F574 lda */
                sta8(ea_abs((uint16_t)ESCORT_MODE));  /* A5F576 sta */
            }
            op_sep(0x20);  /* A5F579 sep */
        }
    case 4:
        lda8(ea_abs((uint16_t)ESCORT_45));  /* A5F57D lda */
        if (IF_Z) {  /* A5F580 bne */
            lda8_v(0x06);  /* A5F582 lda */
            sta8(ea_abs((uint16_t)ESCORT_MODE));  /* A5F584 sta */
        }
        abi_ActorEndPlaced();  /* A5F587 jml */
        return;
    case 6:
        lda8(ea_abs((uint16_t)ESCORT_45));  /* A5F58B lda */
        if (!IF_Z) {  /* A5F58E beq */
            stz8(ea_abs((uint16_t)ESCORT_45));  /* A5F590 stz */
            lda8_v(0x01);  /* A5F593 lda */
            sta8(ea_abs((uint16_t)ESCORT_MODE));  /* A5F595 sta */
            op_rep(0x20);  /* A5F598 rep */
            lda16(ea_dp(ACTOR_X + 1));  /* A5F59A lda */
            sta16(ea_abs((uint16_t)ESCORT_X_PIXELS));  /* A5F59C sta */
            lda16(ea_dp(ACTOR_Y + 1));  /* A5F59F lda */
            sta16(ea_abs((uint16_t)ESCORT_Y_PIXELS));  /* A5F5A1 sta */
            op_sep(0x20);  /* A5F5A4 sep */
        }
        abi_ActorEndPlaced();  /* A5F5A6 jml */
        return;
    case 8:
        lda8_v(0x01);  /* A5F5AA lda */
        sta8(ea_abs((uint16_t)ESCORT_46));  /* A5F5AC sta */
        abi_ActorEndPlaced();  /* A5F5AF jml */
        return;
    case 10:
        stz8(ea_abs((uint16_t)ESCORT_46));  /* A5F5B3 stz */
        abi_ActorEndPlaced();  /* A5F5B6 jml */
        return;
    default:
        jump_routine(0xA50000u | rd16(0xA50000u | (uint16_t)(KIND91_VARIANTS + cpu.x)));
        return;
    }
}
