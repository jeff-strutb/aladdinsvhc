; wram_state.inc: work RAM that holds the game's settings and progress, and
; the joypads as the game reads them (the same names as the C's
; wram_state.h, in lower case).

includeonce

; Kept across a reset: power-on finds $0123 at $0000 and the cartridge
; title copied at $B600 when the work RAM still holds them.
!kept_options = $0003           ; three option bytes the options screen sets
!kept_options_sum = $0006       ; their 8-bit sum

!frame_count = $032E            ; counts NMIs (8-bit)
!continues = $035B              ; continues left (3 at the start of a game)
