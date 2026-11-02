/*
 * player_attach.c: the player put at an offset from an actor (below or
 * above it, and to one side by the player's facing), from a table of
 * offsets; or only the spot it would go to, kept.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_PlayerSpotAboveActor(void);
void abi_PlayerSpotBelowActor(void);
void abi_PlayerToActorAbove(void);
void abi_PlayerToActorBelow(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

#define ATTACH_OFFSETS 0xFE44u  /* by number, the player's offset from the actor: x, y (bytes, in the bank DB names) */
#define PLAYER_X_PIXELS (PLAYER + ACTOR_X + 1)
#define PLAYER_Y_PIXELS (PLAYER + ACTOR_Y + 1)
#define PLAYER_Y_SAVED 0x145Fu  /* with PLAYER_POS_SAVED: y */

/* His x the actor's plus or less the offset, by his flip. */
static void player_x_by_side(void)
{
    uint8_t flip = rd8(dba(PLAYER + ACTOR_FLIP)), off = rd8(dba(ATTACH_OFFSETS) + cpu.x), top;

    a8_load(flip);
    flag_set(P_C, !flip);
    if (flip) {
        wr8(dba(PLAYER_X_PIXELS), op_adc8(rd8(dpa(ACTOR_X + 1)), off, &cpu.p));
        top = op_adc8(rd8(dpa(ACTOR_X + 2)), 0x00, &cpu.p);
    } else {
        wr8(dba(PLAYER_X_PIXELS), op_sbc8(rd8(dpa(ACTOR_X + 1)), off, &cpu.p));
        top = op_sbc8(rd8(dpa(ACTOR_X + 2)), 0x00, &cpu.p);
    }
    wr8(dba(PLAYER_X_PIXELS + 1), top);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | top);
}

/* His y the actor's less (above) or plus (below) the offset, then his x. */
static void player_to_actor(int below)
{
    uint8_t off, top;

    cpu.x = (uint8_t)((uint8_t)cpu.a << 1);
    off = rd8(dba(ATTACH_OFFSETS + 1) + cpu.x);
    flag_set(P_C, !below);
    if (below) {
        wr8(dba(PLAYER_Y_PIXELS), op_adc8(rd8(dpa(ACTOR_Y + 1)), off, &cpu.p));
        top = op_adc8(rd8(dpa(ACTOR_Y + 2)), 0x00, &cpu.p);
    } else {
        wr8(dba(PLAYER_Y_PIXELS), op_sbc8(rd8(dpa(ACTOR_Y + 1)), off, &cpu.p));
        top = op_sbc8(rd8(dpa(ACTOR_Y + 2)), 0x00, &cpu.p);
    }
    wr8(dba(PLAYER_Y_PIXELS + 1), top);
    player_x_by_side();
}

/* The spot he would be put at, kept (PLAYER_POS_SAVED), his place as it
   was. */
static void player_spot(void (*to)(void))
{
    op_rep(P_X);
    cpu.x = rd16(dba(PLAYER_X_PIXELS));
    phx16();
    cpu.x = rd16(dba(PLAYER_Y_PIXELS));
    phx16();
    op_sep(P_X);
    to();
    op_rep(P_X);
    cpu.x = rd16(dba(PLAYER_X_PIXELS));
    wr16(dba(PLAYER_POS_SAVED), cpu.x);
    cpu.x = rd16(dba(PLAYER_Y_PIXELS));
    wr16(dba(PLAYER_Y_SAVED), cpu.x);
    plx16();
    wr16(dba(PLAYER_Y_PIXELS), cpu.x);
    plx16();
    wr16(dba(PLAYER_X_PIXELS), cpu.x);
    op_sep(P_X);
}

/*
 * @routine 82:BB3A PlayerToActorAbove
 * WHAT IT DOES: Puts the player at offset A from the actor: above it by
 * the offset's y, and by its x to the right when the player faces right,
 * to the left otherwise.
 * HOW: Entry A of the offsets at $80:FE44 (x, y bytes): the
 * player's y pixels = the actor's less y (16 bits, borrowing); facing
 * right (its flip nonzero) its x pixels = the actor's plus x, else
 * less x.
 * Entry: M 8-bit, X 8-bit; A = the offset's number; D = the actor; DB =
 * $80; called by JSL. Exit: X = A * 2; A low byte = the high byte of
 * the player's x.
 */
void abi_PlayerToActorAbove(void)
{
    player_to_actor(0);
}

/*
 * @routine 82:BB26 PlayerToActorBelow
 * WHAT IT DOES: Puts the player at offset A from the actor: below it by
 * the offset's y, and by its x to the right when the player faces right,
 * to the left otherwise.
 * HOW: Entry A of the offsets at $80:FE44 (x, y bytes): the
 * player's y pixels = the actor's plus y (16 bits, carried); then the x
 * as PlayerToActorAbove.
 * Entry: M 8-bit, X 8-bit; A = the offset's number; D = the actor; DB =
 * $80; called by JSL. Exit: as PlayerToActorAbove.
 */
void abi_PlayerToActorBelow(void)
{
    player_to_actor(1);
}

/*
 * @routine 82:BB73 PlayerSpotAboveActor
 * WHAT IT DOES: Works out where PlayerToActorAbove would put the player for
 * offset A, keeping that spot in player_pos_saved and leaving the player
 * where it was.
 * HOW: The player's x and y pushed; PlayerToActorAbove; the x and y it set
 * copied to player_pos_saved (x, then y at +2); the pushed x and y put
 * back.
 * Entry: M 8-bit; A = the offset's number; D = the actor; DB = $80;
 * called by JSL. Exit: M 8-bit, X 8-bit; X = the player's x low byte.
 */
void abi_PlayerSpotAboveActor(void)
{
    player_spot(abi_PlayerToActorAbove);
}

/*
 * @routine 82:BB9C PlayerSpotBelowActor
 * WHAT IT DOES: Works out where PlayerToActorBelow would put the player for
 * offset A, keeping that spot in player_pos_saved and leaving the player
 * where it was.
 * HOW: The player's x and y pushed; PlayerToActorBelow; then as
 * PlayerSpotAboveActor.
 * Entry: M 8-bit; A = the offset's number; D = the actor; DB = $80;
 * called by JSL. Exit: as PlayerSpotAboveActor.
 */
void abi_PlayerSpotBelowActor(void)
{
    player_spot(abi_PlayerToActorBelow);
}
