/*
 * sequences.c: the level's value sequences (g.sequences): ten records,
 * each holding a word for some frames, then the next of its list. The word
 * names a VRAM list the NMI's upload makes (video/).
 */
#include "data/game_tables.h"
#include "level/level.h"
#include "system/state.h"

#define SEQUENCE_ENTRY_BYTES 3  /* the frames, then the word */
#define SEQUENCE_ENDS 0x80      /* the frames' bit 7: the list ends there */
#define NEW_WORD 2

/*
 * @routine 81:EC6F SequenceStart
 * WHAT IT DOES: Starts a value sequence: its list and length from the
 * sequences' table by its number, a first step on the next frame.
 * HOW: Its started byte and index $80 (so its first step wraps the index to
 * the list's first entry), its wait 1.
 */
void SequenceStart(Sequence *s)
{
    SequenceList l = sequence_list(s->number);

    s->started = 0x80;
    s->index = 0x80;
    s->wait = 1;
    s->length = l.length;
    s->list = l.list;
}

/*
 * @routine 81:EC23 SequencesStep
 * WHAT IT DOES: Steps every running value sequence, once a frame: each
 * holds a word for a number of frames, then the next word of its list.
 * HOW: Each of the ten that runs is started if not yet; its wait counted
 * down; at 0 its index steps 3 (back to 0 at its length, in 8 bits as the
 * original) and the entry there gives the wait and the word; a wait with
 * bit 7 set ends the sequence. NEW_WORD marks a new word for the upload.
 */
void SequencesStep(void)
{
    unsigned n;

    for (n = 0; n < SEQUENCES; n++) {
        Sequence *s = &g.sequences[n];
        SequenceEntry e;
        uint8_t index;

        if (!s->running)
            continue;
        if (!s->started)
            SequenceStart(s);
        if (--s->wait != 0)
            continue;
        index = (uint8_t)(s->index + SEQUENCE_ENTRY_BYTES);
        if (index >= s->length)
            index = 0;
        s->index = index;
        e = sequence_entry(s->list, index);
        s->wait = e.frames;
        if (e.frames & SEQUENCE_ENDS) {
            s->running = 0;
            continue;
        }
        g.vram_list[n] = e.vram_list;
        g.vram_list_pending[n] = NEW_WORD;
    }
}
