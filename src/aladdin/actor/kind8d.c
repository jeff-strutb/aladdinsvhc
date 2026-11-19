/*
 * kind8d.c: actor kind $8D, which flies in from the view's left, circles
 * Aladdin twice, flies off and ends.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_ActorMoveByVector(void);
void abi_ActorMoveByVectorTimes(void);
void abi_ActorNearPlayer(void);
void abi_ActorSideOfPlayer(void);
void abi_ActorEndPlaced(void);

#define PLAYER_X_AT (PLAYER + ACTOR_X + 1)  /* the player's x, whole pixels (word) */
#define PLAYER_Y_AT (PLAYER + ACTOR_Y + 1)  /* the player's y, whole pixels (word) */
#define VIEW_X 0x1773u          /* the view's left edge (pixels, word) */
#define VIEW_Y 0x1777u          /* the view's top edge (pixels, word) */
#define V_1A12 0x1A12u          /* cleared as it flies off */
#define KIND8D_ANGLE 0x14u      /* its direction (0-$7F), for the vector moves */
#define KIND8D_TURNS 0x40u      /* circles left; then frames left of its flight off */
#define KIND8D_12 0x12u         /* $18 */
#define KIND8D_29 0x29u         /* word: $890B */

/*
 * @routine 84:99ED Kind8DCircle
 * @actor
 * WHAT IT DOES: Actor kind $8D: starts 16 pixels left of the view and 256
 * below its top, flies on direction $1B until near Aladdin, then circles
 * him twice at radius $48 (its flip toward him), flies off on direction
 * $1D for 80 frames, and ends off the screen.
 * HOW: actor_08 bit 1, actor_09 bits $90 set; KIND8D_12 = $18, KIND8D_29
 * = $890B; x = VIEW_X - $10, y = VIEW_Y + $100; AnimStart 0; direction
 * $1B, flip 0. Each frame (BRK first) ActorMoveByVector with X = $1E
 * until ActorNearPlayer (Y = $2C) clears C. Direction $46, two turns:
 * each frame its centre is his place, the direction steps by one (mod
 * $80), ActorMoveByVectorTimes (X = $10, A = $48), flip by
 * ActorSideOfPlayer; a turn ends at direction $48. Then flip 0, direction
 * $1D, $50 frames of ActorMoveByVector (X = $1E); V_1A12 cleared; its x
 * and y high bytes $FF; ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $8D
 * in state $0C (the table at $83:8DAB, through the stub at $83:9344).
 * Exit: never returns; ends in ActorEndPlaced.
 */
void abi_Kind8DCircle(void)
{
    uint8_t v, angle;

    v = (uint8_t)(rd8(dpa(ACTOR_08)) | 0x02);
    wr8(dpa(ACTOR_08), v);
    v = (uint8_t)(rd8(dpa(ACTOR_09)) | 0x90);
    wr8(dpa(ACTOR_09), v);
    a8_load(0x18);
    wr8(dpa(KIND8D_12), 0x18);
    op_rep(0x20);
    dp16_set(KIND8D_29, 0x890B);
    flag_set(P_C, 1);
    cpu.a = op_sbc16(rd16(dba(VIEW_X)), 0x0010, &cpu.p);
    dp16_set(ACTOR_X + 1, cpu.a);
    flag_set(P_C, 0);
    cpu.a = op_adc16(rd16(dba(VIEW_Y)), 0x0100, &cpu.p);
    dp16_set(ACTOR_Y + 1, cpu.a);
    op_sep(0x20);
    a8_load(0x00);
    jsl_frame(0x849A1Au);
    abi_AnimStart();
    cpu.s = (uint16_t)(cpu.s + 3);
    a8_load(0x1B);
    wr8(dpa(KIND8D_ANGLE), 0x1B);
    wr8(dpa(ACTOR_FLIP), 0);

    /* in, until near him */
    do {
        actor_yield(0x849A24u);
        x8_load(0x1E);
        abi_ActorMoveByVector();
        y8_load(0x2C);
        jsl_frame(0x849A2Eu);
        abi_ActorNearPlayer();
        cpu.s = (uint16_t)(cpu.s + 3);
    } while (cpu.p & P_C);

    /* twice round him */
    a8_load(0x46);
    wr8(dpa(KIND8D_ANGLE), 0x46);
    a8_load(0x02);
    wr8(dpa(KIND8D_TURNS), 0x02);
    for (;;) {
        actor_yield(0x849A3Cu);
        op_rep(0x20);
        dp16_set(ACTOR_X + 1, rd16(dba((uint16_t)PLAYER_X_AT)));
        a16_load(rd16(dba((uint16_t)PLAYER_Y_AT)));
        dp16_set(ACTOR_Y + 1, cpu.a);
        op_sep(0x20);
        angle = (uint8_t)((rd8(dpa(KIND8D_ANGLE)) + 1) & 0x7F);
        a8_load(angle);
        wr8(dpa(KIND8D_ANGLE), angle);
        x8_load(0x10);
        a8_load(0x48);
        jsl_frame(0x849A57u);
        abi_ActorMoveByVectorTimes();
        cpu.s = (uint16_t)(cpu.s + 3);
        jsl_frame(0x849A5Bu);
        abi_ActorSideOfPlayer();
        cpu.s = (uint16_t)(cpu.s + 3);
        wr8(dpa(ACTOR_FLIP), (uint8_t)cpu.a);
        angle = rd8(dpa(KIND8D_ANGLE));
        a8_load(angle);
        cmp8_of(angle, 0x48);
        if (angle != 0x48)
            continue;
        if (dec8_at(dpa(KIND8D_TURNS)))
            continue;
        break;
    }

    /* off, for 80 frames */
    wr8(dpa(ACTOR_FLIP), 0);
    a8_load(0x1D);
    wr8(dpa(KIND8D_ANGLE), 0x1D);
    a8_load(0x50);
    wr8(dpa(KIND8D_TURNS), 0x50);
    do {
        actor_yield(0x849A76u);
        x8_load(0x1E);
        abi_ActorMoveByVector();
    } while (dec8_at(dpa(KIND8D_TURNS)));
    wr8(dba(V_1A12), 0);
    a8_load(0xFF);
    wr8(dpa(ACTOR_X + 2), 0xFF);
    wr8(dpa(ACTOR_Y + 2), 0xFF);
    abi_ActorEndPlaced();
}
