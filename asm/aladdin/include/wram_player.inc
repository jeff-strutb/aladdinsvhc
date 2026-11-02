; wram_player.inc: work RAM the player's code keeps (player/), as the C's
; wram_player.h names it (in lower case).

includeonce

!player_top_speed = $141D       ; the fastest the player runs (word, the speed's whole and fraction)
!player_run_accel = $1440       ; added to the speed while the pad holds the facing (word)
!player_run_decel = $1442       ; added while no direction is held (word, negative)
!player_turn_decel = $1444      ; added while the pad holds the other way (word, negative)
!player_move = $1420            ; the player's move (standing, running, jumping ...): picks its animation
!player_dash = $1459            ; 1 while the dash is held: picks the faster speed rows
!player_slowed = $1457          ; counts the frames the speed was held up to its least
!player_pos_saved = $145D       ; a position kept for the player: X, then Y at +2 (pixels, words)
!v_0012 = $0012                 ; direction bits the player's code reads
!v_1428 = $1428                 ; nonzero: the player's direction comes from !v_1434
!v_1434 = $1434
!v_141c = $141C
!v_142d = $142D
!v_144d = $144D
!v_1a08 = $1A08
!player_ground_y = $1455        ; the player's Y (pixels) where it last stood (word)

; The player's test box and hurt time (player/player_box.asm,
; player/player_bounds.asm).
!player_box_wanted = $1448      ; the box of the player's box table the code wants the test box set from
!player_box = $1449             ; the box the test box was last set from
!player_hurt_time = $03F5       ; frames left before the player can be hit again
