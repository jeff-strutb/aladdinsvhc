/*
 * scene_figures.c: scripted figures: actor kind $93, which walks in once
 * the scene's cue $00C2 is given, and the player started with $07 = 15,
 * who stands at (128, 176) for a scene. Drafted by cdraft.py; named and
 * described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"
#include "wram_player.h"

void abi_ActorMoveX(void);
void abi_ActorSpeedXSet(void);
void abi_AnimStart(void);
void abi_PlayerMoveStart(void);

#define SCENE_CUE_C2 0x00C2u  /* counted up by a scripted actor when it has waited; this one waits for it */
#define FIGURE_29 0x29u  /* word: its picture list ($891F, $8907) */
#define V_145B 0x145Bu
#define V_1430 0x1430u

/*
 * @routine A5:F3F0 Kind93WalkIn
 * @actor
 * WHAT IT DOES: Actor kind $93: stands facing left until the scene's cue
 * (SCENE_CUE_C2), then walks left until its x is below 162, takes its
 * second pose and stays.
 * HOW: $09 bits $90 set, $2C = 2, redraw marked, $29 = $891F, facing and
 * flip 1, animation 0; BRK until SCENE_CUE_C2; ActorSpeedXSet $72; each
 * frame ActorMoveX while its x pixels' low byte is 162 or more; animation
 * 1; BRK for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
 * the actor loop's jump for kind $93 in state $0C (through the stub at
 * $83:9389). Exit: never returns.
 */
void abi_Kind93WalkIn(void)
{
    lda8_v(0x90);  /* A5F3F0 lda */
    tsb8(ea_dp(ACTOR_09));  /* A5F3F2 tsb */
    lda8_v(0x02);  /* A5F3F4 lda */
    sta8(ea_dp(ACTOR_2C));  /* A5F3F6 sta */
    lda8_v(0xFF);  /* A5F3F8 lda */
    sta8(ea_dp(ACTOR_REDRAW));  /* A5F3FA sta */
    op_rep(0x20);  /* A5F3FC rep */
    lda16_v(0x891F);  /* A5F3FE lda */
    sta16(ea_dp(FIGURE_29));  /* A5F401 sta */
    op_sep(0x20);  /* A5F403 sep */
    lda8_v(0x01);  /* A5F405 lda */
    sta8(ea_dp(ACTOR_FACING));  /* A5F407 sta */
    sta8(ea_dp(ACTOR_FLIP));  /* A5F409 sta */
    lda8_v(0x00);  /* A5F40B lda */
    abi_AnimStart();  /* A5F40D jsl */
L_A5F411:
    actor_yield(0xA5F411u);  /* A5F411 brk */
    lda8(ea_abs(SCENE_CUE_C2));  /* A5F413 lda */
    if (IF_Z) goto L_A5F411;  /* A5F416 beq */
    ldy8_v(0x72);  /* A5F418 ldy */
    abi_ActorSpeedXSet();  /* A5F41A jsl */
L_A5F41E:
    actor_yield(0xA5F41Eu);  /* A5F41E brk */
    abi_ActorMoveX();  /* A5F420 jsl */
    lda8(ea_dp(ACTOR_X + 1));  /* A5F424 lda */
    cmp8_v(0xA2);  /* A5F426 cmp */
    if (IF_C) goto L_A5F41E;  /* A5F428 bcs */
    lda8_v(0x01);  /* A5F42A lda */
    abi_AnimStart();  /* A5F42C jsl */
L_A5F430:
    actor_yield(0xA5F430u);  /* A5F430 brk */
    goto L_A5F430;  /* A5F432 bra */
}


/*
 * @routine 81:E23E PlayerSceneStand
 * @actor
 * WHAT IT DOES: The player started with $07 = 15: Aladdin set at (128,
 * 176) facing right for a scene, standing; after $20 frames V_142D is
 * counted up, and he stays.
 * HOW: $29 = $8907, x 128, y 176; V_145B = 1; facing and flip 0;
 * PlayerMoveStart 8; V_1430 up; $09 = $D0, redraw marked, $06 cleared,
 * TASK_PARAM cleared; COP $20; V_142D up; BRK for good.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $81; reached by
 * the jump through the player's start table by $07 at $81:CB61 (81:CB56).
 * Exit: never returns.
 */
void abi_PlayerSceneStand(void)
{
    op_rep(0x20);  /* 81E23E rep */
    lda16_v(0x8907);  /* 81E240 lda */
    sta16(ea_dp(FIGURE_29));  /* 81E243 sta */
    lda16_v(0x0080);  /* 81E245 lda */
    sta16(ea_dp(ACTOR_X + 1));  /* 81E248 sta */
    lda16_v(0x00B0);  /* 81E24A lda */
    sta16(ea_dp(ACTOR_Y + 1));  /* 81E24D sta */
    op_sep(0x20);  /* 81E24F sep */
    lda8_v(0x01);  /* 81E251 lda */
    sta8(ea_abs(V_145B));  /* 81E253 sta */
    stz8(ea_dp(ACTOR_FACING));  /* 81E256 stz */
    stz8(ea_dp(ACTOR_FLIP));  /* 81E258 stz */
    lda8_v(0x08);  /* 81E25A lda */
    abi_PlayerMoveStart();  /* 81E25C jsr */
    MEM8(inc, ea_abs(V_1430));  /* 81E25F inc */
    lda8_v(0xD0);  /* 81E262 lda */
    sta8(ea_dp(ACTOR_09));  /* 81E264 sta */
    lda8_v(0xFF);  /* 81E266 lda */
    sta8(ea_dp(ACTOR_REDRAW));  /* 81E268 sta */
    stz8(ea_dp(ACTOR_06));  /* 81E26A stz */
    stz8(ea_abs(TASK_PARAM));  /* 81E26C stz */
    lda8_v(0x20);  /* 81E26F lda */
    actor_wait(0x81E271u);  /* 81E271 cop */
    MEM8(inc, ea_abs(V_142D));  /* 81E273 inc */
L_81E276:
    actor_yield(0x81E276u);  /* 81E276 brk */
    goto L_81E276;  /* 81E278 bra */
}
