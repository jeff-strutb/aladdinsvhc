/*
 * input_tables.h: the input's own tables (the input session's file, stage 6).
 */
#ifndef ALADDIN_INPUT_TABLES_H
#define ALADDIN_INPUT_TABLES_H

#include <stdint.h>

/* the demos' pad scripts (system/input_modes.c) */
typedef struct DemoStep {
    uint8_t buttons;            /* pad 1's high byte: B Y Select Start Up Down Left Right */
    uint8_t frames;             /* how long it is held */
} DemoStep;

/* the entry at byte offset `at` of the scripts, as g.demo_input_at keeps it */
DemoStep demo_step_at(uint16_t at);

#endif
