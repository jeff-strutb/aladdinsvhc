; cartridge: the ROM read through bank $80 and through bank $00 must be
; the same (a LoROM mirror), and the cartridge must have no RAM at $70.
; A copier fails one or the other: the game then slows to a stop. Drafted
; by cdraft.py; completed and described by hand.
; C: src/aladdin/game/mirror_check_task.c.

incsrc "../include/macros.inc"

arch 65816

!MIRROR_PROBE = $9000          ; two ROM bytes read through banks $80 and $00
!CART_RAM_PROBE = $0080        ; in bank $70, from X = $80: $0080 and $0100 written and read back
!V_03FB = $03FB                ; set to $3F, over and over, once caught

; @routine 81:A37A MirrorCheckTask
; WHAT IT DOES: Checks for a copier, then ends: the two bytes at $9000
; the same through banks $80 and $00, and $70:0080 not keeping a byte
; written to it. Caught (no script is): it waits 30 times 63 frames, then
; forever sets V_03FB to $3F every 255 frames.
; HOW: DB kept; Y and X = $80:9000 and $9001; with DB 0, CPY and CPX the
; same bytes; DB = $80 - $10 = $70, X = $80; $23 to $0000,X ($70:0080)
; and compared; equal: $45 to CART_RAM_PROBE,X ($70:0100) and compared.
; Not equal: DB back, TaskExit. Caught: DB back; TaskWait $3F 30 times;
; then V_03FB $3F and TaskWait $FF, for ever.
; Entry: the task's start: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF
; is work RAM. Exit: none: the task ends, or never.
%at($81A37A)
MirrorCheckTask:
    phb
    lda.b #$80
    pha
    plb
    ldy.w !MIRROR_PROBE
    ldx.w !MIRROR_PROBE+1
    lda.b #$00
    pha
    plb
    cpy.w !MIRROR_PROBE
    bne .caught
    cpx.w !MIRROR_PROBE+1
    bne .caught
    ldx.b #$80
    sec
    lda.b #$80
    sbc.b #$10
    pha
    plb
    lda.b #$23
    sta.w 0,x
    cmp.w 0,x
    bne .retail
    lda.b #$45                     ; (no script runs this)
    sta.w !CART_RAM_PROBE,x
    cmp.w !CART_RAM_PROBE,x
    bne .retail
.caught:
    plb
    ldx.b #30
.slow:
    lda.b #$3F
    jsl.l TaskWait
    dex
    bne .slow
.stop:
    lda.b #$3F
    sta.w !V_03FB
    lda.b #$FF
    jsl.l TaskWait
    bra .stop
.retail:
    plb
    jml.l TaskExit
