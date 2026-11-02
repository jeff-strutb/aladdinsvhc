/*
 * actor_circle_wipe.c: a circle wipe that grows round an actor: one step a
 * call, the window table copied where the HDMA reads it. Drafted by
 * cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_CircleWipeStep(void);

#define WIPE_RADIUS 0x45u  /* the circle's radius, one up each step (word) */
#define LAYER0_X 0x1773u  /* layer 0's x pixels (word) */
#define LAYER0_Y 0x1777u  /* layer 0's y pixels (word) */

/*
 * @routine 83:EAAF ActorCircleWipeStep
 * WHAT IT DOES: One step of a circle wipe round the actor: the radius
 * grows by one and the circle is drawn round its place on screen (8 pixels
 * above its y), then the $200-byte window table is copied from $7E:5000 to
 * $7F:F500.
 * HOW: $45 up (8-bit); X = its x less LAYER0_X, Y = its y less LAYER0_Y
 * less 8, A = $45 (16-bit); CircleWipeStep; MVN $7E:5000 to $7F:F500, $200
 * bytes, DB kept; M and X 8-bit.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
 * (83:EA38, 83:EA56). Exit: M and X 8-bit; A = $FFFF, X = $5200, Y = $F700
 * (low bytes).
 */
void abi_ActorCircleWipeStep(void)
{
    uint16_t dy;

    wr8(dpa(WIPE_RADIUS), (uint8_t)(rd8(dpa(WIPE_RADIUS)) + 1));
    op_rep(0x30);
    /* the actor's place on the screen, 8 pixels up */
    flag_set(P_C, 1);
    cpu.x = op_sbc16(dp16(ACTOR_X + 1), rd16(dba(LAYER0_X)), &cpu.p);
    flag_set(P_C, 1);
    dy = op_sbc16(dp16(ACTOR_Y + 1), rd16(dba(LAYER0_Y)), &cpu.p);
    flag_set(P_C, 1);
    cpu.y = op_sbc16(dy, 0x0008, &cpu.p);
    a16_load(dp16(WIPE_RADIUS));
    abi_CircleWipeStep();
    /* the window table it drew, $7F:5000, to $7E:F500 */
    phb();
    cpu.x = 0x5000;
    cpu.y = 0xF500;
    a16_load(0x01FF);
    mvn(0x7E, 0x7F);
    plb();
    op_sep(0x30);
}
