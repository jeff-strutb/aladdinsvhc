/*
 * data.h: the cartridge's data as typed tables (plan section 3, "ROM data").
 *
 * data_load() reads assets/cart.dat (tools/extract_assets.py: the player's
 * own cartridge, checked by SHA-1, with the code left out) once, at init,
 * through nshvc_asset_*. Every table's place in the cartridge is written
 * once, in data/ only; the rest of the game sees each table by what it holds,
 * through a reader here, and never a bank, an address or a raw read. Data
 * that names other data in its own format is passed on as a DataRef, which
 * only data/ looks inside.
 */
#ifndef ALADDIN_DATA_H
#define ALADDIN_DATA_H

#include <stdint.h>

/* where more of the cartridge's data is, as the data itself names it; opaque
   outside data/ */
typedef struct DataRef { uint32_t at; } DataRef;

void data_load(void);           /* at init; ends the run with a message when cart.dat is missing or short */

/* ---- sound: the upload lists (audio/spc_upload.c's SpcUpload, the table
   of 3-byte entries the decomp lists in config/aladdin/spclists.csv) ---- */

/* one block of a list: bytes for the sound memory at `to`. Blocks of the
   SPC700 driver's own code come back as zeros (cart.dat leaves code out;
   the native driver is C). */
typedef struct SoundBlock {
    uint16_t to;                /* where the original put it in the sound memory */
    uint16_t length;            /* bytes */
    const uint8_t *bytes;
} SoundBlock;

/* list `list` (the original's X divided by 3: 0 the driver, 1 and 2 the
   sample sets, then the songs). Returns its block count, or -1 for an
   entry the cartridge leaves empty. */
int sound_list_blocks(unsigned list);
SoundBlock sound_list_block(unsigned list, unsigned block);  /* block < sound_list_blocks(list) */
uint16_t sound_list_start(unsigned list);   /* the address its end names: where the driver goes on */

/* ---- video ---- */

/* the window effects' quarter sine: 64 steps rising 0 to 15
   (video/window_effects.c's HalfSineBuild) */
uint8_t quarter_sine_step(unsigned step);   /* step < 64 */

/* ---- input: the demos' pad scripts (system/input_modes.c) ---- */

typedef struct DemoStep {
    uint8_t buttons;            /* pad 1's high byte: B Y Select Start Up Down Left Right */
    uint8_t frames;             /* how long it is held */
} DemoStep;

/* the entry at byte offset `at` of the scripts, as g.demo_input_at keeps it */
DemoStep demo_step_at(uint16_t at);

#endif
