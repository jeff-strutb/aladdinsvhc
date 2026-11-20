/*
 * level_tables.c: the places of the tables level_tables.h reads, all in
 * bank $80 (each reading routine runs with DB = $80). The demos' scripts'
 * starts at $B724 (words, offsets in the demo scripts input_tables reads),
 * their sequences, stages and restart points at $B6FD, $B70A, $B717; the
 * progress table at $B73E (a sequence's first entry, then the levels); by
 * level: the player's $06 and $07 at $B764, $B781, the companion at $B7BB,
 * the song at $B8EF, the exits by the view at $B96B; the next sequence at
 * $B988; the control options' masks at $B9E2 (four tables of 8 bytes, a
 * word an option); the view's edge limits at $C9B9; the spot lists at
 * $EC1E (a word a level: its list's offset from there; at it the count,
 * then 6 bytes a spot: x, y, a byte, where it leads); the restart points'
 * x at $F015 (a word a level).
 */
#include "data/data_internal.h"
#include "data/level_tables.h"

#define BANK 0x80
#define DEMO_INPUTS 0xB724
#define DEMO_SEQUENCES 0xB6FD
#define DEMO_STAGES 0xB70A
#define DEMO_RESTARTS 0xB717
#define PROGRESS 0xB73E
#define PLAYER_KINDS 0xB764
#define PLAYER_BYTES 0xB781
#define COMPANIONS 0xB7BB
#define SONGS 0xB8EF
#define EXIT_BY_VIEW 0xB96B
#define NEXT_SEQUENCE 0xB988
#define CONTROL_MASKS 0xB9E2
#define CONTROL_TABLE_BYTES 8
#define VIEW_EDGE_LIMITS 0xC9B9
#define SPOT_TABLE 0xEC1E
#define SPOT_BYTES 6
#define RESTART_X 0xF015

static uint8_t at8(uint16_t addr) { return cart8(cart_offset(BANK, addr)); }
static uint16_t at16(uint16_t addr) { return cart16(cart_offset(BANK, addr)); }

uint16_t demo_input_start(uint8_t demo) { return at16((uint16_t)(DEMO_INPUTS + 2 * demo)); }
uint8_t demo_sequence(uint8_t demo) { return at8((uint16_t)(DEMO_SEQUENCES + demo)); }
uint8_t demo_stage(uint8_t demo) { return at8((uint16_t)(DEMO_STAGES + demo)); }
uint8_t demo_restart(uint8_t demo) { return at8((uint16_t)(DEMO_RESTARTS + demo)); }

uint8_t progress_first(uint8_t sequence) { return at8((uint16_t)(PROGRESS + sequence)); }
uint8_t progress_level(uint8_t entry) { return at8((uint16_t)(PROGRESS + entry)); }

uint8_t level_player_kind(uint8_t level) { return at8((uint16_t)(PLAYER_KINDS + level)); }
uint8_t level_player_byte(uint8_t level) { return at8((uint16_t)(PLAYER_BYTES + level)); }
uint8_t level_companion(uint8_t level) { return at8((uint16_t)(COMPANIONS + level)); }
uint8_t level_song(uint8_t level) { return at8((uint16_t)(SONGS + level)); }

/* the record's order: action 1's table first ($B9E2), then 0's, 2's, 3's */
uint16_t control_option_mask(uint8_t option, unsigned action)
{
    static const uint8_t table_of_action[4] = { 1, 0, 2, 3 };

    return at16((uint16_t)(CONTROL_MASKS + CONTROL_TABLE_BYTES * table_of_action[action & 3] + 2 * option));
}

uint16_t level_restart_x(uint8_t level) { return at16((uint16_t)(RESTART_X + 2 * level)); }
bool level_exit_by_view(uint8_t level) { return at8((uint16_t)(EXIT_BY_VIEW + level)) != 0; }
uint8_t progress_next_sequence(uint8_t sequence) { return at8((uint16_t)(NEXT_SEQUENCE + sequence)); }

static uint16_t spot_offset(uint16_t level) { return at16((uint16_t)(SPOT_TABLE + 2 * level)); }

uint8_t level_spot_count(uint16_t level) { return at8((uint16_t)(SPOT_TABLE + spot_offset(level))); }

DataRef level_spot_list(uint16_t level)
{
    DataRef r;

    r.at = cart_offset(BANK, (uint16_t)(SPOT_TABLE + 1 + spot_offset(level)));
    return r;
}

uint16_t spot_x(DataRef list, unsigned spot) { return cart16(list.at + SPOT_BYTES * spot); }
uint16_t spot_y(DataRef list, unsigned spot) { return cart16(list.at + SPOT_BYTES * spot + 2); }
uint8_t spot_leads_to(DataRef list, unsigned spot) { return cart8(list.at + SPOT_BYTES * spot + 5); }

uint16_t view_edge_limit(unsigned which) { return at16((uint16_t)(VIEW_EDGE_LIMITS + 2 * which)); }
