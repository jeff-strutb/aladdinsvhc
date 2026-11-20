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
 * x at $F015 (a word a level); the switch flags' masks by stage at $D27E
 * and the eight bits at $BCA1; the flagged spots by level from $0E at
 * $D24A (a byte a level: the offset of its list from $D24A; there the
 * count, then 4 bytes a spot: x, y); each level's progress sequence and
 * stage at $B933 (2 bytes a level; the record says $81, the routine runs
 * with DB = $80); level 6's bands' speeds at $C5D6, steps at $C5C0 and
 * most at $C5EC (11 words each); the level-end screen's tables from $FDC5
 * (the record says bank $81, the task runs with DB = $80): step addresses
 * $FDC5 (words), column limits $FDCD, palettes $FDD1 (4 a part), pictures
 * $FDD5 (4 a part from 1), picture tiles $FDF1, column attributes $FDF7,
 * blocks $FDFA (a byte a part), blank blocks $FE02 (16 words). The
 * password screen reads the same bytes: the start pictures at $FDD1, the
 * passwords at $FDD5 (4 pictures each, eight of them: the level end's
 * pictures of its parts), the parts at $FE36, the health at $FE3C, the
 * pictures' map bits at $FE22 and the slots' addresses at $FE2E (words).
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

#define FLAG_MASKS 0xD27E
#define FLAG_BITS 0xBCA1

uint8_t level_flag_mask(uint8_t stage) { return at8((uint16_t)(FLAG_MASKS + stage)); }
uint8_t flag_bit(unsigned flag) { return at8((uint16_t)(FLAG_BITS + (flag & 7))); }

#define FLAG_SPOTS 0xD24A
#define LEVEL_PROGRESS 0xB933

static uint8_t flag_spots_at(uint8_t level)
{
    return at8((uint16_t)(FLAG_SPOTS + (uint8_t)((uint8_t)(level - 0x0E) << 1)));
}

uint8_t flag_spot_count(uint8_t level) { return at8((uint16_t)(FLAG_SPOTS + flag_spots_at(level))); }
uint16_t flag_spot_x(uint8_t level, unsigned i) { return at16((uint16_t)(FLAG_SPOTS + 1 + (uint8_t)(flag_spots_at(level) + 4 * i))); }
uint16_t flag_spot_y(uint8_t level, unsigned i) { return at16((uint16_t)(FLAG_SPOTS + 3 + (uint8_t)(flag_spots_at(level) + 4 * i))); }

uint8_t level_progress_sequence(uint8_t level) { return at8((uint16_t)(LEVEL_PROGRESS + (uint8_t)(level << 1))); }
uint8_t level_progress_stage(uint8_t level) { return at8((uint16_t)(LEVEL_PROGRESS + 1 + (uint8_t)(level << 1))); }

#define BAND_STARTS 0xC5D6
#define BAND_STEPS 0xC5C0
#define BAND_MOST 0xC5EC

uint16_t band_start_speed(unsigned band) { return at16((uint16_t)(BAND_STARTS + 2 * band)); }
uint16_t band_speed_step(unsigned band) { return at16((uint16_t)(BAND_STEPS + 2 * band)); }
uint16_t band_most_speed(unsigned band) { return at16((uint16_t)(BAND_MOST + 2 * band)); }

#define END_STEP_VRAM 0xFDC5
#define END_COLUMN_LIMITS 0xFDCD
#define END_PALETTES 0xFDD1
#define END_PICTURES 0xFDD5
#define END_PICTURE_TILES 0xFDF1
#define END_COLUMN_ATTRS 0xFDF7
#define END_BLOCKS 0xFDFA
#define END_BLANKS 0xFE02

uint16_t level_end_step_vram(unsigned step) { return at16((uint16_t)(END_STEP_VRAM + 2 * step)); }
uint8_t level_end_column_limit(unsigned column) { return at8((uint16_t)(END_COLUMN_LIMITS + column)); }
uint8_t level_end_palette(uint8_t part, unsigned i) { return at8((uint16_t)(END_PALETTES + (uint8_t)(part << 2) + i)); }
uint8_t level_end_picture(uint8_t part, unsigned column) { return at8((uint16_t)(END_PICTURES + (uint8_t)((part - 1) << 2) + column)); }
uint8_t level_end_picture_tile(uint8_t picture) { return at8((uint16_t)(END_PICTURE_TILES + picture)); }
uint8_t level_end_column_attr(unsigned column) { return at8((uint16_t)(END_COLUMN_ATTRS + column)); }
uint8_t level_end_block(uint8_t part) { return at8((uint16_t)(END_BLOCKS + part)); }
uint16_t level_end_blank_block(unsigned i) { return at16((uint16_t)(END_BLANKS + 2 * i)); }

#define PASSWORD_START 0xFDD1
#define PASSWORD_LIST 0xFDD5
#define PASSWORD_PARTS 0xFE36
#define PASSWORD_HEALTH 0xFE3C
#define PASSWORD_ROW_BITS 0xFE22
#define PASSWORD_SLOT_VRAM 0xFE2E

uint8_t password_start_picture(unsigned slot) { return at8((uint16_t)(PASSWORD_START + slot)); }
uint8_t password_entry(unsigned password, unsigned slot) { return at8((uint16_t)(PASSWORD_LIST + 4 * password + slot)); }
uint8_t password_part(unsigned password) { return at8((uint16_t)(PASSWORD_PARTS + password)); }
uint8_t password_health(uint8_t part) { return at8((uint16_t)(PASSWORD_HEALTH + part)); }
uint16_t password_row_bits(unsigned picture) { return at16((uint16_t)(PASSWORD_ROW_BITS + 2 * picture)); }
uint16_t password_slot_vram(unsigned slot) { return at16((uint16_t)(PASSWORD_SLOT_VRAM + 2 * slot)); }

#define LEVEL6_BANDS 0xB991

uint8_t level6_band_count(void) { return at8(LEVEL6_BANDS); }
uint8_t level6_band_lines(unsigned band) { return at8((uint16_t)(LEVEL6_BANDS + 1 + band)); }
