/*
 * text_bands.c: a screen of text shown band by band (the ending's text): a
 * page of lines written to the text tilemap, and the bands' window bytes
 * handed to the per-line channel 5 a frame at a time.
 */
#include "data/video_tables.h"
#include "system/state.h"
#include "system/task.h"
#include "video/video.h"
#include "video/video_tasks.h"

#define TEXT_BANDS_CHANNEL 5
#define BAND_PAIRS 4                /* the bands are set two at a time, over 4 steps */
#define BAND_STEP_FRAMES 4
#define BAND_SHOWN 0x00             /* a band's window byte: shown */
#define BAND_MASKED 0x08            /* masked */
#define CONCEAL_WAIT_FRAMES 40

/*
 * @routine 82:B270 TextBandsCommit
 * WHAT IT DOES: Hands the 16 window bytes of the text bands to channel 5,
 * through whichever of its two buffers is not in use.
 * HOW: By bit 0 of the task's flip: clear, the bytes go to the first buffer
 * and the channel's table is the one pointing at it (its rows $80:CEEF);
 * set, to the second and $80:CF3B. The flip steps up.
 */
void TextBandsCommit(Task *t)
{
    unsigned buffer = t->own.text_bands.flip & 1;

    for (unsigned i = 0; i < sizeof g.text_bands; i++)
        g.text_band_values[16 * buffer + i] = g.text_bands[i];
    g.line_channel[TEXT_BANDS_CHANNEL].cart = text_band_table(buffer);
    t->own.text_bands.flip++;
}

/*
 * @routine 82:B190 TextPageDraw
 * WHAT IT DOES: Writes a page of text into the text tilemap, each line at
 * the next place from the list, with the line's attribute byte, then has
 * the NMI upload the tilemap.
 * HOW: The page's first byte is its line count. Per line: the list's next
 * word is its tilemap offset; the line's attribute byte and length, then
 * its characters, each the low byte of a tilemap word whose high byte is
 * the attribute (a length of 0 is 256).
 */
void TextPageDraw(DataRef page, DataRef *places)
{
    unsigned lines = text_page_begin(&page);

    do {
        TextPageLine l = text_page_line(&page, places);
        unsigned word = l.at / 2u, n = l.length ? l.length : 256u;

        for (unsigned i = 0; i < n; i++, word++)
            if (word < 32 * 32)
                g.text_map[word] = (uint16_t)(l.chars[i] | l.attribute << 8);
    } while (--lines & 0xFF);
    g.text_map_upload++;
}

/* the two bands of pair `step` set to `value`, committed, then 4 frames */
static TaskRun band_pair(Task *t, uint8_t value)
{
    g.text_bands[text_band_row_a(t->own.text_bands.step)] = value;
    g.text_bands[text_band_row_b(t->own.text_bands.step)] = value;
    TextBandsCommit(t);
    return task_wait(t, BAND_STEP_FRAMES);
}

/*
 * @routine 82:B224 TextBandsReveal
 * WHAT IT DOES: Shows the text band by band from the middle out: a pair
 * of bands every 4 frames, over 4 steps. A waiting routine: TASK_GOES_ON
 * (the caller calls again at the same point) until it has finished.
 */
TaskRun TextBandsReveal(Task *t)
{
    uint16_t *at = task_enter(t);

    switch (*at) {
    case 0:
        t->own.text_bands.step = 0;
        *at = 1;
        return band_pair(t, BAND_SHOWN);
    case 1:
        if (++t->own.text_bands.step != BAND_PAIRS)
            return band_pair(t, BAND_SHOWN);
        break;
    }
    return task_return(t);
}

/*
 * @routine 82:B248 TextBandsConceal
 * WHAT IT DOES: Masks the text band by band from the outside in, a pair
 * of bands every 4 frames, then waits 40 frames. A waiting routine as
 * TextBandsReveal.
 */
TaskRun TextBandsConceal(Task *t)
{
    uint16_t *at = task_enter(t);

    switch (*at) {
    case 0:
        t->own.text_bands.step = BAND_PAIRS - 1;
        *at = 1;
        return band_pair(t, BAND_MASKED);
    case 1:
        if (t->own.text_bands.step--)
            return band_pair(t, BAND_MASKED);
        *at = 2;
        return task_wait(t, CONCEAL_WAIT_FRAMES);
    }
    return task_return(t);
}
