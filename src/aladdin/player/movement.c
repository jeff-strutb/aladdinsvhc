/*
 * movement.c: how actors move from their speeds, and the player's run
 * settings. Drafted by cdraft.py; named and described by hand.
 *
 * An actor's position and speeds are 24-bit, a fraction byte below the
 * pixels (actor.h). Gravity adds to the vertical speed each frame, up to
 * a fall speed of 5.5 pixels a frame.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram_pad.h"
#include "wram_player.h"

void abi_PadFacing(void);

/* The fastest an actor falls: 5.5 pixels a frame */
#define FALL_SPEED_MAX 0x0580

/*
 * @routine 81:D41C ActorStepX
 * WHAT IT DOES: Moves an actor across by its speed, the way it faces.
 * HOW: Facing right ($14 = 0), its 24-bit X plus its speed; facing left,
 * minus.
 * Entry: M 8-bit; D = the actor's record. Exit: A low = X's top byte; N,
 * V, Z, C from the last add or subtract; X, Y unchanged.
 */
void abi_ActorStepX(void)
{
    uint8_t left = rd8(dpa(ACTOR_FACING)), v = left;

    a8_load(left);
    /* the 24-bit x moved by the speed, its way */
    flag_set(P_C, left != 0);
    for (int i = 0; i < 3; i++) {
        uint8_t x = rd8(dpa(ACTOR_X + i)), s = rd8(dpa(ACTOR_SPEED_X + i));

        v = left ? op_sbc8(x, s, &cpu.p) : op_adc8(x, s, &cpu.p);
        wr8(dpa(ACTOR_X + i), v);
    }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
}

/* The fall speed's cap: the speed's word above 5.5 becomes 5.5 */
/* The fall speed held to FALL_SPEED_MAX. */
static void fall_cap(void)
{
    uint16_t speed = dp16(ACTOR_SPEED_Y);

    cpu.a = FALL_SPEED_MAX;
    cmp16_of(FALL_SPEED_MAX, speed);
    if (FALL_SPEED_MAX < speed)
        dp16_set(ACTOR_SPEED_Y, FALL_SPEED_MAX);
}

/*
 * @routine 81:D448 ActorFallCap
 * WHAT IT DOES: Keeps an actor that is falling from falling faster than
 * 5.5 pixels a frame.
 * HOW: Rising (its vertical speed negative): nothing; else the speed
 * above 5.5 is cut to 5.5.
 * Entry: M 8-bit; D = the actor's record. Exit: M 8-bit; rising: A low =
 * the speed's top byte, N set; else A = $0580, N, Z, C from comparing it
 * with the speed (before the cut); X, Y unchanged.
 */
void abi_ActorFallCap(void)
{
    uint8_t top = rd8(dpa(ACTOR_SPEED_Y + 2));

    a8_load(top);
    if (!(top & 0x80))
        fall_cap();
}

/*
 * @routine 81:D44D ActorGravity
 * WHAT IT DOES: Gravity: an actor's vertical speed grows by its
 * acceleration, falling no faster than 5.5 pixels a frame.
 * HOW: The 24-bit speed plus the 8-bit acceleration; when the speed is
 * then downward, it is cut to 5.5.
 * Entry: M 8-bit; D = the actor's record. Exit: M 8-bit; rising: A low =
 * the speed's top byte, N set, C from the add; else A = $0580, N, Z, C
 * from comparing it with the speed (before the cut); X, Y unchanged.
 */
void abi_ActorGravity(void)
{
    uint8_t v;

    flag_set(P_C, 0);
    wr8(dpa(ACTOR_SPEED_Y), op_adc8(rd8(dpa(ACTOR_SPEED_Y)), rd8(dpa(ACTOR_ACCEL_Y)), &cpu.p));
    wr8(dpa(ACTOR_SPEED_Y + 1), op_adc8(rd8(dpa(ACTOR_SPEED_Y + 1)), 0x00, &cpu.p));
    v = op_adc8(rd8(dpa(ACTOR_SPEED_Y + 2)), 0x00, &cpu.p);
    wr8(dpa(ACTOR_SPEED_Y + 2), v);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (!(v & 0x80))
        fall_cap();
}

/*
 * @routine 81:D164 PlayerGroundYSave
 * WHAT IT DOES: Notes the height the player stands at, from which falls
 * are measured.
 * HOW: Its Y in pixels goes to PLAYER_GROUND_Y.
 * Entry: M 8-bit; D = the player's record; DB a bank whose $0000-$1FFF
 * is work RAM. Exit: M 8-bit; A = the Y; N and Z from it; X, Y unchanged.
 */
void abi_PlayerGroundYSave(void)
{
    uint16_t y = dp16(ACTOR_Y + 1);

    wr16(dba(PLAYER_GROUND_Y), y);
    a16_load(y);
}

/*
 * @routine 81:D16E PlayerTopSpeedHold
 * WHAT IT DOES: Makes the player's present speed the fastest it may run
 * (as when it lands from a jump at that speed).
 * HOW: The speed's low two bytes go to PLAYER_TOP_SPEED.
 * Entry: M 8-bit; D = the player's record; DB a bank whose $0000-$1FFF
 * is work RAM. Exit: A low = the speed's second byte; N, Z from it; the
 * rest unchanged.
 */
void abi_PlayerTopSpeedHold(void)
{
    wr8(dba(PLAYER_TOP_SPEED), rd8(dpa(ACTOR_SPEED_X)));
    a8_load(rd8(dpa(ACTOR_SPEED_X + 1)));
    wr8(dba(PLAYER_TOP_SPEED + 1), (uint8_t)cpu.a);
}

/*
 * @routine 81:D179 PlayerTopSpeedCap
 * WHAT IT DOES: Keeps the player's top running speed at 1.5 pixels a
 * frame or under.
 * HOW: A top speed above $0180 becomes $0180.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: M 8-bit;
 * A = $0180; N, Z, C from comparing it with the top speed (before); X, Y
 * unchanged.
 */
void abi_PlayerTopSpeedCap(void)
{
    uint16_t top = rd16(dba(PLAYER_TOP_SPEED));

    cpu.a = 0x0180;
    cmp16_of(0x0180, top);
    if (0x0180 >= top)
        wr16(dba(PLAYER_TOP_SPEED), 0x0180);
}

/*
 * @routine 81:D189 PlayerRunRatesSet
 * WHAT IT DOES: Sets how fast the player gains and loses speed running on
 * the ground.
 * HOW: Accelerating $20, slowing $FFF8 (-8), turning $FFC0 (-64), in
 * fractions of a pixel a frame each frame.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: M 8-bit;
 * A = $FFC0; N set, Z clear; the rest unchanged.
 */
void abi_PlayerRunRatesSet(void)
{
    wr16(dba(PLAYER_RUN_ACCEL), 0x0020);
    wr16(dba(PLAYER_RUN_DECEL), 0xFFF8);
    wr16(dba(PLAYER_TURN_DECEL), 0xFFC0);
    a16_load(0xFFC0);
}

/*
 * @routine 81:CE70 PlayerFlipByPad
 * WHAT IT DOES: Turns the player's sprite the way the joypad points, when
 * it points one way.
 * HOW: PadFacing; when it gives a facing (not negative), that goes to the
 * record's ACTOR_FLIP.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80. Exit: as
 * PadFacing.
 */
void abi_PlayerFlipByPad(void)
{
    abi_PadFacing();
    if (!(cpu.p & P_N))
        wr8(dpa(ACTOR_FLIP), (uint8_t)cpu.a);
}
