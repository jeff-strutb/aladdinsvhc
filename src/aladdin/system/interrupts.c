/*
 * interrupts.c: the interrupt vectors' code and the handlers of the NMI
 * (each frame's vertical blank: the uploads and the PPU's registers from
 * their shadows, the waiting tasks counted down) and of the IRQ (the raster
 * interrupt that turns the display on below the status bar).
 * They are entered with the interrupt's frame on the stack (@interrupt):
 * a handler's RTI pulls it (rti()), and the call returns where the frame
 * says.
 */
#include "machine.h"
#include "native.h"
#include "ops65.h"
#include "hw.h"
#include "wram.h"
#include "wram_state.h"

void abi_ActorYieldEntry(void);
void abi_ActorWaitEntry(void);
void abi_LayersCopyToVram(void);
void abi_Mode7MatrixWrite(void);
void abi_NmiDisable(void);
void abi_NmiEnable(void);
void abi_NmiHandler(void);
void abi_IrqHandler(void);
void abi_PaletteUploadFrom(void);
void abi_ScrollRegistersWrite(void);
void abi_SoundCommandSend(void);
void abi_TextBufferPartsUpload(void);
void abi_TextBufferUpload(void);
void abi_TileBlockQueueFlush(void);
void abi_UploadBlockA(void);
void abi_UploadBlockB(void);
void abi_UploadCharsets(void);
void abi_UploadStepped(void);
void abi_VramListsUpload(void);
void abi_VramQueueFlush(void);
void abi_VramUploadByNumber(void);
void abi_VramWordsUpload(void);
void abi_WramMovesRun(void);

/* A JSL at `site`: its return address on the stack while the callee runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

#define V_0008 0x0008u              /* cleared at each NMI; purpose not yet established */
#define V_032A 0x032Au              /* counted up at each NMI (a second frame count) */
#define V_0839 0x0839u              /* nonzero: the NMI leaves the HDMA tables' addresses as they are */
#define V_082A 0x082Au              /* nonzero: the NMI skips the layer and VRAM-list uploads */
#define V_1C7D 0x1C7Du              /* nonzero: the IRQ line is 8 (else from the V counter) */
#define BG1SC_SHADOW 0x03A0u
#define BG2SC_SHADOW 0x03A1u
#define BG3SC_SHADOW 0x03A2u
#define BG12NBA_SHADOW 0x03A4u
#define BG34NBA_SHADOW 0x03A5u
#define WH0_SHADOW 0x03A6u          /* WH0-WH3's shadows: the windows' edges */
#define WH1_SHADOW 0x03A7u
#define WH2_SHADOW 0x03A8u
#define WH3_SHADOW 0x03A9u
#define W12SEL_SHADOW 0x03AAu
#define W34SEL_SHADOW 0x03ABu
#define WBGLOG_SHADOW 0x03ADu
#define WOBJLOG_SHADOW 0x03AEu
#define MOSAIC_SHADOW 0x03B1u
#define TM_SHADOW 0x0399u           /* the main screen's layers */
#define TS_SHADOW 0x039Au           /* the sub screen's layers */
#define BGMODE_SHADOW 0x039Du
#define UPLOADS_THIS_FRAME 0x0403u  /* uploads queued this frame (the NMI clears it) */

/*
 * @routine 80:FFA4 NmiVector
 * @alias 00:FFA4
 * @interrupt
 * WHAT IT DOES: Where the NMI vector leads (at $00:FFA4): on to the NMI's
 * handler in the fast bank.
 * HOW: JML NmiHandler.
 * Entry: an NMI's frame on the stack (P, then the PC and bank it returns
 * to); run at $00:FFA4. Exit: as NmiHandler.
 */
void abi_NmiVector(void)
{
    abi_NmiHandler();
}

/*
 * @routine 80:FFA8 IrqVector
 * @alias 00:FFA8
 * @interrupt
 * WHAT IT DOES: Where the IRQ vector leads (at $00:FFA8): on to the IRQ's
 * handler.
 * HOW: JML IrqHandler.
 * Entry: an IRQ's frame on the stack; run at $00:FFA8. Exit: as IrqHandler.
 */
void abi_IrqVector(void)
{
    abi_IrqHandler();
}

/*
 * @routine 80:FFAC BrkVector
 * @alias 00:FFAC
 * @interrupt
 * WHAT IT DOES: Where the BRK vector leads (at $00:FFAC): an actor yields
 * for a frame (ActorYieldEntry).
 * HOW: JML ActorYieldEntry.
 * Entry: a BRK's frame on the stack (the actor's resume address past the
 * BRK's signature byte, P); D = the actor's record; run at $00:FFAC.
 * Exit: as ActorYieldEntry.
 */
void abi_BrkVector(void)
{
    abi_ActorYieldEntry();
}

/*
 * @routine 80:FFB0 CopVector
 * @alias 00:FFB0
 * @interrupt
 * WHAT IT DOES: Where the COP vector leads (at $00:FFB0): an actor waits A
 * frames (ActorWaitEntry).
 * HOW: JML ActorWaitEntry.
 * Entry: a COP's frame on the stack; D = the actor's record; A low byte =
 * the frames; run at $00:FFB0. Exit: as ActorWaitEntry.
 */
void abi_CopVector(void)
{
    abi_ActorWaitEntry();
}

/*
 * @routine 80:8111 NmiHandler
 * @interrupt
 * WHAT IT DOES: The vertical blank's work, each frame: the screen blanked
 * while it uploads the frame's queued graphics, sprites, palettes and
 * scroll values, the PPU's registers set from their shadows, the HDMA and
 * raster IRQ set up for the next frame, the tasks that wait counted down
 * (one whose count runs out is made ready), the sound command sent; then
 * the display and the NMI back on.
 * HOW: All registers saved (16-bit), D = 0, DB = $80; FRAME_COUNT and
 * V_032A up; HDMA off, NmiDisable, forced blank, RDNMI read;
 * ScrollRegistersWrite, WramMovesRun; HDMAEN from its shadow unless an IRQ
 * is on (NMITIMEN_SHADOW bits 4-5); unless V_0839, the HDMA channels 4-7
 * on in the shadow get their tables' addresses; with OAM_0857 set, the
 * sprite table by DMA, VramQueueFlush and (V_082A clear) LayersCopyToVram;
 * the VRAM lists, tile blocks, words, text buffer and its parts, uploads
 * by number (these skipped when V_082A); character sets, stepped uploads,
 * blocks B and A, the palette; the windows, layer bases, window logic,
 * colour math, fixed colour, screens, mosaic and tile bases from their
 * shadows; with an IRQ on, its H position $98 and its line (8 when V_1C7D,
 * else from the V counter latched, plus 3 when 5 or more and not
 * negative), the HDMA tables restarted (A2A = A1T, NTRL 1) for channels
 * 1-7; BGMODE from its shadow; Mode7MatrixWrite; INIDISP from its shadow
 * unless an IRQ is on (the IRQ turns it on); UPLOADS_THIS_FRAME 0; each task
 * slot 7 to 0 waiting (state 1) counts its wait down, at 0 state 4 (ready);
 * SoundCommandSend, NmiEnable; the registers back; RTI.
 * Entry: an NMI's frame on the stack; any registers. Exit: by RTI with
 * every register as at the NMI.
 */
void abi_NmiHandler(void)
{
    op_rep(0x30);  /* 808111 rep */
    pha16();  /* 808113 pha */
    phb();  /* 808114 phb */
    phd();  /* 808115 phd */
    phx16();  /* 808116 phx */
    phy16();  /* 808117 phy */
    cld();  /* 808118 cld */
    lda16_v(0x0000);  /* 808119 lda */
    tcd();  /* 80811C tcd */
    op_sep(0x30);  /* 80811D sep */
    stz8(ea_abs(V_0008));  /* 80811F stz */
    lda8_v(0x80);  /* 808122 lda */
    pha8();  /* 808124 pha */
    plb();  /* 808125 plb */
    MEM8(inc, ea_abs(FRAME_COUNT));  /* 808126 inc */
    stz8(ea_abs(HDMAEN));  /* 808129 stz */
    jsl_call(0x80812Cu, abi_NmiDisable);  /* 80812C jsl */
    lda8_v(0x80);  /* 808130 lda */
    sta8(ea_abs(INIDISP));  /* 808132 sta */
    MEM8(inc, ea_abs(V_032A));  /* 808135 inc */
    lda8(ea_abs(RDNMI));  /* 808138 lda */
    call_frame(0x813D, abi_ScrollRegistersWrite);  /* 80813B jsr */
    call_frame(0x8140, abi_WramMovesRun);  /* 80813E jsr */
    lda8(ea_abs(NMITIMEN_SHADOW));  /* 808141 lda */
    and8_v(0x30);  /* 808144 and */
    if (!IF_Z) goto L_80814E;  /* 808146 bne */
    lda8(ea_abs(HDMAEN_SHADOW));  /* 808148 lda */
    sta8(ea_abs(HDMAEN));  /* 80814B sta */
L_80814E:
    lda8(ea_abs(V_0839));  /* 80814E lda */
    if (!IF_Z) goto L_808182;  /* 808151 bne */
    lda8(ea_abs(HDMAEN_SHADOW));  /* 808153 lda */
    and8_v(0xF0);  /* 808156 and */
    if (IF_Z) goto L_808182;  /* 808158 beq */
    op_rep(0x10);  /* 80815A rep */
    ACC8(asl);  /* 80815C asl */
    if (!IF_C) goto L_808165;  /* 80815D bcc */
    ldx16(ea_abs((HDMA_TABLE_AT + 6)));  /* 80815F ldx */
    stx16(ea_abs(A1T7L));  /* 808162 stx */
L_808165:
    ACC8(asl);  /* 808165 asl */
    if (!IF_C) goto L_80816E;  /* 808166 bcc */
    ldx16(ea_abs((HDMA_TABLE_AT + 4)));  /* 808168 ldx */
    stx16(ea_abs(A1T6L));  /* 80816B stx */
L_80816E:
    ACC8(asl);  /* 80816E asl */
    if (!IF_C) goto L_808177;  /* 80816F bcc */
    ldx16(ea_abs((HDMA_TABLE_AT + 2)));  /* 808171 ldx */
    stx16(ea_abs(A1T5L));  /* 808174 stx */
L_808177:
    ACC8(asl);  /* 808177 asl */
    if (!IF_C) goto L_808180;  /* 808178 bcc */
    ldx16(ea_abs(HDMA_TABLE_AT));  /* 80817A ldx */
    stx16(ea_abs(A1T4L));  /* 80817D stx */
L_808180:
    op_sep(0x10);  /* 808180 sep */
L_808182:
    lda8(ea_abs(OAM_0857));  /* 808182 lda */
    if (IF_Z) goto L_8081C0;  /* 808185 beq */
    stz8(ea_abs(OAMADDL));  /* 808187 stz */
    stz8(ea_abs(OAMADDH));  /* 80818A stz */
    lda8_v(0x00);  /* 80818D lda */
    sta8(ea_abs(DMAP0));  /* 80818F sta */
    lda8_v(0x04);  /* 808192 lda */
    sta8(ea_abs(BBAD0));  /* 808194 sta */
    lda8_v(0x80);  /* 808197 lda */
    sta8(ea_abs(A1T0L));  /* 808199 sta */
    lda8_v(0xFA);  /* 80819C lda */
    sta8(ea_abs(A1T0H));  /* 80819E sta */
    lda8_v(0x7E);  /* 8081A1 lda */
    sta8(ea_abs(A1B0));  /* 8081A3 sta */
    lda8_v(0x20);  /* 8081A6 lda */
    sta8(ea_abs(DAS0L));  /* 8081A8 sta */
    lda8_v(0x02);  /* 8081AB lda */
    sta8(ea_abs(DAS0H));  /* 8081AD sta */
    lda8_v(0x01);  /* 8081B0 lda */
    sta8(ea_abs(MDMAEN));  /* 8081B2 sta */
    call_frame(0x81B7, abi_VramQueueFlush);  /* 8081B5 jsr */
    lda8(ea_abs(V_082A));  /* 8081B8 lda */
    if (!IF_Z) goto L_8081D2;  /* 8081BB bne */
    call_frame(0x81BF, abi_LayersCopyToVram);  /* 8081BD jsr */
L_8081C0:
    call_frame(0x81C2, abi_VramListsUpload);  /* 8081C0 jsr */
    call_frame(0x81C5, abi_TileBlockQueueFlush);  /* 8081C3 jsr */
    call_frame(0x81C8, abi_VramWordsUpload);  /* 8081C6 jsr */
    call_frame(0x81CB, abi_TextBufferUpload);  /* 8081C9 jsr */
    call_frame(0x81CE, abi_TextBufferPartsUpload);  /* 8081CC jsr */
    call_frame(0x81D1, abi_VramUploadByNumber);  /* 8081CF jsr */
L_8081D2:
    call_frame(0x81D4, abi_UploadCharsets);  /* 8081D2 jsr */
    call_frame(0x81D7, abi_UploadStepped);  /* 8081D5 jsr */
    call_frame(0x81DA, abi_UploadBlockB);  /* 8081D8 jsr */
    call_frame(0x81DD, abi_UploadBlockA);  /* 8081DB jsr */
    call_frame(0x81E0, abi_PaletteUploadFrom);  /* 8081DE jsr */
    lda8(ea_abs(WH0_SHADOW));  /* 8081E1 lda */
    sta8(ea_abs(WH0));  /* 8081E4 sta */
    lda8(ea_abs(WH1_SHADOW));  /* 8081E7 lda */
    sta8(ea_abs(WH1));  /* 8081EA sta */
    lda8(ea_abs(WH2_SHADOW));  /* 8081ED lda */
    sta8(ea_abs(WH2));  /* 8081F0 sta */
    lda8(ea_abs(WH3_SHADOW));  /* 8081F3 lda */
    sta8(ea_abs(WH3));  /* 8081F6 sta */
    lda8(ea_abs(BG1SC_SHADOW));  /* 8081F9 lda */
    sta8(ea_abs(BG1SC));  /* 8081FC sta */
    lda8(ea_abs(BG2SC_SHADOW));  /* 8081FF lda */
    sta8(ea_abs(BG2SC));  /* 808202 sta */
    lda8(ea_abs(BG3SC_SHADOW));  /* 808205 lda */
    sta8(ea_abs(BG3SC));  /* 808208 sta */
    lda8(ea_abs(W12SEL_SHADOW));  /* 80820B lda */
    sta8(ea_abs(W12SEL));  /* 80820E sta */
    lda8(ea_abs(W34SEL_SHADOW));  /* 808211 lda */
    sta8(ea_abs(W34SEL));  /* 808214 sta */
    lda8(ea_abs(WOBJSEL_SHADOW));  /* 808217 lda */
    sta8(ea_abs(WOBJSEL));  /* 80821A sta */
    lda8(ea_abs(WBGLOG_SHADOW));  /* 80821D lda */
    sta8(ea_abs(WBGLOG));  /* 808220 sta */
    lda8(ea_abs(WOBJLOG_SHADOW));  /* 808223 lda */
    sta8(ea_abs(WOBJLOG));  /* 808226 sta */
    lda8(ea_abs(CGWSEL_SHADOW));  /* 808229 lda */
    sta8(ea_abs(CGWSEL));  /* 80822C sta */
    lda8(ea_abs(CGADSUB_SHADOW));  /* 80822F lda */
    sta8(ea_abs(CGADSUB));  /* 808232 sta */
    lda8(ea_abs(COLDATA_RED));  /* 808235 lda */
    and8_v(0x1F);  /* 808238 and */
    ora8_v(0x20);  /* 80823A ora */
    sta8(ea_abs(COLDATA));  /* 80823C sta */
    lda8(ea_abs(COLDATA_GREEN));  /* 80823F lda */
    and8_v(0x1F);  /* 808242 and */
    ora8_v(0x40);  /* 808244 ora */
    sta8(ea_abs(COLDATA));  /* 808246 sta */
    lda8(ea_abs(COLDATA_BLUE));  /* 808249 lda */
    and8_v(0x1F);  /* 80824C and */
    ora8_v(0x80);  /* 80824E ora */
    sta8(ea_abs(COLDATA));  /* 808250 sta */
    lda8(ea_abs(TM_SHADOW));  /* 808253 lda */
    ldx8(ea_abs(TS_SHADOW));  /* 808256 ldx */
    sta8(ea_abs(TM));  /* 808259 sta */
    stx8(ea_abs(TS));  /* 80825C stx */
    lda8(ea_abs(TMW_SHADOW));  /* 80825F lda */
    sta8(ea_abs(TMW));  /* 808262 sta */
    lda8(ea_abs(TSW_SHADOW));  /* 808265 lda */
    sta8(ea_abs(TSW));  /* 808268 sta */
    lda8(ea_abs(MOSAIC_SHADOW));  /* 80826B lda */
    sta8(ea_abs(MOSAIC));  /* 80826E sta */
    lda8(ea_abs(BG12NBA_SHADOW));  /* 808271 lda */
    sta8(ea_abs(BG12NBA));  /* 808274 sta */
    lda8(ea_abs(BG34NBA_SHADOW));  /* 808277 lda */
    sta8(ea_abs(BG34NBA));  /* 80827A sta */
    lda8(ea_abs(NMITIMEN_SHADOW));  /* 80827D lda */
    and8_v(0x30);  /* 808280 and */
    if (IF_Z) goto L_8082F0;  /* 808282 beq */
    lda8_v(0x98);  /* 808284 lda */
    sta8(ea_abs(HTIMEL));  /* 808286 sta */
    stz8(ea_abs(HTIMEH));  /* 808289 stz */
    lda8(ea_abs(V_1C7D));  /* 80828C lda */
    if (!IF_Z) goto L_8082A5;  /* 80828F bne */
    lda8_v(0x08);  /* 808291 lda */
    ldy8(ea_abs(SLHV));  /* 808293 ldy */
    ldy8(ea_abs(STAT78));  /* 808296 ldy */
    ldy8(ea_abs(OPVCT));  /* 808299 ldy */
    if (IF_N) goto L_8082A5;  /* 80829C bmi */
    cpy8_v(0x05);  /* 80829E cpy */
    if (!IF_C) goto L_8082A5;  /* 8082A0 bcc */
    tya8();  /* 8082A2 tya */
    adc8_v(0x03);  /* 8082A3 adc */
L_8082A5:
    sta8(ea_abs(VTIMEL));  /* 8082A5 sta */
    stz8(ea_abs(VTIMEH));  /* 8082A8 stz */
    op_rep(0x20);  /* 8082AB rep */
    lda16(ea_abs(A1T1L));  /* 8082AD lda */
    sta16(ea_abs(A2A1L));  /* 8082B0 sta */
    lda16(ea_abs(A1T2L));  /* 8082B3 lda */
    sta16(ea_abs(A2A2L));  /* 8082B6 sta */
    lda16(ea_abs(A1T3L));  /* 8082B9 lda */
    sta16(ea_abs(A2A3L));  /* 8082BC sta */
    lda16(ea_abs(A1T4L));  /* 8082BF lda */
    sta16(ea_abs(A2A4L));  /* 8082C2 sta */
    lda16(ea_abs(A1T5L));  /* 8082C5 lda */
    sta16(ea_abs(A2A5L));  /* 8082C8 sta */
    lda16(ea_abs(A1T6L));  /* 8082CB lda */
    sta16(ea_abs(A2A6L));  /* 8082CE sta */
    lda16(ea_abs(A1T7L));  /* 8082D1 lda */
    sta16(ea_abs(A2A7L));  /* 8082D4 sta */
    op_sep(0x20);  /* 8082D7 sep */
    lda8_v(0x01);  /* 8082D9 lda */
    sta8(ea_abs(NTRL1));  /* 8082DB sta */
    sta8(ea_abs(NTRL2));  /* 8082DE sta */
    sta8(ea_abs(NTRL3));  /* 8082E1 sta */
    sta8(ea_abs(NTRL4));  /* 8082E4 sta */
    sta8(ea_abs(NTRL5));  /* 8082E7 sta */
    sta8(ea_abs(NTRL6));  /* 8082EA sta */
    sta8(ea_abs(NTRL7));  /* 8082ED sta */
L_8082F0:
    lda8(ea_abs(BGMODE_SHADOW));  /* 8082F0 lda */
    sta8(ea_abs(BGMODE));  /* 8082F3 sta */
    call_frame(0x82F8, abi_Mode7MatrixWrite);  /* 8082F6 jsr */
    lda8(ea_abs(NMITIMEN_SHADOW));  /* 8082F9 lda */
    and8_v(0x30);  /* 8082FC and */
    if (!IF_Z) goto L_808306;  /* 8082FE bne */
    lda8(ea_abs(INIDISP_SHADOW));  /* 808300 lda */
    sta8(ea_abs(INIDISP));  /* 808303 sta */
L_808306:
    stz8(ea_abs(UPLOADS_THIS_FRAME));  /* 808306 stz */
    ldx8_v(0xC0);  /* 808309 ldx */
L_80830B:
    lda8(ea_absx((TASK_STATE - 24)));  /* 80830B lda */
    cmp8_v(0x01);  /* 80830E cmp */
    if (!IF_Z) goto L_80831C;  /* 808310 bne */
    MEM8(dec, ea_absx((TASK_WAIT - 24)));  /* 808312 dec */
    if (!IF_Z) goto L_80831C;  /* 808315 bne */
    lda8_v(0x04);  /* 808317 lda */
    sta8(ea_absx((TASK_STATE - 24)));  /* 808319 sta */
L_80831C:
    sec();  /* 80831C sec */
    txa8();  /* 80831D txa */
    sbc8_v(0x18);  /* 80831E sbc */
    tax8();  /* 808320 tax */
    if (!IF_Z) goto L_80830B;  /* 808321 bne */
    call_frame(0x8325, abi_SoundCommandSend);  /* 808323 jsr */
    jsl_call(0x808326u, abi_NmiEnable);  /* 808326 jsl */
    op_rep(0x30);  /* 80832A rep */
    ply16();  /* 80832C ply */
    plx16();  /* 80832D plx */
    pld();  /* 80832E pld */
    plb();  /* 80832F plb */
    pla16();  /* 808330 pla */
    rti();  /* 808331 rti */
}

/*
 * @routine 80:8332 IrqHandler
 * @interrupt
 * WHAT IT DOES: The raster interrupt: at the line below the status bar it
 * waits for the horizontal blank and turns the display on: HDMA, the
 * brightness, the NMI back to the vertical blank only, and the screens.
 * HOW: A and DB saved (M 8-bit), DB = $80; TIMEUP read (the IRQ
 * acknowledged); HVBJOY until bit 6 (horizontal blank); HDMAEN and INIDISP
 * from their shadows; NMITIMEN = $81; TM and TS from their shadows; DB and
 * A back; RTI.
 * Entry: an IRQ's frame on the stack. Exit: by RTI with A, DB and P as at
 * the IRQ, M 8-bit within.
 */
void abi_IrqHandler(void)
{
    op_sep(0x20);  /* 808332 sep */
    pha8();  /* 808334 pha */
    phb();  /* 808335 phb */
    lda8_v(0x80);  /* 808336 lda */
    pha8();  /* 808338 pha */
    plb();  /* 808339 plb */
    lda8(ea_abs(TIMEUP));  /* 80833A lda */
L_80833D:
    bit8_v(rd8(ea_abs(HVBJOY)));  /* 80833D bit */
    if (!IF_V) goto L_80833D;  /* 808340 bvc */
    lda8(ea_abs(HDMAEN_SHADOW));  /* 808342 lda */
    sta8(ea_abs(HDMAEN));  /* 808345 sta */
    lda8(ea_abs(INIDISP_SHADOW));  /* 808348 lda */
    sta8(ea_abs(INIDISP));  /* 80834B sta */
    lda8_v(0x81);  /* 80834E lda */
    sta8(ea_abs(NMITIMEN));  /* 808350 sta */
    lda8(ea_abs(TM_SHADOW));  /* 808353 lda */
    sta8(ea_abs(TM));  /* 808356 sta */
    lda8(ea_abs(TS_SHADOW));  /* 808359 lda */
    sta8(ea_abs(TS));  /* 80835C sta */
    plb();  /* 80835F plb */
    pla8();  /* 808360 pla */
    rti();  /* 808361 rti */
}
