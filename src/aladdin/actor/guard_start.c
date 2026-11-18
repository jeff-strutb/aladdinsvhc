/*
 * guard_start.c: the start of bank $A5's actors of kind $08 (the
 * guard and the scripted figures that share its kind): its record set up
 * at the player's place, a count kept in cartridge RAM, and the start its
 * $07 names; the guard's own start (a short walk, then stopped).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"
#include "wram_player.h"

void abi_ActorMoveX(void);
void abi_ActorSnapToGround(void);
void abi_ActorSpeedXClear(void);
void abi_ActorSpeedXFromA5(void);
void abi_ActorPlaceAtSpot(void);
void abi_AnimStart(void);
void abi_EscortStaySpot18(void);
void abi_EscortWaitSpot0(void);
void abi_EscortWaitSpot0C(void);
void abi_EscortWaitSpot10(void);
void abi_GuardPlacedHold(void);
void abi_GuardStart(void);
void abi_GuardRestart(void);
void abi_GuardStop(void);
void abi_PlayerSceneDropIn(void);
void abi_PlayerSceneHops(void);
void abi_PlayerSceneLeap(void);
void abi_PlayerSceneWalkOff(void);

#define G_KIND_TIMES2 0x1476u   /* the kind * 2 it was started with (from the actor loop's X) */
#define G_COUNT 0x1475u         /* the count in cartridge RAM, after its increment */
#define G_CART_COUNTS 0x700000u /* cartridge RAM: a count by kind * 2 (the board has none: open bus) */
#define G_SHOWN 0x49u           /* nonzero: started by $07 = 12 (the guard shown at once) */
#define G_STATE 0x4Du           /* its state: the entry of GuardWait's table */
#define G_MOVING 0x41u
#define G_LANDING 0x42u
#define G_IN_AIR 0x44u
#define G_GONE 0x45u
#define G_BACK_OFF 0x46u
#define G_CODE_SET 0x146Bu
#define G_146F 0x146Fu
#define STARTS 0xE56Du          /* GuardStart's table (bank $A5) */

/*
 * @routine 83:9205 Kind08Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $08 go on from the actor loop in its
 * start state (the table at $83:8CAB): GuardStart.
 * HOW: JML to GuardStart ($A5:E515).
 * Entry: D = the actor's record; X = twice its kind; reached by the actor
 * loop's jump (JMP (table,X)) by kind. Exit: never returns: as GuardStart.
 */
void abi_Kind08Enter(void)
{
    abi_GuardStart();
}

/* The guard's own start ($A5:E58B): state 1, a walk of 40 frames, then
   stopped (on into GuardStop). */
static void guard(void)
{
    a8_load(0x01);
    wr8(dpa(G_STATE), 0x01);
    a8_load(0x28);
    wr8(dpa(G_LANDING), 0x28);
    y8_load(0x06);
    abi_ActorSpeedXFromA5();
    a8_load(0x01);
    abi_AnimStart();
    do {
        actor_yield(0xA5E59Eu);
        abi_ActorMoveX();
        abi_ActorSnapToGround();
    } while (dec8_at(dpa(G_LANDING)));
    a8_load(0x00);
    abi_AnimStart();
    abi_ActorSpeedXClear();
    abi_GuardStop();
}

/*
 * @routine A5:E515 GuardStart
 * @actor
 * WHAT IT DOES: The start of bank $A5's actors of kind $08: shown and
 * hittable, facing right, put where the player is (16 lower), a count in
 * cartridge RAM by its kind stepped up, its fields cleared, then on by
 * its $07: 0 and 12 the guard (a walk of 40 frames, then stopped), 2 and 3
 * the scripted figure's ways, 7 to 10 the escort's spots, others their
 * own starts.
 * HOW: G_KIND_TIMES2 = X; ACTOR_08 bit 0, ACTOR_09 bits $90 set; $28
 * $FF; facing and flip 0; $16 $CF8F, $29 $8909; its x = the player's, y =
 * his + $10; 8-bit: the byte of G_CART_COUNTS by X, plus 1, to G_COUNT
 * and back; G_SHOWN cleared; on into GuardRestart ($A5:E554), its fields
 * cleared and the table at $A5:E56D by $07. $07 12 ($A5:E587): G_SHOWN 1, then as 0 ($A5:E58B): G_STATE 1,
 * G_LANDING $28, ActorSpeedXFromA5 6, AnimStart 1; each frame (BRK)
 * ActorMoveX and ActorSnapToGround until G_LANDING runs out; AnimStart 0,
 * ActorSpeedXClear, and on into GuardStop.
 * Entry: M 8-bit, X 16-bit or 8-bit (twice its kind, $10); D = the
 * actor's record; DB = $A5; reached by Kind08Enter's JML. Exit: never
 * returns.
 */
void abi_GuardStart(void)
{
    uint8_t n;

    wr8(dba(G_KIND_TIMES2), (uint8_t)cpu.x);
    a8_load(0x01);
    tsb_at(dpa(ACTOR_08), 0x01);
    a8_load((uint8_t)(rd8(dpa(ACTOR_09)) | 0x90));
    wr8(dpa(ACTOR_09), (uint8_t)cpu.a);
    a8_load(0xFF);
    wr8(dpa(0x28), 0xFF);
    wr8(dpa(ACTOR_FACING), 0x00);
    wr8(dpa(ACTOR_FLIP), 0x00);
    op_rep(0x20);
    a16_load(0xCF8F);
    dp16_set(0x16, 0xCF8F);
    a16_load(0x8909);
    dp16_set(0x29, 0x8909);
    a16_load(rd16(dba(PLAYER + ACTOR_X + 1)));
    dp16_set(ACTOR_X + 1, cpu.a);
    flag_set(P_C, 0);
    a16_load(op_adc16(rd16(dba(PLAYER + ACTOR_Y + 1)), 0x0010, &cpu.p));
    dp16_set(ACTOR_Y + 1, cpu.a);
    op_sep(0x30);
    n = (uint8_t)(rd8(G_CART_COUNTS + (uint8_t)cpu.x) + 1);
    a8_load(n);
    wr8(dba(G_COUNT), n);
    wr8(G_CART_COUNTS + (uint8_t)cpu.x, n);
    wr8(dpa(G_SHOWN), 0x00);
    abi_GuardRestart();
}

/*
 * @routine A5:E554 GuardRestart
 * @actor
 * WHAT IT DOES: GuardStart past its setting up: the kind $08 actor's
 * fields cleared and on by its $07 (GuardStart falls into it;
 * PlayerSceneRuns and PlayerSceneFallPose jump to it with $07 set to 0).
 * HOW: $40, G_STATE, G_MOVING, G_IN_AIR, G_GONE, G_BACK_OFF, G_CODE_SET
 * and G_146F cleared; JMP through the table at $A5:E56D by $07 (doubled).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5. Exit: never
 * returns: as the start its $07 names.
 */
void abi_GuardRestart(void)
{
    uint8_t v07;

    wr8(dpa(ACTOR_40), 0x00);
    wr8(dpa(G_STATE), 0x00);
    wr8(dpa(G_MOVING), 0x00);
    wr8(dpa(G_IN_AIR), 0x00);
    wr8(dpa(G_GONE), 0x00);
    wr8(dpa(G_BACK_OFF), 0x00);
    wr8(dba(G_CODE_SET), 0x00);
    wr8(dba(G_146F), 0x00);
    v07 = rd8(dpa(ACTOR_07));
    a8_load((uint8_t)(v07 << 1));               /* ASL */
    flag_set(P_C, v07 & 0x80);
    x8_load((uint8_t)cpu.a);                    /* TAX */
    switch (v07 & 0x7F) {
    case 0:
        guard();
        return;
    case 2:
        abi_PlayerSceneLeap();
        return;
    case 3:
        abi_PlayerSceneHops();
        return;
    case 4:
        abi_PlayerSceneDropIn();
        return;
    case 5:
        abi_PlayerSceneWalkOff();
        return;
    case 7:
        abi_EscortStaySpot18();
        return;
    case 8:
        abi_EscortWaitSpot0();
        return;
    case 9:
        abi_EscortWaitSpot0C();
        return;
    case 10:
        abi_EscortWaitSpot10();
        return;
    case 11:
        abi_GuardPlacedHold();
        return;
    case 12:
        a8_load(0x01);
        wr8(dpa(G_SHOWN), 0x01);
        guard();
        return;
    default:                                    /* 1 and 6 (PlayerSceneRuns, PlayerSceneFallPose: they go on at this restart), and past the table's 13 words */
        jump_routine(0xA50000u | rd16(0xA50000u | (uint16_t)(STARTS + cpu.x)));
        return;
    }
}

/*
 * @routine A5:F00E GuardPlacedHold
 * @actor
 * WHAT IT DOES: Bank $A5's start for $07 = 11: a guard put at spot $20,
 * in state 4 (holding back while the player is to its left), stopped.
 * No script meets it: written from its bytes.
 * HOW: G_STATE 4; ActorPlaceAtSpot with X = $20; AnimStart 0; on at
 * GuardStop.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
 * the jump through the table at $A5:E56D for $07 = 11. Exit: never
 * returns.
 */
void abi_GuardPlacedHold(void)
{
    a8_load(0x04);
    wr8(dpa(G_STATE), 0x04);
    x8_load(0x20);
    abi_ActorPlaceAtSpot();
    a8_load(0x00);
    abi_AnimStart();
    abi_GuardStop();
}
