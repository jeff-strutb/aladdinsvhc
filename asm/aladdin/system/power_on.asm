; power_on.asm: the game's start at power-on and reset: the hardware and
; work RAM cleared, the task slots laid out, the kept options checked, and
; the first task started under the scheduler.
; C: src/aladdin/system/power_on.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

!pon_signature = $0000           ; word: $0123 once the game has started (kept over a reset)
!pon_000a = $000A                ; cleared at the start; purpose not yet established
!pon_slot_data = $0074           ; by task slot ($18 bytes each), the byte after the stack top
!pon_0004 = $0004                ; set to $80 on a cold start; purpose not yet established
!pon_0005 = $0005                ; cleared on a cold start; purpose not yet established
!pon_kept_sum = $0006            ; the kept options' sum (KeptOptionsSum's result)
!pon_kept = $0003                ; the kept options' first byte
!pon_0009 = $0009                ; cleared on a cold start
!pon_005f = $5F                  ; direct page: set to $88; purpose not yet established
!pon_slot_tops = $B60B           ; ROM, bank $80: each slot's stack top and the next byte (2 bytes, 8 slots)
!pon_title = $B600               ; ROM, bank $80: the 8 bytes the cartridge header's title must start with
!pon_header = $FFC0              ; ROM: the cartridge header's title
!pon_first_task = $818000        ; the first task (slot 0)
!pon_scheduler = $81BC6D         ; the task scheduler
!pon_ram_high = $1FFF            ; with DB $7E and X from $E000 down to 1: $7E:2000-$7E:FFFF cleared
!pon_ram_low = $0000             ; X from $1FFF down to $000C: $00:000C-$00:1FFF cleared

; @routine 80:8000 PowerOn
; WHAT IT DOES: The game's start, from the reset vector: the S-CPU into
; native mode in the fast bank, the I/O registers, the PPU's and work RAM
; cleared, each task slot's stack laid out; on a cold start (the RAM's
; signature or the header's title not as left by the game, or the kept
; options' sum wrong) the signature and the kept state set afresh; then
; the NMI on, the first task created (slot 0 at $81:8000) and the
; scheduler entered for good.
; HOW: SEI; native mode (CLC, XCE); JML into bank $80. NMITIMEN, MDMAEN,
; HDMAEN 0; INIDISP $8F (forced blank); $000A 0; MEMSEL 1; then $4202-$420D
; 0 (X $0D down to 2); WRIO $FF. S = $0326; DB = $80. PpuRegistersZero for
; $2101-$210C, $2115-$211A and $2123-$2133. DB $7E: $7E:2000-$7E:FFFF
; cleared; $000C-$1FFF cleared; the palette buffer ($200 bytes) cleared.
; The 8 slots' stack tops and the next byte from $80:B60B. The title's
; first 8 bytes compared with $80:B600 (16-bit, X 0-6), then the signature
; with $0123: both equal and KeptOptionsSum equal to the kept sum: a warm
; start, the kept state left as it is. Else the signature $0123, the kept
; sum $80, the kept options and $0005 0, $0004 $80, $0009 0. NMITIMEN's
; shadow 1, NmiEnable, CLI; dp $5F $88; TASK_SLOT_COUNT 8; TaskCreate
; (slot 0, parameter 0, $81:8000); JML to the scheduler ($81:BC6D).
; Entry: the reset vector (emulation mode, at $00:8000), or a unit call.
; Exit: never returns: in the scheduler with S $0326.
%at($808000)
PowerOn:
    sei
    clc
    xce
    jml.l .fast
.fast:
    stz.w !NMITIMEN
    stz.w !MDMAEN
    stz.w !HDMAEN
    lda.b #$8F
    sta.w !INIDISP
    stz.w !pon_000a
    lda.b #$01
    sta.w !MEMSEL
    ldx.b #$0D
.io:
    cpx.b #$01
    beq .io_done
    stz.w !NMITIMEN,x
    dex
    bpl .io
.io_done:
    lda.b #$FF
    sta.w !WRIO
    rep.b #$10
    ldx.w #$0326
    txs
    sep.b #$10
    lda.b #$80
    pha
    plb
    ldx.b #$01
    ldy.b #$0B
    jsr.w PpuRegistersZero
    ldx.b #$15
    ldy.b #$05
    jsr.w PpuRegistersZero
    ldx.b #$23
    ldy.b #$10
    jsr.w PpuRegistersZero
    phb
    lda.b #$7E
    pha
    plb
    rep.b #$10
    ldx.w #$E000
.clear_high:
    stz.w !pon_ram_high,x
    dex
    bne .clear_high
    plb
    ldx.w #$1FFF
.clear_low:
    stz.w !pon_ram_low,x
    dex
    cpx.w #$000B
    bne .clear_low
    ldx.w #$01FF
    lda.b #$00
.clear_palette:
    sta.l !palette_buffer,x
    dex
    bpl .clear_palette
    sep.b #$30
    ldy.b #$A8
    ldx.b #$0E
.slots:
    lda.w !pon_slot_tops,x
    sta.w !task_stack_top,y
    lda.w !pon_slot_tops+1,x
    sta.w !pon_slot_data,y
    sec
    tya
    sbc.b #$18
    tay
    dex
    dex
    bpl .slots
    rep.b #$20
    ldx.b #$FE
.title:
    inx
    inx
    lda.w !pon_title,x
    cmp.w !pon_header,x
    bne .cold
    cpx.b #$06
    bne .title
    lda.w #$0123
    cmp.w !pon_signature
    sep.b #$20
    bne .cold
    jsr.w KeptOptionsSum
    cmp.w !pon_kept_sum
    beq .started
.cold:
    lda.b #$23
    sta.w !pon_signature
    lda.b #$01
    sta.w !pon_signature+1
    lda.b #$80
    sta.w !pon_kept_sum
    stz.w !pon_kept
    stz.w !pon_0005
    lda.b #$80
    sta.w !pon_0004
    stz.w !pon_0009
.started:
    sep.b #$20
    lda.b #$01
    sta.w !nmitimen_shadow
    jsl.l NmiEnable
    cli
    lda.b #$88
    sta.b !pon_005f
    lda.b #$08
    sta.w !task_slot_count
    jsl.l TaskCreate
    db 0*24, 0                  ; slot 0, parameter 0
    dl !pon_first_task
    jml.l !pon_scheduler
