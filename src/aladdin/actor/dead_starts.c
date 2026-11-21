/*
 * dead_starts.c: actor code in banks $84 and $A5 that nothing in the ROM starts (no call, jump, table word or resume address holds its
 * address): a hovering spawner (84:9A88) and its two subroutines. cov_dead_starts starts it by a record's resume address. The
 * escort's start at A5:EFDE is in restart_far.c. Written instruction for instruction from the assembly (dead_starts.asm).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorCreateChild(void);
void abi_ActorDecelerateY(void);
void abi_ActorMove(void);
void abi_ActorMoveX(void);
void abi_AnimStart(void);
void abi_RandomFromList(void);
void abi_SoundQueueAdd(void);
void abi_HoverFollowPlayerY(void);
void abi_HoverSpawnChild(void);

#define PLAYER_Y_AT (PLAYER + ACTOR_Y + 1)  /* the player's y, whole pixels (word) */
#define VIEW_X 0x1773u                      /* the view's left edge (pixels, word) */
#define VIEW_Y 0x1777u                      /* the view's top edge (pixels, word) */
#define V_1A12 0x1A12u                      /* cleared once it hovers */
#define HOVER_FRAMES ACTOR_40               /* frames left of a wait */
#define HOVER_RANDOM ACTOR_42               /* the child's random pick (word) */
#define HOVER_CHILD ACTOR_NEXT              /* the child's record (word) */
#define HOVER_CHILD_41 0xD86Au              /* by the pick: the child's $41 */
#define HOVER_CHILD_FACING 0xD85Fu          /* by the pick: the child's facing */

/* A JSL at `site` to fn: its return address on the stack while fn runs, as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 84:9A88 HoverSpawnerStart
 * @actor
 * WHAT IT DOES: An actor nothing starts (no table, call or resume address
 * holds $84:9A88): it appears at the view's left edge plus $30 and top
 * edge less $20, falls with its speed ($0180 across, $0300 down, gravity 8)
 * losing its downward speed (ActorDecelerateY) until it is 0, then hovers,
 * following Aladdin's y, and every $60 frames (twice $30) plays sound $28
 * and animation 3 and makes a kind $8E child (HoverSpawnChild), then
 * animation 2. cov_dead_starts starts it by a record's resume address.
 * HOW: x = VIEW_X + $30, y = VIEW_Y - $20 (16-bit); speed $000180 across,
 * $000300 down; gravity $08; facing and flip 0; HOVER_FRAMES $80. Each
 * frame (BRK first): ActorDecelerateY, ActorMove, until the downward
 * speed's two low bytes are 0. V_1A12 and HOVER_RANDOM (low byte) 0. Then
 * for ever: $30 frames (BRK, HoverFollowPlayerY); SoundQueueAdd $28;
 * AnimStart 3; HoverSpawnChild, $30 frames (BRK, HoverFollowPlayerY);
 * AnimStart 2.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by the
 * actor loop's RTI at the resume address. Exit: never returns.
 */
void abi_HoverSpawnerStart(void)
{
    op_rep(P_M);
    lda16(ea_abs(VIEW_X));
    clc();
    adc16_v(0x0030);
    sta16(ea_dp(ACTOR_X + 1));
    lda16(ea_abs(VIEW_Y));
    sec();
    sbc16_v(0x0020);
    sta16(ea_dp(ACTOR_Y + 1));
    lda16_v(0x0180);
    sta16(ea_dp(ACTOR_SPEED_X));
    lda16_v(0x0300);
    sta16(ea_dp(ACTOR_SPEED_Y));
    op_sep(P_M);
    stz8(ea_dp(ACTOR_SPEED_X + 2));
    stz8(ea_dp(ACTOR_SPEED_Y + 2));
    lda8_v(0x08);
    sta8(ea_dp(ACTOR_ACCEL_Y));
    stz8(ea_dp(ACTOR_FACING));
    stz8(ea_dp(ACTOR_FLIP));
    lda8_v(0x80);
    sta8(ea_dp(HOVER_FRAMES));
    do {
        actor_yield(0x849AB8u);
        jsl_call(0x849ABAu, abi_ActorDecelerateY);
        jsl_call(0x849ABEu, abi_ActorMove);
        lda8(ea_dp(ACTOR_SPEED_Y));
        ora8_v(rd8(ea_dp(ACTOR_SPEED_Y + 1)));
    } while (!IF_Z);
    stz8(ea_abs(V_1A12));
    stz8(ea_dp(HOVER_RANDOM));
    for (;;) {
        lda8_v(0x30);
        sta8(ea_dp(HOVER_FRAMES));
        do {
            actor_yield(0x849AD1u);
            call_frame(0x9AD5, abi_HoverFollowPlayerY);
            MEM8(dec, ea_dp(HOVER_FRAMES));
        } while (!IF_Z);
        lda8_v(0x28);
        jsl_call(0x849ADCu, abi_SoundQueueAdd);
        lda8_v(0x03);
        jsl_call(0x849AE2u, abi_AnimStart);
        lda8_v(0x30);
        sta8(ea_dp(HOVER_FRAMES));
        call_frame(0x9AEC, abi_HoverSpawnChild);
        do {
            actor_yield(0x849AEDu);
            call_frame(0x9AF1, abi_HoverFollowPlayerY);
            MEM8(dec, ea_dp(HOVER_FRAMES));
        } while (!IF_Z);
        lda8_v(0x02);
        jsl_call(0x849AF8u, abi_AnimStart);
    }
}

/*
 * @routine 84:9AFE HoverSpawnChild
 * WHAT IT DOES: Makes a kind $8E child of HoverSpawnerStart's actor, if a
 * record is free, with a random facing and $41 from tables. Called only by
 * HoverSpawnerStart.
 * HOW: ACTOR_SPAWN_AT $7E, $07 2; ActorCreateChild with A = $8E; carry set
 * (none free): RTS. Else the child's record kept in HOVER_CHILD (16-bit),
 * X 8-bit; RandomFromList with X = $16 gives HOVER_RANDOM (a word, high
 * byte 0); X 16-bit, X = the child, Y = the pick: child $41 from
 * HOVER_CHILD_41 and child facing from HOVER_CHILD_FACING by Y; X 8-bit.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR.
 * Exit: M and X 8-bit; the child's fields set.
 */
void abi_HoverSpawnChild(void)
{
    lda8_v(0x7E);
    sta8(ea_dp(ACTOR_SPAWN_AT));
    lda8_v(0x02);
    sta8(ea_dp(ACTOR_07));
    lda8_v(0x8E);
    jsl_call(0x849B08u, abi_ActorCreateChild);
    if (IF_C)
        return;
    stx16(ea_dp(HOVER_CHILD));
    op_sep(P_X);
    ldx8_v(0x16);
    jsl_call(0x849B14u, abi_RandomFromList);
    sta8(ea_dp(HOVER_RANDOM));
    stz8(ea_dp(HOVER_RANDOM + 1));
    op_rep(P_X);
    ldx16(ea_dp(HOVER_CHILD));
    ldy16(ea_dp(HOVER_RANDOM));
    lda8(ea_absy(HOVER_CHILD_41));
    sta8(ea_absx(ACTOR_41));
    lda8(ea_absy(HOVER_CHILD_FACING));
    sta8(ea_absx(ACTOR_FACING));
    op_sep(P_X);
}

/*
 * @routine 84:9B31 HoverFollowPlayerY
 * WHAT IT DOES: Moves the hovering actor across (ActorMoveX) and puts its
 * y $10 above Aladdin's. Called only by HoverSpawnerStart.
 * HOW: ActorMoveX; 16-bit A: y pixels = Aladdin's y - $10.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR.
 * Exit: M 8-bit.
 */
void abi_HoverFollowPlayerY(void)
{
    jsl_call(0x849B31u, abi_ActorMoveX);
    op_rep(P_M);
    lda16(ea_abs(PLAYER_Y_AT));
    sec();
    sbc16_v(0x0010);
    sta16(ea_dp(ACTOR_Y + 1));
    op_sep(P_M);
}
