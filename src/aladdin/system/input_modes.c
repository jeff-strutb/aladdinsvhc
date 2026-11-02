/*
 * input_modes.c: where a frame's buttons come from: the pads, the demo's
 * script (the title's attract play), or the pads with a recorder that
 * writes them into cartridge RAM (a development mode retail never enters).
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "wram_pad.h"

void abi_JoypadRead(void);
void abi_ControlsRead(void);
void abi_InputRecord(void);

/*
 * @routine 81:BDB1 InputRead
 * WHAT IT DOES: Takes this frame's buttons and turns them into the game's
 * actions: from the pads, or in the demo from its script (only the real
 * pad's START kept, so a player can end the demo), or from the pads while
 * the recorder writes them down.
 * HOW: Input mode 2 (the demo): reads the pads, clears every held and
 * pressed byte, makes pad 1's high byte the real START plus the script's
 * buttons and its pressed byte the ones not held last frame, and steps the
 * script when its frame count runs out. Otherwise reads the pads, and in
 * mode 1 calls InputRecord. Both end in ControlsRead.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
 * whose $4212-$421B are the CPU's registers; called by JSR.
 * Exit: as ControlsRead; X and Y unchanged; M 8-bit, X 8-bit.
 */
void abi_InputRead(void)
{
    goto L_81BDB1;
L_81BD53:
    phx8();  /* 81BD53 phx */
    phy8();  /* 81BD54 phy */
    lda8(ea_abs((JOY_HELD + 1)));  /* 81BD55 lda */
    pha8();  /* 81BD58 pha */
    abi_JoypadRead();  /* 81BD59 jsr */
    stz8(ea_abs(JOY_HELD));  /* 81BD5C stz */
    stz8(ea_abs(JOY_PRESSED));  /* 81BD5F stz */
    stz8(ea_abs((JOY_HELD + 2)));  /* 81BD62 stz */
    stz8(ea_abs((JOY_PRESSED + 2)));  /* 81BD65 stz */
    stz8(ea_abs((JOY_HELD + 3)));  /* 81BD68 stz */
    stz8(ea_abs((JOY_PRESSED + 3)));  /* 81BD6B stz */
    pla8();  /* 81BD6E pla */
    eor8_v(0xFF);  /* 81BD6F eor */
    sta8(ea_abs((JOY_PRESSED + 1)));  /* 81BD71 sta */
    lda8(ea_abs((JOY_HELD + 1)));  /* 81BD74 lda */
    and8_v(0x10);  /* 81BD77 and */
    ora8_v(rd8(ea_abs(DEMO_BUTTONS)));  /* 81BD79 ora */
    sta8(ea_abs((JOY_HELD + 1)));  /* 81BD7C sta */
    and8_v(rd8(ea_abs((JOY_PRESSED + 1))));  /* 81BD7F and */
    sta8(ea_abs((JOY_PRESSED + 1)));  /* 81BD82 sta */
    MEM8(dec, ea_abs(DEMO_FRAMES));  /* 81BD85 dec */
    if (!IF_Z) goto L_81BDA7;  /* 81BD88 bne */
    op_rep(0x10);  /* 81BD8A rep */
    ldx16(ea_abs(DEMO_POS));  /* 81BD8C ldx */
    MEM8(inc, ea_abs(DEMO_POS));  /* 81BD8F inc */
    MEM8(inc, ea_abs(DEMO_POS));  /* 81BD92 inc */
    lda8(ea_longx(DEMO_SCRIPT));  /* 81BD95 lda */
    and8_v(0xEF);  /* 81BD99 and */
    sta8(ea_abs(DEMO_BUTTONS));  /* 81BD9B sta */
    lda8(ea_longx((DEMO_SCRIPT + 1)));  /* 81BD9E lda */
    sta8(ea_abs(DEMO_FRAMES));  /* 81BDA2 sta */
    op_sep(0x10);  /* 81BDA5 sep */
L_81BDA7:
    abi_ControlsRead();  /* 81BDA7 jsr */
    ply8();  /* 81BDAA ply */
    plx8();  /* 81BDAB plx */
    return;  /* 81BDAC rts */
L_81BDB1:
    lda8(ea_abs(INPUT_MODE));  /* 81BDB1 lda */
    cmp8_v(0x02);  /* 81BDB4 cmp */
    if (IF_Z) goto L_81BD53;  /* 81BDB6 beq */
    phx8();  /* 81BDB8 phx */
    phy8();  /* 81BDB9 phy */
    abi_JoypadRead();  /* 81BDBA jsr */
    lda8(ea_abs(INPUT_MODE));  /* 81BDBD lda */
    ACC8(dec);  /* 81BDC0 dec */
    if (!IF_Z) goto L_81BDC6;  /* 81BDC1 bne */
    abi_InputRecord();  /* 81BDC3 jsr */
L_81BDC6:
    abi_ControlsRead();  /* 81BDC6 jsr */
    ply8();  /* 81BDC9 ply */
    plx8();  /* 81BDCA plx */
    return;  /* 81BDCB rts */
}

/*
 * @routine 81:BE5A InputRecord
 * WHAT IT DOES: The development recorder: writes pad 1's buttons into
 * cartridge RAM as runs (the buttons and how many frames they were held),
 * until its buffer is full; then holds the game until pad 2's DOWN and
 * starts over. This board has no cartridge RAM: the writes go nowhere and
 * each read is the bus's last byte ($70).
 * HOW: By its step, through its table: step 0 clears the buffer's first
 * 256 bytes, its offset and pad 1's held buttons, and goes on to step 1;
 * step 1 compares the buttons with the run at the offset: the same, the
 * run's frame count goes up; else, or when the count would wrap past $FF,
 * it starts the next run, 2 bytes on (its buttons, its count up by one),
 * and at offset $FE keeps the offset, waits for pad 2's DOWN (JoypadRead
 * in a loop) and sets the step back to 0. The wrap ($81:BE97) needs a
 * read of $FF at $70:0001 up: on this board the read is $70, so it never
 * runs, and no script covers it.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSR (InputRead, input mode 1).
 * Exit: M 8-bit, X 8-bit; A, X, Y, flags as its last step left them.
 */
void abi_InputRecord(void)
{
    uint16_t to;

    lda8(ea_abs(RECORD_STEP));  /* 81BE5A lda */
    ACC8(asl);  /* 81BE5D asl */
    tax8();  /* 81BE5E tax */
    to = rd16(0x810000u | (uint16_t)(0xBE62 + cpu.x));  /* 81BE5F jmp (BE62,x) */
    if (to == 0xBE66) goto L_81BE66;
    if (to == 0xBE7F) goto L_81BE7F;
    jump_routine(0x810000u | to); return;
L_81BE66:
    ldx8_v(0x00);  /* 81BE66 ldx */
    txa8();  /* 81BE68 txa */
L_81BE69:
    sta8(ea_longx(RECORD_BUF));  /* 81BE69 sta */
    dex8();  /* 81BE6D dex */
    if (!IF_Z) goto L_81BE69;  /* 81BE6E bne */
    stz8(ea_abs(RECORD_POS));  /* 81BE70 stz */
    stz8(ea_abs((RECORD_POS + 1)));  /* 81BE73 stz */
    stz8(ea_abs(JOY_HELD));  /* 81BE76 stz */
    stz8(ea_abs((JOY_HELD + 1)));  /* 81BE79 stz */
    MEM8(inc, ea_abs(RECORD_STEP));  /* 81BE7C inc */
L_81BE7F:
    ldx8(ea_abs(RECORD_POS));  /* 81BE7F ldx */
    lda8(ea_abs((JOY_HELD + 1)));  /* 81BE82 lda */
    cmp8_v(rd8(ea_longx(RECORD_BUF)));  /* 81BE85 cmp */
    if (!IF_Z) goto L_81BE9D;  /* 81BE89 bne */
    tay8();  /* 81BE8B tay */
    lda8(ea_longx((RECORD_BUF + 1)));  /* 81BE8C lda */
    ACC8(inc);  /* 81BE90 inc */
    sta8(ea_longx((RECORD_BUF + 1)));  /* 81BE91 sta */
    if (!IF_Z) goto L_81BEB3;  /* 81BE95 bne */
    ACC8(dec);  /* 81BE97 dec */
    sta8(ea_longx((RECORD_BUF + 1)));  /* 81BE98 sta */
    tya8();  /* 81BE9C tya */
L_81BE9D:
    sta8(ea_longx((RECORD_BUF + 2)));  /* 81BE9D sta */
    lda8(ea_longx((RECORD_BUF + 3)));  /* 81BEA1 lda */
    ACC8(inc);  /* 81BEA5 inc */
    sta8(ea_longx((RECORD_BUF + 3)));  /* 81BEA6 sta */
    inx8();  /* 81BEAA inx */
    inx8();  /* 81BEAB inx */
    cpx8_v(0xFE);  /* 81BEAC cpx */
    if (IF_Z) goto L_81BEB4;  /* 81BEAE beq */
    stx8(ea_abs(RECORD_POS));  /* 81BEB0 stx */
L_81BEB3:
    return;  /* 81BEB3 rts */
L_81BEB4:
    stx8(ea_abs(RECORD_END));  /* 81BEB4 stx */
L_81BEB7:
    abi_JoypadRead();  /* 81BEB7 jsr */
    lda8(ea_abs((JOY_HELD + 3)));  /* 81BEBA lda */
    bit8_i(0x04);  /* 81BEBD bit */
    if (IF_Z) goto L_81BEB7;  /* 81BEBF beq */
    stz8(ea_abs(RECORD_STEP));  /* 81BEC1 stz */
    return;  /* 81BEC4 rts */
}
