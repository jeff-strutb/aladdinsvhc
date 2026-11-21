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
 * every field of g whose original address lies there (the list below is
 * made from state.h's address comments: a field added to state.h in those
 * ranges is added here, or it keeps its value from the level before, as
 * scroll_kind did in the ending's stage 8).
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
    CLEAR(g.band_table_buffer);         /* $03FC: in the original's cleared range */
    CLEAR(g.uploads_held);              /* $0403: in the original's cleared range */
    CLEAR(g.mode7_scale);               /* $0825: in the original's cleared range */
    CLEAR(g.mode7_how);                 /* $0829: in the original's cleared range */
    CLEAR(g.scroll_kind);               /* $083D: in the original's cleared range */
    CLEAR(g.backdrop_stage);            /* $0841: in the original's cleared range */
    CLEAR(g.player_hurt_count);         /* $141B: in the original's cleared range */
    CLEAR(g.player_in_air);             /* $141C: in the original's cleared range */
    CLEAR(g.player_move_was);           /* $141F: in the original's cleared range */
    CLEAR(g.player_skid_started);       /* $1422: in the original's cleared range */
    CLEAR(g.level_exiting);             /* $1423: in the original's cleared range */
    CLEAR(g.player_fallen_out);         /* $1424: in the original's cleared range */
    CLEAR(g.player_no_actor_landing);   /* $1427: in the original's cleared range */
    CLEAR(g.player_on_actor);           /* $1428: in the original's cleared range */
    CLEAR(g.player_ground_pushed);      /* $1429: in the original's cleared range */
    CLEAR(g.player_stand_held);         /* $142B: in the original's cleared range */
    CLEAR(g.player_anim_held);          /* $142D: in the original's cleared range */
    CLEAR(g.player_hurt_frames);        /* $142F: in the original's cleared range */
    CLEAR(g.player_scene);              /* $1430: in the original's cleared range */
    CLEAR(g.player_actor_ground);       /* $1434: in the original's cleared range */
    CLEAR(g.kind66_touched);            /* $1435: in the original's cleared range */
    CLEAR(g.player_touched_66);         /* $1435: in the original's cleared range */
    CLEAR(g.player_no_crouch_jump);     /* $1446: in the original's cleared range */
    CLEAR(g.player_air_throw);          /* $144B: in the original's cleared range */
    CLEAR(g.player_landed);             /* $144E: in the original's cleared range */
    CLEAR(g.player_bounce_frames);      /* $144F: in the original's cleared range */
    CLEAR(g.player_bounce_row);         /* $1450: in the original's cleared range */
    CLEAR(g.player_air_frames);         /* $1451: in the original's cleared range */
    CLEAR(g.stages_ended);              /* $1454: in the original's cleared range */
    CLEAR(g.player_let_go);             /* $1458: in the original's cleared range */
    CLEAR(g.player_no_landing);         /* $145A: in the original's cleared range */
    CLEAR(g.view_edge_rule);            /* $145B: in the original's cleared range */
    CLEAR(g.scripted_cue);              /* $146C: in the original's cleared range */
    CLEAR(g.rider_kind);                /* $146F: in the original's cleared range */
    CLEAR(g.guard_count);               /* $1475: in the original's cleared range */
    CLEAR(g.guard_kind_times2);         /* $1476: in the original's cleared range */
    CLEAR(g.flier_slots);               /* $1478: in the original's cleared range */
    CLEAR(g.scene_exits);               /* $147B: in the original's cleared range */
    CLEAR(g.test_box_x);                /* $1484: in the original's cleared range */
    CLEAR(g.test_box_y);                /* $1486: in the original's cleared range */
    CLEAR(g.test_box_half_w);           /* $1488: in the original's cleared range */
    CLEAR(g.test_box_half_h);           /* $148A: in the original's cleared range */
    CLEAR(g.test_box_offset_x);         /* $148C: in the original's cleared range */
    CLEAR(g.test_box_offset_y);         /* $148E: in the original's cleared range */
    CLEAR(g.draw_list);                 /* $1492: in the original's cleared range */
    CLEAR(g.draw_count);                /* $1592: in the original's cleared range */
    CLEAR(g.palette_slot_users);        /* $15E9: in the original's cleared range */
    CLEAR(g.palette_slot_kind);         /* $15ED: in the original's cleared range */
    CLEAR(g.palette_cycle);             /* $15F1: in the original's cleared range */
    CLEAR(g.camera_window_step);        /* $1A09: in the original's cleared range */
    CLEAR(g.camera_push);               /* $1A0A: in the original's cleared range */
    CLEAR(g.scroll_x_held);             /* $1A10: in the original's cleared range */
    CLEAR(g.scroll_y_held);             /* $1A12: in the original's cleared range */
    CLEAR(g.sprites_take_bg2_shift);    /* $1A16: in the original's cleared range */
    CLEAR(g.backdrop_offset_y_high);    /* $1A2D: in the original's cleared range */
    CLEAR(g.boss_beaten);               /* $1B14: in the original's cleared range */
    CLEAR(g.chain_head);                /* $1B14: in the original's cleared range */
    CLEAR(g.chain_tail);                /* $1B16: in the original's cleared range */
    CLEAR(g.chain_go);                  /* $1B18: in the original's cleared range */
    CLEAR(g.chain_step);                /* $1B19: in the original's cleared range */
    CLEAR(g.riser_send);                /* $1B1A: in the original's cleared range */
    CLEAR(g.riser_course);              /* $1B1B: in the original's cleared range */
    CLEAR(g.chain_phase);               /* $1B1C: in the original's cleared range */
    CLEAR(g.chain_round);               /* $1B1D: in the original's cleared range */
    CLEAR(g.chain_round_or);            /* $1B1E: in the original's cleared range */
    CLEAR(g.scene_go_on);               /* $1B1F: in the original's cleared range */
    CLEAR(g.head_anim);                 /* $1B20: in the original's cleared range */
    CLEAR(g.head_anim_shown);           /* $1B21: in the original's cleared range */
    CLEAR(g.head_cue);                  /* $1B22: in the original's cleared range */
    CLEAR(g.head_hit_frames);           /* $1B29: in the original's cleared range */
    CLEAR(g.head_flash);                /* $1B2A: in the original's cleared range */
    CLEAR(g.chain_head_struck);         /* $1B2B: in the original's cleared range */
    CLEAR(g.chain_untouchable);         /* $1B2C: in the original's cleared range */
    CLEAR(g.chain_rise_speed);          /* $1B2D: in the original's cleared range */
    CLEAR(g.rise_sounded);              /* $1B31: in the original's cleared range */
    CLEAR(g.close_count);               /* $1B32: in the original's cleared range */
    CLEAR(g.head_turns);                /* $1B33: in the original's cleared range */
    CLEAR(g.first_sweeps);              /* $1B34: in the original's cleared range */
    CLEAR(g.head_pose);                 /* $1B35: in the original's cleared range */
    CLEAR(g.split_line);                /* $1C7D: in the original's cleared range */
    CLEAR(g.text_bands);                /* $1C8E: in the original's cleared range */
    CLEAR(g.text_band_values);          /* $1C9E: in the original's cleared range */
    memset(g.line_data, 0, sizeof g.line_data);   /* $7E:F400-$F9FF: the per-line tables and the backdrop bands' positions */
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
