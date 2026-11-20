/*
 * text.c: drawing the text layouts into the text layer's map (g.text_map),
 * and having it, or a part of it, sent.
 */
#include <stdio.h>

#include "data/video_tables.h"
#include "system/state.h"
#include "video/video.h"

#define TEXT_WORDS (32 * 32)
#define BLANK 0x0020

/* one map word at byte offset `at` (the original's index), kept to the map */
static void put(uint16_t at, uint16_t word)
{
    if (at / 2u >= TEXT_WORDS) {
        static int told;

        if (!told) {
            told = 1;
            fprintf(stderr, "video: a text layout reaches past the text map (offset %04X); left out\n", at);
        }
        return;
    }
    g.text_map[at / 2u] = word;
}

/*
 * @routine 81:9734 TextLayoutDraw
 * WHAT IT DOES: Draws text layout `layout` (its low 7 bits) into the text
 * layer's map, or blanks the places it covers (bit 7 set).
 * HOW: Each run until a 0 byte: its count, its place, its words or one word
 * repeated (TextLayerCopyWords 81:9774 and TextLayerBlankWords 81:9787,
 * folded in: a blank run passes over the words as a copy would).
 */
static void layout_draw(uint8_t layout)
{
    DataRef at = text_layout(layout);
    unsigned i = 0;
    uint8_t run;

    while ((run = text_layout_byte(at, i)) != 0) {
        unsigned count = run & 0x3F, repeat = run & 0x40;
        uint16_t place = text_layout_word(at, i + 1);

        i += 3;
        for (unsigned k = 0; k < count; k++) {
            put((uint16_t)(place + 2 * k), layout & 0x80 ? BLANK : text_layout_word(at, i));
            if (!repeat)
                i += 2;
        }
        if (repeat)
            i += 2;
        /* a count of 0 is the original's 65,536 words; no layout has one (reported) */
        if (count == 0)
            fprintf(stderr, "video: text layout %02X has a run of 0 words\n", layout);
    }
}

/*
 * @routine 81:972D TextLayoutShow
 * WHAT IT DOES: Draws text layout `layout` (or blanks it, bit 7 set) and
 * has the text map sent at the frame's end.
 */
void TextLayoutShow(int layout)
{
    layout_draw((uint8_t)layout);
    g.text_map_upload++;
}

/*
 * @routine 81:9718 TextLayoutShowFirst
 * WHAT IT DOES: Draws text layout `layout` and has the first part of the
 * text map sent.
 */
void TextLayoutShowFirst(uint8_t layout)
{
    layout_draw(layout);
    g.text_parts_pending[0]++;
}

/*
 * @routine 81:971F TextLayoutShowSecond
 * WHAT IT DOES: As TextLayoutShowFirst, the second part.
 */
void TextLayoutShowSecond(uint8_t layout)
{
    layout_draw(layout);
    g.text_parts_pending[1]++;
}

/*
 * @routine 81:9726 TextLayoutShowThird
 * WHAT IT DOES: As TextLayoutShowFirst, the third part.
 */
void TextLayoutShowThird(uint8_t layout)
{
    layout_draw(layout);
    g.text_parts_pending[2]++;
}

/*
 * @routine 81:979A TextLayoutDrawBig
 * WHAT IT DOES: Draws text layout `layout`'s first run as large characters,
 * each four tiles (t, t + 1 in a row, t + 2, t + 3 in the row below, all
 * with the layout's high byte), each character two words right of the
 * last; has the text map sent. A count of 0 draws nothing.
 * HOW: The record says nothing calls it; the cartridge has a JSL to it at
 * 81:8475 (X = $40), in code the record has no routine for.
 */
void TextLayoutDrawBig(uint8_t layout)
{
    DataRef at = text_layout(layout);
    uint8_t first = text_layout_byte(at, 0);

    if (first) {
        unsigned count = first & 0x3F;
        uint16_t place = text_layout_word(at, 1);
        uint16_t high = (uint16_t)(text_layout_byte(at, 3) << 8);

        for (unsigned c = 0; c < count; c++) {
            uint16_t tile = text_layout_word(at, 4 + 2 * c);
            uint16_t p = (uint16_t)(place + 4 * c);

            put(p, (uint16_t)(tile | high));
            put((uint16_t)(p + 2), (uint16_t)((tile + 1) | high));
            put((uint16_t)(p + 0x40), (uint16_t)((tile + 2) | high));
            put((uint16_t)(p + 0x42), (uint16_t)((tile + 3) | high));
        }
    }
    g.text_map_upload++;
}
