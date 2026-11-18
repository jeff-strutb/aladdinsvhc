/*
 * player_frame.c: the player's work each frame his actor waits: his move's
 * animation started when the move changes, the hurt time counted down,
 * the view-edge rule run, his picture sent, the fall out of the level
 * tested, and the test box set. Drafted by cdraft.py; named and described
 * by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

void abi_ActorPictureSend(void);
void abi_AnimStep(void);
void abi_PlayerBoxUpdate(void);
void abi_PlayerEnter8CFrom1Long(void);
void abi_PlayerFallenTest(void);
void abi_PlayerHurtCountdown(void);
void abi_PlayerMoveStart(void);
void abi_TestBoxPlace(void);
void abi_ViewEdgeRuleRun(void);

#define PFW_1435 0x001435u        /* cleared at the start of the player's frame work */
/* Nonzero: a scene moves the player, and only his animation and picture
   are kept up; bit 6 set: not his animation; bit 7 set: not while
   PFW_081E is 0 */
#define PFW_SCENE 0x001430u
#define PFW_081E 0x00081Eu
#define PFW_MOVE_WAS 0x00141Fu    /* the move whose animation was last started */
#define PFW_1A0A 0x001A0Au        /* nonzero: an index into PFW_03F1_VALUES */
#define PFW_03F1 0x0003F1u        /* set from PFW_03F1_VALUES by PFW_1A0A */
#define PFW_03F1_VALUES 0xC7E8u   /* in bank $80: bytes */
#define PFW_FLAGS 0x000347u       /* nonzero: bit 7 of the player's $08 set */
#define PFW_142C 0x00142Cu        /* cleared each frame with V_1428 and V_1434 */
#define PFW_NO_FALL_1 0x07        /* levels where falling out of the bottom does not hurt */
#define PFW_NO_FALL_2 0x12

/*
 * @routine 81:E864 PlayerFrameWork
 * WHAT IT DOES: The player's work each frame his actor waits: starts his
 * move's animation when the move has changed, counts down the time he
 * cannot be hurt, steps his animation, runs the view-edge rule, sends his
 * picture, hurts him when he falls out of the bottom of the level, and
 * sets the test box from his box. While a scene moves him (PFW_SCENE
 * nonzero), only his animation and picture are kept up and the test box
 * placed.
 * HOW: PFW_1435 cleared. PFW_SCENE nonzero (the code at $81:E84D, before
 * the entry): bit 7 set with PFW_081E 0, or bit 6 set, skips AnimStep;
 * then ActorPictureSend and a jump to TestBoxPlace. Otherwise:
 * PLAYER_MOVE unlike PFW_MOVE_WAS: kept there, PlayerMoveStart with it;
 * PlayerHurtCountdown; AnimStep unless V_142D; ViewEdgeRuleRun;
 * ActorPictureSend; PFW_1A0A nonzero: PFW_03F1 from PFW_03F1_VALUES by it.
 * With health left, on a level but 7 and $12, PlayerFallenTest clear (he
 * has fallen): bit 7 of his $09 cleared, PlayerEnter8CFrom1Long. Bit 7 of
 * his $08 set from PFW_FLAGS nonzero, else cleared; V_1428, V_1434 and
 * PFW_142C cleared; PlayerBoxUpdate, and on into TestBoxPlace. No script
 * runs bit 7 of PFW_SCENE set (the read of PFW_081E): that path is
 * written from its bytes.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSL (ActorKind00Wait). Exit: as TestBoxPlace leaves them (M and X 8-bit;
 * A = the test box's y; its RTL is this routine's).
 */
/* A JSL from `site` into fn: its return frame pushed, popped after. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

void abi_PlayerFrameWork(void)
{
    stz8(ea_abs(PFW_1435));                           /* 81E864 */
    lda8(ea_abs(PFW_SCENE));
    if (!IF_Z) {                                      /* a scene moves him: 81E84D */
        if (IF_N) {
            lda8(ea_abs(PFW_081E));
            if (IF_Z)
                goto picture;
        }
        bit8_v(rd8(ea_abs(PFW_SCENE)));
        if (!IF_V)
            jsl_call(0x81E859u, abi_AnimStep);
picture:
        jsl_call(0x81E85Du, abi_ActorPictureSend);
        abi_TestBoxPlace();                           /* JMP */
        return;
    }

    /* his move's animation, when the move changed */
    lda8(ea_abs(PLAYER_MOVE));
    cmp8_v(rd8(ea_abs(PFW_MOVE_WAS)));
    if (!IF_Z) {
        sta8(ea_abs(PFW_MOVE_WAS));
        lda8(ea_abs(PLAYER_MOVE));
        call_frame(0xE87C, abi_PlayerMoveStart);
    }
    call_frame(0xE87F, abi_PlayerHurtCountdown);
    lda8(ea_abs(V_142D));
    if (IF_Z)
        jsl_call(0x81E885u, abi_AnimStep);
    call_frame(0xE88B, abi_ViewEdgeRuleRun);
    jsl_call(0x81E88Cu, abi_ActorPictureSend);
    ldx8(ea_abs(PFW_1A0A));
    if (!IF_Z) {
        lda8(ea_absx(PFW_03F1_VALUES));
        sta8(ea_abs(PFW_03F1));
    }

    /* fallen out of the bottom of the level: hurt */
    lda8(ea_abs(HEALTH));
    if (!IF_Z) {
        lda8(ea_abs(LEVEL));
        cmp8_v(PFW_NO_FALL_1);
        if (!IF_Z) {
            cmp8_v(PFW_NO_FALL_2);
            if (!IF_Z) {
                call_frame(0xE8AD, abi_PlayerFallenTest);
                if (!IF_C) {
                    lda8(ea_dp(ACTOR_09));
                    and8_v(0x7F);
                    sta8(ea_dp(ACTOR_09));
                    jsl_call(0x81E8B6u, abi_PlayerEnter8CFrom1Long);
                }
            }
        }
    }

    /* bit 7 of his $08 from PFW_FLAGS */
    lda8(ea_dp(ACTOR_08));
    and8_v(0x7F);
    ldx8(ea_abs(PFW_FLAGS));
    if (!IF_Z) {
        lda8_v(0x80);
        ora8_v(rd8(ea_dp(ACTOR_08)));
    }
    sta8(ea_dp(ACTOR_08));
    stz8(ea_abs(V_1428));
    stz8(ea_abs(V_1434));
    stz8(ea_abs(PFW_142C));
    call_frame(0xE8D4, abi_PlayerBoxUpdate);
    abi_TestBoxPlace();                               /* on into it */
}
