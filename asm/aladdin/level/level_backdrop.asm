; level_backdrop.asm: what a level does to its backdrop layer as it starts,
; by level from the table at $81:8E0E: most levels nothing; level 6 has
; the backdrop draw no edges; levels 7 and $17 also clear its tilemap.
; C: src/aladdin/level/level_backdrop.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram_video.inc"

arch 65816

!BACKDROP_EDGES = !layer1+!LAYER_EDGES
!BACKDROP_TILEMAP = $3800

; @routine 81:8E06 LevelBackdropStart
; WHAT IT DOES: The level's own start for its backdrop layer: none for
; most levels; for level 6 the backdrop draws no new edges as it scrolls;
; for levels 7 and $17 its tilemap (VRAM $3800, $1000 words) is cleared
; as well.
; HOW: JMP through BACKDROP_STARTS by LEVEL: an RTS (levels 0-5 each have
; their own, the others share one); level 6: BACKDROP_EDGES 0; levels 7
; and $17: VMAIN $80, VMADD $3800, $1000 zero words to VMDATA (16-bit
; registers), then BACKDROP_EDGES 0. The RTSes of levels 1-3 are not run
; by a script.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
; $2100-$21FF the PPU; called by JSR from the level's start (81:8222).
; Exit: M 8-bit, X 8-bit; most levels: A = the level times 2, X too, N, Z
; and C from the shift and the TAX; levels 7 and $17: A = $3800, X = 0, Z
; set, N clear.
%at($818E06)
LevelBackdropStart:
    lda.w !level
    asl a
    tax
    jmp.w (.starts,x)
.starts:
    dw .rts0, .rts1, .rts2, .rts3, .rts4, .rts5, .no_edges, .clear
    dw .rts, .rts, .rts, .rts, .rts, .rts, .rts, .rts
    dw .rts, .rts, .rts, .rts, .rts, .rts, .rts, .clear
    dw .rts, .rts, .rts, .rts, .rts, .rts, .rts, .rts
    dw .rts, .rts, .rts, .rts, .rts, .rts, .rts, .rts
    dw .rts, .rts, .rts, .rts, .rts, .rts, .rts, .rts
    dw .rts, .rts, .rts, .rts, .rts, .rts, .rts, .rts
    dw .rts, .rts, .rts, .rts, .rts, .rts, .rts, .rts
    dw .rts, .rts, .rts, .rts
.rts:
    rts
.rts0:
    rts
.rts1:
    rts
.rts2:
    rts
.rts3:
    rts
.rts4:
    rts
.rts5:
    rts
.no_edges:
    stz.w !BACKDROP_EDGES
    rts
.clear:
    lda.b #$80
    sta.w !VMAIN
    rep.b #$30
    ldx.w #$1000
    lda.w #!BACKDROP_TILEMAP
    sta.w !VMADDL
.word:
    stz.w !VMDATAL
    dex
    bne .word
    sep.b #$30
    stz.w !BACKDROP_EDGES
    rts
