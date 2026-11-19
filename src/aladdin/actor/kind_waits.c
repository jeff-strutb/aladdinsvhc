/*
 * kind_waits.c: the routines some actor kinds run each frame they wait (by the kind
 * table at $83:8F4F, or a kind's stub): animation, shots, touches and
 * their range. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram_state.h"
#include "wram_video.h"
#include "wram.h"
#include "actor.h"
#include "wram_player.h"

void abi_ActorEndOutOfRange(void);
void abi_ActorFreePlaced(void);
void abi_ActorHitsTestBoxLong(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);
void abi_ActorDownedByShot(void);
void abi_ActorStompOrHurt(void);
void abi_ActorTakesShot(void);
void abi_ActorStopsShot(void);
void abi_ActorZoneHasTestBox(void);
void abi_PlayerEnter8CFacing(void);
void abi_PlayerEnter8CFacingLong(void);
void abi_ActorFreeOutOfRangeLong(void);
void abi_ActorHurtsPlayerOnTouchLong(void);
void abi_ActorStopsShotLong(void);

void abi_ActorCreateChild8(void);
void abi_ActorHitsTestBoxIfPlayerFree(void);
void abi_ActorYByFrame(void);
void abi_BackdropActorShow(void);
void abi_BackdropPictureStep(void);
void abi_PlayerOnFirmGround(void);
void abi_PlayerOnFirmGroundProbe4(void);
void abi_PlayerOnFirmGroundProbe8(void);
void abi_ActorEndsShot(void);
void abi_ActorStopsTouchingKind2D(void);
void abi_Kind68PlayerOn(void);

#define ACTOR_43 0x43u
#define KIND68_ON 0x16u        /* nonzero: the player rides it this frame */
#define KIND68_WAS_ON 0x17u    /* the same, the frame before */
#define KIND68_PLAYER_X 0x1Du  /* the player's x pixels kept (word) */
#define KIND68_PAUSE 0x20u     /* frames left before it carries again */
#define KIND68_VARIANT 0x47u   /* which of its shapes (0: flat) */
#define KIND68_SPOTS 0xD61Fu   /* in bank $80, by variant, flip: its spot's x and y offsets (words; 4 bytes) */
#define KIND68_BOXES 0xD5FFu   /* in bank $80, by variant, flip: x and y offsets, half-width, half-height (words; 8 bytes) */
#define KIND68_DIRS 0xD62Fu    /* in bank $80, by variant times 2 plus flip: the player's direction word on it */
#define SPOT_X 0x00002Cu       /* scratch: its spot's x (word) */
#define SPOT_Y 0x00002Eu       /* scratch: its spot's y (word) */
#define FEET_Y 0x00001Cu       /* scratch: the player's feet, his y plus $25 or $20 (word) */
#define TOP_Y 0x00001Eu        /* scratch: its top under him (word) */
#define CHAIN_HEAD_6D 0x1B14u     /* kind $6D variant $1C: set, it enters state $8C */
#define RANGE_X_6D 0x1A44u        /* the point actors are kept in range of (x) */
#define RANGE_Y 0x001A46u      /* the point actors are kept in range of (y) */
#define PLAYER_X (PLAYER + ACTOR_X + 1)
#define PLAYER_Y (PLAYER + ACTOR_Y + 1)
#define PLAYER_SPEED_Y_HI (PLAYER + ACTOR_SPEED_Y + 2) /* negative: he moves up */
#define V_1427 0x001427u
#define V_141B 0x00141Bu
#define ACTOR_END_PLACED 0x8396CAu
#define KIND2D_STOPPED 0x4Fu   /* kind $2D: nonzero once an actor has stopped it */
#define HIT_BOXES 0xC29Au      /* the hit boxes: 4 bytes each, the half-width first (data bank) */
#define POOL_SCAN_END 0x141Cu  /* where ActorStopsTouchingKind2D's walk stops: one past $141B */
#define ACTOR_4E 0x4Eu   /* word: the record of its holder (kind $53) */

/*
 * @routine 83:E645 ActorCount42Down
 * WHAT IT DOES: Counts the actor's word at $42 down by 1, not below 0.
 * HOW: A 16-bit subtraction; negative becomes 0.
 * Entry: M 16-bit; D = the actor's record; called by JSR (83:E60C,
 * 83:E633). Exit: M 16-bit; A = the new count; C from the subtraction.
 */
void abi_ActorCount42Down(void)
{
    flag_set(P_C, 1);
    cpu.a = op_sbc16(dp16(ACTOR_42), 0x0001, &cpu.p);
    if (cpu.a & 0x8000)                   /* not below 0 */
        a16_load(0x0000);
    dp16_set(ACTOR_42, cpu.a);
}


/*
 * @routine 83:A012 Kind27Wait
 * WHAT IT DOES: Kind $27's wait: animates; once its $42 is set, touching
 * the player it plays sound $4F, counts $036C up and ends; it ends at once
 * when $0367 is 0, or when out of range.
 * HOW: AnimStep; $42 clear returns. $0367 zero: ActorEndOutOfRange with
 * box $58. Else ActorHitsTestBoxLong with box $24: touched, SoundQueueAdd
 * $4F, $036C up, and into ActorEndOutOfRange past its range test (its
 * return dropped, freed); not touched, ActorEndOutOfRange with box $58.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Its $42 is set only by its untraced start for $07 bit 3 (83:9E58). Exit:
 * $42 clear: as AnimStep. Else in range: as ActorEndOutOfRange; ended:
 * does not return.
 */
void abi_Kind27Wait(void)
{
    jsl_frame(0x83A012u);
    abi_AnimStep();
    cpu.s = (uint16_t)(cpu.s + 3);
    a8_load(rd8(dpa(ACTOR_42)));
    if (!(uint8_t)cpu.a)
        return;
    a8_load(rd8(dba(HEALTH)));
    if ((uint8_t)cpu.a) {
        y8_load(0x24);
        jsl_frame(0x83A022u);
        abi_ActorHitsTestBoxLong();
        cpu.s = (uint16_t)(cpu.s + 3);
        if (!(cpu.p & P_C)) {             /* touched: sound $4F, counted, it ends */
            a8_load(0x4F);
            jsl_frame(0x83A02Fu);
            abi_SoundQueueAdd();
            cpu.s = (uint16_t)(cpu.s + 3);
            inc8_at(dba(0x036C));
            op_sep(0x30);                 /* $83:96C6 */
            pla8();
            pla8();
            abi_ActorFreePlaced();
            actor_leave(0x8394BFu);
            return;
        }
    }
    x8_load(0x58);
    abi_ActorEndOutOfRange();
}


/*
 * @routine 83:AEEC Kind31Wait
 * WHAT IT DOES: Kind $31's wait: with its $42 set it turns round at x
 * $0F00 and beyond in levels below 7, and may be downed by a shot; it
 * animates and is ended when out of range.
 * HOW: $42 set: level below 7 and x pixels $0F00 or more, facing and flip
 * toggled; ActorDownedByShot, downed: ActorStompOrHurt. Then AnimStep and
 * ActorEndOutOfRange with box $38 (by JML).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Called by JSR: it ends by JML to ActorEndOutOfRange, which returns to
 * the loop. Exit: as ActorEndOutOfRange.
 */
void abi_Kind31Wait(void)
{
    uint8_t level;
    uint16_t x;

    a8_load(rd8(dpa(ACTOR_42)));
    if ((uint8_t)cpu.a) {
        /* before level 7, past x $0F00 it turns */
        level = rd8(dba(LEVEL));
        a8_load(level);
        cmp8_of(level, 0x07);
        if (level < 0x07) {
            x = dp16(ACTOR_X + 1);
            cpu.a = x;
            cmp16_of(x, 0x0F00);
            if (x >= 0x0F00) {
                a8_load((uint8_t)(rd8(dpa(ACTOR_FACING)) ^ 0x01));
                wr8(dpa(ACTOR_FACING), (uint8_t)cpu.a);
                wr8(dpa(ACTOR_FLIP), (uint8_t)cpu.a);
            }
        }
        call_frame(0xAF0C, abi_ActorDownedByShot);
        if (cpu.p & P_C)
            call_frame(0xAF11, abi_ActorStompOrHurt);
    }
    jsl_frame(0x83AF12u);
    abi_AnimStep();
    cpu.s = (uint16_t)(cpu.s + 3);
    x8_load(0x38);
    abi_ActorEndOutOfRange();
}


/*
 * @routine 83:B94A Kind30Wait
 * WHAT IT DOES: Kind $30's wait: animates; variant 7 (low three bits of
 * $07) notes the first shot that reaches it ($43); ended when out of
 * range.
 * HOW: AnimStep; variant 7 with $43 clear: ActorTakesShot, hit: $43 up.
 * Then ActorEndOutOfRange with box $18.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Exit: as ActorEndOutOfRange.
 */
void abi_Kind30Wait(void)
{
    uint8_t variant;

    jsl_frame(0x83B94Au);
    abi_AnimStep();
    cpu.s = (uint16_t)(cpu.s + 3);
    variant = rd8(dpa(ACTOR_07)) & 0x07;
    a8_load(variant);
    cmp8_of(variant, 0x07);
    if (variant == 0x07) {                /* variant 7: one shot counts */
        a8_load(rd8(dpa(ACTOR_43)));
        if (!(uint8_t)cpu.a) {
            call_frame(0xB95C, abi_ActorTakesShot);
            if (!(cpu.p & P_C))
                inc8_at(dpa(ACTOR_43));
        }
    }
    x8_load(0x18);
    abi_ActorEndOutOfRange();
}


/*
 * @routine 83:C94A Kind46Wait
 * WHAT IT DOES: Kind $46's wait: a shot it stops sets its $43; with $07
 * bit 1 it is ended when out of range.
 * HOW: ActorStopsShot: stopped (C clear), $43 = 1. $07 bit 1:
 * ActorEndOutOfRange with box 0; else returns.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Exit: M 8-bit; A low = $07 AND 2; or as ActorEndOutOfRange.
 */
void abi_Kind46Wait(void)
{
    call_frame(0xC94C, abi_ActorStopsShot);
    if (!(cpu.p & P_C)) {
        a8_load(0x01);
        wr8(dpa(ACTOR_43), 0x01);
    }
    a8_load(rd8(dpa(ACTOR_07)) & 0x02);
    if (!(uint8_t)cpu.a)
        return;
    x8_load(0x00);
    abi_ActorEndOutOfRange();
}




/*
 * @routine 83:D64A Kind53Wait
 * WHAT IT DOES: Kind $53's wait: stomped or hurting as touched; sets its
 * $45 once the record its $4E names has $46 set; animates.
 * HOW: ActorStompOrHurt; $45 clear: the byte at $46 of the record at $4E
 * (16-bit X) nonzero, $45 = 1. AnimStep.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Exit: as AnimStep.
 */
void abi_Kind53Wait(void)
{
    call_frame(0xD64C, abi_ActorStompOrHurt);
    a8_load(rd8(dpa(ACTOR_45)));
    if (!(uint8_t)cpu.a) {                /* its holder's let-go copied in */
        op_rep(P_X);
        cpu.x = dp16(ACTOR_4E);
        a8_load(rd8(dba(ACTOR_46) + cpu.x));
        op_sep(P_X);
        if ((uint8_t)cpu.a) {
            a8_load(0x01);
            wr8(dpa(ACTOR_45), 0x01);
        }
    }
    jsl_frame(0x83D660u); abi_AnimStep(); cpu.s = (uint16_t)(cpu.s + 3);
}


/*
 * @routine 83:E679 Kind67Wait
 * WHAT IT DOES: Kind $67's wait: puts the player into state $8C while the
 * player is across x $04F8-$0E47 and the test box is within its zone (from
 * $1A2C) up and down.
 * HOW: The player's x pixels less $09A0 plus $04A8 below $0950; then the
 * test box's y against $C0 less $1A2C, by the test box's half-height plus
 * $4C (in $000C, doubled in $000E): inside, PlayerEnter8CFacingLong.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Exit: M 8-bit, X 8-bit; as PlayerEnter8CFacingLong, or flags from the
 * comparison.
 */
void abi_Kind67Wait(void)
{
    uint16_t d, half;

    /* the player within $04A8 of x $09A0, and his test box (by its half
       height) at the backdrop's height: he is caught */
    op_rep(P_M);
    flag_set(P_C, 1);
    d = op_sbc16(rd16(dba(PLAYER + ACTOR_X + 1)), 0x09A0, &cpu.p);
    flag_set(P_C, 0);
    d = op_adc16(d, 0x04A8, &cpu.p);
    cpu.a = d;
    cmp16_of(d, 0x0950);
    if (d < 0x0950) {
        flag_set(P_C, 0);
        half = op_adc16(rd16(dba(0x148A)), 0x004C, &cpu.p);
        wr16(dba(SCRATCH), half);
        flag_set(P_C, half & 0x8000);
        wr16(dba(SCRATCH + 2), (uint16_t)(half << 1));
        d = (uint16_t)((rd16(dba(0x1A2C)) ^ 0xFFFF) + 1);
        flag_set(P_C, 0);
        d = op_adc16(d, 0x00C0, &cpu.p);
        flag_set(P_C, 1);
        d = op_sbc16(d, rd16(dba(0x1486)), &cpu.p);
        flag_set(P_C, 0);
        d = op_adc16(d, rd16(dba(SCRATCH)), &cpu.p);
        cpu.a = d;
        cmp16_of(d, rd16(dba(SCRATCH + 2)));
        if (d < rd16(dba(SCRATCH + 2))) {
            op_sep(0x30);
            jsl_frame(0x83E6B3u);
            abi_PlayerEnter8CFacingLong();
            cpu.s = (uint16_t)(cpu.s + 3);
            return;
        }
    }
    op_sep(0x30);
}







/*
 * @routine 83:EF92 Kind6DWait
 * WHAT IT DOES: Kind $6D's wait: by variant (bits 2-4 of $07) stomped or
 * hurting and downed by shots, its height set by frame (variant 4), and
 * ended when out of range; variant $1C also enters state $8C while $1B14
 * is set and ends itself 144 pixels or more from the range point.
 * HOW: Variant $14: ActorEndOutOfRange only. Others: ActorStompOrHurt,
 * ActorDownedByShot (downed: $48 up), AnimStep, variant 4 ActorYByFrame,
 * ActorEndOutOfRange with box 0. Variant $1C: $1B14 set, state bits $8C
 * set and $09 bit 7 cleared; ActorStompOrHurt, ActorDownedByShot ($48 up),
 * AnimStep; distance from RANGE_X 144 or more: into ActorEndOutOfRange
 * past its range test.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Variant $1C is reached only by cov_kind6d_variant.box. Exit: as
 * ActorEndOutOfRange; variant $1C near: M 8-bit, C clear; far: does not
 * return.
 */
void abi_Kind6DWait(void)
{
    uint8_t v = (uint8_t)(rd8(dpa(ACTOR_07)) & 0x1C);
    uint16_t d;

    a8_load(v);
    cmp8_of(v, 0x1C);
    if (v == 0x1C) {
        a8_load(rd8(dba(CHAIN_HEAD_6D)));
        if ((uint8_t)cpu.a) {             /* the chain's head set: into state $8C, not hittable */
            a8_load(0x8C);
            tsb_at(dpa(ACTOR_STATE), 0x8C);
            a8_load(0x80);
            trb_at(dpa(ACTOR_09), 0x80);
        }
        call_frame(0xEFCB, abi_ActorStompOrHurt);
        call_frame(0xEFCE, abi_ActorDownedByShot);
        if (!(cpu.p & P_C))
            inc8_at(dpa(ACTOR_48));
        jsl_frame(0x83EFD3u); abi_AnimStep(); cpu.s = (uint16_t)(cpu.s + 3);
        op_rep(0x20);
        flag_set(P_C, 1);
        d = rd16(dba((uint16_t)RANGE_X_6D));
        d = op_sbc16(d, dp16(ACTOR_X + 1), &cpu.p);
        a16_load(d);
        if (d & 0x8000)
            a16_load((uint16_t)((d ^ 0xFFFF) + 1));
        cmp16_of(cpu.a, 0x0090);
        op_sep(0x20);
        if (cpu.p & P_C) {                /* 144 pixels or more away: the return dropped, it ends ($83:96C6) */
            op_sep(0x30);
            pla8();
            pla8();
            abi_ActorFreePlaced();
            actor_leave(0x8394BFu);
        }
        return;
    }
    cmp8_of(v, 0x14);
    if (v != 0x14) {
        call_frame(0xEFA0, abi_ActorStompOrHurt);
        call_frame(0xEFA3, abi_ActorDownedByShot);
        if (!(cpu.p & P_C))
            inc8_at(dpa(ACTOR_48));
        jsl_frame(0x83EFA8u); abi_AnimStep(); cpu.s = (uint16_t)(cpu.s + 3);
        v = (uint8_t)(rd8(dpa(ACTOR_07)) & 0x1C);
        a8_load(v);
        cmp8_of(v, 0x04);
        if (v == 0x04)
            call_frame(0xEFB6, abi_ActorYByFrame);
    }
    x8_load(0x00);
    abi_ActorEndOutOfRange();
}

/*
 * @routine 83:F1D5 Kind6EWait
 * WHAT IT DOES: Kind $6E's wait: a sound ($33) every 63 frames while its
 * $49 is clear; its backdrop picture stepped and shown; touching the test
 * box (moved by its variant's offset for the test) while the player can be
 * hit, the player enters state $8C.
 * HOW: $49 clear: $48 down; at 0, back to $3F and SoundQueueAdd $33.
 * BackdropPictureStep, BackdropActorShow. Its x pixels kept at $4C and the
 * variant's offset (word at $D6AB) added; ActorHitsTestBoxIfPlayerFree;
 * met: PlayerEnter8CFacingLong. Its x put back.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Its $09 has bit 6 clear in play, so the touch test never meets: the hurt
 * is reached only by unit_kind6e_touch.box. Exit: M 8-bit; A = its x
 * pixels.
 */
void abi_Kind6EWait(void)
{
    uint16_t x;

    a8_load(rd8(dpa(ACTOR_49)));
    if (!(uint8_t)cpu.a && !dec8_at(dpa(ACTOR_48))) {   /* its sound every $3F frames */
        a8_load(0x3F);
        wr8(dpa(ACTOR_48), 0x3F);
        a8_load(0x33);
        jsl_frame(0x83F1E3u);
        abi_SoundQueueAdd();
        cpu.s = (uint16_t)(cpu.s + 3);
    }
    call_frame(0xF1E9, abi_BackdropPictureStep);
    call_frame(0xF1EC, abi_BackdropActorShow);
    /* its touch tested at its x moved by its variant's offset */
    cpu.x = (uint8_t)(rd8(dpa(ACTOR_07)) << 1);
    op_rep(P_M);
    x = dp16(ACTOR_X + 1);
    dp16_set(ACTOR_4C, x);
    flag_set(P_C, 0);
    cpu.a = op_adc16(x, rd16(dba(0xD6AB) + cpu.x), &cpu.p);
    dp16_set(ACTOR_X + 1, cpu.a);
    op_sep(P_M);
    call_frame(0xF201, abi_ActorHitsTestBoxIfPlayerFree);
    if (!(cpu.p & P_C))
        { jsl_frame(0x83F204u); abi_PlayerEnter8CFacingLong(); cpu.s = (uint16_t)(cpu.s + 3); }
    op_rep(P_M);
    a16_load(dp16(ACTOR_4C));
    dp16_set(ACTOR_X + 1, cpu.a);
    op_sep(P_M);
}


/*
 * @routine 83:F908 ActorStopsTouchingKind2D
 * WHAT IT DOES: Stops every actor of kind $2D that touches the actor and
 * is not stopped yet: it is marked ($4F = 1), its per-frame routine
 * turned off, and it goes into state $8C.
 * HOW: X walks the records from ACTOR_POOL, $50 apart, while below
 * POOL_SCAN_END (so the 80 bytes at $141B, past the pool's 28, are taken
 * as a record too). A record in use, kind $2D, its $4F zero: its box's
 * half-width plus 4 (HIT_BOXES by its kind) in scratch, twice it at
 * scratch + 2; its x pixels plus that less the actor's below twice it
 * (unsigned), and the same with the half-height and the y pixels: $4F =
 * 1, bit 7 of $09 cleared, bit 7 of $08 set, state $8C. No script runs
 * the test past the kind (no kind $2D in the pool).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80, the hit
 * boxes' bank; called by JSR. Exit: M 8-bit, X 8-bit; X = $6B (the
 * walk's end, $146B, low byte), A = $146B (16 bits); C set, N and Z
 * clear. Scratch $000C-$000F as the last test left it.
 */
void abi_ActorStopsTouchingKind2D(void)
{
    op_rep(0x10);  /* 83F908 rep */
    ldx16_v(ACTOR_POOL);  /* 83F90A ldx */
L_83F90D:
    lda8(ea_absx(ACTOR_STATE));  /* 83F90D lda */
    if (IF_Z) goto L_83F97D;  /* 83F910 beq */
    lda8(ea_absx(ACTOR_06));  /* 83F912 lda */
    cmp8_v(0x2D);  /* 83F915 cmp */
    if (!IF_Z) goto L_83F97D;  /* 83F917 bne */
    lda8(ea_absx(KIND2D_STOPPED));  /* 83F919 lda */
    if (!IF_Z) goto L_83F97D;  /* 83F91C bne */
    op_rep(0x20);  /* 83F91E rep */
    lda16(ea_absx(ACTOR_06));  /* 83F920 lda */
    and16_v(0x00FF);  /* 83F923 and */
    ACC16(asl);  /* 83F926 asl */
    ACC16(asl);  /* 83F927 asl */
    tay16();  /* 83F928 tay */
    lda16(ea_absy(HIT_BOXES));  /* 83F929 lda */
    clc();  /* 83F92C clc */
    adc16_v(0x0004);  /* 83F92D adc */
    sta16(ea_abs(SCRATCH));  /* 83F930 sta */
    ACC16(asl);  /* 83F933 asl */
    sta16(ea_abs(SCRATCH + 2));  /* 83F934 sta */
    lda16(ea_absx(ACTOR_X + 1));  /* 83F937 lda */
    clc();  /* 83F93A clc */
    adc16_v(rd16(ea_abs(SCRATCH)));  /* 83F93B adc */
    sec();  /* 83F93E sec */
    sbc16_v(rd16(ea_dp(ACTOR_X + 1)));  /* 83F93F sbc */
    cmp16_v(rd16(ea_abs(SCRATCH + 2)));  /* 83F941 cmp */
    if (IF_C) goto L_83F97D;  /* 83F944 bcs */
    lda16(ea_absy(HIT_BOXES + 2));  /* 83F946 lda */
    clc();  /* 83F949 clc */
    adc16_v(0x0004);  /* 83F94A adc */
    sta16(ea_abs(SCRATCH));  /* 83F94D sta */
    ACC16(asl);  /* 83F950 asl */
    sta16(ea_abs(SCRATCH + 2));  /* 83F951 sta */
    lda16(ea_absx(ACTOR_Y + 1));  /* 83F954 lda */
    clc();  /* 83F957 clc */
    adc16_v(rd16(ea_abs(SCRATCH)));  /* 83F958 adc */
    sec();  /* 83F95B sec */
    sbc16_v(rd16(ea_dp(ACTOR_Y + 1)));  /* 83F95C sbc */
    cmp16_v(rd16(ea_abs(SCRATCH + 2)));  /* 83F95E cmp */
    if (IF_C) goto L_83F97D;  /* 83F961 bcs */
    op_sep(0x20);  /* 83F963 sep */
    lda8_v(0x01);  /* 83F965 lda */
    sta8(ea_absx(KIND2D_STOPPED));  /* 83F967 sta */
    MEM8(asl, ea_absx(ACTOR_09));  /* 83F96A asl */
    MEM8(lsr, ea_absx(ACTOR_09));  /* 83F96D lsr */
    lda8(ea_absx(ACTOR_08));  /* 83F970 lda */
    ora8_v(0x80);  /* 83F973 ora */
    sta8(ea_absx(ACTOR_08));  /* 83F975 sta */
    lda8_v(0x8C);  /* 83F978 lda */
    sta8(ea_absx(ACTOR_STATE));  /* 83F97A sta */
L_83F97D:
    op_rep(0x21);  /* 83F97D rep */
    txa16();  /* 83F97F txa */
    adc16_v(0x0050);  /* 83F980 adc */
    tax16();  /* 83F983 tax */
    cpx16_v(POOL_SCAN_END);  /* 83F984 cpx */
    op_sep(0x20);  /* 83F987 sep */
    if (!IF_C) goto L_83F90D;  /* 83F989 bcs, 83F98B jmp */
    op_sep(0x10);  /* 83F98E sep */
}
/*
 * @routine 83:D518 Kind52Wait
 * WHAT IT DOES: Kind $52's wait: animates; with its $48 set it tests a
 * shot; stomped or hurting as touched; on animation frames 0-3 and 10 it
 * hurts the player when its zone meets the test box; ended when out of
 * range.
 * HOW: AnimStep; $48 set: ActorEndsShot. ActorStompOrHurt. Its
 * frame ($27) looked up among five; found: ActorZoneHasTestBox with that
 * frame's zone, met: PlayerEnter8CFacing. Then ActorEndOutOfRange with box
 * 0.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Exit: as ActorEndOutOfRange.
 */
void abi_Kind52Wait(void)
{
    jsl_frame(0x83D518u); abi_AnimStep(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D518 jsl */
    lda8(ea_dp(0x48));  /* 83D51C lda */
    if (IF_Z) goto L_83D523;  /* 83D51E beq */
    call_frame(0xD522, abi_ActorEndsShot);  /* 83D520 jsr */
L_83D523:
    call_frame(0xD525, abi_ActorStompOrHurt);  /* 83D523 jsr */
    ldx8_v(0x04);  /* 83D526 ldx */
    lda8(ea_dp(0x27));  /* 83D528 lda */
L_83D52A:
    cmp8_v(rd8(ea_absx(0xD382)));  /* 83D52A cmp */
    if (IF_Z) goto L_83D534;  /* 83D52D beq */
    dex8();  /* 83D52F dex */
    if (IF_N) goto L_83D540;  /* 83D530 bmi */
    goto L_83D52A;  /* 83D532 bra */
L_83D534:
    lda8(ea_absx(0xD387));  /* 83D534 lda */
    tay8();  /* 83D537 tay */
    call_frame(0xD53A, abi_ActorZoneHasTestBox);  /* 83D538 jsr */
    if (IF_C) goto L_83D540;  /* 83D53B bcs */
    call_frame(0xD53F, abi_PlayerEnter8CFacing);  /* 83D53D jsr */
L_83D540:
    ldx8_v(0x00);  /* 83D540 ldx */
    abi_ActorEndOutOfRange(); return;  /* 83D542 jmp */
}
/*
 * @routine 83:F991 KindWaitF991
 * WHAT IT DOES: The per-frame routine of the kind whose stub at 83:930C
 * starts at 83:F77F: animates; variant 0 sets its $42 when it stops a shot
 * (until its $43 is set); other variants with $44 set hurt the player on
 * touch and stop the kind $2D actors that touch them; freed when out of
 * range.
 * HOW: AnimStep; $07 zero: $43 clear, ActorStopsShotLong, stopped: $42 =
 * 1. $07 nonzero: $44 set, ActorHurtsPlayerOnTouchLong and
 * ActorStopsTouchingKind2D. Then ActorFreeOutOfRangeLong with box 0 (by JML).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSL
 * from the kind's stub (83:9310). Exit: as ActorFreeOutOfRangeLong.
 */
void abi_KindWaitF991(void)
{
    jsl_frame(0x83F991u);  /* 83F991 jsl */
    abi_AnimStep();
    cpu.s = (uint16_t)(cpu.s + 3);
    lda8(ea_dp(0x07));  /* 83F995 lda */
    if (!IF_Z) goto L_83F9A9;  /* 83F997 bne */
    lda8(ea_dp(0x43));  /* 83F999 lda */
    if (!IF_Z) goto L_83F9B4;  /* 83F99B bne */
    jsl_frame(0x83F99Du);  /* 83F99D jsl */
    abi_ActorStopsShotLong();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (IF_C) goto L_83F9B4;  /* 83F9A1 bcs */
    lda8_v(0x01);  /* 83F9A3 lda */
    sta8(ea_dp(0x42));  /* 83F9A5 sta */
    goto L_83F9B4;  /* 83F9A7 bra */
L_83F9A9:
    lda8(ea_dp(0x44));  /* 83F9A9 lda */
    if (IF_Z) goto L_83F9B4;  /* 83F9AB beq */
    jsl_frame(0x83F9ADu);  /* 83F9AD jsl */
    abi_ActorHurtsPlayerOnTouchLong();
    cpu.s = (uint16_t)(cpu.s + 3);
    call_frame(0xF9B3, abi_ActorStopsTouchingKind2D);  /* 83F9B1 jsr */
L_83F9B4:
    ldx8_v(0x00);  /* 83F9B4 ldx */
    abi_ActorFreeOutOfRangeLong(); return;  /* 83F9B6 jml */
}

/*
 * @routine 83:E8A9 Kind68Wait
 * WHAT IT DOES: Kind $68's wait: carries the player along across while he
 * rides it (unless that puts him into firm ground), with a pause of 8
 * frames after he leaves; it ends itself while the scroll Y limit is
 * below 256.
 * HOW: Its $16 cleared and the player's x pixels kept at $1D; $20 counting
 * down: one less. Else Kind68PlayerOn; on it: $16 = 1, the player's x
 * moved by its move across ($000C less $4C), put back when
 * PlayerOnFirmGroundProbe8, Probe4 or PlayerOnFirmGround says firm. The
 * scroll Y limit's high byte 0: its return dropped and into
 * ActorEndPlaced. Then left this frame ($16 clear after set): $20 = 8;
 * $17 = $16.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Exit: M 8-bit; X = its $16; or does not return when it ends.
 */
void abi_Kind68Wait(void)
{
    stz8(ea_dp(KIND68_ON));  /* 83E8A9 stz */
    lda8(ea_abs(PLAYER_X));  /* 83E8AB lda */
    sta8(ea_dp(KIND68_PLAYER_X));  /* 83E8AE sta */
    lda8(ea_abs(PLAYER_X + 1));  /* 83E8B0 lda */
    sta8(ea_dp(KIND68_PLAYER_X + 1));  /* 83E8B3 sta */
    lda8(ea_dp(KIND68_PAUSE));  /* 83E8B5 lda */
    if (IF_Z) goto L_83E8BD;  /* 83E8B7 beq */
    MEM8(dec, ea_dp(KIND68_PAUSE));  /* 83E8B9 dec */
    goto L_83E8F2;  /* 83E8BB bra */
L_83E8BD:
    call_frame(0xE8BF, abi_Kind68PlayerOn);  /* 83E8BD jsr */
    if (IF_C) goto L_83E8F2;  /* 83E8C0 bcs */
    lda8_v(0x01);  /* 83E8C2 lda */
    sta8(ea_dp(KIND68_ON));  /* 83E8C4 sta */
    op_rep(0x20);  /* 83E8C6 rep */
    sec();  /* 83E8C8 sec */
    lda16(ea_dp(ACTOR_X + 1));  /* 83E8C9 lda */
    sbc16_v(rd16(ea_dp(ACTOR_4C)));  /* 83E8CB sbc */
    clc();  /* 83E8CD clc */
    adc16_v(rd16(ea_abs(PLAYER_X)));  /* 83E8CE adc */
    sta16(ea_abs(PLAYER_X));  /* 83E8D1 sta */
    op_sep(0x20);  /* 83E8D4 sep */
    jsl_frame(0x83E8D6u);  /* 83E8D6 jsl */
    abi_PlayerOnFirmGroundProbe8();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (IF_C) goto L_83E8E8;  /* 83E8DA bcs */
    jsl_frame(0x83E8DCu);  /* 83E8DC jsl */
    abi_PlayerOnFirmGroundProbe4();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (IF_C) goto L_83E8E8;  /* 83E8E0 bcs */
    jsl_frame(0x83E8E2u);  /* 83E8E2 jsl */
    abi_PlayerOnFirmGround();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (!IF_C) goto L_83E8F2;  /* 83E8E6 bcc */
L_83E8E8:
    lda8(ea_dp(KIND68_PLAYER_X));  /* 83E8E8 lda */
    sta8(ea_abs(PLAYER_X));  /* 83E8EA sta */
    lda8(ea_dp(KIND68_PLAYER_X + 1));  /* 83E8ED lda */
    sta8(ea_abs(PLAYER_X + 1));  /* 83E8EF sta */
L_83E8F2:
    lda8(ea_abs(SCROLL_LIMIT_Y + 1));  /* 83E8F2 lda */
    if (!IF_Z) goto L_83E8FC;  /* 83E8F5 bne */
    pla8();  /* 83E8F7 pla */
    pla8();  /* 83E8F8 pla */
    actor_leave(ACTOR_END_PLACED); return;  /* 83E8F9 jmp */
L_83E8FC:
    ldx8(ea_dp(KIND68_ON));  /* 83E8FC ldx */
    if (!IF_Z) goto L_83E908;  /* 83E8FE bne */
    lda8(ea_dp(KIND68_WAS_ON));  /* 83E900 lda */
    if (IF_Z) goto L_83E908;  /* 83E902 beq */
    lda8_v(0x08);  /* 83E904 lda */
    sta8(ea_dp(KIND68_PAUSE));  /* 83E906 sta */
L_83E908:
    stx8(ea_dp(KIND68_WAS_ON));  /* 83E908 stx */
}

/*
 * @routine 83:E90D Kind68PlayerOn
 * WHAT IT DOES: Tells whether the player is on kind $68 (C clear) and then
 * stands him on it: his feet on its top, his direction from its own.
 * HOW: Out (C set, by the SEC RTS just before it) when its y is $C0 or
 * more from RANGE_Y, $1427 is set, or V_141C is set while he moves up. By
 * its variant ($47) and flip: its spot (KIND68_SPOTS added to its $4C and
 * its y, kept at SPOT_X and SPOT_Y) and its box (KIND68_BOXES: offsets,
 * half-width, half-height). Out when his x is not within the half-width
 * of the box's x, or his y plus $25 ($20 when hurt with $141B set, or
 * not hurt and moving up) not within the half-height of the box's y.
 * Its top: SPOT_Y plus 8 (variant 0) or less half the distance across
 * from SPOT_X to him; out unless his feet are at most 24 above it and not
 * below. Then his y = the top less 32, V_1434 from KIND68_DIRS, V_1428
 * up, C clear. No script runs the $20 nor the variant other than 0.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80, the tables'
 * bank; called by JSR. Exit: M 8-bit, X 8-bit. On: C clear; A = V_1428's
 * bank; called by JSR. Exit: M 8-bit, X 8-bit. On: C clear; A = the
 * direction word (16 bits); N and Z from V_1428's INC. Out: C set.
 * Scratch $000C-$0017, $001C-$001F and $002C-$002F as its tests left
 * them.
 */
void abi_Kind68PlayerOn(void)
{
    goto L_83E90D;
L_83E90B:
    sec();  /* 83E90B sec */
    return;  /* 83E90C rts */
L_83E90D:
    op_rep(0x20);  /* 83E90D rep */
    sec();  /* 83E90F sec */
    lda16(ea_dp(ACTOR_Y + 1));  /* 83E910 lda */
    sbc16_v(rd16(ea_abs(RANGE_Y)));  /* 83E912 sbc */
    clc();  /* 83E915 clc */
    adc16_v(0x00C0);  /* 83E916 adc */
    cmp16_v(0x0180);  /* 83E919 cmp */
    op_sep(0x20);  /* 83E91C sep */
    if (IF_C) goto L_83E90B;  /* 83E91E bcs */
    lda8(ea_abs(V_1427));  /* 83E920 lda */
    if (!IF_Z) goto L_83E90B;  /* 83E923 bne */
    lda8(ea_abs(V_141C));  /* 83E925 lda */
    if (IF_Z) goto L_83E92F;  /* 83E928 beq */
    lda8(ea_abs(PLAYER_SPEED_Y_HI));  /* 83E92A lda */
    if (IF_N) goto L_83E90B;  /* 83E92D bmi */
L_83E92F:
    lda8(ea_dp(KIND68_VARIANT));  /* 83E92F lda */
    ACC8(asl);  /* 83E931 asl */
    ora8_v(rd8(ea_dp(ACTOR_FLIP)));  /* 83E932 ora */
    ACC8(asl);  /* 83E934 asl */
    ACC8(asl);  /* 83E935 asl */
    tax8();  /* 83E936 tax */
    ACC8(asl);  /* 83E937 asl */
    tay8();  /* 83E938 tay */
    op_rep(0x20);  /* 83E939 rep */
    clc();  /* 83E93B clc */
    lda16(ea_dp(ACTOR_4C));  /* 83E93C lda */
    adc16_v(rd16(ea_absx(KIND68_SPOTS)));  /* 83E93E adc */
    sta16(ea_abs(SPOT_X));  /* 83E941 sta */
    clc();  /* 83E944 clc */
    adc16_v(rd16(ea_absy(KIND68_BOXES)));  /* 83E945 adc */
    sta16(ea_abs(SCRATCH));  /* 83E948 sta */
    clc();  /* 83E94B clc */
    lda16(ea_dp(ACTOR_Y + 1));  /* 83E94C lda */
    adc16_v(rd16(ea_absx(KIND68_SPOTS + 2)));  /* 83E94E adc */
    sta16(ea_abs(SPOT_Y));  /* 83E951 sta */
    clc();  /* 83E954 clc */
    adc16_v(rd16(ea_absy(KIND68_BOXES + 2)));  /* 83E955 adc */
    sta16(ea_abs(SCRATCH + 2));  /* 83E958 sta */
    lda16(ea_absy(KIND68_BOXES + 4));  /* 83E95B lda */
    sta16(ea_abs(SCRATCH + 4));  /* 83E95E sta */
    ACC16(asl);  /* 83E961 asl */
    sta16(ea_abs(SCRATCH + 6));  /* 83E962 sta */
    lda16(ea_absy(KIND68_BOXES + 6));  /* 83E965 lda */
    sta16(ea_abs(SCRATCH + 8));  /* 83E968 sta */
    ACC16(asl);  /* 83E96B asl */
    sta16(ea_abs(SCRATCH + 10));  /* 83E96C sta */
    sec();  /* 83E96F sec */
    lda16(ea_abs(PLAYER_X));  /* 83E970 lda */
    sbc16_v(rd16(ea_abs(SCRATCH)));  /* 83E973 sbc */
    clc();  /* 83E976 clc */
    adc16_v(rd16(ea_abs(SCRATCH + 4)));  /* 83E977 adc */
    cmp16_v(rd16(ea_abs(SCRATCH + 6)));  /* 83E97A cmp */
    if (IF_C) goto L_83E9F4;  /* 83E97D bcs */
    lda16_v(0x0025);  /* 83E97F lda */
    ldx8(ea_abs(HEALTH));  /* 83E982 ldx */
    if (!IF_Z) goto L_83E98E;  /* 83E985 bne */
    ldx8(ea_abs(PLAYER_SPEED_Y_HI));  /* 83E987 ldx */
    if (IF_N) goto L_83E993;  /* 83E98A bmi */
    goto L_83E996;  /* 83E98C bra */
L_83E98E:
    ldx8(ea_abs(V_141B));  /* 83E98E ldx */
    if (IF_Z) goto L_83E996;  /* 83E991 beq */
L_83E993:
    lda16_v(0x0020);  /* 83E993 lda */
L_83E996:
    clc();  /* 83E996 clc */
    adc16_v(rd16(ea_abs(PLAYER_Y)));  /* 83E997 adc */
    sta16(ea_abs(FEET_Y));  /* 83E99A sta */
    sec();  /* 83E99D sec */
    sbc16_v(rd16(ea_abs(SCRATCH + 2)));  /* 83E99E sbc */
    clc();  /* 83E9A1 clc */
    adc16_v(rd16(ea_abs(SCRATCH + 8)));  /* 83E9A2 adc */
    cmp16_v(rd16(ea_abs(SCRATCH + 10)));  /* 83E9A5 cmp */
    if (IF_C) goto L_83E9F4;  /* 83E9A8 bcs */
    ldx8(ea_dp(KIND68_VARIANT));  /* 83E9AA ldx */
    if (!IF_Z) goto L_83E9B3;  /* 83E9AC bne */
    lda16_v(0x0008);  /* 83E9AE lda */
    goto L_83E9C5;  /* 83E9B1 bra */
L_83E9B3:
    sec();  /* 83E9B3 sec */
    lda16(ea_abs(SPOT_X));  /* 83E9B4 lda */
    sbc16_v(rd16(ea_abs(PLAYER_X)));  /* 83E9B7 sbc */
    if (!IF_N) goto L_83E9C0;  /* 83E9BA bpl */
    eor16_v(0xFFFF);  /* 83E9BC eor */
    ACC16(inc);  /* 83E9BF inc */
L_83E9C0:
    ACC16(lsr);  /* 83E9C0 lsr */
    eor16_v(0xFFFF);  /* 83E9C1 eor */
    ACC16(inc);  /* 83E9C4 inc */
L_83E9C5:
    clc();  /* 83E9C5 clc */
    adc16_v(rd16(ea_abs(SPOT_Y)));  /* 83E9C6 adc */
    sta16(ea_abs(TOP_Y));  /* 83E9C9 sta */
    sec();  /* 83E9CC sec */
    sbc16_v(rd16(ea_abs(FEET_Y)));  /* 83E9CD sbc */
    if (IF_C) goto L_83E9F4;  /* 83E9D0 bcs */
    cmp16_v(0xFFE8);  /* 83E9D2 cmp */
    if (!IF_C) goto L_83E9F4;  /* 83E9D5 bcc */
    sec();  /* 83E9D7 sec */
    lda16(ea_abs(TOP_Y));  /* 83E9D8 lda */
    sbc16_v(0x0020);  /* 83E9DB sbc */
    sta16(ea_abs(PLAYER_Y));  /* 83E9DE sta */
    lda16(ea_dp(KIND68_VARIANT));  /* 83E9E1 lda */
    ACC16(asl);  /* 83E9E3 asl */
    ora16_v(rd16(ea_dp(ACTOR_FLIP)));  /* 83E9E4 ora */
    tax8();  /* 83E9E6 tax */
    lda16(ea_absx(KIND68_DIRS));  /* 83E9E7 lda */
    sta16(ea_abs(V_1434));  /* 83E9EA sta */
    MEM16(inc, ea_abs(V_1428));  /* 83E9ED inc */
    op_sep(0x20);  /* 83E9F0 sep */
    clc();  /* 83E9F2 clc */
    return;  /* 83E9F3 rts */
L_83E9F4:
    op_sep(0x21);  /* 83E9F4 sep */
}
