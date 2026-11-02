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
    lda8_v(0x80);
    sta8(ea_dp(SEQ_STARTED));
    sta8(ea_dp(SEQ_INDEX));
    lda8_v(0x01);
    sta8(ea_dp(SEQ_WAIT));
    ldx8(ea_dp(SEQ_ID));
    lda8(ea_absx(SEQUENCE_TABLE));
    sta8(ea_dp(SEQ_LENGTH));
    op_rep(0x20);
    lda16(ea_absx(SEQUENCE_TABLE + 1));
    sta16(ea_dp(SEQ_LIST));
    op_sep(0x20);
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
    lda8_v(SEQUENCES >> 8);
    xba();
    lda8_v(SEQUENCES & 0xFF);
    tcd();
    do {
        lda8(ea_dp(SEQ_ACTIVE));
        if (IF_Z)
            goto next;
        lda8(ea_dp(SEQ_STARTED));
        if (IF_Z)
            abi_SequenceStart();
        MEM8(dec, ea_dp(SEQ_WAIT));
        if (!IF_Z)
            goto next;
        lda8(ea_dp(SEQ_INDEX));
        clc();
        adc8_v(0x03);
        cmp8_v(rd8(ea_dp(SEQ_LENGTH)));
        if (IF_C)
            lda8_v(0x00);
        sta8(ea_dp(SEQ_INDEX));
        tay8();
        lda8(ea_dpiy(SEQ_LIST));
        sta8(ea_dp(SEQ_WAIT));
        if (IF_N) {
            stz8(ea_dp(SEQ_ACTIVE));
            goto next;
        }
        iny8();
        op_rep(0x20);
        lda16(ea_dpiy(SEQ_LIST));
        sta16(ea_dp(SEQ_WORD));
        op_sep(0x20);
        lda8_v(0x02);
        sta8(ea_dp(SEQ_NEW));
next:
        op_rep(0x20);
        tdc();
        clc();
        adc16_v(SEQUENCE_SIZE);
        tcd();
        cmp16_v(SEQUENCES_END);
        op_sep(0x20);
    } while (!IF_Z);
    pld();
}
