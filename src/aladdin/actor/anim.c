/*
 * anim.c: actors' animations.
 *
 * Each actor kind has an animation set: a table of animations, each a
 * list of frames (a time, then a picture), ended by a time of 0 with how
 * many frames to step back. An actor's record holds the animation, the
 * frame, the time it has left and the picture's graphics.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

static uint16_t dp(uint16_t off)
{
    return (uint16_t)(cpu.d + off);
}

static void set_a8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* PHB, LDA #$7E, PHA, PLB: the data bank $7E (the caller's kept by the
   C); A low = $7E, N and Z from it */
static void db_7e(void)
{
    set_a8(0x7E);
    cpu.db = 0x7E;
    cpu.p = nz8(cpu.p, 0x7E);
}

/*
 * @routine 81:9158 AnimFrameLookup
 * WHAT IT DOES: Looks up an actor's current frame: its time and its
 * picture, and the picture's graphics.
 * HOW: The animation's frame list from the set's table, the frame's word
 * in it; its high byte is the picture, whose graphics come from the
 * picture list, offset by the set's graphics base.
 * Entry: any widths; D = the actor's record; DB = $7E. Exit: M 16-bit,
 * X 16-bit; A = X = the frame's word (the time low, the picture high); Y =
 * its offset in the picture list; N and Z from it; C from the last add;
 * $000C = the frame list's offset.
 */
void abi_AnimFrameLookup(void)
{
    op_rep(0x30);
    op_lda16(dp(ACTOR_ANIM));
    op_and16(0x00FF);
    op_add16(rd16(dp(ACTOR_ANIM_TABLE)));
    op_tay16();
    op_lda16(ANIM_DATA + cpu.y);
    wr16(SCRATCH, cpu.a);
    op_lda16(dp(ACTOR_FRAME));
    op_and16(0x00FF);
    op_asl16();
    op_add16(rd16(SCRATCH));
    op_add16(rd16(dp(ACTOR_ANIM_TABLE)));
    op_tay16();
    op_lda16(ANIM_DATA + cpu.y);
    op_tax16();
    op_xba();
    op_and16(0x00FF);
    op_sep(0x20);
    wr8(dp(ACTOR_PICTURE), (uint8_t)cpu.a);
    op_rep(0x20);
    op_asl16();
    op_add16(rd16(dp(ACTOR_GFX_BASE)));
    op_tay16();
    op_lda16(PICTURE_LIST + cpu.y);
    op_add16(0x2000);
    op_add16(rd16(dp(ACTOR_GFX_BASE)));
    wr16(dp(ACTOR_PICTURE_GFX), cpu.a);
    op_txa16();
}

/* AnimStart and AnimResume: `first`, start at the first frame. */
static void anim_start(int first)
{
    uint8_t db = cpu.db;

    cpu.x = rd8(dp(ACTOR_06));
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    if (first)
        wr8(dp(ACTOR_FRAME), 0);
    op_sep(0x30);
    wr8(dp(ACTOR_PICTURE), (uint8_t)cpu.a);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | ((cpu.a >> 7) & 1));
    set_a8((uint8_t)(cpu.a << 1));
    wr8(dp(ACTOR_ANIM), (uint8_t)cpu.a);
    set_a8(rd8(dp(ACTOR_08)));
    set_a8((uint8_t)cpu.a | 0x08);
    wr8(dp(ACTOR_08), (uint8_t)cpu.a);
    db_7e();
    op_rep(0x30);
    op_txa16();
    op_and16(0x00FF);
    op_tax16();
    op_lda16(ANIM_SET_OF_KIND + cpu.x);
    op_and16(0x00FF);
    op_asl16();
    op_tax16();
    op_lda16(ANIM_SET_GFX + cpu.x);
    wr16(dp(ACTOR_GFX_BASE), cpu.a);
    op_lda16(ANIM_SET_TABLE + cpu.x);
    wr16(dp(ACTOR_ANIM_TABLE), cpu.a);
    op_add16(rd16(dp(ACTOR_ANIM)));
    op_tay16();
    op_lda16(ANIM_DATA + cpu.y);
    op_add16(rd16(dp(ACTOR_ANIM_TABLE)));
    op_tay16();
    op_lda16(ANIM_DATA + cpu.y);
    op_tax16();
    op_xba();
    op_and16(0x00FF);
    op_sep(0x30);
    wr8(dp(ACTOR_LOOP_BACK), (uint8_t)cpu.a);
    set_a8((uint8_t)cpu.x);
    wr8(dp(ACTOR_FRAME_TIME), (uint8_t)cpu.a);
    set_a8(rd8(dp(ACTOR_PICTURE)));
    abi_AnimFrameLookup();
    op_sep(0x30);
    set_a8(0xFF);
    wr8(dp(ACTOR_REDRAW), 0xFF);
    cpu.db = db;
    cpu.p = nz8(cpu.p, db);
}

/*
 * @routine 81:90C2 AnimStart
 * WHAT IT DOES: Starts an actor's animation A from its first frame.
 * HOW: AnimResume, with the frame set to 0 first.
 * Entry: M 8-bit, X 8-bit; A = the animation; D = the actor's record;
 * called by JSL. Exit: as AnimResume.
 */
void abi_AnimStart(void)
{
    anim_start(1);
}

/*
 * @routine 81:90BE AnimResume
 * WHAT IT DOES: Switches an actor to animation A at the frame it is on.
 * HOW: The animation's number and offset go in the record and it is
 * marked animated; the actor's kind gives its animation set (its
 * graphics base and its table); the animation's first frame word gives
 * the time and the frames to step back at its end (its time 0 entry);
 * then AnimFrameLookup, and the picture is marked changed.
 * Entry: M 8-bit, X 8-bit; A = the animation; D = the actor's record;
 * called by JSL. Exit: M and X 8-bit; A = $FF; X, Y as AnimFrameLookup
 * leaves them (8-bit); DB unchanged, N and Z from it; C from
 * AnimFrameLookup's last add.
 */
void abi_AnimResume(void)
{
    anim_start(0);
}

/*
 * @routine 81:912B AnimStep
 * WHAT IT DOES: Plays an actor's animation on by one frame of time: when
 * the frame's time runs out, on to the next frame, or back the
 * animation's step at its end.
 * HOW: The time steps down; at 0 the frame steps on and AnimFrameLookup
 * gives its time; a time of 0 is the end: the frame steps back by the
 * end's count and is looked up again. The new time goes in the record.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSL.
 * Exit: time left: M and X 8-bit, N and Z from it, the rest unchanged.
 * Else M and X 8-bit; A = the new time; X, Y as AnimFrameLookup leaves
 * them (8-bit); DB unchanged, N and Z from it.
 */
void abi_AnimStep(void)
{
    uint8_t db, t;

    op_sep(0x30);
    t = (uint8_t)(rd8_rmw(dp(ACTOR_FRAME_TIME)) - 1);
    wr8(dp(ACTOR_FRAME_TIME), t);
    cpu.p = nz8(cpu.p, t);
    if (t != 0)
        return;
    db = cpu.db;
    db_7e();
    t = (uint8_t)(rd8_rmw(dp(ACTOR_FRAME)) + 1);
    wr8(dp(ACTOR_FRAME), t);
    cpu.p = nz8(cpu.p, t);
    abi_AnimFrameLookup();
    op_and16(0x00FF);
    if (cpu.a == 0) {
        op_txa16();
        op_xba();
        op_and16(0x00FF);
        op_sep(0x20);
        cpu.p |= P_C;
        set_a8(op_sbc8((uint8_t)cpu.a, rd8(dp(ACTOR_LOOP_BACK)), &cpu.p));
        wr8(dp(ACTOR_FRAME), (uint8_t)cpu.a);
        abi_AnimFrameLookup();
        op_and16(0x00FF);
    }
    op_sep(0x30);
    wr8(dp(ACTOR_FRAME_TIME), (uint8_t)cpu.a);
    cpu.db = db;
    cpu.p = nz8(cpu.p, db);
}
