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

; Demos and the start of play (game/progress.asm).
!cheat_0009 = $0009             ; nonzero: play starts with 10 health marks and 99 at $0369; the title acts on it too
!play_mode = $0331              ; 2: a demo plays, the pad read from its stream; 1: the pads recorded (development); 0: the pads
!demo_input_at = $033B          ; the demo's next pad entry: byte offset from $84:F700 (word)
!demo_input_frames = $033E      ; frames left on its current pad entry
!demo_number = $033F            ; the demo being played
!health_marks = $0365           ; the player's health marks (most health)
!health = $0367                 ; the player's health
