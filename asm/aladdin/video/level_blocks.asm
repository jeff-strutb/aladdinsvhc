; level_blocks.asm: the level's blocks as it loads.
; C: src/aladdin/video/level_blocks.c.

incsrc "../include/macros.inc"
incsrc "../include/wram_video.inc"

arch 65816

; @routine 81:C025 BlockSourceAdd
; WHAT IT DOES: While a level loads, lists one more block's source.
; HOW: Stores A at the list's end and steps the end on by a word.
; Entry: M 16-bit, X 16-bit; A = the block's source; DB a bank whose
; $0000-$1FFF is work RAM. Exit: X = the entry's offset; N and Z from the
; new end; A, Y and the other flags unchanged.
%at($81C025)
BlockSourceAdd:
    ldx.w !block_source_end
    sta.w !block_sources,x
    inc.w !block_source_end
    inc.w !block_source_end
    rts
