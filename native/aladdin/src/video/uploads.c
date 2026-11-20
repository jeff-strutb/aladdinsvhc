/*
 * uploads.c: the uploads the NMI made each frame besides the queue, the
 * sprites and the palette (video.c): the value sequences' lists, the single
 * words, the text map's parts and the uploads by number.
 */
#include <stdio.h>

#include "data/data.h"
#include "data/video_tables.h"
#include "system/state.h"
#include "video/video.h"
#include "video/video_internal.h"

/* the text map as the original held it, low byte first */
static uint8_t text_bytes[2 * 32 * 32];

static const uint8_t *text_map_bytes(void)
{
    for (unsigned i = 0; i < 32 * 32; i++) {
        text_bytes[2 * i] = (uint8_t)g.text_map[i];
        text_bytes[2 * i + 1] = (uint8_t)(g.text_map[i] >> 8);
    }
    return text_bytes;
}

/* an upload's bytes; NULL (reported) when `bytes` from there run past its array */
static const uint8_t *source_bytes(UploadSource s, uint32_t bytes)
{
    switch (s.from) {
    case FROM_UNPACKED:
        if (s.at + bytes <= sizeof g.unpacked)
            return &g.unpacked[s.at];
        break;
    case FROM_TEXT_MAP:
        if (s.at + bytes <= sizeof text_bytes)
            return text_map_bytes() + s.at;
        break;
    default:
        return s.cart;
    }
    fprintf(stderr, "video: an upload of %u bytes at %04X runs past its source; left out\n", bytes, s.at);
    return NULL;
}

/*
 * @routine 80:8899 VramListUpload
 * WHAT IT DOES: When value sequence `slot` has a new word, makes the list of
 * uploads it names.
 * HOW: Its new-word byte cleared; each record of the list in turn, while a
 * record says another follows.
 */
static void list_upload(int slot)
{
    if (!g.vram_list_pending[slot])
        return;
    g.vram_list_pending[slot] = 0;
    for (unsigned i = 0;; i++) {
        VramListRecord r = vram_list_record(g.vram_list[slot], i);
        const uint8_t *from = source_bytes(r.source, r.bytes);

        if (from)
            vram_write(r.to, from, r.bytes / 2u, VRAM_WORDS);
        if (!r.more)
            break;
    }
}

/*
 * @routine 80:8883 VramListsUpload
 * WHAT IT DOES: Makes the lists of the ten value sequences that have a new
 * word, last first, unless uploads are held this frame.
 * HOW: Held while g.uploads_held or the byte before the play field's layer
 * record ($1737) is nonzero. No instruction of the record names $1737 but
 * this read, so it has no field (any write to it would be an indexed one
 * not yet found; listed in the report).
 */
void VramListsUpload(void)
{
    if (g.uploads_held)
        return;
    for (int slot = 9; slot >= 0; slot--)
        list_upload(slot);
}

/*
 * @routine 80:884C VramWordsUpload
 * WHAT IT DOES: Writes the list of single words, when one is waiting: each
 * word to its own video address.
 */
void VramWordsUpload(void)
{
    if (!g.vram_words_pending)
        return;
    g.vram_words_pending = 0;
    for (unsigned i = 0; i < g.vram_words_count && i < VRAM_WORDS_SIZE; i++) {
        uint8_t w[2] = { (uint8_t)g.vram_words[i].word, (uint8_t)(g.vram_words[i].word >> 8) };

        vram_write(g.vram_words[i].to, w, 1, VRAM_WORDS);
    }
}

/*
 * @routine 80:84AA TextBufferPartUpload
 * WHAT IT DOES: Sends part `part` of the text map when it was asked for.
 */
static void text_part_upload(unsigned part)
{
    TextPart p;

    if (!g.text_parts_pending[part])
        return;
    g.text_parts_pending[part] = 0;
    p = text_part(part);
    if (p.from + (uint32_t)p.bytes <= sizeof text_bytes)
        vram_write(p.to, text_map_bytes() + p.from, p.bytes / 2u, VRAM_WORDS);
}

/*
 * @routine 80:849E TextBufferPartsUpload
 * WHAT IT DOES: Sends whichever of the text map's three parts were asked
 * for, in order.
 */
void TextBufferPartsUpload(void)
{
    for (unsigned part = 0; part < 3; part++)
        text_part_upload(part);
}

/*
 * @routine 80:84E2 VramUploadByNumber
 * WHAT IT DOES: Makes the fixed upload the game asked for by number, if
 * any: from the unpack buffer or the text map, whole words, or the low or
 * high bytes alone (Mode 7's map and pixels).
 */
void VramUploadByNumber(void)
{
    static const VramWrite how[3] = { VRAM_WORDS, VRAM_LOW_BYTES, VRAM_HIGH_BYTES };
    NumberedUpload u;
    const uint8_t *from;

    if (!g.vram_upload_number)
        return;
    if (g.vram_upload_number > NUMBERED_UPLOADS) {
        fprintf(stderr, "video: upload number %u is not in the table\n", g.vram_upload_number);
        g.vram_upload_number = 0;
        return;
    }
    u = numbered_upload(g.vram_upload_number);
    g.vram_upload_number = 0;
    if ((from = source_bytes(u.source, u.bytes)) != NULL)
        vram_write(u.to, from, u.how == UPLOAD_WORDS ? u.bytes / 2u : u.bytes, how[u.how]);
}
