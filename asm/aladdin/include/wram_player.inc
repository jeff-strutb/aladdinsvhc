; wram_player.inc: work RAM the player's code keeps (player/), as the C's
; wram_player.h names it (in lower case).

includeonce

!player_top_speed = $141D       ; the fastest the player runs (word, the speed's whole and fraction)
!player_run_accel = $1440       ; added to the speed while the pad holds the facing (word)
!player_run_decel = $1442       ; added while no direction is held (word, negative)
!player_turn_decel = $1444      ; added while the pad holds the other way (word, negative)
!player_move = $1420            ; the player's move (standing, running, jumping ...): picks its animation
!player_ground_y = $1455        ; the player's Y (pixels) where it last stood (word)
