/*
 * vram_copy.c: copying a block of the unpack buffer to video memory, from
 * an entry of a table of copies.
 */
#include "data/video_tables.h"
#include "system/state.h"
#include "video/video.h"

static void copy_entry(unsigned table, int entry)
{
    VramCopyEntry e = vram_copy_entry(table, entry);

    vram_write(e.to, &g.unpacked[e.from], e.words, VRAM_WORDS);
}

/*
 * @routine 81:F5DC VramCopy
 * WHAT IT DOES: Copies the block of words entry `entry` of the copy table
 * names from the unpack buffer to video memory.
 * HOW: The entry gives the source, the video address and the count of
 * words; the original sends them through the video port, the address
 * stepping a word at a time.
 */
void VramCopy(int entry)
{
    copy_entry(0, entry);
}

/*
 * @routine 81:F60D VramCopy2
 * WHAT IT DOES: VramCopy for an entry of the second copy table.
 * HOW: The original adds $103 to the entry's offset: the table 37 entries
 * on.
 */
void VramCopy2(int entry)
{
    copy_entry(1, entry);
}
