/*
 * tilemap.c: the text layer's map (g.text_map) and plain map fills.
 * Setting g.text_map_upload has the map sent at the frame's end.
 */
#include "system/state.h"
#include "video/video.h"

/*
 * @routine 81:95B8 TextLayerFill
 * WHAT IT DOES: Fills the whole text layer's map with one map word.
 */
void TextLayerFill(uint16_t entry)
{
    for (int i = 0; i < 32 * 32; i++)
        g.text_map[i] = entry;
}

/*
 * @routine 81:95C8 TextLayerClear
 * WHAT IT DOES: Blanks the text layer (tile $20, the blank) and has it
 * sent.
 */
void TextLayerClear(void)
{
    TextLayerFill(0x0020);
    g.text_map_upload++;
}

/*
 * @routine 81:9607 TextLayerClearFront
 * WHAT IT DOES: Blanks the text layer with blank tiles in front of the
 * other layers (tile $20, priority set), and has it sent.
 */
void TextLayerClearFront(void)
{
    TextLayerFill(0x2020);
    g.text_map_upload++;
}

/*
 * @routine 81:95F5 VramFillMap
 * WHAT IT DOES: Fills a map's first 32 x 32 page with one map word (the
 * record's callers name $4000 and $4800: the backdrop's and the play
 * field's).
 */
void VramFillMap(VideoMap map, uint16_t entry)
{
    static const uint16_t first_word[VIDEO_MAP_KINDS] = { 0x4000, 0x4800, 0x5C00 };

    vram_fill(first_word[map], entry, 32 * 32);
}
