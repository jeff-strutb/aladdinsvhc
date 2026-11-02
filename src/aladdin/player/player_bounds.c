/*
 * player_bounds.c: where the player is against the screen and the level:
 * kept on screen across, fallen out of the bottom, and the countdown of
 * the time it cannot be hurt. Drafted by cdraft.py; named and described by
 * hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram_video.h"
#include "actor.h"
#include "wram_player.h"

/* The play field's scroll (whole pixels, words; see wram_video.h) */
#define FIELD_SCROLL_X (LAYER0 + LAYER_SCROLL_X)
#define FIELD_SCROLL_Y (LAYER0 + LAYER_SCROLL_Y)
/* In bank $80: the least and most x pixel from the screen's left edge */
#define SCREEN_X_LIMITS 0xC9B9u
/* In bank $80: the least and most y pixel from the screen's top */
#define SCREEN_Y_LIMITS 0xC9C9u
/* The player's y pixels (word), and its top byte */
#define PLAYER_Y_PIXELS (PLAYER + ACTOR_Y + 1)
#define V_0347 0x000347u

/*
 * @routine 81:E8EC PlayerFallenTest
 * WHAT IT DOES: Tells whether the player is still above the bottom of the
 * level: C clear once it has fallen 230 pixels below the scroll's lowest
 * place.
 * HOW: Its y pixels' top byte negative (above the level): C set. Else the
 * scroll Y limit, or the scroll Y itself while an empty block stops the
 * scroll up or down, plus $E6, compared with its y pixels.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: C set: above; C
 * clear: below. M 8-bit, X 8-bit; X = CAMERA_STOPPED_Y's low byte (unless
 * above).
 */
void abi_PlayerFallenTest(void)
{
    uint8_t high = rd8(dba(PLAYER_Y_PIXELS + 1)), stopped;

    flag_set(P_C, 1);
    a8_load(high);
    if (high & 0x80)                      /* above the map: not fallen */
        return;
    /* below the screen's bottom ($E6 below the field's top, or the
       scroll limit's when the camera keeps going) */
    op_rep(P_M);
    cpu.a = rd16(dba(SCROLL_LIMIT_Y));
    stopped = rd8(dba(CAMERA_STOPPED_Y));
    x8_load(stopped);
    if (stopped)
        a16_load(rd16(dba(FIELD_SCROLL_Y)));
    flag_set(P_C, 0);
    cpu.a = op_adc16(cpu.a, 0x00E6, &cpu.p);
    cmp16_of(cpu.a, rd16(dba(PLAYER_Y_PIXELS)));
    op_sep(P_M);
}


/*
 * @routine 81:E95C PlayerHurtCountdown
 * WHAT IT DOES: Counts down the time the player cannot be hurt; when it
 * runs out, the player can be hit again.
 * HOW: PLAYER_HURT_TIME, when nonzero, steps down; at 0, bit 4 of the
 * player's $08, bit 7 of the actor's $08 and bit 2 of $0347 are cleared.
 * Entry: M 8-bit; D = the player's record; called by JSR. Exit: M 8-bit;
 * ran out: A low = the new $0347, N and Z from it; else A low = the count
 * before its step.
 */
void abi_PlayerHurtCountdown(void)
{
    uint8_t t = rd8(dba(PLAYER_HURT_TIME));

    a8_load(t);
    if (!t || dec8_at(dba(PLAYER_HURT_TIME)))
        return;
    /* the time runs out: he can be hit again */
    a8_load((uint8_t)(rd8(dba(PLAYER + ACTOR_08)) & 0xEF));
    wr8(dba(PLAYER + ACTOR_08), (uint8_t)cpu.a);
    a8_load((uint8_t)(rd8(dpa(ACTOR_08)) & 0x7F));
    wr8(dpa(ACTOR_08), (uint8_t)cpu.a);
    a8_load((uint8_t)(rd8(dba(V_0347)) & 0xFB));
    wr8(dba(V_0347), (uint8_t)cpu.a);
}


/* The actor's position word `field` kept between the screen's edges:
   the scroll plus the limit pair at `limits` (the lower, then the upper). */
static void keep_within(uint16_t scroll, uint16_t limits, uint16_t field)
{
    uint16_t edge;

    op_rep(P_M);
    flag_set(P_C, 0);
    edge = op_adc16(rd16(dba(scroll)), rd16(dba(limits)), &cpu.p);
    cpu.a = edge;
    cmp16_of(edge, dp16(field));
    if (edge >= dp16(field)) {
        dp16_set(field, edge);
    } else {
        flag_set(P_C, 0);
        edge = op_adc16(rd16(dba(scroll)), rd16(dba(limits + 2)), &cpu.p);
        cpu.a = edge;
        cmp16_of(edge, dp16(field));
        if (edge < dp16(field))
            dp16_set(field, edge);
    }
    op_sep(P_M);
}

/*
 * @routine 81:E990 PlayerKeepOnScreenX
 * WHAT IT DOES: Keeps the actor's x on screen: between 16 and 240 pixels
 * from the play field's left edge.
 * HOW: The play field's scroll X plus each limit (from the table) compared
 * with its x pixels, unsigned; past one, its x set to it.
 * Entry: M 8-bit; D = the actor's record; DB = $80; called by JSR. Exit: M
 * 8-bit; A = the last limit computed; C from its comparison.
 */
void abi_PlayerKeepOnScreenX(void)
{
    keep_within(FIELD_SCROLL_X, SCREEN_X_LIMITS, ACTOR_X + 1);
}

/*
 * @routine 81:E9AD PlayerKeepOnScreen
 * WHAT IT DOES: Keeps the actor on screen: x between 16 and 240 pixels
 * from the play field's left edge, y between 24 and 216 from its top.
 * HOW: PlayerKeepOnScreenX; then the play field's scroll Y plus each y
 * limit (from the table) compared with its y pixels, unsigned; past one,
 * its y set to it.
 * Entry: M 8-bit; D = the actor's record; DB = $80; called by JSR. Exit: M
 * 8-bit; A = the last limit computed; C from its comparison.
 */
void abi_PlayerKeepOnScreen(void)
{
    abi_PlayerKeepOnScreenX();
    keep_within(FIELD_SCROLL_Y, SCREEN_Y_LIMITS, ACTOR_Y + 1);
}

