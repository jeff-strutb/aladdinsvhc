; sound_driver.asm: the SPC700 sound driver (draft).
; C: src/aladdin/audio/sound_driver.c.

incsrc "../include/macros.inc"

%spc($02C0, $8580BD)

; @routine 85:80BD R02C0
; @spc 02C0
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R02C0:
    clrp
    mov x,#$CF
    mov sp,x
    mov a,#$00
    mov x,a
.l02C7:
    mov (x+),a
    cmp x,#$E0
    bne .l02C7
    setp
    mov x,a
.l02CE:
    mov (x+),a
    cmp x,#$B0
    bne .l02CE
    clrp
    mov x,a
.l02D5:
    mov.w $0200+x,a
    inc x
    cmp x,#$C0
    bne .l02D5
    call R0425
    mov a,#$40
    mov.w $00FA,a
    mov a,#$F1
    mov.w $00F1,a
    mov a,#$01
    mov.b $B2,a
    mov.w $00F6,a
    dec a
    mov.w $00F7,a
.l02F5:
    call R043E
    mov.w a,$00FD
    clrc
    adc.b a,$B1
    mov.b $B1,a
    beq .l02F5
    dec.b $B1
    mov y,#$02
    mov a,#$F5
    push y
    push a
    clrc
    adc $B0,#$80
    bpl .l0313
    jmp .l0399
.l0313:
    cmp $CC,$CD
    beq .l032D
    bbs $CD.7,.l0329
    mov y,#$6C
    mov a,#$00
    call DspWrite
    mov y,#$4D
    mov a,#$FF
    call DspWrite
.l0329:
    inc.b $CD
    bra .l0348
.l032D:
    mov y,#$4D
    mov.b a,$B8
    eor a,#$FF
    and.b a,$BC
    or.b a,$B7
    call DspWrite
    mov y,#$2C
    mov.b a,$CE
    call DspWrite
    mov y,#$3C
    mov.b a,$CF
    call DspWrite
.l0348:
    mov x,#$07
    mov y,#$57
    mov.b a,$BA
    or.b a,$B5
    push a
.l0351:
    push y
    bbc $B5.7,.l035F
    mov.w a,$0238+y
    mov.b $A3,a
    mov.w a,$0240+y
    bra .l0375
.l035F:
    bbc $BD.7,.l036A
    mov.w a,$0250+x
    call R08C4
    bra .l0381
.l036A:
    bbc $BA.7,.l0381
    mov.w a,$0238+x
    mov.b $A3,a
    mov.w a,$0240+x
.l0375:
    mov y,#$07
    call DspWriteVoice
    mov y,#$05
    mov.b a,$A3
    call DspWriteVoice
.l0381:
    asl.b $B5
    asl.b $BD
    asl.b $BA
    pop y
    dec y
    dec x
    bpl .l0351
    mov y,#$5C
    mov a,#$00
    call DspWrite
    pop a
    mov y,#$4C
    jmp DspWrite
.l0399:
    clr1 $B0.7
    bbs $B0.6,.l03D7
    mov $B3,#$80
    mov $B9,$B8
    mov x,#$07
    movw ya,$BE
    addw ya,$C0
    movw $A9,ya
    mov y,#$00
    mov.b a,$C2
    addw ya,$A9
    mov.b $A8,y
    mov.b $C2,a
.l03B6:
    mov.b a,$08+x
    or.b a,$00+x
    beq .l03BF
    call R06B1
.l03BF:
    dec x
    asl.b $B9
    lsr.b $B3
    bne .l03B6
    movw ya,$C7
    addw ya,$C9
    bcc .l03D5
    mov $C7,#$00
    mov $C8,#$00
    mov y,#$FF
    mov a,y
.l03D5:
    movw $C9,ya
.l03D7:
    set1 $B0.7
    mov $B3,#$80
    mov $B9,#$00
    mov x,#$57
.l03E1:
    mov.b a,$B8
    and.b a,$B3
    beq .l0400
    mov.w a,$0260+x
    mov.b $AA,a
    mov.w a,$0258+x
    mov.b $A9,a
    mov y,#$00
    mov.w a,$0268+x
    addw ya,$A9
    mov.b $A8,y
    mov.w $0268+x,a
    call R06B1
.l0400:
    dec x
    lsr.b $B3
    bne .l03E1
    mov.b a,$B6
    or.b a,$BB
    mov $B6,#$00
    mov $BB,#$00
    mov y,#$5C
    jmp DspWrite

%spc_end()

%spc($0425, $858222)

; @routine 85:8222 R0425
; @spc 0425
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R0425:
    mov y,#$03
.l0427:
    mov.w a,$0437+y
    mov.w $00F2,a
    mov.w a,$043A+y
    mov.w $00F3,a
    dbnz y,.l0427
    jmp P09FA
    db $2D,$3D,$5D,$00,$00,$33

; @routine 85:823B R043E
; @spc 043E
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R043E:
    mov.w a,$00F4
    cmp.w a,$00F4
    bne R043E
    mov y,a
    beq P045D
    push a
    inc.b $B2
    call R045E
    mov a,#$31
    mov.w $00F1,a
    mov.b a,$B2
    mov.w $00F6,a
    pop a
    mov.w $00F7,a

P045D:
    ret

; @routine 85:825B R045E
; @spc 045E
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R045E:
    cmp a,#$F0
    bcc .l0465
    jmp P050A
.l0465:
    cmp.w a,$1DB0
    bcc .l0470
    setc
    sbc.w a,$1DB0
    bra .l0465
.l0470:
    asl a
    mov x,a
    mov.w a,$1DB2+x
    mov.b $A1,a
    mov.w a,$1DB3+x
    mov.b $A0,a
    or.b a,$A1
    beq P045D
    mov y,#$00
    mov a,($A0)+y
    bne .l0489
    jmp P0570
.l0489:
    mov.b $A4,a
    mov $B3,#$80
    mov x,#$57
.l0490:
    mov y,#$00
    incw $A0
    mov a,($A0)+y
    mov.b $A3,a
    incw $A0
    mov a,($A0)+y
    mov.b $A2,a
    mov a,($A2)+y
    cmp a,#$17
    beq .l04DE
    mov.b a,$A4
    cmp.w a,$0250+x
    bcc .l04DE
    or $B8,$B3
    mov.b a,$B3
    eor a,#$FF
    mov.b $A5,a
    and $B7,$A5
    and $BA,$A5
    call R04E4
    call VoiceRelease
    mov.b a,$A4
    mov.w $0250+x,a
    movw ya,$A2
    mov.b $08+x,y
    mov.b $00+x,a
    mov a,#$10
    mov.w $0100+x,a
    mov a,#$B8
    mov.w $0240+x,a
    mov.w a,$00F5
    clrc
    adc a,#$80
    mov.w $0248+x,a
.l04DE:
    dec x
    lsr.b $B3
    bne .l0490
    ret

; @routine 85:82E1 R04E4
; @spc 04E4
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R04E4:
    mov a,x
    clrc
    adc a,#$08
.l04E8:
    mov y,a
    mov a,#$00
    mov.w $0200+y,a
    mov a,y
    clrc
    adc a,#$08
    cmp a,#$C0
    bcc .l04E8
    setp
    call R04FB
    clrp

; @routine 85:82F8 R04FB
; @spc 04FB
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R04FB:
    push x
.l04FC:
    mov a,#$00
    mov (x),a
    mov a,x
    clrc
    adc a,#$08
    mov x,a
    cmp x,#$A0
    bcc .l04FC
    pop x
    ret

P050A:
    and a,#$0F
    asl a
    mov x,a
    jmp ($0511+x)
    db $2D,$05,$30,$05,$49,$05,$DC,$05,$E0,$05,$16,$06
    db $1A,$06,$50,$06,$53,$06,$56,$06,$67,$05,$6A,$05
    db $B1,$05,$D6,$05

; @routine 85:832A R052D
; @spc 052D
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R052D:
    call R0549

P0530:
    mov $B3,#$80
    mov x,#$57
.l0535:
    bbc $B8.7,.l053B
    call R099C
.l053B:
    dec x
    asl.b $B8
    lsr.b $B3
    bne .l0535
    mov $B5,#$00
    mov $B7,#$00
    ret

; @routine 85:8346 R0549
; @spc 0549
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R0549:
    call R05F9
    mov a,#$00
    mov.b $BC,a
    mov x,#$08
.l0552:
    mov.w $0200+x,a
    inc x
    cmp x,#$58
    bne .l0552
    setp
    call R055F
    clrp

; @routine 85:835C R055F
; @spc 055F
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R055F:
    mov x,#$00
.l0561:
    mov (x+),a
    cmp x,#$50
    bne .l0561
    ret

P0567:
    call R0656

P056A:
    mov $A1,#$0D
    mov $A0,#$1F

P0570:
    call R0549
    mov y,#$00
    mov a,y
    movw $C0,ya
    mov.b $C4,a
    mov.b $C5,a
    dec a
    mov.b $C3,a
    call R0645
    mov x,#$07
.l0584:
    mov y,#$00
    incw $A0
    mov a,($A0)+y
    mov.b $A3,a
    incw $A0
    mov a,($A0)+y
    mov.b $A2,a
    mov a,($A2)+y
    cmp a,#$17
    beq .l05AD
    movw ya,$A2
    mov.b $08+x,y
    mov.b $00+x,a
    mov a,#$10
    mov.w $0100+x,a
    mov a,#$B8
    mov.w $0240+x,a
    mov a,#$80
    mov.w $0248+x,a
.l05AD:
    dec x
    bpl .l0584
    ret

P05B1:
    mov.w a,$00F5
    push p
    bpl .l05BA
    eor a,#$FF
    inc a
.l05BA:
    push a
    mov y,#$69
    mul ya
    mov $A3,#$00
    mov.b $A2,y
    pop a
    mov y,#$03
    mul ya
    addw ya,$A2
    pop p
    bpl .l05D3
    movw $A2,ya
    mov y,#$00
    mov a,y
    subw ya,$A2
.l05D3:
    movw $C0,ya
    ret

P05D6:
    mov.w a,$00F5
    mov.b $C5,a
    ret

P05DC:
    set1 $B0.6
    bra R05F9

P05E0:
    clr1 $B0.6
    mov $BA,#$00
    mov $B3,#$80
    mov x,#$57
.l05EA:
    mov.b a,$B8
    and.b a,$B3
    bne .l05F3
    call R09A7
.l05F3:
    dec x
    lsr.b $B3
    bne .l05EA
    ret

; @routine 85:83F6 R05F9
; @spc 05F9
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R05F9:
    mov x,#$07
    mov $B3,#$80
.l05FE:
    mov.b a,$B8
    and.b a,$B3
    bne .l060A
    call VoiceRelease
    or $BB,$B3
.l060A:
    dec x
    lsr.b $B3
    bne .l05FE
    mov $BA,#$00
    mov $BD,#$00
    ret

P0616:
    mov x,#$FF
    bra P061A_l061C

P061A:
    mov x,#$01
.l061C:
    cmp.b x,$C6
    beq .l063C
    mov.w a,$00F6
    or.w a,$00F7
    beq R0645
    mov.w a,$00F6
    mov.w y,$00F7
    asl.b $C6
    beq .l063D
    call R064B
    mov y,#$FF
    mov a,y
    subw ya,$C9
    movw $C9,ya
.l063C:
    ret
.l063D:
    mov $C9,#$00
    mov $CA,#$00
    bra R064B

; @routine 85:8442 R0645
; @spc 0645
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R0645:
    mov a,#$00
    mov y,a
    mov x,a
    movw $C9,ya

; @routine 85:8448 R064B
; @spc 064B
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R064B:
    movw $C7,ya
    mov.b $C6,x
    ret

P0650:
    clr1 $B0.5
    ret

P0653:
    set1 $B0.5
    ret

; @routine 85:8453 R0656
; @spc 0656
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R0656:
    call R052D
    mov a,#$AA
    mov.w $00F4,a
    mov a,#$BB
    mov.w $00F5,a
.l0663:
    mov.w a,$00F4
    cmp a,#$CC
    bne .l0663
    bra .l068C
.l066C:
    mov.w y,$00F4
    bne .l066C
.l0671:
    cmp.w y,$00F4
    bne .l0685
    mov.w a,$00F5
    mov.w $00F4,y
    mov ($A0)+y,a
    inc y
    bne .l0671
    inc.b $A1
    bra .l0671
.l0685:
    bpl .l0671
    cmp.w y,$00F4
    bpl .l0671
.l068C:
    mov.w a,$00F6
    mov.w y,$00F7
    movw $A0,ya
    mov.w y,$00F4
    mov.w a,$00F5
    beq .l06A1
    mov.w $00F4,y
    bra .l066C
.l06A1:
    mov.w $00F6,a
    inc a
    mov.b $B2,a
    mov.w a,$00FD
    mov $B1,#$00
    mov.w $00F4,y
    ret

; @routine 85:84AE R06B1
; @spc 06B1
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R06B1:
    mov $B4,$B3
    eor $B4,#$FF
    mov.b a,$18+x
    beq .l06E9
    mov.b y,$28+x
    mov a,y
    or.b a,$20+x
    beq .l06D7
    mov.b a,$20+x
    subw ya,$A9
    beq .l06CA
    bcs .l06D3
.l06CA:
    bbs $B9.7,.l06D0
    call VoiceAdsr1Set
.l06D0:
    mov y,#$00
    mov a,y
.l06D3:
    mov.b $28+x,y
    mov.b $20+x,a
.l06D7:
    mov.b a,$18+x
    setc
    sbc.b a,$A8
    mov.b $18+x,a
    beq .l06E9
    bcc .l06E9
    cmp a,#$C1
    bcs .l06E9
    jmp P0AC3
.l06E9:
    mov y,#$06
    mov a,#$F2
    push y
    push a
    jmp .l0803
    push a
    xcn a
    lsr a
    and a,#$07
    dec a
    mov y,a
    mov.b a,$10+x
    and a,#$30
    bne .l0704
    mov.w a,$07E5+y
    bra .l0716
.l0704:
    and a,#$10
    bne .l070D
    mov.w a,$07DE+y
    bra .l0716
.l070D:
    mov.b a,$10+x
    and a,#$EF
    mov.b $10+x,a
    mov.w a,$07EC+y
.l0716:
    clrc
    adc.b a,$18+x
    mov.b $18+x,a
    pop a
    and a,#$1F
    bne .l0721
    ret
.l0721:
    push a
    mov.b a,$10+x
    and a,#$0F
    mov y,a
    pop a
    clrc
    adc.w a,$07F3+y
    bbs $B0.7,.l0732
    clrc
    adc.b a,$C4
.l0732:
    clrc
    adc.w a,$0210+x
    mov.b $A1,a
    mov $A0,#$00
    mov y,#$00
    mov.w a,$0218+x
    bpl .l0743
    dec y
.l0743:
    addw ya,$A0
    cmp y,#$61
    bcc .l074B
    mov y,#$60
.l074B:
    setp
    movw $A2,ya
    mov.b a,$00+x
    mov.b $A4,a
    clr1 $A4.0
    mov.b a,$28+x
    beq .l0760
    mov.b a,$40+x
    beq .l0760
    cmp.b a,$A3
    bne .l0764
.l0760:
    mov.b a,$A2
    bra .l0772
.l0764:
    bcs .l076A
    set1 $A4.1
    bra .l076C
.l076A:
    clr1 $A4.1
.l076C:
    inc.b $A4
    mov.b y,$40+x
    mov.b a,$48+x
.l0772:
    push a
    mov.b a,$A4
    mov.b $00+x,a
    mov.b a,$A3
    mov.b $40+x,a
    mov.b a,$A2
    mov.b $48+x,a
    pop a
    mov.b $30+x,y
    mov.b $38+x,a
    clrp
    mov.b a,$10+x
    bmi .l07B3
    mov.w a,$0100+x
    mov y,a
    and a,#$10
    beq .l079C
    mov a,y
    and a,#$3F
    mov.w $0100+x,a
    mov a,#$00
    mov.w $0120+x,a
.l079C:
    bbc $B0.7,.l07A7
    or $B5,$B3
    or $B6,$B3
    bra .l07B0
.l07A7:
    bbs $B9.7,.l07BD
    or $BA,$B3
    or $BB,$B3
.l07B0:
    call VoiceRelease
.l07B3:
    bbs $B9.7,.l07BD
    setp
    call R0BE9
    call R0B17
.l07BD:
    mov.b a,$10+x
    mov y,a
    and a,#$7F
    mov.b $A3,a
    mov a,y
    and a,#$40
    asl a
    or.b a,$A3
    mov.b $10+x,a
    bpl .l07D3
    mov y,#$00
    mov a,y
    bra .l07D9
.l07D3:
    mov.b y,$18+x
    mov.w a,$0208+x
    mul ya
.l07D9:
    mov.b $28+x,y
    mov.b $20+x,a
    ret
    db $02,$04,$08,$10,$20,$40,$80,$03,$06,$0C,$18,$30
    db $60,$C0,$00,$09,$12,$24,$48,$90,$00,$00,$0C,$18
    db $24,$30,$3C,$48,$54,$18,$24,$30,$3C,$48,$54,$60
    db $6C
.l0803:
    call R0820
    cmp a,#$20
    bcs P0830
    mov.b $A7,a
    mov y,#$08
    push y
    mov y,#$03
    push y
    asl a
    mov y,a
    mov.w a,$0832+y
    push a
    mov.w a,$0831+y
    push a
    cmp y,#$08
    bcc P0830

; @routine 85:861D R0820
; @spc 0820
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R0820:
    mov.b y,$08+x
    mov.b a,$00+x
    movw $A0,ya
    mov y,#$00
    mov a,($A0)+y
    inc.b $00+x
    bne P0830
    inc.b $08+x

P0830:
    ret
    db $71,$08,$75,$08,$79,$08,$7F,$08,$71,$09,$86,$08
    db $A1,$08,$A5,$08,$BA,$08,$FC,$08,$07,$09,$0D,$09
    db $11,$09,$15,$09,$19,$09,$1D,$09,$21,$09,$25,$09
    db $19,$09,$1D,$09,$21,$09,$25,$09,$5B,$09,$7C,$09
    db $B7,$09,$BE,$09,$C8,$09,$EE,$09,$A3,$0A,$BD,$0A
    db $30,$08,$30,$08

P0871:
    mov a,#$20
    bra P087F_l0881

P0875:
    mov a,#$40
    bra P087F_l0881

P0879:
    mov a,#$10
    or.b a,$10+x
    bra P087F_l0883

P087F:
    mov a,#$08
.l0881:
    eor.b a,$10+x
.l0883:
    mov.b $10+x,a
    ret

P0886:
    push a
    call R0820
    pop y
    bbc $B0.7,.l089B
    mov.w $0258+x,a
    mov a,y
    mov.w $0260+x,a
    mov a,#$00
    mov.w $0268+x,a
    ret
.l089B:
    movw $BE,ya
    mov $C2,#$00
    ret

P08A1:
    mov.w $0208+x,a
    ret

P08A5:
    mov y,a
    call R0B8E
    bbs $B0.7,.l08B6
    push a
    mov.w y,$1DB1
    call R0B8E
    pop y
    mul ya
    mov a,y
.l08B6:
    mov.w $0230+x,a
    ret

P08BA:
    bbs $B0.7,P08C5
    inc a
    mov.w $0250+x,a
    bbs $B9.7,P08FB

; @routine 85:86C1 R08C4
; @spc 08C4
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R08C4:
    dec a

P08C5:
    mov y,#$06
    mul ya
    movw $A0,ya
    clrc
    adc $A0,#$38
    adc $A1,#$34
    mov y,#$01
    mov a,($A0)+y
    mov.w $0238+x,a
    dec y
    push x
    mov a,x
    xcn a
    and a,#$70
    or a,#$04
    mov x,a
.l08E1:
    mov a,($A0)+y
    mov.w $00F2,x
    mov.w $00F3,a
    inc x
    inc y
    cmp y,#$04
    bne .l08E1
    pop x
    mov a,($A0)+y
    mov.w $0220+x,a
    inc y
    mov a,($A0)+y
    mov.w $0228+x,a

P08FB:
    ret

P08FC:
    mov.b $A3,a
    mov.b a,$10+x
    and a,#$F8
    or.b a,$A3
    mov.b $10+x,a
    ret

P0907:
    bbs $B0.7,.l090C
    mov.b $C4,a
.l090C:
    ret

P090D:
    mov.w $0210+x,a
    ret

P0911:
    mov.w $0218+x,a
    ret

P0915:
    mov.w $0128+x,a
    ret

P0919:
    mov y,#$30
    bra P0925_l0927

P091D:
    mov y,#$38
    bra P0925_l0927

P0921:
    mov y,#$40
    bra P0925_l0927

P0925:
    mov y,#$48
.l0927:
    mov.b $A3,a
    mov.b $A0,y
    mov a,x
    clrc
    adc.b a,$A0
    mov y,a
    mov $A1,#$00
    mov $A0,#$00
    cmp $A7,#$12
    bcs .l094C
    mov a,($A0)+y
    beq .l0946
    dec a
    mov ($A0)+y,a
    beq P095B_l0965
    bra .l0958
.l0946:
    mov.b a,$A3
    mov ($A0)+y,a
    bra .l0958
.l094C:
    mov a,($A0)+y
    dec a
    bne P095B_l0965
    mov ($A0)+y,a
    mov.b a,$A3
    call R0971
.l0958:
    call R0820

P095B:
    push a
    call R0820
    mov.b $00+x,a
    pop a
    mov.b $08+x,a
    ret
.l0965:
    mov a,#$02
    clrc
    adc.b a,$00+x
    mov.b $00+x,a
    bcc .l0970
    inc.b $08+x
.l0970:
    ret

; @routine 85:876E R0971
; @spc 0971
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R0971:
    mov.b $A3,a
    mov.b a,$10+x
    and a,#$97
    or.b a,$A3
    mov.b $10+x,a
    ret

P097C:
    pop a
    pop a
    pop a
    pop a
    bbs $B0.7,.l0996
    mov a,#$00
    mov.b $08+x,a
    mov.b $00+x,a
    mov.w $0250+x,a
    bbs $B9.7,.l0995
    call VoiceRelease
    or $BB,$B3
.l0995:
    ret
.l0996:
    and $B8,$B4
    and $B7,$B4

; @routine 85:8799 R099C
; @spc 099C
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R099C:
    mov a,#$00
    mov.w $0250+x,a
    call VoiceRelease
    or $B6,$B3

; @routine 85:87A4 R09A7
; @spc 09A7
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R09A7:
    push x
    mov a,x
    setc
    sbc a,#$50
    mov x,a
    mov.w a,$0250+x
    beq .l09B5
    or $BD,$B3
.l09B5:
    pop x
    ret

P09B7:
    clrc
    adc a,#$80
    mov.w $0248+x,a
    ret

P09BE:
    bbs $B0.7,.l09C7
    mov y,a
    call R0B8E
    mov.b $C3,a
.l09C7:
    ret

P09C8:
    push a
    call R0820
    pop y
    cmp y,#$03
    bne .l09DF
    xcn a
    mov.b $A3,a
    mov.w a,$0100+x
    and a,#$EF
    or.b a,$A3
    mov.w $0100+x,a
    ret
.l09DF:
    push a
    mov a,#$08
    mul ya
    mov.b $A4,x
    clrc
    adc.b a,$A4
    mov y,a
    pop a
    mov.w $0108+y,a
    ret

P09EE:
    call R0820
    mov y,#$08
    mul ya
    cmp.b a,$CB
    beq P09FA_l0A52
    mov.b $CB,a

P09FA:
    push x
    mov x,a
    mov y,#$0C
    mov.w a,$0D10+x
    call DspWrite
    mov y,#$1C
    mov.w a,$0D11+x
    call DspWrite
    mov.w a,$0D12+x
    mov.b $CE,a
    mov.w a,$0D13+x
    mov.b $CF,a
    call R0A53
    mov y,#$6D
    mov.w a,$0D14+x
    call DspWrite
    mov a,#$7D
    mov.w $00F2,a
    mov.w a,$00F3
    and a,#$0F
    inc a
    eor a,#$FF
    bbc $CD.7,.l0A34
    clrc
    adc.b a,$CD
.l0A34:
    mov.b $CD,a
    mov.w a,$0D15+x
    mov.w $00F3,a
    mov a,#$20
    clrc
    adc.b a,$CC
    mov.b $CC,a
    mov y,#$0D
    mov.w a,$0D16+x
    call DspWrite
    mov.w a,$0D17+x
    call R0A6C
    pop x
.l0A52:
    ret

; @routine 85:8850 R0A53
; @spc 0A53
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R0A53:
    mov y,#$03
.l0A55:
    mov.w a,$0A68+y
    mov.w $00F2,a
    mov a,#$00
    mov.w $00F3,a
    dbnz y,.l0A55
    mov y,#$6C
    mov a,#$20
    jmp DspWrite
    db $2C,$3C,$4D

; @routine 85:8869 R0A6C
; @spc 0A6C
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R0A6C:
    push x
    mov y,#$08
    mul ya
    mov x,a
    mov y,#$0F
.l0A73:
    mov.w a,$0A83+x
    call DspWrite
    inc x
    mov a,y
    clrc
    adc a,#$10
    mov y,a
    bpl .l0A73
    pop x
    ret
    db $7F,$00,$00,$00,$00,$00,$00,$00,$58,$BF,$DB,$F0
    db $FE,$07,$0C,$0C,$0C,$21,$2B,$2B,$13,$FE,$F3,$F9
    db $34,$33,$00,$D9,$E5,$01,$FC,$EB

P0AA3:
    and a,#$01
    lsr a
    bcc .l0AAA
    mov.b a,$B3
.l0AAA:
    bbs $B0.7,.l0AB5
    and $BC,$B4
    or.b a,$BC
    mov.b $BC,a
    ret
.l0AB5:
    and $B7,$B4
    or.b a,$B7
    mov.b $B7,a
    ret

P0ABD:
    or a,#$A0
    mov.w $0240+x,a
    ret

P0AC3:
    setp
    mov.b a,$18+x
    clrc
    adc.b a,$20+x
    mov.b $20+x,a
    bcc .l0AD4
    mov.b a,$00+x
    clrc
    adc a,#$40
    mov.b $00+x,a
.l0AD4:
    mov.b a,$00+x
    mov.b $A4,a
    bbc $A4.0,.l0B0D
    mov $A1,#$00
    mov.b a,$28+x
    asl a
    mov.b $A0,a
    mov.b a,$40+x
    mov.b $A3,a
    mov.b a,$48+x
    mov.b $A2,a
    mov.b y,$30+x
    mov.b a,$38+x
    bbs $A4.1,.l0AFC
    subw ya,$A0
    bcc .l0B02
    cmpw ya,$A2
    bcc .l0B02
    bra .l0B09
.l0AFC:
    addw ya,$A0
    cmpw ya,$A2
    bcc .l0B09
.l0B02:
    mov.b a,$A4
    dec a
    mov.b $00+x,a
    movw ya,$A2
.l0B09:
    mov.b $30+x,y
    mov.b $38+x,a
.l0B0D:
    mov.w a,$00B9
    bpl .l0B14
    clrp
    ret
.l0B14:
    call R0BD6

; @routine 85:8914 R0B17
; @spc 0B17
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R0B17:
    mov.b y,$10+x
    mov.b a,$00+x
    asl a
    asl a
    mov.b a,$20+x
    bcs .l0B23
    eor a,#$FF
.l0B23:
    mul ya
    movw $A2,ya
    mov y,#$7E
    mov a,#$81
    subw ya,$A2
    call R0B90
    mov y,a
    clrp
    bbs $B0.7,.l0B47
    mov.b a,$C3
    mul ya
    push y
    movw ya,$C9
    bbs $C6.7,.l0B42
    mov y,#$FF
    mov a,y
    subw ya,$C9
.l0B42:
    call R0B88
    pop y
    mul ya
.l0B47:
    mov.w a,$0230+x
    mul ya
    mov.b $A3,y
    bbc $B0.5,.l0B54
    mov a,#$80
    bra .l0B57
.l0B54:
    mov.w a,$0248+x
.l0B57:
    mov y,#$14
    mul ya
    movw $A0,ya
    mov $A5,#$01
.l0B5F:
    mov.b y,$A1
    mov.w a,$0BC1+y
    setc
    sbc.w a,$0BC0+y
    mov.b y,$A0
    mul ya
    mov a,y
    mov.b y,$A1
    clrc
    adc.w a,$0BC0+y
    mov.b y,$A3
    mul ya
    mov a,y
    mov.b y,$A5
    call DspWriteVoice
    mov y,#$14
    mov a,#$00
    subw ya,$A0
    movw $A0,ya
    dec.b $A5
    bpl .l0B5F
    ret

; @routine 85:8985 R0B88
; @spc 0B88
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R0B88:
    mov.b $A1,y
    mov y,#$04
    bra P0B94

; @routine 85:898B R0B8E
; @spc 0B8E
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R0B8E:
    mov a,#$FF

; @routine 85:898D R0B90
; @spc 0B90
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R0B90:
    mov.b $A1,y
    mov y,#$03

P0B94:
    lsr.b $A1
    ror a
    dbnz y,P0B94
    mov.b $A0,a
    mov.b y,$A1
    mov.w a,$0BB0+y
    setc
    sbc.w a,$0BAF+y
    mov.b y,$A0
    mul ya
    mov a,y
    mov.b y,$A1
    clrc
    adc.w a,$0BAF+y
    ret
    db $00,$0C,$19,$26,$33,$40,$4C,$59,$66,$73,$80,$8C
    db $99,$B3,$CC,$E6,$FF,$00,$01,$03,$07,$0D,$15,$1E
    db $29,$34,$42,$51,$5E,$67,$6E,$73,$77,$7A,$7C,$7D
    db $7E,$7F,$7F

; @routine 85:89D3 R0BD6
; @spc 0BD6
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R0BD6:
    mov.b a,$00+x
    and a,#$01
    or.b a,$08+x
    bne R0BE9
    mov.w a,$00B0
    bmi .l0BE8
    mov.w a,$00C5
    bne P0BED
.l0BE8:
    ret

; @routine 85:89E6 R0BE9
; @spc 0BE9
; WHAT IT DOES: (draft, to be named).
; HOW: (draft).
; Entry: (draft). Exit: (draft).
R0BE9:
    mov.b a,$08+x
    bne P0BED_l0BF3

P0BED:
    mov.b y,$30+x
    mov.b a,$38+x
    bra .l0C22
.l0BF3:
    asl a
    mov y,#$0C
    mul ya
    movw $A0,ya
    mov.b a,$00+x
    push p
    asl a
    asl a
    mov.b a,$20+x
    bcc .l0C04
    eor a,#$FF
.l0C04:
    push a
    mov.b y,$A0
    mul ya
    mov.b $A0,y
    pop a
    mov.b y,$A1
    mul ya
    mov $A1,#$00
    addw ya,$A0
    movw $A0,ya
    mov.b y,$30+x
    mov.b a,$38+x
    pop p
    bmi .l0C20
    addw ya,$A0
    bra .l0C22
.l0C20:
    subw ya,$A0
.l0C22:
    clrp
    bbs $B0.7,.l0C2E
    mov $A1,$C5
    mov $A0,#$00
    addw ya,$A0
.l0C2E:
    setp
    dec y
    mov.b $A1,y
    mov.b $A0,a
    mov a,y
    mov y,#$00
    setc
    sbc a,#$34
    bcs .l0C45
    mov.b a,$A1
    setc
    sbc a,#$13
    bcs .l0C49
    dec y
    asl a
.l0C45:
    addw ya,$A0
    movw $A0,ya
.l0C49:
    push x
    mov.b a,$A1
    asl a
    mov y,#$00
    mov x,#$18
    div ya,x
    mov x,a
    mov.w a,$0CC3+y
    mov.b $A3,a
    mov.w a,$0CC2+y
    mov.b $A2,a
    mov.w a,$0CC5+y
    push a
    mov.w a,$0CC4+y
    pop y
    subw ya,$A2
    mov.b y,$A0
    mul ya
    mov a,y
    mov y,#$00
    addw ya,$A2
    mov.b $A1,y
    asl a
    rol.b $A1
.l0C74:
    cmp x,#$08
    beq .l0C7E
    lsr.b $A1
    ror a
    inc x
    bra .l0C74
.l0C7E:
    mov.b $A0,a
    pop x
    mov.w a,$0228+x
    mov.b y,$A1
    mul ya
    movw $A2,ya
    mov.w a,$0228+x
    mov.b y,$A0
    mul ya
    push y
    mov.w a,$0220+x
    mov.b y,$A0
    mul ya
    addw ya,$A2
    movw $A2,ya
    mov.w a,$0220+x
    mov.b y,$A1
    mul ya
    mov y,a
    pop a
    addw ya,$A2
    and a,#$F0
    push y
    mov y,#$02
    call DspWriteVoice
    pop a
    inc y
    bra DspWrite
%spc_end()
