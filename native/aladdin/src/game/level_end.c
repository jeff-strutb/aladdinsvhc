/*
 * level_end.c: the screen between a level's acts: a picture of the part
 * just played grown in four columns as the walker (kinds $8F and $94, in
 * the level $24 the screen sets up) walks across it, a pause, then the wait
 * for a button.
 *
 * Not converted, with their proof: the PAL console's screen of its own
 * (81:F622 on: taken only with $005D set, which only a PAL console gives;
 * the board is NTSC); and the copier-RAM check at the end of the task
 * (the count at $70:1399, written and read back: with no RAM at $70 the
 * read gives the bus's last byte, the bank $70, which is not the count
 * plus one; so on the retail board the branch that sets the part to 1 or 6
 * is never taken).
 */
#include "audio/sound.h"
#include "data/level_tables.h"
#include "game/game.h"
#include "system/state.h"
#include "system/system.h"
#include "system/task.h"
#include "video/video.h"
#include "video/video_tasks.h"

#define FADE_SLOT 1
#define END_LEVEL 0x24              /* the level whose RAM and actors the screen uses */
#define BLANK_TILE 0x080E           /* the map word a blanked block gets */
#define GROW_START_X 0x20           /* the walker's x at which the first block of the picture is drawn */
#define GROW_BLOCK_EVERY 0x10       /* then a block each time it has walked this much further */
#define GROW_END_X 0xE0             /* the walker's x at which the columns begin */
#define LAST_PART 7                 /* the part after which play goes straight on */
#define PART_WITH_TILES 5           /* the part whose picture has five loose blocks */
#define BG3_X 0xFC
#define SHOW_LAYERS 0x17
#define TEXT_LAYOUT 0x6A
#define PAUSE_FIRST 0xFF            /* frames of the first pause (255), the second (16), the third (72) */
#define PAUSE_SECOND 0x10
#define PAUSE_THIRD 0x48
#define PRESS_BUTTONS (PAD_A | PAD_B | PAD_X | PAD_Y | PAD_START)

enum {
    END_START, END_PART_TILES, END_PART_TILES_2, END_PART_TILES_STEP, END_FADE_START, END_FADE,
    END_GROW, END_COLUMNS, END_PAUSE_1, END_PAUSE_2, END_PAUSE_3, END_PRESS, END_FADE_OUT
};

/*
 * @routine 81:F973 TileBlockBlank
 * @routine 81:F958 LevelEndBlanks
 * WHAT IT DOES: Queues 16 blank 2 x 2-tile blocks for video memory, at the
 * addresses of a table (the last first), and has them sent.
 */
static void LevelEndBlanks(void)
{
    for (int i = LEVEL_END_BLANK_BLOCKS - 1; i >= 0; i--) {
        TileBlock *b = &g.tile_blocks[g.tile_blocks_count % TILE_BLOCK_QUEUE_SIZE];

        b->to = level_end_blank_block((unsigned)i);
        for (int k = 0; k < 4; k++)
            b->words[k] = BLANK_TILE;
        g.tile_blocks_count++;
    }
    g.tile_blocks_pending++;
}

/*
 * @routine 81:F9B4 LevelEndColumnPlace
 * WHAT IT DOES: The VRAM address of column `column`'s piece at step `step`
 * (0, 2, 4, 6) of the picture: the step's address for column 0, less 6
 * words a column.
 */
static uint16_t LevelEndColumnPlace(unsigned column, unsigned step)
{
    return (uint16_t)(level_end_step_vram(step / 2) - 6 * column);
}

/*
 * @routine 81:F98C LevelEndColumnTile
 * WHAT IT DOES: The map word of the picture piece column `column` shows for
 * the part, and counts the piece.
 */
static uint16_t LevelEndColumnTile(Task *t, unsigned column)
{
    uint8_t picture = level_end_picture((uint8_t)g.progress_sequence, column);

    t->own.level_end.placed++;
    return (uint16_t)((level_end_picture_tile(picture) << 1) | (uint16_t)level_end_column_attr(column) << 8);
}

/* one frame of the columns: each column the walker has reached draws its
   next piece (steps 0, 2, 4, 6 draw; every step counts on) */
static void columns_frame(Task *t)
{
    static const uint8_t offset_of_step[4] = { 0, 6, 2, 4 };

    for (unsigned column = 0; column < LEVEL_END_COLUMNS; column++) {
        uint8_t *step = &t->own.level_end.step[column];

        if (g.end_walker_x < level_end_column_limit(column))
            continue;
        if (*step < 8 && !(*step & 1)) {
            uint16_t place = LevelEndColumnPlace(column, *step);
            uint16_t tile = LevelEndColumnTile(t, column);

            TileBlockQueueOne(place, (uint16_t)(tile + offset_of_step[*step / 2]));
        }
        ++*step;
    }
}

/* the picture's text, palettes and layers, then the fade-in's start a
   frame later (the original goes on from the tiles with no wait) */
static TaskRun level_end_text(Task *t, uint16_t *at, uint8_t part)
{
    NmiDisable();
    TextLayoutShow(TEXT_LAYOUT);
    for (unsigned i = 0; i < 4; i++)
        PaletteFromRom(level_end_palette(part, i), (uint8_t)i);
    g.bg_x[2] = (g.bg_x[2] & ~0xFF00u) | (uint32_t)BG3_X << 8;
    g.main_layers = SHOW_LAYERS;
    g.sub_layers = SHOW_LAYERS;
    g.palette[0] = 0x1806;
    NmiEnable();
    *at = END_FADE_START;
    return task_wait(t, 1);
}

/*
 * @routine 81:F61D LevelEndTask
 * WHAT IT DOES: The screen between a level's acts: the level-end picture
 * set up (level $24's RAM, its graphics, palettes and text), faded in, its
 * four columns grown as the walker walks, a pause, then the wait for a
 * button; faded out, and on to the story card (or, after part 7, the next
 * part).
 * HOW: The part kept in the third kept option and sealed; the display shut
 * and cleared; the gem sets added to the total; level $24, LevelRamSetup;
 * sets $0E and $23 unpacked with nine copies, copy 2 of $70/7, palette $4D,
 * the first map cleared, the 16 blanks; a frame; palette cycles $20 and
 * $21, the paused mark, set $19, copy $23/7, palette $3D. Part 5: five
 * loose tile blocks over two frames; else the part's block of the picture.
 * The text, the part's four palettes, layer 3 across by $FC, colour 0.
 * Faded in a frame at a time, with the frame's update (FrameStep each
 * frame, as every frame of this screen). Then, while the walker's x is
 * under $E0, each time it passes the next mark ($20, then 16 more) the
 * picture's next block; then the columns each frame while the walker's x
 * (high byte 0 or 1) allows; then 255, 16 and 72 frames; then a frame at a
 * time until a button is pressed; faded out and the screen blanked.
 */
TaskRun LevelEndTask(Task *t)
{
    uint16_t *at = task_enter(t);
    uint8_t part = (uint8_t)g.progress_sequence;

    switch (*at) {
    case END_START:
        g.kept_options[2] = part;
        KeptOptionsSeal();
        NmiDisable();
        ScreenBlank();
        ScrollStateClear();
        OamBufferClear();
        DisplayDefaults();
        t->own.level_end.placed = 0;
        t->own.level_end.asked = 0;
        g.gem_sets_total = (uint8_t)(g.gem_sets_total + g.gem_sets);
        g.level = END_LEVEL;
        LevelRamSetup();
        UnpackSet(0x0E);
        VramCopy(0x7E / 7);
        UnpackSet(0x23);
        VramCopy(0x69 / 7);
        for (int x = 0x93; x <= 0xE7; x += 0x0E)
            VramCopy(x / 7);
        VramCopy2(0x70 / 7);
        PaletteLoad(0x4D);
        VramFillMap(VIDEO_MAP_BACK, 0);
        LevelEndBlanks();
        NmiEnable();
        *at = END_PART_TILES;
        return task_wait(t, 1);
    case END_PART_TILES:
        NmiDisable();
        PaletteCycleStart(0, 0x20);
        PaletteCycleStart(1, 0x21);
        g.paused = 1;
        UnpackSet(0x19);
        VramCopy(0x23 / 7);
        PaletteLoad(0x3D);
        NmiEnable();
        if (part != PART_WITH_TILES) {
            TileBlockQueueRun(0x488F, (uint16_t)(level_end_block(part) + 0x04C0));
            return level_end_text(t, at, part);
        }
        TileBlockQueueOne(0x488C, 0x0435);
        TileBlockQueueOne(0x4886, 0x043B);
        TileBlockQueueOne(0x4888, 0x0454);
        *at = END_PART_TILES_2;
        return task_wait(t, 1);
    case END_PART_TILES_2:
        TileBlockQueueOne(0x488A, 0x0572);
        TileBlockQueueOne(0x4884, 0x0570);
        *at = END_PART_TILES_STEP;
        return task_wait(t, 1);
    case END_PART_TILES_STEP:
        if (FrameStep())
            return task_exec_take(t);
        return level_end_text(t, at, part);
    case END_FADE_START:
        TaskCreate(FADE_SLOT, 2, TASK_ENTRY_BRIGHTNESS_UP);
        *at = END_FADE;
        return task_wait(t, 1);
    case END_FADE:
        if (FrameStep())
            return task_exec_take(t);
        if (task_busy(FADE_SLOT))
            return task_wait(t, 1);
        g.brightness = 0x0F;
        g.blank = 0;
        t->own.level_end.next_at = GROW_START_X;
        t->own.level_end.block = 0;
        *at = END_GROW;
        return task_wait(t, 1);
    case END_GROW:
        if (FrameStep())
            return task_exec_take(t);
        if (g.end_walker_x >> 8)
            return task_wait(t, 1);
        if (g.end_walker_x >= GROW_END_X) {
            g.paused = 0;
            for (unsigned i = 0; i < LEVEL_END_COLUMNS; i++)
                t->own.level_end.step[i] = 0;
            *at = END_COLUMNS;
            return task_wait(t, 1);
        }
        if (g.end_walker_x >= t->own.level_end.next_at) {
            unsigned block = t->own.level_end.block;

            t->own.level_end.placed++;
            t->own.level_end.next_at = (uint8_t)(t->own.level_end.next_at + GROW_BLOCK_EVERY);
            TileBlockQueueRun((uint16_t)(2 * block + 0x4AE4), (uint16_t)(4 * block + 0x0890));
            t->own.level_end.block++;
        }
        return task_wait(t, 1);
    case END_COLUMNS:
        if (FrameStep())
            return task_exec_take(t);
        if ((g.end_walker_x >> 8) == 1)
            return task_wait(t, 1);
        if (g.end_walker_x >> 8) {
            t->own.level_end.count = PAUSE_FIRST;
            *at = END_PAUSE_1;
            return task_wait(t, 1);
        }
        columns_frame(t);
        return task_wait(t, 1);
    case END_PAUSE_1:
    case END_PAUSE_2:
    case END_PAUSE_3:
        if (FrameStep())
            return task_exec_take(t);
        if (--t->own.level_end.count != 0)
            return task_wait(t, 1);
        if (*at == END_PAUSE_1) {
            t->own.level_end.count = PAUSE_SECOND;
        } else if (*at == END_PAUSE_2) {
            t->own.level_end.asked++;
            t->own.level_end.count = PAUSE_THIRD;
        } else {
            *at = END_PRESS;
            return task_wait(t, 1);
        }
        ++*at;
        return task_wait(t, 1);
    case END_PRESS:
        if (FrameStep())
            return task_exec_take(t);
        if (!(g.pad_pressed[0] & PRESS_BUTTONS))
            return task_wait(t, 1);
        TaskCreate(FADE_SLOT, 1, TASK_ENTRY_BRIGHTNESS_DOWN);
        *at = END_FADE_OUT;
        return task_wait(t, 1);
    case END_FADE_OUT:
        if (task_busy(FADE_SLOT))
            return task_wait(t, 1);
        g.brightness = 0;
        g.blank = 1;
        g.gem_sets = 0;
        g.gem_sets_found = 0;
        return task_exec(t, part == LAST_PART ? TASK_ENTRY_NEXT_PART : TASK_ENTRY_STORY_CARD);
    }
    return task_exit(t);
}
