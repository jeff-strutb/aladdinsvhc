/*
 * sequences.c: the level's value sequences: ten records of 16 bytes from
 * $1645, each holding a word for some frames, then the next of its list.
 *
 * A record, through the direct page (D = the record): $00 nonzero while it
 * runs; $01 its number (an index into the table at $CA17 of its bank); $02
 * its list's length in bytes and $03 the index of its entry; $04 = 2 when
 * a new word is out; $05 the frames left of the word; $06 its list's
 * address; $08 the word; $0C nonzero once started. An entry: the frames
 * (byte, negative ends the list), then the word.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"

#define SEQUENCES 0x1645u
#define SEQUENCES_END 0x16E5u
#define SEQUENCE_SIZE 0x10
#define SEQ_ACTIVE 0x00
#define SEQ_ID 0x01
#define SEQ_LENGTH 0x02
#define SEQ_INDEX 0x03
#define SEQ_NEW 0x04
#define SEQ_WAIT 0x05
#define SEQ_LIST 0x06
#define SEQ_WORD 0x08
#define SEQ_STARTED 0x0C
#define SEQUENCE_TABLE 0xCA17u      /* by number: the length (byte), the list's address (word) */

/*
 * @routine 81:EC6F SequenceStart
 * WHAT IT DOES: Starts a value sequence: its list and length from the
 * table at $CA17 by its number, a first step on the next frame.
 * HOW: Its started byte and index $80, its wait 1; its number ($01) indexes
 * the table: the length byte, then the list's address (word).
 * Entry: M 8-bit, X 8-bit; D = the record; DB the table's bank; called by
 * JSR from SequencesStep. Exit: M 8-bit; A = the list's address (16 bits);
 * X = the number; N, Z from the address; C, V, Y unchanged.
 */
void abi_SequenceStart(void)
{
    uint8_t length;
    uint16_t list;

    a8_load(0x80);
    wr8(dpa(SEQ_STARTED), 0x80);
    wr8(dpa(SEQ_INDEX), 0x80);
    a8_load(0x01);
    wr8(dpa(SEQ_WAIT), 0x01);
    x8_load(rd8(dpa(SEQ_ID)));
    length = rd8(dba(SEQUENCE_TABLE) + cpu.x);
    a8_load(length);
    wr8(dpa(SEQ_LENGTH), length);
    list = rd16(dba(SEQUENCE_TABLE + 1) + cpu.x);
    dp16_set(SEQ_LIST, list);
    a16_load(list);
}

/*
 * @routine 81:EC23 SequencesStep
 * WHAT IT DOES: Steps every running value sequence, once a frame: each
 * holds a word for a number of frames, then the next word of its list.
 * HOW: With D at each of the ten records in turn, one with its active byte
 * nonzero is started (SequenceStart) if not yet; its wait counted down; at
 * 0 its index steps 3 (back to 0 at its length) and the entry there gives
 * the wait and the word; a negative wait ends the sequence (active byte
 * cleared). $04 = 2 marks a new word.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM, the
 * sequences' bank; called by JSR. Exit: M 8-bit; D unchanged; N, Z from it
 * (PLD); A = $16E5 (16 bits); C set; X as the last start leaves it; Y the
 * last entry's index plus 1, or as on entry.
 */
void abi_SequencesStep(void)
{
    phd();
    cpu.a = cpu.d = SEQUENCES;
    nz16_of(SEQUENCES);
    do {                                  /* each sequence record */
        uint8_t active = rd8(dpa(SEQ_ACTIVE));

        a8_load(active);
        if (active) {
            a8_load(rd8(dpa(SEQ_STARTED)));
            if (!(uint8_t)cpu.a)
                abi_SequenceStart();
            if (!dec8_at(dpa(SEQ_WAIT))) {
                /* its wait done: the next entry (3 bytes: frames, word),
                   from the first after the last */
                uint32_t list = dba(dp16(SEQ_LIST));
                uint8_t at, wait;

                flag_set(P_C, 0);
                at = op_adc8(rd8(dpa(SEQ_INDEX)), 0x03, &cpu.p);
                a8_load(at);
                cmp8_of(at, rd8(dpa(SEQ_LENGTH)));
                if (at >= rd8(dpa(SEQ_LENGTH))) {
                    at = 0;
                    a8_load(0);
                }
                wr8(dpa(SEQ_INDEX), at);
                cpu.y = at;
                wait = rd8(list + at);
                a8_load(wait);
                wr8(dpa(SEQ_WAIT), wait);
                if (wait & 0x80) {        /* the end: it stops */
                    wr8(dpa(SEQ_ACTIVE), 0);
                } else {
                    cpu.y = (uint8_t)(at + 1);
                    dp16_set(SEQ_WORD, rd16(list + cpu.y));
                    a8_load(0x02);
                    wr8(dpa(SEQ_NEW), 0x02);
                }
            }
        }
        op_rep(P_M);
        flag_set(P_C, 0);
        cpu.a = op_adc16(cpu.d, SEQUENCE_SIZE, &cpu.p);
        cpu.d = cpu.a;
        cmp16_of(cpu.a, SEQUENCES_END);
        op_sep(P_M);
    } while (cpu.d != SEQUENCES_END);
    pld();
}
