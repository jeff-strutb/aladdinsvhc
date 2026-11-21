/*
 * reset.c: clearing the game's state before a new screen or game.
 */
#include <string.h>

#include "system/state.h"
#include "system/system.h"
#include "system/task.h"

#define CLEAR(field) memset(&(field), 0, sizeof(field))

/*
 * @routine 81:9647 LevelCountersClear
 * WHAT IT DOES: Clears the four level counters at $03F5: the player's
 * time before it can be hurt again and the last special sound command
 * queued. ($03F6 and $03F7 are cleared too in the original; no code reads
 * or writes them by name, so they have no field.)
 */
void LevelCountersClear(void)
{
    g.player_hurt_time = 0;
    g.sound_special = 0;
}

/*
 * @routine 81:9655 GameRamClear
 * WHAT IT DOES: Clears the game's state for a new game and ends every task
 * but slot 0's.
 * HOW: The original cleared work RAM $03F9-$1CCD and $7E:F400-$F9FF; here,
 * every field of g whose original address lies there. A field added to
 * state.h in those ranges is added here.
 */
void GameRamClear(void)
{
    uint8_t slot;

    CLEAR(g.play_starts);               /* $03FA */
    CLEAR(g.pause_block);               /* $0404 */
    CLEAR(g.paused);                    /* $0405 */
    CLEAR(g.palette_saved);             /* $0406 */
    CLEAR(g.vram_words_pending);        /* $0616 */
    CLEAR(g.vram_words_count);          /* $0617 */
    CLEAR(g.vram_words);                /* $0619 */
    CLEAR(g.tile_changes_count);        /* $0719 */
    CLEAR(g.tile_changes_end);          /* $071B */
    CLEAR(g.tile_changes);              /* $071D */
    CLEAR(g.palette_saved_at);          /* $0606 */
    CLEAR(g.level_spot_count);          /* $0821 */
    CLEAR(g.level_spots);               /* $0822 */
    CLEAR(g.vram_upload_number);        /* $0834 */
    CLEAR(g.text_map_upload);           /* $0835 */
    CLEAR(g.text_parts_pending);        /* $0836 */
    CLEAR(g.bg2_x_held);                /* $083C */
    CLEAR(g.layer_setup_mode);          /* $083F */
    CLEAR(g.palette_upload);            /* $0844 */
    CLEAR(g.menu_blink_hidden);         /* $0847 */
    CLEAR(g.menu_blink_frames);         /* $0848 */
    CLEAR(g.menu_choice);               /* $0849 */
    CLEAR(g.menu_choice_was);           /* $084A */
    CLEAR(g.menu_cursor_moves);         /* $084B */
    CLEAR(g.frame_ready);               /* $0857 */
    CLEAR(g.vram_queue_pending);        /* $0858 */
    CLEAR(g.vram_queue_count);          /* $0859 */
    CLEAR(g.vram_queue);                /* $085A */
    CLEAR(g.actors);                    /* $08DB */
    CLEAR(g.player_top_speed);          /* $141D */
    CLEAR(g.player_move);               /* $1420 */
    CLEAR(g.player_run_accel);          /* $1440 */
    CLEAR(g.player_run_decel);          /* $1442 */
    CLEAR(g.player_turn_decel);         /* $1444 */
    CLEAR(g.continue_frames);           /* $142E */
    CLEAR(g.frame_start_x);             /* $143A */
    CLEAR(g.frame_start_y);             /* $143D */
    CLEAR(g.screen_signal);             /* $146B */
    CLEAR(g.player_box_wanted);         /* $1448 */
    CLEAR(g.player_box);                /* $1449 */
    CLEAR(g.player_ground_y);           /* $1455 */
    CLEAR(g.player_slowed);             /* $1457 */
    CLEAR(g.player_dash);               /* $1459 */
    CLEAR(g.player_saved_x);            /* $145D */
    CLEAR(g.player_saved_y);            /* $145F */
    CLEAR(g.play_frames);               /* $147C */
    CLEAR(g.play_seconds);              /* $147D */
    CLEAR(g.play_digits);               /* $147E */
    CLEAR(g.free_pool);                 /* $15A2 */
    CLEAR(g.free_pool_b);               /* $15DA */
    CLEAR(g.free_pool_top);             /* $15E4 */
    CLEAR(g.free_pool_b_top);           /* $15E6 */
    CLEAR(g.palette_cycle_running);     /* $15F1 */
    CLEAR(g.sequences);                 /* $1645 */
    CLEAR(g.vram_list_pending);         /* $1649 + $10 n */
    CLEAR(g.vram_list);                 /* $164D + $10 n */
    CLEAR(g.layers);                    /* $1738, $188E */
    CLEAR(g.scroll_x_before);           /* $19E5 */
    CLEAR(g.scroll_y_before);           /* $19E9 */
    CLEAR(g.bg_x);                      /* $19EC */
    CLEAR(g.bg_y);                      /* $19F0 */
    CLEAR(g.scroll_limit_x);            /* $1A04 */
    CLEAR(g.scroll_limit_y);            /* $1A06 */
    CLEAR(g.camera_window);             /* $1A08 */
    CLEAR(g.camera_stopped_x);          /* $1A0C */
    CLEAR(g.camera_stopped_y);          /* $1A0E */
    CLEAR(g.bg1_held);                  /* $1A14 */
    CLEAR(g.bg2_follows);               /* $1A15 */
    CLEAR(g.bg1_x_shift);               /* $1A18 */
    CLEAR(g.layer_offset_y);            /* $1A1C */
    CLEAR(g.bg2_x_shift);               /* $1A20 */
    CLEAR(g.bg2_y_shift);               /* $1A24 */
    CLEAR(g.backdrop_offset_y);         /* $1A2B */
    CLEAR(g.band_drift);                /* $1A2F */
    CLEAR(g.scroll_x_min);              /* $1A40 */
    CLEAR(g.scroll_x_max);              /* $1A42 */
    CLEAR(g.range_x);                   /* $1A44 */
    CLEAR(g.range_y);                   /* $1A46 */
    CLEAR(g.column_queue);              /* $1A48 */
    CLEAR(g.row_queue);                 /* $1A68 */
    CLEAR(g.column_queue_head);         /* $1A88 */
    CLEAR(g.column_queue_tail);         /* $1A8A */
    CLEAR(g.row_queue_head);            /* $1A8C */
    CLEAR(g.row_queue_tail);            /* $1A8E */
    CLEAR(g.fill_layers_left);          /* $1A90 */
    CLEAR(g.fill_columns_left);         /* $1A91 */
    CLEAR(g.tile_blocks);               /* $1B44 */
    CLEAR(g.tile_blocks_count);         /* $1BE4 */
    CLEAR(g.tile_blocks_pending);       /* $1BE6 */
    CLEAR(g.block_sources);             /* $1BF5 */
    memset(&g.line_data[0x400], 0, 11 * 4);   /* $7E:F800: the drifting backdrop bands' positions */
    for (slot = TASK_SLOTS - 1; slot >= 1; slot--)
        TaskSlotFree(slot);
}

/*
 * @routine 81:968F Table15F1Clear
 * WHAT IT DOES: Stops the level's six palette cycles.
 */
void Table15F1Clear(void)
{
    CLEAR(g.palette_cycle_running);
}

/*
 * @routine 81:967B LevelTablesClear
 * WHAT IT DOES: Stops the level's palette cycles and its ten value
 * sequences (each one's running byte cleared).
 */
void LevelTablesClear(void)
{
    unsigned n;

    Table15F1Clear();
    for (n = 0; n < SEQUENCES; n++)
        g.sequences[n].running = 0;
}
